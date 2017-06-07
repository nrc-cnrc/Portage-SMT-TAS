#!/bin/bash

[[ $DEPLOY ]] && cp PortageLiveAPI.wsdl ../PortageLiveLib.php PortageLiveAPI.php /var/www/html/

# Web Unittest that use a functional web service.
python -m unittest discover -s tests -p 'test*.py'
