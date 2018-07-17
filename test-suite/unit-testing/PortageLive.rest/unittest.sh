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
readonly context=unittest.rev.en-fr
readonly document_model_id=PORTAGE_UNITTEST_4da35

function check_dependencies() {
   if [[ "$(php --version | grep -o 'PHP [0-9]\.[0-9]\+\.[0-9]\+')" < "PHP 5.4" ]]; then
      echo 'Warning: We need php >= 5.4'
      exit 0
   fi

   if [[ ! $PHPUNIT_HOME ]]; then
      if [[ -d $PORTAGE/third-party/phpunit ]]; then
         export PHPUNIT_HOME=$PORTAGE/third-party/phpunit
      else
         echo "Warning: cannot find phpunit. Please download the right version of phpunit for your version of php at https://phpunit.de/ and set PHPUNIT_HOME to the directory where you saved it." >&2
         exit 0
      fi
   fi
   PHPUNIT=`\ls -1 {$PHPUNIT_HOME,$PORTAGE/third-party/phpunit}/phpunit*.phar 2> /dev/null | head -1`
   if [[ ! -s $PHPUNIT ]]; then
      echo "Warning: cannot find phpunit*.phar in PHPUNIT_HOME=$PHPUNIT_HOME" >&2
      exit 0
   fi

   if ! which-test.sh apirunner; then
      echo "Warning: cannot find Rester's apirunner. Please install it from source:"
      echo "   pip install git+https://github.com/chitamoor/Rester.git@master"
      echo " OR"
      echo "   git clone https://github.com/chitamoor/Rester"
      echo "   cd Rester"
      echo "   pip install -e ."
      exit 0
   fi >&2
}

function verbose() {
   echo $'\n'============= $* =============
}

function deploy_code() {
   verbose ${FUNCNAME[0]}
   rm -fr plive
   mkdir -p plive
   cp ../../../../PortageLive/www/rest/translate.php .
   cp ../../../../PortageLive/www/rest/incrAddSentence.php .
   cp ../../../../PortageLive/www/rest/getAllContexts.php .
   cp ../../../../PortageLive/www/rest/incrStatus.php .
}

function prepare_scenarios() {
   verbose prepare_scenarios
   mkdir -p scenarios/no_contexts/models
   mkdir -p scenarios/one_context/models
   ln -fs ../../../tests/models/$context scenarios/one_context/models/
   mkdir -p scenarios/several_contexts/models
   ln -fs ../../../tests/models/$context scenarios/several_contexts/models/
   ln -fs $PORTAGE/test-suite/systems/toy-regress-en2fr scenarios/several_contexts/models/
   ln -fs $PORTAGE/test-suite/systems/toy-regress-en2fr.nnjm scenarios/several_contexts/models/
   ln -fs $PORTAGE/test-suite/systems/toy-regress-ch2en scenarios/several_contexts/models/
   ln -fs $PORTAGE/test-suite/systems/toy-regress-fr2en scenarios/several_contexts/models/
}

function phpunit_testcase() {
   verbose ${FUNCNAME[0]}
   php \
      --define 'include_path=../../../../PortageLive/www:../../../../PortageLive/www/rest' \
      $PHPUNIT \
      --colors=always  \
      tests/incrAddSentence.php
}

readonly doc_root=doc_root

function start_php_server() {
   verbose 'Starting php web server ....'
   mkdir -p $doc_root
   pushd $doc_root &> /dev/null
   ln -fs ../tests .
   ln -fs $PORTAGE/test-suite/systems/toy-regress-en2fr tests/models/

   deploy_code
   # elinks 'http://127.0.0.1:8765/incrAddSentence.php?document_model_ID=5&source=S&target=T'
   function try_starting_server() {
      server_port=$(($RANDOM % 5000 + 52000))
      echo "Using port $server_port"
      php \
         --define 'include_path=../../../../PortageLive/www:../../../../PortageLive/www/rest' \
         --server $server_ip:$server_port \
         --docroot . \
         &> log.server &
      server_pid=$!
      sleep 1
   }
   try_starting_server

   popd &> /dev/null

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
   verbose ${FUNCNAME[0]}
   # Running https://github.com/chitamoor/Rester
   sed "s/PHP_PORT/$server_port/" < tests/testSuite.yaml.template > tests/testSuite.yaml
   apirunner --ts tests/testSuite.yaml
}

