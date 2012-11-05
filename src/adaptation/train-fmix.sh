#!/bin/bash

# @file train-fmix.sh 
# @brief Train feature-specific mixture LM for a given source file.
# 
# @author George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

# argument processing

error_exit() {
   for msg in "$@"; do echo $msg >&2; done
   echo "Use -h for help." >&2
   exit 1
}

usage() {
   for msg in "$@"; do echo $msg >&2; done
   cat <<==EOF== >&2

Usage: train-fmix.sh [-a1][-e list][-s w][-n n][-c col][-g glm][-w workdir]
                     [-p pfx][-r n][-m m]
                     src id srcmodels tgtmodels mixlm

  Train feature-specific sentence-level mixture LM <mixlm> for source file
  <src>. Each sentence in <src> is characterized by a feature value from a
  selected field in id file <id>, which must be line-aligned with <src>. One
  set of mixture weights is estimated for each set of sentences which share the
  same feature value.

  The <srcmodels> and <tgtmodels> directories must contain LMs named
  <val>.binlm.gz, one for each feature value <val> to mix over. The mixture for
  sentences tagged with <val> consists of <val>.binlm.gz versus the global
  model (all.binlm.gz by default) - or just all.binlm.gz if <val>.binlm.gz
  doesn't exist. Alternately, if -a is selected, the mixture consists of all
  models of the form *.binlm.gz. 

  Options:
  -a  Mix over all LMs found in <srcmodels>/<tgtmodels> [use binary mixture as
      described above]
  -1  Learn mixture weights for each sentence, rather than for sets of sentences
      with the same feature value. The models to mix over are still determined
      by each sentence's feature value (unless -a is given, in which case all
      models are always used).
  -e  For each m in <list>, exclude m.binlm.gz from mixture if -a is in use.
  -s  If -a, do MAP smoothing with a prior estimated over all of <src>. <w> is
      the weight on the prior [0 = no smoothing]. 
  -n  Use a mixture consisting of the union of the <n> highest-weighted
      cmpts for each sentence (remaining cmpts are pruned out). [0 = don't prune]
  -c  Use feature values in whitespace-separated column <col> of id file. [1]
  -g  Use global model named <glm> [all.binlm.gz]
  -w  Store working files in <workdir> [work.b.col, where b is basename of src]
  -p  Use path prefix <pfx> for model directories in output mixlm [current dir]
  -r  Run <n> training jobs in parallel [1]
  -m  Use <m> CPUs per job [1]

==EOF==

   exit 1
}

workdir=
col=1
model_pfx=`pwd`
global_lm=all.binlm.gz
all_models=0
exclude_list=
n=0
prior_wt=0
npar=1
ncpus=1
sentwise=0

while getopts hc:w:g:p:e:n:s:r:m:a1 opt ; do
   case "$opt" in
      h) usage ;;
      c) col=$OPTARG ;;
      w) workdir=$OPTARG ;;
      p) model_pfx=$OPTARG ;;
      g) global_lm=$OPTARG ;;
      a) all_models=1 ;;
      1) sentwise=1 ;;
      e) exclude_list=$OPTARG ;;
      n) n=$OPTARG ;;
      s) prior_wt=$OPTARG ;;
      r) npar=$OPTARG ;;
      m) ncpus=$OPTARG ;;
      \?) usage ;;
   esac
done

shift $((OPTIND-1))

