#!/bin/bash

# ngram-count-big.sh - wrapper around ngram-count to make large n-gram count
#                      files using the sorted-count / merge process.  Intended
#                      to replace srilm's make-big-lm script, dealing with all
#                      the bookkeeping automatically, and splitting very large
#                      files automatically.
#
# PROGRAMMER: Eric Joanis and Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada
# Copyright 2006, Her Majesty in Right of Canada

echo 'ngram-count-big.sh, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada'
echo 'Wrapper around SRILM software - use only if you have a valid SRILM license'

usage() {
   for msg in "$@"; do
      echo $msg
   done
   cat <<==EOF==

Usage: ngram-count-big.sh [options] output-ngrams text1[.gz] text2[.gz] ...

  Make ngram count files for each input text file, as well as a single count
  file merging all of them, output-ngrams

Options:

  -h[elp]         Print this help message.
  -N nodes        Number of nodes for run-parallel.sh [3].
  -order N        Order of n-grams to count. [3]
  -c[hunk-size] S Amount of text to process in each chunk, in a format so that
                  "split -C S" is legal, e.g., 1500k, 500m. [250m]
  -merge-only     Assume the inputs are ngram-count files rather than texts,
                  and merge them.
  -n              "Not really" (just show what the program would do)
  -debug          debug mode doesn't remove temporary files

==EOF==

   exit 1
}

error_exit() {
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

echo ""
echo $0 $*

echo "Using $TMPDIR as TMPDIR"

ORDER=3
CHUNK_SIZE=250m
N=3
while [ $# -gt 0 ]; do
   case "$1" in
   -c|-chunk-size)      arg_check 1 $# $1; CHUNK_SIZE=$2; shift;;
   -order)              arg_check 1 $# $1; ORDER=$2; shift;;
   -N)                  arg_check 1 $# $1; N=$2; shift;;
   -n)                  NOT_REALLY=1;;
   -h|-help)            usage;;
   -merge-only)         MERGE_ONLY=1;;
   -debug)              DEBUG=1;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

if [ "`expr $ORDER + 0 2> /dev/null`" != "$ORDER" ]; then
   error_exit "$ORDER is not an integer."
fi

WORK_DIR=ngram-count-big.$$
COUNT_CMDS_FILE=$WORK_DIR/cmds.count
MERGE_CMDS_FILE=$WORK_DIR/cmds.merge

# Echo a command and then run it
run () {
   cmd="$*"
   echo ""
   echo '#' `date`
   echo "$cmd"
   if [ ! $NOT_REALLY ]; then
      eval $cmd || exit 3
   fi
}

# merge_ngrams out_file in_file1 in_file2 ...
# will merge the n-gram counts files in_file1, in_file2, ... into a single one,
# out_file.
merge_ngrams() {
   out_file=$1
   shift
   #echo initial arg list: "$@"

   # With only one file to merge, copy it, preserving the correctness of the
   # assumption that .gz files are gzipped and others aren't.
   if [ $# = 1 ]; then
      # The input has the same name as the output
      test "$1" = "$out_file" && return
      
      if [ ${1%.gz}.gz = $1 ]; then
         if [ ${out_file%.gz}.gz = $out_file ]; then
            run cp $1 $out_file
         else
            run gzip -cdqf \< $1 \> $out_file
         fi
      else
         if [ ${out_file%.gz}.gz = $out_file ]; then
            run gzip \< $1 \> $out_file
         else
            run cp $1 $out_file
         fi
      fi
      return
   fi
   # Turns out it's better to call only once ngram-merge with all files instead
   # of making a pyramid shape merging.
   run ngram-merge -write $out_file $@
}

# count_ngrams outfile infile
# Do the actual ngram-count, splitting if necessary, for a given text file
count_ngrams() {
   out_file=$1
   text=$2
   out_prefix=`basename $text`
   chunk=$WORK_DIR/$out_prefix.$CHUNK_SIZE
   run gzip -cdqf < $text \| split -C $CHUNK_SIZE - $chunk.
   if [ $? != 0 ]; then
      error_exit "split failed - bailing out"
   fi
   ls -l $chunk.??
   for file in $chunk.?? ; do
      # By adding the test guard this allows the user to run-parallel.sh
      # $COUNT_CMDS_FILE N if something fail, thus resuming only the failed
      # jobs.  Remember that we automatically delete the input chunks as a
      # success marker.
      CMD="ngram-count -order $ORDER -sort -text $file -write $file.${ORDER}grams"
      echo "test ! -f $file || ($CMD && rm $file)" >> $COUNT_CMDS_FILE
   done

   # Best to call ngram-count-big.sh instead of ngram-merge because if there is
   # only one file to merge ngram-merge is not smart enough to handle it
   # properly.
   sub_merge="$chunk.??.${ORDER}grams"
   CMD="ngram-count-big.sh -merge-only -order $ORDER $out_file $sub_merge"
   echo "test -f $out_file || ($CMD && rm $sub_merge)" >> $MERGE_CMDS_FILE
}

echo ""
echo Starting

global_counts_out=$1; shift
input_files=$@
merge_files=
if [ ! $MERGE_ONLY ]; then
   test -d $WORK_DIR || mkdir $WORK_DIR
   for in_file in $input_files; do
      out_file=`basename $in_file`
      out_file=${out_file%.gz}.${ORDER}grams.gz
      count_ngrams $out_file $in_file
      merge_files="$merge_files $out_file"
   done

   # if we need to calculate the count, do it
   if [ -e $COUNT_CMDS_FILE ]; then
      run run-parallel.sh $COUNT_CMDS_FILE $N
   fi

   # is we need to merge the counts of individual input files
   if [ -e $MERGE_CMDS_FILE ]; then
      run run-parallel.sh $MERGE_CMDS_FILE $N
   fi
else
   merge_files=$input_files
fi

# Merge all into one file
merge_ngrams $global_counts_out $merge_files

# We are done, do some clean up
test ! $DEBUG && test -d $WORK_DIR && rm -rf $WORK_DIR

echo ""
echo "Done"
date
