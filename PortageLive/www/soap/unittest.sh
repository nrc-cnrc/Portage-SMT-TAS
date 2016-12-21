#!/bin/bash

[[ $DEPLOY ]] && cp ../PortageLiveLib.php PortageLiveAPI.php /var/www/html/

# Web Unittest that use a functional web service.
python  tests/testIncrementalTrainingAddSentencePair.py
