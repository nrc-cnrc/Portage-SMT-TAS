#!/bin/bash

# rat.sh
#
# PROGRAMMER: George Foster, Samuel Larkin
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

echo 'rat.sh, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada'

usage() {
   for msg in "$@"; do
      echo $msg
   done
   cat <<==EOF==

rat.sh [cp-opts] MODE [-v][-F]
       [-f cfg][-K nb][-n #nodes][-J #jobs_per_ff]
       [-s SPROXY][-p pfx][-msrc MSRC][-o MODEL_OUT]
       [-bleu | -per | -wer]
       MODEL SRC [REFS]

Train or translate with a rescoring model: first generate nbest lists & ffvals
for a given source file, then generate external feature files required for
rescoring, then either train a model using reference texts, or use an existing
model to translate the source text. Decoding and feature generation can be run
in parallel. All intermediate files are written to a working directory created
below the current directory, whose name depends on the current arguments.

Arguments are:

MODE     Required keyword, either 'train' or 'trans' to select training or
         translation mode. In training mode, one or more reference texts must
         be provided. In translation mode, the rescored translation is written
         to the file MSRC.rat (where MSRC is the argument to -msrc).
MODEL    The rescoring model. Use rescore_train -h for a description of the
         syntax, and rescore_train -H for a list of features. If training, the
         final model is written to MODEL.out. Rat automatically generates a
         file 'ffvals' containing all decoder feature values, so entries like
         'FileFF:ffvals,i' can be used in MODEL to refer to columns in this
         file.
         Rat gives special interpretation to three magic tags within MODEL:
         <src>        will be replaced by SRC's basename - this can be used to
                      specify feature arguments that depend on the current
                      source file.
         <ffval-wts>  designates an automatically-generated rescoring model
                      that contains decoder features and their weights - this
                      is required by the nbest* features.
         <pfx>        will be replaced by the prefix for all intermediate files,
                      WDIR/PFX (where WDIR is the work directory, and PFX is the
                      argument to -p) - this can be used for features like the
                      nbest* ones, which need to access the intermediate files.
SRC      Source file, in standard format.
REFS     One or more reference translations, in standard format.

Options:

cp-opts  Options to canoe-parallel.sh (e.g., -n). Only the options that come
         before canoe-parallel.sh's 'canoe' keyword are legal here.
-v       Verbose output.
-F       Force overwrite of existing feature function files. [don't overwrite]
-f       Canoe's config file. [canoe.ini]
-K       Size of nbest lists. [1000]
-n       Number of parallel jobs for feature generation. This is distinct
         from the number of jobs for translation, which is set by the -n
         parameter in cp-opts. [the N value in cp-opts, if specified, else 3]
-J       Specifies degree of horizontal parallelism. Don't use unless you know
         what this means. [2*ceil(#nodes/# of non-ffvals features)]
-s       Use SPROXY instead of SRC when substituting for <src> tag in MODEL.
-p       Prefix for all intermediate files (single best, nbest, ffvals, feature
         files) within the working directory. This switch can be used to store
         results from several different runs in the working directory. If PFX
         is non-empty, the output in translation mode is named PFXrat. [""]
-msrc    A version of the source file marked up with rule-based translations,
         used for canoe input but not for feature generation [SRC]
-o       In train mode, write the final model to MODEL_OUT [MODEL.out]
-bleu    Perform training using BLEU as the metric [do]
-per     Perform training using PER as the metric [don't]
-wer     Perform training using WER as the metric [don't]

Note: if any intermediate files already exist, this script will not overwrite
them (unles -F is specified), so that when you add extra features to an
existing model, only the new feature is calculated.

==EOF==

   exit 1
}

# error_exit "some error message" "optionally a second line of error message"
# will exit with an error status, printing the specified error message(s) on
# STDERR.
error_exit() {
   for msg in "$@"; do
      echo $msg
   done
   echo "Use -h for help."
   exit 1
}

# Verify that enough args remain on the command line
# syntax: one_arg_check <args needed> $# <arg name>
# Note that this function expects to be in a while/case structure for
# handling parameters, so that $# still includes the option itself.
# exits with error message if the check fails.
arg_check() {
   if [ $2 -le $1 ]; then
      error_exit "Missing argument to $3 option."
   fi
}

# Get the CPOPTS string before regular arg processing

WHOLEARGS="$*"
LOAD_BALANCING=
N=3

CPOPTS=""
MODE=
while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)     usage;;
   train)        MODE="train"; break;;
   trans)        MODE="trans"; break;;
   -lb)          LOAD_BALANCING=1; CPOPTS="$CPOPTS $1";; # catch load-balacing passed as a cp-opts
   -n|-num)      arg_check 1 $# $1; N=$2; CPOPTS="$CPOPTS -n $2"; shift;;
   *)            CPOPTS="$CPOPTS $1"
   esac
   shift
