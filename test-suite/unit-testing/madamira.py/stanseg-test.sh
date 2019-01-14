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
VERBOSE="cat" # Quiet mode
VERBOSE="tee /dev/stderr" # Verbose mode

function testcaseDescription() {
   echo -e "${BOLD}Testcase:${RESET} $@" >&2
}

function error_message() {
   echo -e "${BOLDRED}Error: ${RED}$@${RESET}" >&2
   ERROR_COUNT=$((ERROR_COUNT + 1))
}

if [[ ! -d $STANFORD_SEGMENTER_HOME ]]; then
   echo -e "${YELLOW}Skipping TestSuite since you don't have the Stanford Segmenter installed${RESET}" >&2
   # It is not an error to skip stanseg.py's testsuite.
   exit 0
fi

STANSEG_PY=${STANSEG_PY:-stanseg.py}
which $STANSEG_PY &> /dev/null \
|| error_message "Can't find stanseg.py"


function ascii() {
   set -o errexit
   testcaseDescription "Using ascii sentence"
   #/opt/PortageII/models/ar2en-0.4/plugins/tokenize_plugin  <<< "La tour Eiffel"
   #La tour Eiffel

   $STANSEG_PY \
      <<< 'La tour Eiffel a<b>c R&D' \
   | $VERBOSE \
   | grep 'La tour Eiffel a < b > c R & D' --quiet \
   || error_message "Invalid ascii output (1)"


   testcaseDescription "Using ascii sentence with xml markup"
   #/opt/PortageII/models/ar2en-0.4/plugins/tokenize_plugin  <<< "La tour Eiffel"
   #La tour Eiffel

   $STANSEG_PY \
      -m \
      <<< 'La tour Eiffel a<b>c R&D' \
   | $VERBOSE \
   | grep 'La tour Eiffel a &lt; b &gt; c R &amp; D' --quiet \
   || error_message "Invalid ascii output (2)"
}


function ascii_hashtag() {
   set -o errexit
   testcaseDescription "ascii hashtag: Mark non Arabic words."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -n <<< "#La_tour_Eiffel"
   # #La_tour_Eiffel
   $STANSEG_PY \
      <<< '#La_tour_Eiffel  a<b>c R&D ' \
   | $VERBOSE \
   | grep '# La tour Eiffel a < b > c R & D' --quiet \
   || error_message "Invalid ascii hashtag output (1)"


   testcaseDescription "ascii hashtag: xmlishify Arabic hashtags."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -m <<< "#La_tour_Eiffel"
   # <hashtag> La_tour_Eiffel </hashtag>
   $STANSEG_PY \
      -m \
      <<< '#La_tour_Eiffel a<b>c R&D' \
   | $VERBOSE \
   | grep '<hashtag> La tour Eiffel </hashtag> a &lt; b &gt; c R &amp; D' --quiet \
   || error_message "Invalid ascii hashtag output (2)"
}


function arabic_hashtag() {
   set -o errexit

   testcaseDescription "Arabic hashtag: vanilla."
   #~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar <<< "#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ"
   # # dyslr _ b+ sbb
   echo '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | normalize-unicode.pl ar \
   | $STANSEG_PY \
   | $VERBOSE \
   | grep '# ديسلربسبب' --quiet \
   || error_message "Invalid Arabic hashtag output (0)"

   testcaseDescription "Arabic hashtag: xmlishify hashtags."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -m <<< "#ديسلر_بسبب"
   # <hashtag> dyslr b+ </hashtag> sbb
   echo '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | normalize-unicode.pl ar \
   | $STANSEG_PY \
      -m \
   | $VERBOSE \
   | grep '<hashtag> ديسلر ب+ سبب </hashtag>' --quiet \
   || error_message "Invalid Arabic hashtag output (1)"
}


function beginWithWaw() {
   set -o errexit
   testcaseDescription "Handling Waws at the beginning of a sentence."
   # Examples from:
   # /home/corpora/arabic-gigaword-v5/data/aaw_arb/aaw_arb_201012.gz
   echo 'ﻮﺑﺮﻴﻃﺎﻨﻳﺍ، ومحيط ﺐﺧﺭﻮﺟ ﻁﺭﺪﻴﻧ ﻒﻘﻃ ﻢﻧ ﺎﻠﻴﻤﻧ..' \
   | normalize-unicode.pl ar \
   | $STANSEG_PY \
      -w \
   | $VERBOSE \
   | grep 'بريطانيا , و+ محيط ب+ خروج طردين فقط من اليمن ..' --quiet \
   || error_message "We should have removed the Waw at the beginning of the sentence.  (new)"

   echo 'ﻭﺄﻤﻳﺮﻛﺍ، ﻮﻣﺍ ﺯﺎﻟ ﺎﻠﺒﺤﺛ ﺝﺍﺮﻳﺍ ﺐﻴﻧ ﻩﺬﻫ' \
   | normalize-unicode.pl ar \
   | $STANSEG_PY \
      -w \
   | $VERBOSE \
   | grep 'اميركا , و+ +ما زال البحث جاريا بين هذه' --quiet \
   || error_message "We should have removed the Waw at the beginning of the sentence."
}

ascii
ascii_hashtag
arabic_hashtag
beginWithWaw

if [[ $ERROR_COUNT -gt 0 ]]; then
   error_message "Testsuite failed. ($0)"
   exit 1
else
   echo "All tests PASSED. ($0)"
fi
