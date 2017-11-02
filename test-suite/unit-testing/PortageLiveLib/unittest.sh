#!/bin/bash

test=$1

[[ -n $test ]] || { echo "Error: non-empty test argument required." >&2; exit 1; }

mkdir -p plive

# Command line Unittest where no server is required.
export PATH=$PWD:$PATH  # we MUST use $PWD and not '.'
which incr-update.sh
which incr-add-sentence.sh
php \
   --define 'include_path=.:../../../PortageLive/www' \
   $PHPUNIT_HOME/phpunit-4.8.phar \
   --colors=always \
   $test
