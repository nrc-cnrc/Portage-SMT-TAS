#!/bin/bash

# @file rescore-bwd-lm.sh 
# @brief bash script using gen_feature_values to score the nbest according to
# backward language model.
#
# USAGE: in rescore-model.ini
#        SCRIPT:"rescore-bwd-lm.sh <src> <nbest> your_backward_lm"
# 
# @author Samuel Larkin
# 
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada


if [[ $# -ne 3 ]]; then
    [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
    echo "Usage: $PROG  <src corpus>  <N-best list>  <backward LM file>"
    echo ""
    echo "Inverts all translations in N-best list and rescores with given backward LM"
    if [[ $1 != "-h" ]]; then
        echo "Your call: $*"
    fi
    exit 1
fi

src=$1
Nbest=$2
lm=$3

# WARNING not in parallel
# reverse the nbest on the fly, no tmp objects
gen_feature_values NgramFF $lm $src "zcat $Nbest | perl -nle 'print join(\" \", reverse split(/\s+/))' |"
# gen_feature_values feature arg src nbest

