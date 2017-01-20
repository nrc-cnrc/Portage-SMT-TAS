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



[[ $DEPLOY ]] && cp ../PortageLiveLib.php /var/www/html/
[[ $DEPLOY ]] && cp incrementalTrainingAddSentencePair.php /var/www/html/
[[ $DEPLOY ]] && cp translate.php /var/www/html/language/translate/

php -d 'include_path=.:..' $PHPUNIT_HOME/phpunit-4.8.phar  --colors=always  tests/incrementalTrainingAddSentencePair.php

apirunner --ts tests/testSuite.yaml