done

if [ "$MODE" = "" ]; then
   error_exit "Missing MODE keyword"
fi
shift

# Command line processing

DEBUG=
VERBOSE=0
DASHV=
CANOE_CONFIG="canoe.ini"
K=1000
PFX=
SPROXY=
MSRC=
MODEL_OUT=
WORKDIR="workdir"
FORCE_OVERWRITE=
JOBS_PER_FF=
TRAINING_TYPE="-bleu"

while [ $# -gt 0 ]; do
   case "$1" in
   -v|-verbose) VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)   DEBUG="-d";;
   -h|-help)    usage;;

   -f)          arg_check 1 $# $1; CANOE_CONFIG=$2; shift;;
   -K)          arg_check 1 $# $1; K=$2; shift;;
   -n)          arg_check 1 $# $1; N=$2; shift;;
   -J)          arg_check 1 $# $1; JOBS_PER_FF="-J $2"; shift;;
   -s)          arg_check 1 $# $1; SPROXY=$2; shift;;
   -p)          arg_check 1 $# $1; PFX=$2; shift;;
   -msrc)       arg_check 1 $# $1; MSRC=$2; shift;;
   -o)          arg_check 1 $# $1; MODEL_OUT=$2; shift;;
   
   -bleu)       TRAINING_TYPE="-bleu";;
   -per)        TRAINING_TYPE="-per";;
   -wer)        TRAINING_TYPE="-wer";;

   -F)          FORCE_OVERWRITE="-F";;

   --)          shift; break;;
   -*)          error_exit "Unknown option $1.";;
   *)           break;;
   esac
   shift
done

