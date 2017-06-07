#!/bin/bash

mkdir -p plive

# Command line Unittest where no server is required.
php $PHPUNIT_HOME/phpunit-4.8.phar  --colors=always  tests/testIncrAddSentence.php
