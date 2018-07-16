#!/bin/bash

test=$1

[[ -n $test ]] || { echo "Error: non-empty test argument required." >&2; exit 1; }

if [[ "$(php --version | grep -o 'PHP [0-9]\.[0-9]\+\.[0-9]\+')" < "PHP 5.4" ]]; then
   echo 'Warning: We need php >= 5.4'
   exit 0
fi

if [[ ! $PHPUNIT_HOME ]]; then
   if [[ -d $PORTAGE/third-party/phpunit ]]; then
      export PHPUNIT_HOME=$PORTAGE/third-party/phpunit;
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

mkdir --parents plive

# Command line Unittest where no server is required.
export PATH=$PWD:$PATH  # we MUST use $PWD and not '.'
which incr-update.sh
which incr-add-sentence.sh
php \
   --define 'include_path=.:../../../PortageLive/www' \
   $PHPUNIT \
   --colors=always \
   $test
