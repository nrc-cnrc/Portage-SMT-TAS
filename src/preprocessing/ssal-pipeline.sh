#!/bin/bash

# @file ssal-pipeline.sh 
# @brief Run a full sentence alignment pipeline on a pair of files
# 
# @author Eric Joanis
# 
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2020, Sa Majeste la Reine du Chef du Canada /
# Copyright 2020, Her Majesty in Right of Canada

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

Usage: $PROG [-c CONFIG] [options] L1_FILE L2_FILE

  Run a full sentence alignment pipeline on L1_FILE and L2_FILE:
   1. Clean the corpus files
   2. Align paragraphs, using the IBM model provided
   3. Sentence split
   4. Tokenize
   5. Align sentences within paragraphs, using the IBM model provided

  The output will be cleaned, non-tokenized sentences.

  Config file CONFIG should contain the configuration settings parameters for
  the language pair, as bash variable definitions.
  These variables must be defined:
     L1_CLEAN, L2_CLEAN: commands to clean the input
     L1_SS, L2_SS: commands to sentence split
     L1_TOK, L2_TOK: commands to tokenized and maybe lowercase for the IBM model
     IBM_L1_GIVEN_L2, IBM_L2_GIVEN_L1: IBM models
  E.g.:
     L1=en
     L2=iu
     L1_CLEAN="clean-utf8-text.pl"
     L2_CLEAN="clean-utf8-text.pl | normalize-iu-spelling.pl"
     L1_SS="utokenize.pl -lang=en -notok -ss -paraline -p"
     L2_SS="utokenize.pl -lang=iu -notok -ss -paraline -p"
     MODELS=/space/project/portage/corpora/Inuktitut-2017/Hansard-Nunavut-2017/Hansard-best
     L1_BPE=\$MODELS/02-oppl-clean/Hansard-en-iu.bpe-lc
     L2_BPE=\$MODELS/02-oppl-clean/Hansard-en-iu.bpe-lc
     L1_TOK="utokenize.pl -lang=en -noss | utf8_casemap | subword-nmt apply-bpe -c \$L1_BPE | sed 's/  *$//'"
     L2_TOK="utokenize.pl -lang=iu -noss | utf8_casemap | subword-nmt apply-bpe -c \$L2_BPE | sed 's/  *$//'"
     IBM_L1_GIVEN_L2=\$MODELS/06-ibm-model/hmm.en_given_iu.bin
     IBM_L2_GIVEN_L1=\$MODELS/06-ibm-model/hmm.iu_given_en.bin

Options:

  -c CONFIG    Config file to read
  -n(otreally) Show what will happen but don't run anything
  -header N    Assume the first N lines of the files are fixed headers.
  -h(elp)      print this help message
  -v(erbose)   increment the verbosity level by 1 (may be repeated)
  -d(ebug)     print debugging information

==EOF==

   exit 1
}

VERBOSE=0
CONFIG_FILE=""
while [[ $# -gt 0 ]]; do
   case "$1" in
   -c)                  arg_check 1 $# $1; CONFIG_FILE=$2; shift;;
   -n|-notreally)       NOTREALLY=1;;
   -header)             arg_check 1 $# $1; HEADER=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

[[ $CONFIG_FILE ]] || error_exit "-c CONFIG argument is required."
[[ -r $CONFIG_FILE ]] || error_exit "Cannot read config file $CONFIG_FILE."
[[ $# -eq 2 ]] || error_exit "Exactly two arguments are required: L1_FILE and L2_FILE."
L1_FILE=$1
L2_FILE=$2

source $CONFIG_FILE || or error_exit "Error reading config file $CONFIG_FILE."

WD=`mktemp -d ssal-pipeline.tmp.XXXX` || error_exit "Cannot create temporary working directory."
[[ $VERBOSE > 0 ]] && echo "Working Directory $WD"

set -o pipefail

r() {
   [[ $VERBOSE > 0 || $NOTREALLY ]] && echo "$1"
   if [[ ! $NOTREALLY ]]; then
      eval "$1"
      rc=$?
      [[ $rc == 0 ]] || error_exit "Exit status $rc is not zero - aborting."
   fi
}


# Step 1: clean the input
r "cat $L1_FILE | $L1_CLEAN > $WD/l1.clean"
r "cat $L2_FILE | $L2_CLEAN > $WD/l2.clean"

# Remove the header if requested
if [[ $HEADER ]]; then
   r "tail +$((1+$HEADER)) $WD/l1.clean > $WD/l1.clean-body"
   r "tail +$((1+$HEADER)) $WD/l2.clean > $WD/l2.clean-body"
   CLEAN_IN=clean-body
else
   CLEAN_IN=clean
fi

# Step 2: align paragraphs using the IBM model
r "cat $WD/l1.$CLEAN_IN | $L1_TOK > $WD/l1.p.tok"
r "cat $WD/l2.$CLEAN_IN | $L2_TOK > $WD/l2.p.tok"

r "ssal -ibm_1g2 $IBM_L1_GIVEN_L2 -ibm_2g1 $IBM_L2_GIVEN_L1 -a $WD/p.scores $WD/l1.p.tok $WD/l2.p.tok"

r "select-lines.py -a 1 --joiner=$'\\n' --separator=$'\\n__PARAGRAPH__\\n' $WD/p.scores $WD/l1.$CLEAN_IN > $WD/l1.p.al"
r "select-lines.py -a 2 --joiner=$'\\n' --separator=$'\\n__PARAGRAPH__\\n' $WD/p.scores $WD/l2.$CLEAN_IN > $WD/l2.p.al"

# Step 3: align sentences within paragraphs using the IBM model
r "cat $WD/l1.p.al | $L1_SS > $WD/l1.s"
r "cat $WD/l2.p.al | $L2_SS > $WD/l2.s"

L1_PARAGRAPH_STR=$(echo __PARAGRAPH__ | eval $L1_TOK)
L2_PARAGRAPH_STR=$(echo __PARAGRAPH__ | eval $L2_TOK)
r "cat $WD/l1.s | $L1_TOK | sed 's/$L1_PARAGRAPH_STR/__PARAGRAPH__/' > $WD/l1.s.tok"
r "cat $WD/l2.s | $L2_TOK | sed 's/$L2_PARAGRAPH_STR/__PARAGRAPH__/' > $WD/l2.s.tok"

r "ssal -hm __PARAGRAPH__ -fm -ibm_1g2 $IBM_L1_GIVEN_L2 -ibm_2g1 $IBM_L2_GIVEN_L1 -a $WD/s.scores $WD/l1.s.tok $WD/l2.s.tok"

r "select-lines.py -a 1 $WD/s.scores $WD/l1.s | sed -e 's/^ *//' -e 's/ *\$//' > $WD/l1.s.al"
r "select-lines.py -a 2 $WD/s.scores $WD/l2.s | sed -e 's/^ *//' -e 's/ *\$//' > $WD/l2.s.al"

[[ $DEBUG ]] || rm -rf $WD
