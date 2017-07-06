#!/bin/bash

mkdir -p plive

# Command line Unittest where no server is required.
export PATH=$PWD:$PATH  # we MUST use $PWD and not '.'
which incr-update.sh
which incr-add-sentence.sh
php \
   --define 'include_path=.:../../../PortageLive/www' \
   $PHPUNIT_HOME/phpunit-4.8.phar \
   --colors=always \
   tests/testIncrAddSentence.php
