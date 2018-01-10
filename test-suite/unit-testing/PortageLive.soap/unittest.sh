#!/bin/bash
# @file unittest.sh
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada


readonly server_ip=127.0.0.1

function check_dependencies() {
   if ! python -c 'import suds'; then
      echo 'ERROR: cannot find Python module "suds". Please install it using pip:'
      echo "   pip install suds"
      exit 1
   fi
}

function verbose() {
   echo $'\n'============= $* =============
}

function deploy() {
   cp ../PortageLiveLib.php PortageLiveAPI.php /var/www/html/
   sed \
      "s/__REPLACE_THIS_WITH_YOUR_IP__/`ifconfig | grep -A8 eth | grep -Po 'addr:\K[\d.]+'`/g" \
      < PortageLiveAPI.wsdl \
      > /var/www/html/PortageLiveAPI.wsdl
}

function prepare_scenarios() {
   mkdir -p scenarios/no_contexts/models
   mkdir -p scenarios/one_context/models
   ln -fs ../../../tests/models/unittest.rev.en-fr scenarios/one_context/models/
   mkdir -p scenarios/several_contexts/models
   ln -fs ../../../tests/models/unittest.rev.en-fr scenarios/several_contexts/models/
   ln -fs $PORTAGE/test-suite/systems/toy-regress-en2fr scenarios/several_contexts/models/
   ln -fs $PORTAGE/test-suite/systems/toy-regress-en2fr.nnjm scenarios/several_contexts/models/
   ln -fs $PORTAGE/test-suite/systems/toy-regress-ch2en scenarios/several_contexts/models/
   ln -fs $PORTAGE/test-suite/systems/toy-regress-fr2en scenarios/several_contexts/models/
}

function start_php_server() {
   verbose start_php_server
   readonly doc_root=doc_root
   mkdir -p $doc_root

   pushd $doc_root &> /dev/null
   ln -fs ../tests .

   function try_starting_server() {
      server_port=$(($RANDOM % 5000 + 52000))
      echo "Using port $server_port"
      php \
         --define 'include_path=.:..' \
         --server $server_ip:$server_port \
         --docroot . \
         &> log.server &
      server_pid=$!
      sleep 1
   }
   try_starting_server

   popd &> /dev/null

   # Is our server listening on the port?
   lsof -i tcp:$server_port &> /dev/null || { echo "Failed to start server" >&2; exit 1; }
   export PHP_PORT=$server_port

   cp \
      ../../../PortageLive/www/PortageLiveLib.php \
      ../../../PortageLive/www/soap/PortageLiveAPI.php \
      $doc_root
   sed \
      "s/__REPLACE_THIS_WITH_YOUR_IP__/$server_ip:$server_port/g" \
      < ../../../PortageLive/www/soap/PortageLiveAPI.wsdl \
      > $doc_root/PortageLiveAPI.wsdl

   #trap "rm -fr $doc_root; kill -9 $server_pid" EXIT
   trap "kill -9 $server_pid" EXIT
}

function python_unittests() {
   # Web Unittest that use a functional web service.
   verbose 'Running Python unittests.'
   python -m unittest discover -v -s tests -p 'test*.py'
}

# Dummy incr-update.sh for speed and because we have a fake PortageLive model.
export PATH=$PWD:$PATH  # we MUST use $PWD and not '.'

[[ $DEPLOY ]] && deploy

check_dependencies
prepare_scenarios
start_php_server
python_unittests
