#!/bin/bash

# @file train-nnjm.sh
# @brief Wrapper around NNJM training scripts starting from a corpus and outputting a model
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Centre de recherche en technologies numeriques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majest/ la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG [options] TRAIN_S TRAIN_T TRAIN_WAL [TRAIN_S2 TRAIN_T2 TRAIN_WAL2 [...]]

  Train an NNJM model from a training and a dev corpus, optionally starting from a
  model pre-trained on a background or larger corpus.

Positional arguments:

  The training corpus should be provided as triplets of files: tokenized source,
  tokenized target, word alignment in s-t SRI format. To use combine multiple corpora
  together as training data, list multiple triplets of src/tgt/wal files.

Mandatory option-like arguments:

  -out NNJMDIR             Output NNJM directory.
  -dev DEV_S DEV_T DEV_WAL Dev corpus files: tokenized source, tokenized target, word alignment
  -cls-s     CLS_S         Source-language word classes (typically 400 classes)
  -cls-t     CLS_T         Target-language word classes (typically 400 classes)

  The CLS files can be in .classes format produced by word2vec or .mmcls format produced
  by wordClasses2MMmap.

Options:

  -workdir WD   place intermediate files in directory WD instead of a temporary one
  -pre-trained-nnjm PRE_NNJM  Start training from PRE_NNJM instead of from scratch
  -train-nnjm-opts "OPTS"     Additional options for train-nnjm.py
  -nnjm-genex-opts "OPTS"     Additional options for nnjm-genex.py
  -n(otreally)  don't do the work, just create the .cmds file
  -h(elp)       print this help message
  -v(erbose)    increment the verbosity level by 1 (may be repeated)
  -d(ebug)      print debugging information

==EOF==

   exit 1
}

VERBOSE=1
TRAIN_NNJM_OPTS=
NNJM_GENEX_OPTS=
while [[ $# -gt 0 ]]; do
   case "$1" in
   -dev)              arg_check 3 $# $1;
                      DEV_S=$2; DEV_T=$3; DEV_WAL=$4;
                      shift; shift; shift;;
   -cls-s)            arg_check 1 $# $1; CLS_S=$2; shift;;
   -cls-t)            arg_check 1 $# $1; CLS_T=$2; shift;;
   -pre-trained-nnjm) arg_check 1 $# $1; PRE_NNJM=$2; shift;;
   -workdir)          arg_check 1 $# $1; WD=$2; shift;;
   -out)              arg_check 1 $# $1; OUT=$2; shift;;
   -train-nnjm-opts)  arg_check 1 $# $1; TRAIN_NNJM_OPTS=$2; shift;;
   -nnjm-genex-opts)  arg_check 1 $# $1; NNJM_GENEX_OPTS=$2; shift;;
   -n|-notreally)     NOTREALLY=1;;
   -v|-verbose)       VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)         DEBUG=1;;
   -h|-help)          usage;;
   --)                shift; break;;
   -*)                error_exit "Unknown option $1.";;
   *)                 break;;
   esac
   shift
done

[[ $DEV_S ]] || error_exit "Missing required -dev flag"
[[ $OUT ]] || error_exit "Missing required -out flag"
[[ $CLS_S ]] || error_exit "Missing required -cls-s flag"
[[ $CLS_T ]] || error_exit "Missing required -cls-t flag"
[[ -r $DEV_S && -s $DEV_S ]] || error_exit "Cannot read dev source file $DEV_S"
[[ -r $DEV_T && -s $DEV_T ]] || error_exit "Cannot read dev target file $DEV_T"
[[ -r $DEV_WAL && -s $DEV_WAL ]] || error_exit "Cannot read dev alignment file $DEV_WAL"
[[ -r $CLS_S && -s $CLS_S ]] || error_exit "Cannot read source classes file $CLS_S"
[[ -r $CLS_T && -s $CLS_T ]] || error_exit "Cannot read target classes file $CLS_T"

for program in word2class nnjm-genex.py train-nnjm.py unpickle.py wordClasses2MMmap; do
   which-test.sh $program || error_exit "Cannot find required program $program."
done

if [[ $WD ]]; then
   mkdir -p $WD || error_exit "Cannot create working directory $WD"
else
   WD=`mktemp -d train-nnjm.workdir-XXX` || error_exit "Cannot create temp directory"
fi

ERROR_STATE=
r_cmd() {
   cmd=$*
   echo "$cmd" >> $WD/cmds
   if [[ $NOTREALLY ]]; then
      echo "$cmd"
   else
      verbose 1 "[`date`] $cmd"
      echo "time-mem $cmd" >> $WD/log.time-mem
      time-mem $cmd >> $WD/log.time-mem 2>&1
      RC=$?
      if [[ $RC == 0 ]]; then
         verbose 2 "[`date` Done rc=$RC]"
      else
         verbose 1 "[`date` Done *** rc=$RC ***]"
         warn "Stopping after failed command. Below are the other commands that would have been run."
         ERROR_STATE=1
         NOTREALLY=1
      fi
      return $RC
   fi
}

r_cmd mkdir -p $OUT

