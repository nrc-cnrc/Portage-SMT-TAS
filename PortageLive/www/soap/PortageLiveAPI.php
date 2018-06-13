<?php
# @file PortageLiveAPI.php
# @brief Implementation of the API to the PortageII SMT software suite.
#
# @author Samuel Larkin, Patrick Paul & Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009 - 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009 - 2016, Her Majesty in Right of Canada


require 'PortageLiveLib.php';

class PortageLiveAPI extends PortageLiveLib {
}


try {
   #error_log("SOAP call: $HTTP_RAW_POST_DATA");
   $server = new SoapServer("PortageLiveAPI.wsdl");
   $server->setClass("PortageLiveAPI");
   $server->handle();
}
catch (SoapFault $exception) {
   print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})<BR/>";
   var_dump($exception);
}
?>
