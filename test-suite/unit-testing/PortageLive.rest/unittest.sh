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

readonly server_ip=127.0.0.1
readonly server_port=8765

function verbose() {
   echo $'\n'============= $* =============
}

function deploy_code() {
   verbose deploy_code
   [[ -d plive ]] && rm -fr plive
   cp ../../../PortageLive/www/rest/translate.php .
   cp ../../../PortageLive/www/rest/incrAddSentence.php .
}

function local_testase() {
   verbose local_testase
   php \
      --define 'include_path=../../../PortageLive/www:../../../PortageLive/www/rest' \
      $PHPUNIT_HOME/phpunit-4.8.phar  \
      --colors=always  \
      tests/incrAddSentence.php
}

function start_php_server() {
   verbose 'Starting php web server....'
   deploy_code
   # elinks 'http://127.0.0.1:8765/incrAddSentence.php?document_level_model_ID=5&source=S&target=T'
   php \
      --define 'include_path=../../../PortageLive/www:../../../PortageLive/www/rest' \
      --server $server_ip:$server_port \
      --docroot . \
      &> log.server &
   server=$!
   sleep 1
   #trap "kill -9 $server; rm -fr plive" EXIT
   trap "kill -9 $server" EXIT
}

function remote_testcase() {
   verbose remote_testcase
   # Runnig https://github.com/chitamoor/Rester
   apirunner --ts tests/testSuite.yaml
}

function curl_testcase() {
   verbose curl_testcase
   # Web request using unicode.
   export CORPORA=./plive/DOCUMENT_LEVEL_MODEL_PORTAGE_UNITTEST_4da35/corpora
   #[[ -s $CORPORA ]] && rm -f $CORPORA
   local tag=`date +"%T"`
   curl \
      --silent \
      --get \
      --data 'context=unittest.rev.en-fr' \
      --data 'source=S%C9' \
      --data "target=GET$tag" \
      --data 'document_level_model_ID=PORTAGE_UNITTEST_4da35' \
      "http://$server_ip:$server_port/incrAddSentence.php" \
   | grep --quiet '{"result":true}' \
   || ! echo "Error Adding sentence pairs" >&2

   curl \
      --silent \
      --get \
      --data 'context=unittest.rev.en-fr' \
      --data 'source=S%C9' \
      --data "target=GET$tag" \
      --data 'document_level_model_ID=PORTAGE_UNITTEST_4da35' \
      "http://$server_ip:$server_port/incrAddSentence.php" \
   | grep --quiet '{"result":true}' \
   || ! echo "Error Adding sentence pairs" >&2

   grep --quiet "GET$tag" $CORPORA \
   || ! echo $'\nError: Cannot find entry.' >&2
}

function curl_post_testcase() {
   verbose curl_post_testcase
   # Web request using unicode.
   export CORPORA=./plive/DOCUMENT_LEVEL_MODEL_PORTAGE_UNITTEST_4da35/corpora
   #[[ -s $CORPORA ]] && rm -f $CORPORA
   local tag=`date +"%T"`

   curl \
      --silent \
      --data 'context=unittest.rev.en-fr' \
      --data 'source=S%C9' \
      --data "target=POST$tag" \
      --data 'document_level_model_ID=PORTAGE_UNITTEST_4da35' \
      "http://$server_ip:$server_port/incrAddSentence.php" \
   | grep --quiet '{"result":true}' \
   || ! echo "Error Adding sentence pairs" >&2

   grep --quiet "POST$tag" $CORPORA \
   || ! echo $'\nError: Cannot find entry.' >&2
}

function lint_php() {
   verbose lint_php
   for c in PortageLiveLib.php rest/incrAddSentence.php; do
      php \
         --define 'include_path=../../../PortageLive/www:../../../PortageLive/www/rest' \
         --syntax-check \
         ../../../PortageLive/www/$c \
      || ! echo "Error in linting $f" >&2
   done
}


rm -fr plive
RC=0
lint_php || RC=1
start_php_server || RC=1
local_testase || RC=1
remote_testcase || RC=1
curl_testcase || RC=1
curl_post_testcase || RC=1

echo
exit $RC

# CLI php will fail because it can't create a directory under /var/www/html/
#QUERY_STRING='source=SÉ&target=T&document_level_model_ID=curl' php incrAddSentence.php
#QUERY_STRING='source=SÉ&target=T&document_level_model_ID=curl' php -e -r 'parse_str(getenv("QUERY_STRING"), $_GET); include "incrAddSentence.php";'

