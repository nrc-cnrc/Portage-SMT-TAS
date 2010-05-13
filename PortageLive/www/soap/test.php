<?php 
  print "Translating... This is a test.<BR/>";
  try {
    $client = new SoapClient("PortageLiveAPI.wsdl"); 
    $translation = $client->getTranslation("This is a test.");
    if ($translation != "") {
      print "<H1><B><FONT COLOR=\"#00FF00\">Successful</FONT></B></H1>";
      print "response of getTranslation: " . $translation; 
    }
    else {
      print "<FONT COLOR=\"#00FF00\"><BR/><H1><B>Error</B></H1></FONT>";
    }
  }
  catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$fault->faultcode}, faultstring: {$fault->faultstring})" . "<BR/>";
    var_dump($soapFault);
  }
?> 
