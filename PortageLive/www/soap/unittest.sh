#!/bin/bash

if [[ $DEPLOY ]];
then
   cp ../PortageLiveLib.php PortageLiveAPI.php /var/www/html/
   sed \
      "s/__REPLACE_THIS_WITH_YOUR_IP__/`ifconfig | grep -A8 eth | grep -Po 'addr:\K[\d.]+'`/g" \
      < PortageLiveAPI.wsdl \
      > /var/www/html/PortageLiveAPI.wsdl
fi

# Web Unittest that use a functional web service.
python -m unittest discover -s tests -p 'test*.py'