if [ $# -lt 2 ]; then error_exit "Expected at least 2 arguments."; fi
MODEL=$1
SRC=$2
shift; shift

if [ "$MODE" = train -a -z "$MODEL_OUT" ]; then
   MODEL_OUT=`basename $MODEL`.out
fi

REFS=$*
if [ "$MODE" = "train" ] && (( $# == 0 )); then
   error_exit "Need reference texts for training."
fi

# In trans mode, run bleumain if references are passed.
#if [ "$MODE" = "trans" ] && (( $# != 0 )); then
#    error_exit "Too many arguments for translation mode."
#fi


# default values for some args

test -z "$MSRC" && MSRC=$SRC;
#test -z "$PFX" && PFX="`basename $MSRC`.";  # don't default to MSRC as a prefix since all is contained within WORKDIR
if (( $VERBOSE )); then DASHV="-v"; fi

WORKDIR=`basename "$WORKDIR"`"-"`basename $MSRC`"-${K}best"
test ! -e $WORKDIR && mkdir $WORKDIR
ORIG_PFX=$PFX
test -z "$ORIG_PFX" && ORIG_PFX="`basename $MSRC`.";
PFX=$WORKDIR/$PFX

# arg checking

if [ ! -r $CANOE_CONFIG ]; then
   error_exit "Error: CANOE_CONFIG file $CANOE_CONFIG is not readable."
fi
if [ ! -e $MODEL ]; then
   error_exit "Error: Model file $MODEL does not exist."
fi
if [ "$MODE" = "train" ]; then
   if [ \( -e $MODEL_OUT -a ! -w $MODEL_OUT \) -o ! -w `dirname $MODEL_OUT` ]
   then
      error_exit "Error: File $MODEL_OUT is not writable."
   fi
fi
if [ "$MODE" = "trans" ]; then
   if [ ! -e $MODEL ]; then
      error_exit "Error: Model file $MODEL does not exist."
   fi
fi
if [ ! -r $MSRC ]; then
   error_exit "Error: MSRC file $MSRC is not readable."
fi
if [ ! -r $SRC ]; then
   error_exit "Error: SRC file $SRC is not readable."
fi

for ref in $REFS ; do
   if [ ! -r $ref ]; then
      error_exit "Error: REFS file $ref is not readable."
   fi
done

if which-test.sh qsub; then
   CLUSTER=1
else
   CLUSTER=0
fi

# RUN

if (( $VERBOSE )); then
   echo $0 $WHOLEARGS
   echo Starting on `date`
   echo ""
fi

# 1. Run canoe to generate nbest lists and ffvals

if (( $VERBOSE )); then
   echo "Generating ${K}best lists:"
fi

if [ ! -e ${PFX}${K}best -a ! -e ${PFX}${K}best.gz ]; then
   if [ -e ${PFX}ffvals.gz ]; then
      echo "Ffvals file ${PFX}ffvals exists already! Overwriting it!"
      \rm ${PFX}ffvals.gz
   fi
   if [ -e ${PFX}pal.gz ]; then
      echo "Phrase alignment file ${PFX}pal exists already! Overwriting it!"
      \rm ${PFX}pal.gz
   fi

   if ! configtool check $CANOE_CONFIG; then
      error_exit "Error: problem with config file $CANOE_CONFIG."
   fi

   if [ -n "$LOAD_BALANCING" ]; then
      test -n "$DEBUG" && echo "Using load-balancing"
      set -o pipefail
      # Not running with append mode and compressed output
      canoe-parallel.sh $CPOPTS \
         canoe -v $VERBOSE -f $CANOE_CONFIG -nbest ${PFX}nb:$K -ffvals -palign \
         < $MSRC | nbest2rescore.pl -canoe > ${PFX}1best

      if (( $? != 0 )); then
         echo "problems with canoe-parallel.sh - quitting!"
         exit 4
      fi
      set +o pipefail
      if [ `wc -l < $MSRC` -ne `wc -l < ${PFX}1best` ]; then
         echo "problems with canoe-parallel.sh - quitting!"
         exit 5
      fi

      # Not in append mode thus we need to merge the nbest list
      ### concatenating separate N-best lists into one big file
      len=`wc -l < $MSRC`
      s=0;
      \rm -f ${PFX}${K}best ${PFX}ffvals ${PFX}pal
      while [ $s -lt $len ]; do
         base_filename=`printf "${PFX}nb.%4.4d.${K}best" $s`
         cat ${base_filename}        >> ${PFX}${K}best
         cat ${base_filename}.ffvals >> ${PFX}ffvals
         cat ${base_filename}.pal    >> ${PFX}pal

         \rm ${base_filename} ${base_filename}.ffvals ${base_filename}.pal

         s=$((s + 1));
      done

      gzip -f ${PFX}${K}best
      gzip -f ${PFX}ffvals
      gzip -f ${PFX}pal
   else
      set -o pipefail
      canoe-parallel.sh $CPOPTS \
         canoe -append -v $VERBOSE -f $CANOE_CONFIG -nbest ${PFX}nb..gz:$K -ffvals -palign \
         < $MSRC | nbest2rescore.pl -canoe > ${PFX}1best

      if (( $? != 0 )); then
         echo "problems with canoe-parallel.sh - quitting!"
         exit 4
      fi
      set +o pipefail
      if [ `wc -l < $MSRC` -ne `wc -l < ${PFX}1best` ]; then
         echo "problems with canoe-parallel.sh - quitting!"
         exit 5
      fi
      \mv ${PFX}nb.nbest.gz  ${PFX}${K}best.gz
      \mv ${PFX}nb.ffvals.gz ${PFX}ffvals.gz
      \mv ${PFX}nb.pal.gz    ${PFX}pal.gz
   fi

   NBEST=${PFX}${K}best.gz
   PAL=${PFX}pal.gz

else
   echo "${PFX}${K}best(.gz) exists - skipping ${K}best and ffvals generation"
   if [ -e ${PFX}${K}best.gz ]; then
      NBEST=${PFX}${K}best.gz
   else
      NBEST=${PFX}${K}best
   fi
   if [ -e ${PFX}pal.gz ]; then
      PAL=${PFX}pal.gz
   else
      PAL=${PFX}pal
   fi

fi

# 2. Generate feature values for nbest lists

if (( $VERBOSE )); then
   echo "Generating feature files:"
   echo ""
fi

MODEL_RAT_IN=$WORKDIR/`basename $MODEL`.rat
MODEL_RAT_OUT=$WORKDIR/`basename $MODEL`.rat.out

DASHV=
test -n "$VERBOSE" && DASHV="-v"
gen-features-parallel.sh $DEBUG $FORCE_OVERWRITE $DASHV \
  -N $N $JOBS_PER_FF -o $MODEL_RAT_IN -c $CANOE_CONFIG -a $PAL -s "$SPROXY" -p $PFX \
  $MODEL $SRC $NBEST

if (( $? != 0 )); then
   echo "problems with gen-features-parallel.sh - quitting!"
   exit 1
fi

# 3. Train or trans with the rescoring model

if (( $VERBOSE )); then
   echo ""
   if [ "$MODE" = "train" ]; then echo "Training rescoring model:"; fi
   if [ "$MODE" = "trans" ]; then echo "Translating with rescoring model:"; fi
   echo ""
fi

if [ "$MODE" = "train" ]; then
   if (( $VERBOSE )); then
      echo rescore_train $DASHV -n -p $TRAINING_TYPE $PFX $MODEL_RAT_IN $MODEL_RAT_OUT $SRC $NBEST $REFS
   fi
   rescore_train $DASHV -n -p $TRAINING_TYPE $PFX $MODEL_RAT_IN $MODEL_RAT_OUT $SRC $NBEST $REFS
else
   if (( $VERBOSE )); then
      echo rescore_translate $DASHV -p $PFX $MODEL_RAT_IN $SRC ${NBEST} \> ${ORIG_PFX}rat
   fi
   # We want the results outside the working dir that's why we are using $ORIG_PFX
   rescore_translate $DASHV -p $PFX $MODEL_RAT_IN $SRC ${NBEST} > ${ORIG_PFX}rat
fi

if (( $? != 0 )); then
   echo "problems with rescoring!"
   exit 1
elif (( $VERBOSE )); then
   echo "Completed on `date`"
fi

if [ $MODE = "trans" ]; then
   if [ -n "$REFS" ]; then
      echo ""
      echo "Evaluation of 1-best output from canoe:"
      if (( $VERBOSE )); then
         echo bleumain -c ${PFX}1best $REFS
      fi
      bleumain -c ${PFX}1best $REFS

      echo ""
      echo "Evaluation of rescoring output:"
      if (( $VERBOSE )); then
         echo bleumain -c ${ORIG_PFX}rat $REFS
      fi
      # Don't forget that the rat translation are outside the workdir
      bleumain -c ${ORIG_PFX}rat $REFS
   fi
else
   # Transform the trained rescore-model from the rat syntax to the normal syntax
   MODEL_TMP=`basename $MODEL`".tmp"
   # Remove the previous weights
   cut -f1 -d' ' $MODEL > $MODEL_TMP
   # Glue the feature function names with their weights
   cut -f2 -d' ' $MODEL_RAT_OUT \
      | paste -d ' ' $MODEL_TMP - \
      > $MODEL_OUT
   rm $MODEL_TMP
fi

