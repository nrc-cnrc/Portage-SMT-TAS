#!/bin/bash
# @file stanseg-test.sh
# @brief Run the part of this test suite for the Stanford Segmenter
#
# @author Eric Joanis, with code liberally borrowed from Samuel Larkin's madamira-test.sh
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2019, Sa Majeste la Reine du Chef du Canada /
# Copyright 2019, Her Majesty in Right of Canada


readonly YELLOW='\e[00;33m'
readonly BOLDRED='\e[01;31m'
readonly RED='\e[0;31m'
readonly BOLD='\e[1m'
readonly RESET='\e[00m'

ERROR_COUNT=0

function testcaseDescription() {
   echo -e "${BOLD}Testcase:${RESET} $@" >&2
}

function error_message() {
   echo -e "${BOLDRED}Error: ${RED}$@${RESET}" >&2
   ERROR_COUNT=$((ERROR_COUNT + 1))
}

STANSEG_PY=${STANSEG_PY:-stanseg}
which $STANSEG_PY &> /dev/null \
|| error_message "Can't find stanseg.py"


function ascii() {
   set -o errexit
   testcaseDescription "Using ascii sentence"
   #/opt/PortageII/models/ar2en-0.4/plugins/tokenize_plugin  <<< "La tour Eiffel"
   #__ascii__La __ascii__tour __ascii__Eiffel

   $STANSEG_PY \
      -n -m \
      <<< 'La tour Eiffel' \
   | grep '__ascii__La __ascii__tour __ascii__Eiffel' --quiet \
   || error_message "Invalid ascii output"
}


function ascii_hashtag() {
   set -o errexit
   testcaseDescription "ascii hashtag: Mark non Arabic words."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -n <<< "#La_tour_Eiffel"
   # __ascii__#La_tour_Eiffel
   $STANSEG_PY \
      -n \
      <<< '#La_tour_Eiffel' \
   | grep '__ascii__#La_tour_Eiffel' --quiet \
   || error_message "Invalid ascii hashtag output (1)"


   testcaseDescription "ascii hashtag: xmlishify Arabic hashtags."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -m <<< "#La_tour_Eiffel"
   # <hashtag> La_tour_Eiffel </hashtag>
   $STANSEG_PY \
      -m \
      <<< '#La_tour_Eiffel' \
   | grep '<hashtag> La tour Eiffel </hashtag>' --quiet \
   || error_message "Invalid ascii hashtag output (2)"


   testcaseDescription "ascii hashtag: xmlishify Arabic hashtags and Mark non Arabic words."
   # /opt/PortageII/models/ar2en-0.4/plugins/tokenize_plugin  <<< "#La_tour_Eiffel"
   # __ascii__#La_tour_Eiffel
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -m -n <<< "#La_tour_Eiffel"
   # __ascii__#La_tour_Eiffel
   $STANSEG_PY \
      -n -m \
      <<< '#La_tour_Eiffel' \
   | grep '__ascii__#La_tour_Eiffel' --quiet \
   || error_message "Invalid ascii hashtag output (3)"
}


function arabic_hashtag() {
   set -o errexit

   testcaseDescription "Arabic hashtag: vanilla."
   #~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar <<< "#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ"
   # # dyslr _ b+ sbb
   $STANSEG_PY \
      <<< '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | grep '# ديسلر ـ ب+ سبب' --quiet \
   || error_message "Invalid Arabic hashtag output (0)"


   testcaseDescription "Arabic hashtag: Mark non Arabic words."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -n <<< "#ديسلر_بسبب"
   # # dyslr _ b+ sbb
   $STANSEG_PY \
      -n \
      <<< '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | grep '# ديسلر ـ ب+ سبب' --quiet \
   || error_message "Invalid Arabic hashtag output (1)"


   testcaseDescription "Arabic hashtag: xmlishify hashtags."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -m <<< "#ديسلر_بسبب"
   # <hashtag> dyslr b+ </hashtag> sbb
   $STANSEG_PY \
      -m \
      <<< '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | grep '<hashtag> ديسلر ب+ سبب </hashtag>' --quiet \
   || error_message "Invalid Arabic hashtag output (2)"

   testcaseDescription "Arabic hashtag: Mark non Arabic words and xmlishify hashtags."
   #/opt/PortageII/models/ar2en-0.4/plugins/tokenize_plugin <<< "#ديسلر_بسبب"
   #<hashtag> dyslr b+ </hashtag> sbb
   $STANSEG_PY \
      -n -m \
      <<< '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | grep '<hashtag> ديسلر ب+ سبب </hashtag>' --quiet \
   || error_message "Invalid Arabic hashtag output (3)"
}


function beginWithWaw() {
   set -o errexit
   testcaseDescription "Handling Waws at the beginning of a sentence."
   # Examples from:
   # /home/corpora/arabic-gigaword-v5/data/aaw_arb/aaw_arb_201012.gz
   $STANSEG_PY \
      -w \
      <<< 'ﻮﺑﺮﻴﻃﺎﻨﻳﺍ، ﻭﺄﻗﺭ ﺐﺧﺭﻮﺟ ﻁﺭﺪﻴﻧ ﻒﻘﻃ ﻢﻧ ﺎﻠﻴﻤﻧ..' \
   | grep 'بريطانيا , و+ اقر ب+ خروج طردين فقط من اليمن . .' --quiet \
   || error_message "We should have remove the Waw at the beginning of the sentence."

   $STANSEG_PY \
      -w \
      <<< 'ﻭﺄﻤﻳﺮﻛﺍ، ﻮﻣﺍ ﺯﺎﻟ ﺎﻠﺒﺤﺛ ﺝﺍﺮﻳﺍ ﺐﻴﻧ ﻩﺬﻫ' \
   | grep 'اميركا , و+ ما زال البحث جاريا بين هذه' --quiet \
   || error_message "We should have remove the Waw at the beginning of the sentence."
}

ascii
ascii_hashtag
arabic_hashtag
beginWithWaw

if [[ $ERROR_COUNT -gt 0 ]]; then
   error_message "Testsuite failed."
   exit 1
else
   echo "All tests PASSED."
fi
