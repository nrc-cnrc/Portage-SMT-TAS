#!/bin/bash

# @file mx-calc-distances.sh 
# @brief Calculate text distances, according to some metric, from a given list
# of corpus components to a given text file.
# 
# @author George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
# Copyright 2006, Her Majesty in Right of Canada

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/tpt directory
   BIN="`dirname $BIN`/utils"
fi
source $BIN/sh_utils.sh

print_nrc_copyright mx-calc-distances.sh 2006
export PORTAGE_INTERNAL_CALL=1

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

mx-calc-distances.sh [-v][-a args][-d pfx][-e ext] metric components textfile 

Calculate text distances, according to some metric, from a given list of corpus
components to a given text file. NB: higher values indicate a better match.
<components> is a file that lists corpus components (these are basenames of
files whose complete paths are specified by -d and -e). <metric> is one of:

   uniform - assign distance of 1 for each component
   ppx     - calculate perplexities according to LMs in components, using:
             multi-ngram-ppx <args> <models> <textfile>
   em      - calculate EM weights according to LMs in components, using:
             train_lm_mixture <args> <models> <textfile>
   em-sri  - calculate EM weights according to LMs in components, using:
             train-lm-mixture <args> <models> <textfile> (SRILM-based script)
             (use this option only if you have a valid SRILM licence)
   tfidf   - calculate tf/idf distances from vocs in components, using:
             get_voc -c <textfile> > vocfile
             cosine_distances <args> <models> vocfile

Output is written to stdout, one distance per line in <components>.

Options:

-v  increment the verbosity level by 1 (may be repeated)
-d  prepend <pfx> to the pathname for component files 
    (NB: pfx should contain '/' if necessary)
-e  append <ext> to the pathname for component files
-a  pass argument(s) <args> to distance calculation 

==EOF==

    exit 1
}

# Command line processing

VERBOSE=0
cmpt_pfx=
cmpt_ext=
args=

while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)            usage;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d)                  arg_check 1 $# $1; cmpt_pfx=$2; shift;;
   -e)                  arg_check 1 $# $1; cmpt_ext=$2; shift;;
   -a)                  arg_check 1 $# $1; args=$2; shift;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

if [ $# -ne 3 ]; then
   error_exit "Expecting 3 arguments!"
fi

metric=$1
components=$2
textfile=$3

# Check arguments

if [ ! -r $components ]; then error_exit "Can't read file $components"; fi
if [ ! -r $textfile ]; then error_exit "Can't read file $textfile"; fi

tmp=`/usr/bin/uuidgen`
models="models.$tmp"
(
    for m in `cat $components`; do
        echo $cmpt_pfx$m$cmpt_ext
    done
) > $models

# Run

export LC_ALL=C

case $metric in
   uniform)
        cat $components | perl -ne 'print "1\n";' ;;
   ppx)
        eval multi-ngram-ppx $args $models $textfile |
            perl -ne 's/.*ppl1=\s*(\S+).*/$1/;
	              $x=1.0/(1e-10+$_);
                      print "$x\n"' ;;
   em-sri)
        eval train_lm_mixture $args $models $textfile 2> /dev/null ;;
   em)
        eval train_lm_mixture $args $models $textfile 2> /dev/null ;;
   tfidf)
        get_voc -c $textfile > voc.$tmp
        eval cosine_distances $args $models voc.$tmp
        rm voc.$tmp ;;

   *)        error_exit "Unknown metric <$metric>!"
esac

# Cleanup 

rm $models
