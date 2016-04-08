#!/bin/bash

# @file plive-optimize-pretrained.sh
# @brief Optimize pretrained models in a PortageLive model by linking to the
#        shared copy in /opt/PortageII/pretrained instead of having a copy
#        in each model
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada

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

Usage: $PROG PLIVE_MODEL

  E.g. $PROG /opt/PortageII/models/mymodel

  Optimize the pretrained models in PLIVE_MODEL by replacing copies of them by
  links to the shared copy, if found in /PLIVEPATH/models/pretrained/.

Options

  -n(otreally) show what would happen but don't do it
  -h(elp)      print this help message
  -v(erbose)   increment the verbosity level by 1 (may be repeated)
  -d(ebug)     print debugging information
==EOF==

   exit 1
}

NOTREALLY=
VERBOSE=0
DEBUG=
while [ $# -gt 0 ]; do
   case "$1" in
   -n|-notreally)       NOTREALLY=1;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1; VERBOSE=$(( $VERBOSE + 1 ));;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done
[[ $# -eq 1 ]] || error_exit "Missing model argument"
MODEL=$1
shift

[[ -d $MODEL ]] || error_exit "Cannot find PortageLive model $MODEL, or it is not a directory"
[[ -r $MODEL/canoe.ini.cow ]] || error_exit "No $MODEL/canoe.ini.cow, is $MODEL really a PortageLive model?"

PRETRAINED_DIR=$MODEL/../pretrained
[[ -d $PRETRAINED_DIR ]] ||
   error_exit 'Cannot find directory "pretrained" next to' \
              "$MODEL." \
              'Pretrained models should be found in a directory called "pretrained" at the' \
              'same level as your models, e.g., /opt/PortageII/models/pretrained/.'
PRETRAINED_DIR=`readlink -f $PRETRAINED_DIR`
[[ $DEBUG ]] && echo PRETRAINED_DIR=$PRETRAINED_DIR

RC=0
REPLACED_A_MODEL=
TPLMS=`find -H $MODEL -type d -name \*.tplm`
for TPLM in $TPLMS; do
   [[ $DEBUG ]] && echo TPLM=$TPLM
   TPLM_BASENAME=`basename $TPLM`
   for PRETR_TPLM in `find -L $PRETRAINED_DIR -type d -name $TPLM_BASENAME`; do
      [[ $DEBUG ]] && echo PRETR_TPLM=$PRETR_TPLM
      if diff -qr $TPLM $PRETR_TPLM &> /dev/null; then
         echo ""
         echo Replacing $TPLM by identical pretrained model $PRETR_TPLM
         [[ $VERBOSE -gt 0 ]] && echo "   mv $TPLM $TPLM-removed"
         if [[ ! $NOTREALLY ]]; then
            if ! mv $TPLM $TPLM-removed; then
               echo "Error renaming $TPLM; skipping this model"
               RC=1
               break
            fi
         fi
         [[ $VERBOSE -gt 0 ]] && echo "   ln -s $PRETR_TPLM $TPLM"
         if [[ ! $NOTREALLY ]]; then
            if ! ln -s $PRETR_TPLM $TPLM; then
               echo "Error linking pretrained model; your models may not work properly anymore."
               RC=1
               break
            fi
         fi
         REPLACED_A_MODEL=1
         break
      else
         [[ $VERBOSE -gt 0 ]] &&
         echo Ignoring pretrained model $PRETR_TPLM with same name as $TPLM, but which is not identical.
      fi
   done
done

echo

if [[ $RC -ne 0 ]]; then
   echo "Something went wrong. Your model is likely not optimized properly."
elif [[ ! $REPLACED_A_MODEL ]]; then
   echo "No models found in $MODEL that can be replaced by pretrained ones."
fi


exit $RC
