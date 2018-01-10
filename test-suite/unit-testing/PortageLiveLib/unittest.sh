#!/bin/bash

test=$1

[[ -n $test ]] || { echo "Error: non-empty test argument required." >&2; exit 1; }

if [[ ! $PHPUNIT_HOME ]]; then
   echo "Error: cannot find phpunit. Please download the right version of phpunit for your version of php at https://phpunit.de/ and set PHPUNIT_HOME to the directory where you saved it." >&2
   exit 1
fi
PHPUNIT=`\ls -1 $PHPUNIT_HOME/phpunit*.phar 2> /dev/null | head -1`
if [[ ! -s $PHPUNIT ]]; then
   echo "Error: cannot find phpunit*.phar in PHPUNIT_HOME=$PHPUNIT_HOME" >&2
   exit 1
fi

mkdir -p plive

# Command line Unittest where no server is required.
export PATH=$PWD:$PATH  # we MUST use $PWD and not '.'
which incr-update.sh
which incr-add-sentence.sh
php \
   --define 'include_path=.:../../../PortageLive/www' \
   $PHPUNIT \
   --colors=always \
   $test
