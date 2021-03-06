#!/bin/bash
# @file madamira-test.sh
# @brief Run the MADAMIRA part of this test suite
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada



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



if [[ ! -d $MADAMIRA_HOME ]]; then
   echo -e "${YELLOW}Skipping TestSuite since you don't have MADAMIRA installed${RESET}" >&2
   # It is not an error to skip madamira.py's testsuite.
   exit 0
fi

MADAMIRA_PY=${MADAMIRA_PY:-madamira.py}
which $MADAMIRA_PY &> /dev/null \
|| error_message "Can't find madamira.py"


function stop_madamira_server() {
   testcaseDescription "Stopping MADAMIRA."
   PID=`pgrep -f MADAMIRA`
   [[ -z $PID ]] \
   || kill $PID &> /dev/null \
   || echo "Cannot kill existing MADAMIRA instance; reusing it" >&2
}


function basic_usage() {
   set -o errexit
   testcaseDescription "Basic usage with MyD3 scheme."
   ridbom.sh \
      < $MADAMIRA_HOME/samples/raw/SampleTextInput.txt \
   | $MADAMIRA_PY \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      --scheme MyD3 \
   | diff --brief --ignore-all-space - $MADAMIRA_HOME/samples/raw/SampleTextInput.txt.MyD3.tok \
   || error_message "MyD3 scheme is not like our reference."
}


function invalid_scheme() {
   set -o errexit
   testcaseDescription "Using invalid scheme."
   ridbom.sh \
      < $MADAMIRA_HOME/samples/raw/SampleTextInput.txt \
   | $MADAMIRA_PY \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      --scheme INVALID_SCHEME \
   |& grep "scheme=INVALID_SCHEME" --quiet \
   || error_message "madamira.sh should've complained about not finding INVALID_SCHEME scheme."
}


function ascii() {
   set -o errexit
   testcaseDescription "Using ascii sentence"
   #/opt/PortageII/models/ar2en-0.4/plugins/tokenize_plugin  <<< "La tour Eiffel"
   #__ascii__La __ascii__tour __ascii__Eiffel

   $MADAMIRA_PY \
      -n -m \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< 'La tour Eiffel' \
   | grep '__ascii__La __ascii__tour __ascii__Eiffel' --quiet \
   || error_message "Invalid ascii output"
}


function ascii_hashtag() {
   set -o errexit
   testcaseDescription "ascii hashtag: Mark non Arabic words."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -n <<< "#La_tour_Eiffel"
   # __ascii__#La_tour_Eiffel
   $MADAMIRA_PY \
      -n \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< '#La_tour_Eiffel' \
   | grep '__ascii__#La_tour_Eiffel' --quiet \
   || error_message "Invalid ascii hashtag output (1)"


   testcaseDescription "ascii hashtag: xmlishify Arabic hashtags."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -m <<< "#La_tour_Eiffel"
   # <hashtag> La_tour_Eiffel </hashtag>
   $MADAMIRA_PY \
      -m \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< '#La_tour_Eiffel' \
   | grep '<hashtag> La tour Eiffel </hashtag>' --quiet \
   || error_message "Invalid ascii hashtag output (2)"


   testcaseDescription "ascii hashtag: xmlishify Arabic hashtags and Mark non Arabic words."
   # /opt/PortageII/models/ar2en-0.4/plugins/tokenize_plugin  <<< "#La_tour_Eiffel"
   # __ascii__#La_tour_Eiffel
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -m -n <<< "#La_tour_Eiffel"
   # __ascii__#La_tour_Eiffel
   $MADAMIRA_PY \
      -n -m \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< '#La_tour_Eiffel' \
   | grep '__ascii__#La_tour_Eiffel' --quiet \
   || error_message "Invalid ascii hashtag output (3)"
}


