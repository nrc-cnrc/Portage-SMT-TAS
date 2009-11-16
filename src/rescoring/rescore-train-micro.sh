#!/bin/bash

# @file rescore-train-micro.sh
# @author George Foster
# @brief Train a rescoring model for each source sentence in SRC, using the
# corresponding nbest list in NBEST and references in REFS.
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

echo 'rescore-train-micro.sh, NRC-CNRC, (c) 2008 - 2009, Her Majesty in Right of Canada'

usage() {
   for msg in "$@"; do
      echo $msg
   done
   cat <<==EOF==

rescore-train-micro.sh [OPTIONS] MODEL_IN MODEL_OUT SRC NBEST REF1 [.. REFN]

Train a rescoring model for each source sentence in SRC, using the
corresponding nbest list in NBEST and references in REFS. Features are read
from MODEL_IN, and results are written to MODEL_OUT, both in standard
rescore_train format. Models are trained in parallel using run-parallel.sh.

MODEL_IN, MODEL_OUT, and NBEST are not files but rather patterns designating
sets of files. Any occurrences of 'IIII' in these arguments will be substituted
for 4-digit sentence indexes 0000, 0001, etc to yield actual sentence-specific
filenames. For example, if nbest lists are in the files 100best.0000.gz,
100best.0001.gz, etc, the NEST argument would be 100best.IIII.gz. NB: the magic
string must be exactly as shown here; you can't use, eg, 'III' to designate
three-digit source-sentence indexes.

Options:

-v             Verbose output
-n N           Number of parallel jobs for run-parallel.sh [3]
-rp-opts OPTS  Options string to pass to run-parallel.sh
-rt-opts OPTS  Options string to pass to rescore_train. Any occurrences of
               'IIII' in this string will be substituted for the 4-digit index
               of the current source string. Eg, to use a single MODEL_IN file
               rather than a set of input models, you could use -rp-opts "-p
               ffvals.IIII" with a model file containing entries of the form
               "FileFF:.ext,i" (where i is the column number). When training on
               sentence 1234, this would read from the file ffvals.1234.ext.

==EOF==

   exit 1
}

error_exit() {
   echo -n "rescore-train-micro.sh fatal error: "
   for msg in "$@"; do
      echo $msg 
   done
   echo "Use -h for help."
   exit 1
}

arg_check() {
   if [ $2 -le $1 ]; then
      error_exit "Missing argument to $3 option."
   fi
}

# patsubst PATTERN SEQ sets FNAME to PATTERN with 'IIII' replaced by SEQ
patsubst() {
   FNAME=`echo "$1" | perl -pe "s/IIII/$2/go;"`
}

# Command line processing

VERBOSE=0
MODEL_IN=
MODEL_OUT=
SRC=
NBEST=
REFS=

N=3
RT_OPTS=
RP_OPTS=

while [ $# -gt 0 ]; do
   case "$1" in
   -v|-verbose) VERBOSE=$(( $VERBOSE + 1 ));;
   -h|-help)    usage;;
   -rt-opts)    arg_check 1 $# $1; RT_OPTS="$2"; shift;;
   -rp-opts)    arg_check 1 $# $1; RP_OPTS="$2"; shift;;
   -n)          arg_check 1 $# $1; N="$2"; shift;;
   --)          shift; break;;
   -*)          error_exit "Unknown option $1.";;
   *)           break;;
   esac
   shift
done

if [ $# -lt 5 ]; then error_exit "Expected at least 5 arguments."; fi
MODEL_IN=$1
MODEL_OUT=$2
SRC=$3
NBEST=$4
shift; shift; shift; shift
REFS=$*

# Check (some) args

for r in $REFS; do
   if [ ! -r $r ]; then error_exit "Can't read reference file $r"; fi
done

# Build parallel command file

WORKDIR=rescore-train-micro.$$
mkdir $WORKDIR
if [ $? != 0 ]; then error_exit "Can't create workdir $WORKDIR"; fi   

CMDFILE="cmds.$$"
cat /dev/null > $CMDFILE
if [ $? != 0 ]; then error_exit "Can't write command file"; fi   

if [ ! -r $SRC ]; then error_exit "Can't read source file $SRC"; fi

nl=`wc -l < $SRC`
if [ $nl -gt 10000 ]; then
   error_exit "Number of lines in source file must be <= 10k"
fi

linenum=1
for i in `seq -w 0 9999 | head -$nl`; do

   if [ -n "$RT_OPTS" ]; then
      patsubst "$RT_OPTS" $i
      rt_opts=$FNAME
   fi 

   patsubst $MODEL_IN $i
   model_in=$FNAME
   if [ ! -r $model_in ]; then error_exit "Can't read model file $model_in"; fi

   patsubst $MODEL_OUT $i
   model_out=$FNAME
   if [ $MODEL_OUT == $model_out ]; then 
      error_exit "Can't write all output models to the file $MODEL_OUT"\
         "... did you forget the pattern?"
   fi

   select-line $linenum < $SRC > $WORKDIR/src.$i
   if [ $? != 0 ]; then error_exit "Can't write source file line(s)"; fi   

   patsubst $NBEST $i
   nbest=$FNAME
   if [ ! -r $nbest ]; then error_exit "Can't read nbest file $nbest"; fi

   j=0
   refs=
   for ref in $REFS; do
      select-line $linenum < $ref > $WORKDIR/ref$j.$i
      if [ $? != 0 ]; then error_exit "Can't write ref file line(s)"; fi   
      refs="$refs $WORKDIR/ref$j.$i"
      j=$((j+1))
   done

   echo -n "rescore_train " >> $CMDFILE
   if [ $VERBOSE -ne 0 ]; then echo -n "-v " >> $CMDFILE; fi
   if [ -n "$rt_opts" ]; then echo -n "$rt_opts" >> $CMDFILE; fi
   echo -n " $model_in $model_out $WORKDIR/src.$i $nbest $refs" >> $CMDFILE
   echo " >& $WORKDIR/log.$i" >> $CMDFILE

   linenum=$((linenum+1))

done

# Run it

if [ $VERBOSE -ne 0 ]; then
   run-parallel.sh $RP_OPTS $CMDFILE $N
   echo "Powell results for each sentence:"
   egrep '^Best score' $WORKDIR/log.* |  cut -d'.' -f3-
else
   run-parallel.sh $RP_OPTS $CMDFILE $N >& /dev/null
fi

if [ $? != 0 ]; then
   error_exit "Problems with run-parallel; working files have been left in $WORKDIR"
fi

# cleanup 

\rm -rf $WORKDIR
\rm -f $CMDFILE
