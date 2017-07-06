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
readonly server_port=8765

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

function start_php_server() {
   verbose start_php_server
   readonly doc_root=doc_root
   mkdir -p $doc_root

   cp \
      ../../../PortageLive/www/PortageLiveLib.php \
      ../../../PortageLive/www/soap/PortageLiveAPI.php \
      $doc_root
   sed \
      "s/__REPLACE_THIS_WITH_YOUR_IP__/$server_ip:$server_port/g" \
      < ../../../PortageLive/www/soap/PortageLiveAPI.wsdl \
      > $doc_root/PortageLiveAPI.wsdl

   pushd $doc_root &> /dev/null
   ln -s ../tests .
   php \
      --define 'include_path=.:..' \
      --server $server_ip:$server_port \
      --docroot . \
      &> log.server &
   server=$!
   popd &> /dev/null
   sleep 1
   trap "rm -fr $doc_root; kill -9 $server" EXIT
   #trap "kill -9 $server" EXIT
}

function python_unittests() {
   # Web Unittest that use a functional web service.
   verbose 'Running Python unittests.'
   python -m unittest discover -s tests -p 'test*.py'
}


[[ $DEPLOY ]] && deploy

start_php_server
python_unittests