function arabic_hashtag() {
   set -o errexit

   testcaseDescription "Arabic hashtag: vanilla."
   #~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar <<< "#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ"
   # # dyslr _ b+ sbb
   $MADAMIRA_PY \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | grep '# ديسلر ـ ب+ سبب' --quiet \
   || error_message "Invalid Arabic hashtag output (0)"


   testcaseDescription "Arabic hashtag: Mark non Arabic words."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -n <<< "#ديسلر_بسبب"
   # # dyslr _ b+ sbb
   $MADAMIRA_PY \
      -n \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | grep '# ديسلر ـ ب+ سبب' --quiet \
   || error_message "Invalid Arabic hashtag output (1)"


   testcaseDescription "Arabic hashtag: xmlishify hashtags."
   # ~/sandboxes/PORTAGEshared/src/textutils/tokenize_plugin_ar ar -m <<< "#ديسلر_بسبب"
   # <hashtag> dyslr b+ </hashtag> sbb
   $MADAMIRA_PY \
      -m \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | grep '<hashtag> ديسلر ب+ سبب </hashtag>' --quiet \
   || error_message "Invalid Arabic hashtag output (2)"

   testcaseDescription "Arabic hashtag: Mark non Arabic words and xmlishify hashtags."
   #/opt/PortageII/models/ar2en-0.4/plugins/tokenize_plugin <<< "#ديسلر_بسبب"
   #<hashtag> dyslr b+ </hashtag> sbb
   $MADAMIRA_PY \
      -n -m \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< '#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ' \
   | grep '<hashtag> ديسلر ب+ سبب </hashtag>' --quiet \
   || error_message "Invalid Arabic hashtag output (3)"
}


function beginWithWaw() {
   set -o errexit
   testcaseDescription "Handling Waws at the beginning of a sentence."
   # Examples from:
   # /home/corpora/arabic-gigaword-v5/data/aaw_arb/aaw_arb_201012.gz
   $MADAMIRA_PY \
      -w \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< 'ﻮﺑﺮﻴﻃﺎﻨﻳﺍ، ﻭﺄﻗﺭ ﺐﺧﺭﻮﺟ ﻁﺭﺪﻴﻧ ﻒﻘﻃ ﻢﻧ ﺎﻠﻴﻤﻧ..' \
   | grep 'بريطانيا , و+ اقر ب+ خروج طردين فقط من اليمن . .' --quiet \
   || error_message "We should have removed the Waw at the beginning of the sentence."

   $MADAMIRA_PY \
      -w \
      --config $MADAMIRA_HOME/samples/sampleConfigFile.xml \
      <<< 'ﻭﺄﻤﻳﺮﻛﺍ، ﻮﻣﺍ ﺯﺎﻟ ﺎﻠﺒﺤﺛ ﺝﺍﺮﻳﺍ ﺐﻴﻧ ﻩﺬﻫ' \
   | grep 'اميركا , و+ ما زال البحث جاريا بين هذه' --quiet \
   || error_message "We should have removed the Waw at the beginning of the sentence."
}


function noConfig() {
   set -o errexit
   testcaseDescription "No specifying a configuration."
   $MADAMIRA_PY \
      <<< 'ﻭﺄﻤﻳﺮﻛﺍ، ﻮﻣﺍ ﺯﺎﻟ ﺎﻠﺒﺤﺛ ﺝﺍﺮﻳﺍ ﺐﻴﻧ ﻩﺬﻫ' \
   | grep 'اميركا , و+ ما زال البحث جاريا بين هذه' --quiet \
   || error_message "Invalid output when not specifying a config."
}


# madamira.py is intended to be used within a PortageLive setup where
# $PORTAGE=/opt/PortageII.  For this testsuite, we would like to log log4
# locally.  By defining PORTAGE to be the current directory, logging should
# happen in ./logs/.
export PORTAGE=`pwd`

mkdir -p logs

stop_madamira_server
basic_usage
invalid_scheme
ascii
ascii_hashtag
arabic_hashtag
beginWithWaw
noConfig

# let's have this test clean up behind itself and kill the madamira server.
stop_madamira_server

if [[ $ERROR_COUNT -gt 0 ]]; then
   error_message "Testsuite failed. ($0)"
   exit 1
else
   echo "All tests PASSED. ($0)"
fi
