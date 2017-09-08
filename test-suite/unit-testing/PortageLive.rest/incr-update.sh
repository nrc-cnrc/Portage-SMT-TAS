#!/bin/bash
# @file incr-update.sh stub
# @brief Unit testing stub for incr-update.sh
#
# Calls the real incr-update.sh if the model isn't unittest.rev.en-fr.
#
# @author Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

if [[ "$@" = *unittest.rev.en-fr/canoe.ini.cow* ]]; then
   rm -f witness
   sleep 2
   echo "Training is done" > witness
else
   me=$(which incr-update.sh)
   save_path=$PATH
   export PATH=${PATH#*:}
   orig_incr_update=$(which incr-update.sh)
   export PATH=$save_path
   if [[ $orig != $me ]]; then
      $orig_incr_update $@
   fi
fi
