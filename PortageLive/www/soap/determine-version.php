<?php
# @file determine-version.php
# @brief Sample PHP code for determining the version of a PortageLive server
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada
?>
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

<h2>Determining the version of a PortageLive server</h2>

<p>This page shows PHP calls that can be used to determine the version of a
PortageLive server automatically. It is intended as pseudo code and
documentation for doing the same in other languages.</p>

<h3>With a single PHP function call, as efficiently as possible</h3>

The determineVersion($wsdl) method used here tries to determine the version of
a PortageLive server as efficiently as possible. It returns the answer as a
string of the form "PortageII-x.y". It returns "bad-wsdl" if it cannot load the
SOAP client at all for the WSDL provided, or "unknown" if it cannot determine
the version (e.g., if the PortageLive server predates PortageII 1.0).

<?php
  function determineVersion($wsdl) {
    $version = "unknown";
    try {
      $client = new SoapClient($wsdl);
    } catch (SoapFault $exception) {
      return "bad-wsdl";
    }

    try {
      $response = $client->getVersion();
      return $response;
    } catch (SoapFault $exception) {
    }

    try {
      $response = $client->getFixedTerms();
    } catch (SoapFault $exception) {
      if ($exception->faultcode == "PortageContext") {
        return "PortageII-2.2";
      }
    }

    try {
      $response = $client->getTranslation2();
    } catch (SoapFault $exception) {
      if ($exception->faultcode == "PortageBadArgs") {
        return "PortageII-2.1";
      }
    }

    try {
      $response = $client->translateSDLXLIFFCE();
    } catch (SoapFault $exception) {
      if ($exception->faultcode == "PortageContext") {
        return "PortageII-2.0";
      }
    }

    try {
      $response = $client->primeModels();
    } catch (SoapFault $exception) {
      if ($exception->faultcode == "PortageContext") {
        return "PortageII-1.0";
      }
    }

    return "unknown";
  }

  print "<p>determineVersion(PortageLiveAPI.wsdl): " . determineVersion("PortageLiveAPI.wsdl") . "</p>";
  print "<p>determineVersion(bad-name): " . determineVersion("bad-name") . "</p>";
  print "<p>determineVersion(pls1): " . determineVersion("http://portagelive-sandbox/PortageLiveAPI.wsdl") . "</p>";
  print "<p>determineVersion(pls2): " . determineVersion("http://portagelive-sandbox2/PortageLiveAPI.wsdl") . "</p>";
  print "<p>determineVersion(219): " . determineVersion("http://132.246.128.219/PortageLiveAPI.wsdl") . "</p>";
  print "<p>determineVersion(220): " . determineVersion("http://132.246.128.220/PortageLiveAPI.wsdl") . "</p>";
?>

<h3>Debugging details and full output of relevant calls (and more)</h3>

