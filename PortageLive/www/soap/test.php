<!DOCTYPE html
	PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
	 "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US">
<head>
<title>PORTAGELive</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
</head>
<body>
<p align="center"><img src="/images/NRC_banner_e.jpg" /></p>

<?php
  print "Translating... This is a test.<BR/>";
  try {
    $client = new SoapClient("PortageLiveAPI.wsdl");
    $translation = $client->translate("This is a test.", "context", "s", 0, 0);
    if ($translation != "") {
      print "<H1><B><FONT COLOR=\"#00FF00\">Successful</FONT></B></H1>";
      print "response from translate(): " . $translation;
    }
    else {
      print "<FONT COLOR=\"#FF0000\"><BR/><H1><B>Error</B></H1></FONT>";
    }
  }
  catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
  }
?>

<hr/>
<table width="100%" cellspacing="0" cellpadding="0" border="0">
   <tr>
      <td width="20%" align="right" valign="center">
	 <img alt="NRC-ICT" src="images/sidenav_graphic.png" height="44" />
      </td>
      <td width="60%" align="center" valign="bottom">
	 <img width="286" alt="National Research Council Canada" src="/images/mainf1.gif" height="44" />
      </td>
      <td width="20%" align="left" valign="center">
	 <img width="93" alt="Government of Canada" src="/images/mainWordmark.gif" height="44" />
      </td>
   </tr>
   <tr>
      <td>
      </td>
      <td align="center" valign="top">
         <small>Traitement multilingue de textes / Multilingual Text Processing <br />
            Centre de recherche en technologies num&eacute;riques / Digital Technologies Research Centre <br />
            Conseil national de recherches Canada / National Research Council Canada <br />
            Copyright 2004&ndash;2018, Sa Majest&eacute; la Reine du Chef du Canada /  Her Majesty in Right of Canada <br />
            <a href="/portage_notices.php">Third party Copyright notices</a>
         </small>
      </td>
   </tr>
</table>

</body>
</html>
