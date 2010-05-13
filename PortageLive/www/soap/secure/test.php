<?php 
  $client = new SoapClient("PortageLiveAPI.wsdl"); 
  print "response of getTranslation: " . $client->getTranslation("This is a test."); 
?> 