function incrAddSentence_with_curl_testcase() {
   verbose ${FUNCNAME[0]}
   # Web request using unicode.
   export CORPORA="./plive/DOCUMENT_MODEL_${context}_${document_model_id}/corpora"
   #[[ -s $CORPORA ]] && rm -f $CORPORA
   local tag=`date +"%T"`

   curl \
      --silent \
      --get \
      --data "context=$context" \
      --data 'source=S%C9' \
      --data "target=GET$tag" \
      --data "document_model_ID=$document_model_id" \
      "http://$server_ip:$server_port/incrAddSentence.php" \
   | grep --quiet '{"result":true}' \
   || ! echo "Error Adding sentence pairs (1)" >&2

   curl \
      --silent \
      --get \
      --data "context=$context" \
      --data 'source=S%C9' \
      --data "target=GET$tag" \
      --data "document_model_ID=$document_model_id" \
      "http://$server_ip:$server_port/incrAddSentence.php" \
   | grep --quiet '{"result":true}' \
   || ! echo "Error Adding sentence pairs (2)" >&2

   grep --quiet "GET$tag" $CORPORA \
   || ! echo $'\nError: Cannot find entry.' >&2
}

function incrAddSentence_curl_post_testcase() {
   verbose ${FUNCNAME[0]}
   # Web request using unicode.
   export CORPORA="./plive/DOCUMENT_MODEL_${context}_${document_model_id}/corpora"
   #[[ -s $CORPORA ]] && rm -f $CORPORA
   local tag=`date +"%T"`

   curl \
      --silent \
      --data "context=$context" \
      --data 'source=S%C9' \
      --data "target=POST$tag" \
      --data "document_model_ID=$document_model_id" \
      "http://$server_ip:$server_port/incrAddSentence.php" \
   | grep --quiet '{"result":true}' \
   || ! echo "Error Adding sentence pairs" >&2

   grep --quiet "POST$tag" $CORPORA \
   || ! echo $'\nError: Cannot find entry.' >&2
}

function test_multi_word_queries() {
   verbose ${FUNCNAME[0]}
   export CORPORA="./plive/DOCUMENT_MODEL_${context}_${document_model_id}/corpora"
   local tag=`date +"%T"`

   curl \
      --silent \
      --data 'q=hello+world+,:;"' \
      --data 'source=en' \
      --data 'target=fr' \
      --data 'prettyprint=false' \
      --data "context=$context" \
      "http://$server_ip:$server_port/translate.php" \
   | grep --quiet --fixed-strings \
      '{"data":{"translations":[{"translatedText":"\";:, dlrow olleh"}]}}' \
   || ! echo "Error translating a multi-word request" >&2

   curl \
      --silent \
      --data "context=$context" \
      --data 'source=This is a test sentence, punctuation,;: "quotes" and stuff' \
      --data "target=The results has tag POST$tag, and ;:\"'" \
      --data "document_model_ID=$document_model_id" \
      "http://$server_ip:$server_port/incrAddSentence.php" \
   | grep --quiet '{"result":true}' \
   || ! echo "Error Adding sentence pairs" >&2

   grep --quiet "has tag POST$tag, and ;:\"'" $CORPORA \
   || ! echo $'\nError with multi-word push: Cannot find entry.' >&2
}

function lint_php() {
   verbose ${FUNCNAME[0]}
   for c in PortageLiveLib.php rest/incrAddSentence.php rest/translate.php rest/getAllContexts.php rest/incrStatus.php; do
      php \
         --define 'include_path=../../../PortageLive/www:../../../PortageLive/www/rest' \
         --syntax-check \
         ../../../PortageLive/www/$c \
      || ! echo "Error in linting $f" >&2
   done
}

function translate_with_single_query_testcase() {
   verbose ${FUNCNAME[0]}

   curl \
      --silent \
      --get \
      --data 'q=hello' \
      --data 'source=en' \
      --data 'target=fr' \
      --data 'invalid_arg=bad_arg' \
      --data 'prettyprint=false' \
      --data "context=$context" \
      "http://$server_ip:$server_port/translate.php" \
   | grep \
      --fixed-strings \
      --quiet \
      '{"data":{"translations":[{"translatedText":"olleh"}]},"warnings":[{"message":"You used invalid argument(s)","arguments":["invalid_arg=bad_arg"]}]}' \
   || ! echo "Error translating a single request (1)" >&2

   curl \
      --silent \
      --data 'q=hello' \
      --data 'source=en' \
      --data 'target=fr' \
      --data 'prettyprint=false' \
      --data 'invalid_arg=bad_arg' \
      --data "context=$context" \
      "http://$server_ip:$server_port/translate.php" \
   | grep \
      --fixed-strings \
      --quiet \
      '{"data":{"translations":[{"translatedText":"olleh"}]},"warnings":[{"message":"You used invalid argument(s)","arguments":["invalid_arg=bad_arg"]}]}' \
   || ! echo "Error translating a single request (2)" >&2
}