<?php

  $wsdl = "PortageLiveAPI.wsdl";
  #$wsdl = "http://portagelive-sandbox2/PortageLiveAPI.wsdl";

  try {
    $client = new SoapClient($wsdl);
  }
  catch (SoapFault $exception) {
    print "<p>Cannot create SOAP client for WSDL $wsdl</p>";
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
    $bad_client = 1;
  }

  if (!$bad_client) {

  $version = "unknown";

  # Works in 3.0 (and later), gives faultcode Client in older versions.
  print "<p>Trying getVersion(): in PortageII 3.0, it will return PortageII-3.0; earlier versions return a Client SOAP fault</p>";
  try {
    $response = $client->getVersion();
    print "<p>getVersion() says: " . $response . "</p>";
    $version = $response;
    print "<p>Identified version as " . $version . "</p>";
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
  }

  # Gives faultcode PortageContext in 2.2 and 3.0, faultcode Client in older versions.
  print "<p>Trying getFixedTerms(), which exists in 2.2...</p>";
  try {
    $response = $client->getFixedTerms();
    print "<p>getFixedTerms() says: " . $response . "</p>";
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
    # If the fault is PortageContext, that means the method exists, it just
    # didn't like being called without arguments.
    if ($exception->faultcode == "PortageContext" and $version == "unknown") {
      $version = "PortageII-2.2";
      print "<p>Identified version as " . $version . "</p>";
    }
  }

  # Works in 2.2 and 3.0 if context has fixed terms
  print "<p>Trying getFixedTerms(context), which exists in 2.2...</p>";
  try {
    $response = $client->getFixedTerms("context");
    print "<p>getFixedTerms(context) says: " . $response . "</p>";
    # This test is not robust: won't work if context has no fixed terms...
    if ($version == "unknown") { $version = "PortageII-2.2"; }
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
  }

  # Gives faultcode PortageContext in 2.2, faultcode Client in all other versions.
  print "<p>Trying translatePlainTextCE(), which exists in 2.2...</p>";
  try {
    $response = $client->translatePlainTextCE();
    print "<p>translatePlainTextCE() says: " . $response . "</p>";
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
    # If the fault is PortageContext, that means the method exists, it just
    # didn't like being called without arguments.
    if ($exception->faultcode == "PortageContext" and $version == "unknown") {
      $version = "PortageII-2.2";
      print "<p>Identified version as " . $version . "</p>";
    }
  }

  # Gives faultcode PortageBadArgs in 2.1 and 2.2, and faultcode PortageContext
  # in 1.0 and 2.0
  print "<p>Trying getTranslation2(), which exists since in 1.0...</p>";
  try {
    $response = $client->getTranslation2();
    print "<p>getTranslation2() says: " . $response . "</p>";
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
    # With PortageII 2.1 and 2.2, calling getTranslation2 without arguments
    # will complain that "newline" has a bad value.
    if ($exception->faultcode == "PortageBadArgs" and $version == "unknown") {
      $version = "PortageII-2.1";
      print "<p>Identified version as " . $version . "</p>";
    }
    # With PortageII 1.0 or 2.0, it will instead complain that context "" is broken
    #if ($exception->faultcode == "PortageContext" and $version == "unknown") {
    #  $version = "PortageII-2.0 or 1.0";
    #  print "<p>Identified version as " . $version . "</p>";
    #}
  }

  # Superfluous, but gives faultcode PortageBadArgs with 2.1 and 2.2
  print "<p>Trying getTranslation2(text, context)...</p>";
  try {
    $response = $client->getTranslation2("text", "context");
    print "<p>getTranslation2(text, context) says: " . $response . "</p>";
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
  }

  # Superfluous: works in 2.0 (which ignores the extra args), 2.1 and 2.2
  print "<p>Trying getTranslation2(text, context, p, 0), which exists in 2.1...</p>";
  try {
    $response = $client->getTranslation2("text", "context", "p", 0);
    print "<p>getTranslation2(text, context, p, 0) says: " . $response . "</p>";
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
  }

  # Gives faultcode PortageContext in 2.0, 2.1 and 2.2
  print "<p>Trying translateSDLXLIFFCE(), which exists in 2.0, 2.1 and 2.2</p>";
  try {
    $response = $client->translateSDLXLIFFCE();
    print "<p>translateSDLXLIFFCE() says: " . $response . "</p>";
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
    if ($exception->faultcode == "PortageContext" and $version == "unknown") {
      $version = "PortageII-2.0";
      print "<p>Identified version as " . $version . "</p>";
    }
  }

  # Gives faultcode PortageContext in 1.0 and more recent
  print "<p>Trying primeModels(), which exists since 1.0</p>";
  try {
    $response = $client->primeModels();
    print "<p>primeModels() says: " . $response . "</p>";
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
    if ($exception->faultcode == "PortageContext" and $version == "unknown") {
      $version = "PortageII-1.0";
      print "<p>Identified version as " . $version . "</p>";
    }
  }

  # This call will only work in 3.0, but getVersion() already identified 3.0
  # earlier, so it's just here as a superfluous example.
  print "<p>Trying translate(text, context, p, 0, 0), which exists in 3.0...</p>";
  try {
    $response = $client->translate("text", "context", "p", 0, 0);
    print "<p>translate(text, context, p, 0, 0) says: " . $response . "</p>";
  } catch (SoapFault $exception) {
    print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})" . "<BR/>";
    var_dump($exception);
  }

  print "<p>The PortageLive version has been identified as: " . $version . "</p>";
  }
?>

<hr/>
<table width="100%" cellspacing="0" cellpadding="0" border="0">
   <tr>
      <td width="20%" align="right" valign="bottom">
	 <img alt="NRC-ICT" src="/images/sidenav_graphictop_e.gif" height="54" />
      </td>
      <td width="60%" align="center" valign="bottom">
	 <img width="286" alt="National Research Council Canada" src="/images/mainf1.gif" height="44" />
      </td>
      <td width="20%" align="left" valign="center">
	 <img width="93" alt="Government of Canada" src="/images/mainWordmark.gif" height="44" />
      </td>
   </tr>
   <tr>
      <td align="right" valign="top">
	 <img alt="NRC-ICT" src="/images/sidenav_graphicbottom_e.gif" />
      </td>
      <td align="center" valign="top">
	 <small>Traitement multilingue de textes / Multilingual Text Processing <br /> Technologies de l'information et des communications / Information and Communications Technologies <br /> Conseil national de recherches Canada / National Research Council Canada <br /> Copyright 2016, Sa Majest&eacute; la Reine du Chef du Canada /  Her Majesty in Right of Canada <br /> <a href="/portage_notices.html">Third party Copyright notices</a>
	 </small>
      </td>
   </tr>
</table>

</body>
</html>