# Step 1: if the training corpus is in multiple files, create a concatenated copy in $WD
[[ $# -gt 3 ]] && MULTIPLE_TRAIN_FILES=1
while [[ $# -ge 3 ]]; do
   TRAIN_S="$TRAIN_S $1"
   TRAIN_T="$TRAIN_T $2"
   TRAIN_WAL="$TRAIN_WAL $3"
   shift; shift; shift
done
[[ $# = 0 ]] || error_exit "Positional arguments should come in SRC TGT WAL tripplets"

verbose 2 TRAIN_S=$TRAIN_S
verbose 2 TRAIN_T=$TRAIN_T
verbose 2 TRAIN_WAL=$TRAIN_WAL

if [[ $MULTIPLE_TRAIN_FILES ]]; then
   r_cmd "zcat -f $TRAIN_S | gzip > $WD/train-s.gz"; TRAIN_S=$WD/train-s.gz
   r_cmd "zcat -f $TRAIN_T | gzip > $WD/train-t.gz"; TRAIN_T=$WD/train-t.gz
   r_cmd "zcat -f $TRAIN_WAL | gzip > $WD/train-wal.gz"; TRAIN_WAL=$WD/train-wal.gz
fi

# Step 2: project all the dev and train files through the classes to create .cls files.
r_cmd "{ zcat -f $TRAIN_S | \
         word2class -no-error -v - $CLS_S | \
         gzip > $WD/train-s.cls.gz ; } >& $WD/log.train-s.cls"
r_cmd "{ zcat -f $TRAIN_T | \
         word2class -no-error -v - $CLS_T | \
         gzip > $WD/train-t.cls.gz ; } >& $WD/log.train-t.cls"
r_cmd word2class -no-error -v $DEV_S $CLS_S $WD/dev-s.cls ">&" $WD/log.dev-s.cls
r_cmd word2class -no-error -v $DEV_T $CLS_T $WD/dev-t.cls ">&" $WD/log.dev-t.cls

# Step 3: use nnjm-genex.py to generate the training examples and (if no pre-trained NNJM
# is provided) the three NNJM vocabulary files.
r_cmd "{ nnjm-genex.py -v -eos -voc $OUT/train-voc -isv 16000 -itv 16000 -ov 32000 \
         -stag $WD/train-s.cls.gz -ttag $WD/train-t.cls.gz  -ng 4 -ws 11 \
         $NNJM_GENEX_OPTS \
         $TRAIN_S $TRAIN_T $TRAIN_WAL \
         | gzip > $WD/train-ex.gz ; } >& $WD/log.train-ex"

# Step 4: use nnjm-genex.py to generate the dev examples
r_cmd "{ nnjm-genex.py -v -eos -voc $OUT/train-voc -r \
         -stag $WD/dev-s.cls -ttag $WD/dev-t.cls -ng 4 -ws 11 \
         $NNJM_GENEX_OPTS \
         $DEV_S $DEV_T $DEV_WAL \
         | gzip > $WD/dev-ex.gz ; } >& $WD/log.dev-ex"

# Step 5: use train-nnjm.py to train the NNJM (outputs nnjm.pkl)
r_cmd "train-nnjm.py -v -train_file $WD/train-ex.gz -dev_file $WD/dev-ex.gz \
       -swin_size 11 -thist_size 3 -embed_size 192 -n_hidden_layers 1 -slice_size 64000 \
       -print_interval 1 -hidden_layer_sizes 512 -n_epochs 60 -self_norm_alpha 0.1 -eta_0 0.3 \
       -rnd_elide_max 3 -rnd_elide_prob 0.1 -val_batch_size 10000 -batches_per_epoch 20000 \
       $TRAIN_NNJM_OPTS \
       $OUT/nnjm.pkl >& $WD/log.train-nnjm.py"

# Step 6: use unpickle.py to convert nnjm.pkl into the binary format required for canoe,
# nnjm.bin
r_cmd "unpickle.py $OUT/nnjm.pkl $OUT/nnjm.bin >& $WD/log.unpickle"

# Figure out how to make a path relative to . relative to $OUT instead
if [[ `readlink -f $OUT/..` == `readlink -f .` ]]; then
   RELATIVE_FROM_OUT="../"
elif [[ `readlink -f $OUT` == `readlink -f .` ]]; then
   RELATIVE_FROM_OUT=""
elif [[ `readlink -f $OUT/../..` == `readlink -f .` ]]; then
   RELATIVE_FROM_OUT="../../"
else
   RELATIVE_FROM_OUT=`pwd`/
fi

# Step 7: tightly pack the class files
if [[ $CLS_S =~ .mmcls$ ]]; then
   if [[ $CLS_S =~ ^/ ]]; then
      MM_CLS_S=$CLS_S
   else
      MM_CLS_S=$RELATIVE_FROM_OUT$CLS_S
   fi
else
   MM_CLS_S=`basename $CLS_S .classes`.mmcls
   r_cmd "wordClasses2MMmap $CLS_S $OUT/$MM_CLS_S >& $WD/log.$MM_CLS_S"
fi

if [[ $CLS_T =~ .mmcls$ ]]; then
   if [[ $CLS_T =~ ^/ ]]; then
      MM_CLS_T=$CLS_T
   else
      MM_CLS_T=$RELATIVE_FROM_OUT$CLS_T
   fi
else
   MM_CLS_T=`basename $CLS_T .classes`.mmcls
   r_cmd "wordClasses2MMmap $CLS_T $OUT/$MM_CLS_T >& $WD/log.$MM_CLS_T"
fi

# Step 8: write the model file required by canoe
cat > $WD/model <<==EOF==
[srcvoc] train-voc.src
[tgtvoc] train-voc.tgt
[outvoc] train-voc.out
[srcclasses] $MM_CLS_S
[tgtclasses] $MM_CLS_T
[format] native
[file] nnjm.bin#0
[file] nnjm.bin#1
[file] nnjm.bin#2
[file] nnjm.bin#3
[selfnorm]
==EOF==
r_cmd cp $WD/model $OUT/model

[[ $ERROR_STATE ]] && error_exit "A command failed. Model not successfully built."