function translate_with_multiple_queries_testcase() {
   verbose ${FUNCNAME[0]}

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
      --data "context=$context" \
      "http://$server_ip:$server_port/translate.php" \
   | grep \
      --fixed-strings \
      --quiet \
      '{"data":{"translations":[{"translatedText":"olleh"},{"translatedText":"eert"},{"translatedText":"rac"}]},"warnings":[{"message":"You used invalid argument(s)","arguments":["invalid_arg=bad_arg"]}]}' \
   || ! echo "Error translating multiple requests (1)" >&2

   curl \
      --silent \
      --data 'q[]=hello' \
      --data 'q[]=tree' \
      --data 'q[]=car' \
      --data 'source=en' \
      --data 'target=fr' \
      --data 'prettyprint=false' \
      --data 'invalid_arg=bad_arg' \
      --data "context=$context" \
      "http://$server_ip:$server_port/translate.php" \
   | grep \
      --fixed-strings \
      --quiet \
      '{"data":{"translations":[{"translatedText":"olleh"},{"translatedText":"eert"},{"translatedText":"rac"}]},"warnings":[{"message":"You used invalid argument(s)","arguments":["invalid_arg=bad_arg"]}]}' \
   || ! echo "Error translating multiple requests (2)" >&2
}

function translate_with_a_document_model_ID() {
   # This function was converted to
   # tests/translate_with_a_document_model_ID.yaml but is quite handy to keep.
   verbose ${FUNCNAME[0]}

   curl \
      --silent \
      --get \
      --data 'context=toy-regress-en2fr' \
      --data 'target=fr' \
      --data 'prettyprint=False' \
      --data 'q=home' \
      --data 'document_model_ID=foo' \
      "http://$server_ip:$server_port/translate.php"
   # We expect "pays"

   for i in {1..1}; do
      curl \
         --silent \
         --data 'context=toy-regress-en2fr' \
         --data 'source=home' \
         --data 'target=MAISON' \
         --data 'document_model_ID=foo' \
         "http://$server_ip:$server_port/incrAddSentence.php"

      curl \
         --silent \
         --get \
         --data 'context=toy-regress-en2fr' \
         --data 'target=fr' \
         --data 'prettyprint=False' \
         --data 'q=home' \
         --data 'document_model_ID=foo' \
         "http://$server_ip:$server_port/translate.php"
   done
}

function handy_debugger() {
   curl \
      --silent \
      --get \
      --data "context=$context" \
      --data 'target=fr' \
      --data 'prettyprint=False' \
      --data 'option=invalid' \
      --data 'q=home' \
      --data 'document_model_ID=foo' \
      "http://$server_ip:$server_port/translate.php"
}

# Use a dummy incr-update.sh for speed when using the fake unittest.rev.en-fr
# PortageLive model. The dummy incr-update.sh calls the real incr-update.sh
# when the model isn't unittest.rev.en-fr.
export PATH=$PWD:$PATH  # we MUST use $PWD and not '.'

rm -fr plive
RC=0
check_dependencies || exit 1
lint_php || exit 1
start_php_server || exit 1
#handy_debugger; exit

# Create an empty document model directory for use by some incrStatus unit tests.
mkdir -p "$doc_root/plive/DOCUMENT_MODEL_${context}_${document_model_id}_empty"

prepare_scenarios || RC=1
cd $doc_root || exit 1
test_multi_word_queries || RC=1
phpunit_testcase || RC=1
Rester_testcase || RC=1
incrAddSentence_with_curl_testcase || RC=1
incrAddSentence_curl_post_testcase || RC=1
translate_with_single_query_testcase || RC=1
translate_with_multiple_queries_testcase || RC=1

echo
exit $RC

# CLI php will fail because it can't create a directory under /var/www/html/
#QUERY_STRING='source=SÉ&target=T&document_model_ID=curl' php incrAddSentence.php
#QUERY_STRING='source=SÉ&target=T&document_model_ID=curl' php -e -r 'parse_str(getenv("QUERY_STRING"), $_GET); include "incrAddSentence.php";'

