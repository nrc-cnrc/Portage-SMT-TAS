#!/bin/bash

# @file rest/unittest.sh
# @brief Make running the unittests easier.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada



function deploy_code() {
   cp ../PortageLiveLib.php /var/www/html/
   cp incrAddSentence.php /var/www/html/
   cp translate.php /var/www/html/language/translate/
   cp ../../../src/utils/incr-add-sentence.sh /opt/PortageII/bin/
}

function local_testase() {
   php \
      -d 'include_path=.:..' \
      $PHPUNIT_HOME/phpunit-4.8.phar  \
      --colors=always  \
      tests/incrAddSentence.php
}

function remote_testcase() {
   # Runnig https://github.com/chitamoor/Rester
   apirunner --ts tests/testSuite.yaml
}

function curl_testcase() {
   # Web request using unicode.
   export CORPORA=/var/www/html/plive/DOCUMENT_LEVEL_MODEL_curl/corpora
   #[[ -s $CORPORA ]] && rm -f $CORPORA
   tag=`date +"%T"`
   curl \
      --get \
      --data 'source=SÉ' \
      --data "target=T$tag" \
      --data 'document_level_model_ID=curl' \
      http://localhost/incrAddSentence.php
   curl \
      --silent \
      --get \
      --data 'source=SÉ' \
      --data "target=T$tag" \
      --data 'document_level_model_ID=curl' \
      http://localhost/incrAddSentence.php \
      | grep --quiet '{"result":true}' \
      || ! echo "Error" &>2
   grep --quiet "T$tag" $CORPORA \
   || ! echo $'\nError: Cannot find entry.' >&2
}


[[ $DEPLOY ]] && deploy_code
local_testase
remote_testcase
curl_testcase

exit

# CLI php will fail because it can't create a directory under /var/www/html/
#QUERY_STRING='source=SÉ&target=T&document_level_model_ID=curl' php incrAddSentence.php
#QUERY_STRING='source=SÉ&target=T&document_level_model_ID=curl' php -e -r 'parse_str(getenv("QUERY_STRING"), $_GET); include "incrAddSentence.php";'

