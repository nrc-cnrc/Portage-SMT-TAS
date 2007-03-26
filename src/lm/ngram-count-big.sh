#!/bin/bash

# ngram-count-big.sh - wrapper around ngram-count to make large n-gram count
#                      files using the sorted-count / merge process.  Intended
#                      to replace srilm's make-big-lm script, dealing with all
#                      the bookkeeping automatically, and splitting very large
#                      files automatically.
#
# PROGRAMMER: Eric Joanis
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada
# Copyright 2006, Her Majesty in Right of Canada

echo 'ngram-count-big.sh, NRC-CNRC, (c) 2006 - 2007, Her Majesty in Right of Canada'

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

ORDER=3
CHUNK_SIZE=250m
while [ $# -gt 0 ]; do
   case "$1" in
   -c|-chunk-size)      arg_check 1 $# $1; CHUNK_SIZE=$2; shift;;
   -order)              arg_check 1 $# $1; ORDER=$2; shift;;
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

# Echo a command and then run it
run () {
   cmd="$*"
   echo ""
   echo '#' `date`
   echo "$cmd"
   if [ ! $NOT_REALLY ]; then
      eval $cmd
   fi
}

# merge_ngrams out_file in_file1 in_file2 ...
# will merge the n-gram counts files in_file1, in_file2, ... into a single one,
# out_file.
merge_ngrams() {
   out_file=$1
   shift
   temp_prefix="$out_file.partial-counts"
   temp_index=0
   declare -a next_list
   #echo initial arg list: "$@"

   # With only one file to merge, copy it, preserving the correctness of the
   # assumption that .gz files are gzipped and others aren't.
   if [ $# = 1 ]; then
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

   # Normal case - merge the count files in a binary tree way (taking 3 files
   # at once at the end of odd-length lists)
   last_iter=
   while [ $# -gt 1 ]; do
      if [ $# -lt 4 ]; then
         last_iter=1
      fi
      next_list=()
      while [ $# -gt 3 -o $# = 2 ]; do
         a=$1; b=$2; shift; shift;
         if [ $last_iter ]; then
            out=$out_file
         else
            temp_index=$((temp_index + 1))
            out=$temp_prefix.$temp_index
         fi
         #echo merging $a and $b into $out
         run ngram-merge -write $out $a $b
         if [ $? != 0 ]; then
            echo error in ngram-merge
         fi
         next_list=("${next_list[@]}" "$out")
      done
      if [ $# = 3 ]; then
         a=$1 b=$2 c=$3; shift; shift; shift;
         if [ $last_iter ]; then
            out=$out_file
         else
            temp_index=$((temp_index + 1))
            out=$temp_prefix.$temp_index
         fi
         #echo merging $a, $b and $c into $out
         run ngram-merge -write $out $a $b $c
         if [ $? != 0 ]; then
            echo error in ngram-merge
         fi
         next_list=("${next_list[@]}" "$out")
      elif [ $# = 1 ]; then
         echo This should never happen.
         next_list=("${next_list[@]}" "$1")
         shift
      fi

      set - "${next_list[@]}"
      #echo new arg list: "$@"
   done
   if [ ! $DEBUG ]; then
      run rm -f $temp_prefix.*
   fi
}

# count_ngrams outfile infile
# Do the actual ngram-count, splitting if necessary, for a given text file
count_ngrams() {
   out_file=$1
   text=$2
   out_prefix=`basename $text`
   run gzip -cdqf < $text \| split -C $CHUNK_SIZE - $out_prefix.$CHUNK_SIZE.
   if [ $? != 0 ]; then
      error_exit "split failed - bailing out"
   fi
   ls -l $out_prefix.$CHUNK_SIZE.??
   for file in $out_prefix.$CHUNK_SIZE.?? ; do
      run ngram-count -order $ORDER \
                      -sort \
                      -text $file \
                      -write $file.${ORDER}grams
      if [ $? = 0 ]; then
         if [ ! $DEBUG ]; then
            run rm $file
         fi
      else
         error_exit "ngram-count failed - bailing out"
      fi
   done

   merge_ngrams $out_file $out_prefix.$CHUNK_SIZE.??.${ORDER}grams
   if [ $? = 0 ]; then
      if [ ! $DEBUG ]; then
         run rm $out_prefix.$CHUNK_SIZE.??.${ORDER}grams
      fi
   else
      error_exit "error in merge_ngrams - bailing out"
   fi
}

echo ""
echo Starting

global_counts_out=$1
shift
input_files=$@
output_files=
for in_file in $input_files; do
   out_file=`basename $in_file`
   out_file=${out_file%.gz}.${ORDER}grams.gz
   if [ ! $MERGE_ONLY ]; then
      count_ngrams $out_file $in_file
   fi
   output_files="$output_files $out_file"
done

merge_ngrams $global_counts_out $output_files

echo ""
echo "Done"
date
