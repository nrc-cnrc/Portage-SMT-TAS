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

function verbose() {
   echo $'\n'============= $* =============
}

function deploy_code() {
   verbose deploy_code
   rm -fr plive
   mkdir -p plive
   cp ../../../PortageLive/www/rest/translate.php .
   cp ../../../PortageLive/www/rest/incrAddSentence.php .
}

function phpunit_testcase() {
   verbose phpunit_testcase
   php \
      --define 'include_path=../../../PortageLive/www:../../../PortageLive/www/rest' \
      $PHPUNIT_HOME/phpunit-4.8.phar  \
      --colors=always  \
      tests/incrAddSentence.php
}

function start_php_server() {
   verbose 'Starting php web server ....'
   deploy_code
   # elinks 'http://127.0.0.1:8765/incrAddSentence.php?document_level_model_ID=5&source=S&target=T'
   function try_starting_server() {
      server_port=$(($RANDOM % 5000 + 52000))
      echo "Using port $server_port"
      php \
         --define 'include_path=../../../PortageLive/www:../../../PortageLive/www/rest' \
         --server $server_ip:$server_port \
         --docroot . \
         &> log.server &
      server_pid=$!
      sleep 1
   }
   try_starting_server

#   counter=5
#   try_starting_server;
#   #while [[ `kill -0 $server_pid` -ne 0 ]] && [[ $counter -gt 0 ]]; do
#   while [[ `lsof -i tcp:$server_port &> /dev/null` ]] && [[ $counter -gt 0 ]]; do
#      ps -p $server_pid
#      kill -0 $server_pid
#      lsof -i tcp:$server_port
#      counter=$((counter - 1))
#      try_starting_server
#   done

   # Is our server listening on the port?
   lsof -i tcp:$server_port &> /dev/null || { echo "Failed to start server" >&2; exit 1; }

   #trap "kill -9 $server_pid; rm -fr plive" EXIT
   trap "kill -9 $server_pid" EXIT
}

function Rester_testcase() {
   verbose Rester_testcase
   # Runnig https://github.com/chitamoor/Rester
   sed "s/PHP_PORT/$server_port/" < tests/testSuite.yaml.template > tests/testSuite.yaml
   apirunner --ts tests/testSuite.yaml
}

function incrAddSentence_with_curl_testcase() {
   verbose incrAddSentence_with_curl_testcase
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

function incrAddSentence_curl_post_testcase() {
   verbose incrAddSentence_curl_post_testcase
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

function translate_with_single_query_testcase() {
   verbose translate_with_single_query_testcase

   curl \
      --silent \
      --get \
      --data 'q=hello' \
      --data 'source=en' \
      --data 'target=fr' \
      --data 'invalid_arg=bad_arg' \
      --data 'prettyprint=false' \
      --data 'key=unittest.rev' \
      "http://$server_ip:$server_port/translate.php" \
   | grep \
      --fixed-strings \
      --quiet \
      '{"data":{"translations":[{"translatedText":"olleh"}]},"warnings":[{"message":"You used invalid argument(s)","arguments":["invalid_arg=bad_arg"]}]}' \
   || ! echo "Error translating a single request" >&2

   curl \
      --silent \
      --data 'q=hello' \
      --data 'source=en' \
      --data 'target=fr' \
      --data 'prettyprint=false' \
      --data 'invalid_arg=bad_arg' \
      --data 'key=unittest.rev' \
      "http://$server_ip:$server_port/translate.php" \
   | grep \
      --fixed-strings \
      --quiet \
      '{"data":{"translations":[{"translatedText":"olleh"}]},"warnings":[{"message":"You used invalid argument(s)","arguments":["invalid_arg=bad_arg"]}]}' \
   || ! echo "Error translating a single request" >&2
}

function translate_with_multiple_queries_testcase() {
   verbose translate_with_multiple_queries_testcase

   curl \
      --silent \
      --get \
      --data 'q[]=hello' \
      --data 'q[]=tree' \
      --data 'q[]=car' \
      --data 'source=en' \
      --data 'target=fr' \
      --data 'prettyprint=false' \
      --data 'invalid_arg=bad_arg' \
      --data 'key=unittest.rev' \
      "http://$server_ip:$server_port/translate.php" \
   | grep \
      --fixed-strings \
      --quiet \
      '{"data":{"translations":[{"translatedText":"olleh"},{"translatedText":"eert"},{"translatedText":"rac"}]},"warnings":[{"message":"You used invalid argument(s)","arguments":["invalid_arg=bad_arg"]}]}' \
   || ! echo "Error translating multiple requests" >&2

   curl \
      --silent \
      --data 'q[]=hello' \
      --data 'q[]=tree' \
      --data 'q[]=car' \
      --data 'source=en' \
      --data 'target=fr' \
      --data 'prettyprint=false' \
      --data 'invalid_arg=bad_arg' \
      --data 'key=unittest.rev' \
      "http://$server_ip:$server_port/translate.php" \
   | grep \
      --fixed-strings \
      --quiet \
      '{"data":{"translations":[{"translatedText":"olleh"},{"translatedText":"eert"},{"translatedText":"rac"}]},"warnings":[{"message":"You used invalid argument(s)","arguments":["invalid_arg=bad_arg"]}]}' \
   || ! echo "Error translating multiple requests" >&2
}

function handy_dbugger() {
   curl \
      --silent \
      --get \
      --data 'key=unittest.rev' \
      --data 'target=fr' \
      --data 'prettyprint=False' \
      --data 'option=invalid' \
      --data 'q=home' \
      "http://$server_ip:$server_port/translate.php"
}

rm -fr plive
RC=0
lint_php || RC=1
start_php_server || RC=1
#handy_dbugger; exit

phpunit_testcase || RC=1
Rester_testcase || RC=1
incrAddSentence_with_curl_testcase || RC=1
incrAddSentence_curl_post_testcase || RC=1
translate_with_single_query_testcase || RC=1
translate_with_multiple_queries_testcase || RC=1

echo
exit $RC

# CLI php will fail because it can't create a directory under /var/www/html/
#QUERY_STRING='source=SÉ&target=T&document_level_model_ID=curl' php incrAddSentence.php
#QUERY_STRING='source=SÉ&target=T&document_level_model_ID=curl' php -e -r 'parse_str(getenv("QUERY_STRING"), $_GET); include "incrAddSentence.php";'