if [[ $# == 0 ]]; then error_exit "Missing src file"; fi
src=$1; shift
if [[ $# == 0 ]]; then error_exit "Missing id file"; fi
id=$1; shift
if [[ $# == 0 ]]; then error_exit "Missing srcmodels dir"; fi
srcmodels=$1; shift
if [[ $# == 0 ]]; then error_exit "Missing tgtmodels dir"; fi
tgtmodels=$1; shift
if [[ $# == 0 ]]; then error_exit "Missing mixlm name"; fi
mixlm=$1; shift

base=`basename $src`

# Determine if val is in list, and set in_list=1 if so. Used by -e option.

valInList() { # (val, list)
   val=$1
   list=$2
   in_list=0
   for v in $list; do
      if [[ $v == $val ]]; then
         in_list=1
         break
      fi
   done
}

# set up workdir

if [[ -z "$workdir" ]]; then workdir=work.$base.$col; fi
if [[ ! -e $workdir ]]; then  
   mkdir $workdir || exit 1
fi

# If we're making binary mixtures, check for global model and write it to mixlm
# as first component of mixture.

if [[ $all_models == 0 ]]; then
   if [[ ! -e $srcmodels/$global_lm ]]; then
      error_exit "$srcmodels/$global_lm doesn't exist"
   fi
   echo "$model_pfx/$tgtmodels/$global_lm 1.0" > $mixlm
fi

# Check consistency of srcmodels/tgtmodels, and write component models to
# mixlm.

if [[ $all_models == 0 ]]; then

   # use only models that match some val in id file

   nmodels=0   # doesn't include global model
   c=$((col-1))
   for val in `perl -nae 'print "$F['$c']\n"' < $id | LC_ALL=C sort -u`; do
      if [[ -e $srcmodels/$val.binlm.gz ]]; then
         if [[ ! -e $tgtmodels/$val.binlm.gz ]]; then
            error_exit "$srcmodels/$val.binlm.gz exists, "\
               "but not $tgtmodels/$val.binlm.gz"
         fi
         echo "$model_pfx/$tgtmodels/$val.binlm.gz 0.0" >> $mixlm
         nmodels=$((nmodels + 1))
      else
         echo "warning: no models for $val: will back off to global" >&2
      fi
   done
   if [[ $nmodels == 0 ]]; then
      error_exit \
         "not one model found in $srcmodels that matches a value in column $col of $id"
   fi

else

   # use all models found except for excluded ones, & write tgt-side
   # counterparts to the mixlm header

   cat /dev/null > $mixlm
   cat /dev/null > $workdir/models
   for f in $srcmodels/*.binlm.gz; do
      val=`basename ${f%.binlm.gz}`
      valInList $val $exclude_list
      if [[ $in_list == 1 ]]; then continue; fi
      if [[ ! -e $tgtmodels/$val.binlm.gz ]]; then
         error_exit "$f exists but not $tgtmodels/$val.binlm.gz"
      fi
      echo "$model_pfx/$tgtmodels/$val.binlm.gz 1.0" >> $mixlm
      echo $srcmodels/$val.binlm.gz >> $workdir/models
   done

fi

# Split source file using the col'th column, and put chunks into workdir.

if [[ $sentwise == 0 ]]; then   # split by tags, into files tag.src
   split_parallel_text_by_tag -r -d $workdir -f $((col - 1)) $id $src
else                            # split by sentences, into files tag.src.sentnum
   perl -nae 'print $F['$((col-1))'],".",$i++,"\n"' < $id > $workdir/fakeid
   split_parallel_text_by_tag -r -d $workdir -f 0 $workdir/fakeid $src
fi

# learn weights for each feature found in the id file

cat /dev/null > $workdir/cmds

if [[ $all_models == 0 ]]; then

   for f in $workdir/*.$base; do
      val=`basename ${f%.$base}`
      tag=$val
      if [[ $sentwise != 0 ]]; then 
         tag=${val/.[0-9]*/}   # tag.123 -> tag
      fi
      if [[ -e $srcmodels/$tag.binlm.gz ]]; then 
         pos=`egrep -n "/$tag[.]binlm[.]gz" $mixlm | cut -d: -f1`  # line in mixlm
         pos=$((pos-1))                                 # first cmpt model is at index 1
         pref=" "`yes | head -$((pos-1)) | perl -pe 's/y/0/go;' | tr '\n' ' '`
         suff=" "`yes | head -$((nmodels-pos)) | perl -pe 's/y/0/go;' | tr '\n' ' '`
# probably faster option, but not tested yet:
#         n=$((pos-1)); pref=" "`perl -e "print ' ',join(' ', (0)x$n);";
#         n=$((nmodels-pos)); suff=" "`perl -e "print ' ',join(' ', (0)x$n);";
         echo $srcmodels/$global_lm > $workdir/$val.models
         echo $srcmodels/$tag.binlm.gz >> $workdir/$val.models
         echo -n "(train_lm_mixture -v $workdir/$val.models $f > $workdir/$val.wts) >& $workdir/$val.log; " >> $workdir/cmds
         echo "echo \`head -1 $workdir/$val.wts\`$pref\`tail -1 $workdir/$val.wts\`$suff > $workdir/$val.wts-line" >> $workdir/cmds
      fi
   done

else

   prior_arg=""
   if [[ $prior_wt != 0 ]]; then
      (train_lm_mixture -v $workdir/models $src > $workdir/prior-wts) \
         >& $workdir/prior-wts.log
      prior_arg="-prior $workdir/prior-wts -w $prior_wt"
   fi
   for f in $workdir/*.$base; do
      val=`basename ${f%.$base}`
      echo -n "(train_lm_mixture -v $prior_arg $workdir/models $f > $workdir/$val.wts) >& $workdir/$val.log; " >> $workdir/cmds
      echo "cat $workdir/$val.wts | tr '\n' ' ' > $workdir/$val.wts-line; echo >> $workdir/$val.wts-line" >> $workdir/cmds
   done

fi

run-parallel.sh -psub -$ncpus $workdir/cmds $npar >& $workdir/log.cmds

echo "sent-level mixture v1.0" >> $mixlm

# add sentence-specific weights to each line of mixlm

suff=" "`yes | head -${nmodels} | perl -pe 's/y/0/go;' | tr '\n' ' '`
c=$((col-1))
if [[ $sentwise != 0 ]]; then  # use "fakeid" file to determine wts
   id=$workdir/fakeid
   c=0
fi
for val in `perl -nae 'print "$F['$c']\n"' < $id`; do
   if [[ -e $workdir/$val.wts-line ]]; then
      cat $workdir/$val.wts-line >> $mixlm
   else
      echo "1.0"$suff >> $mixlm  # default to global model only
   fi
done

# prune if called for

if [[ $n != 0 ]]; then
   prune_mixlm -n $n $mixlm $mixlm.pruned
   mv $mixlm.pruned $mixlm
fi
