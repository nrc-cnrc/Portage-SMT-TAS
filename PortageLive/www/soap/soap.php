<?php
# $Id$
# @file soap.php
# @brief Test web page for the SOAP API to PortageLive
#
# @author Patrick Paul, Eric Joanis and Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009 - 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009 - 2011, Her Majesty in Right of Canada

# This works if this file and the WSDL and PortageLive service are on the same
# machine and in the same directory:
$WSDL="PortageLiveAPI.wsdl";
# In the general case, uncomment and modify this statement to point to where
# your PortageLive server actually exists:
#$WSDL="http://__REPLACE_THIS_WITH_YOUR_IP__/PortageLiveAPI.wsdl";

$context = "";
$button = "";
$monitor_token = "";
#print_r( $_POST);  // Nice for debug POST's values.
if ( $_POST ) {
   if ( array_key_exists('context', $_POST) ) {
      $context = $_POST['context'];
      if ( array_key_exists('TranslateBox', $_POST) )
         $button = "TranslateBox";
      if ( array_key_exists('TranslateTMX', $_POST) )
         $button = "TranslateTMX";
      if ( array_key_exists('TranslateSDLXLIFF', $_POST) )
         $button = "TranslateSDLXLIFF";
   }

   if ( array_key_exists('context', $_POST) ) {
      $context = $_POST['context'];
      if ( array_key_exists('Prime', $_POST) )
         $button = "Prime";
   }

   if ( array_key_exists('ce_threshold', $_POST) )
      $ce_threshold = $_POST['ce_threshold'] + 0;
   else
      $ce_threshold = 0;

   if ( array_key_exists('monitor_token', $_POST) )
      $monitor_token = $_POST['monitor_token'];
   if ( array_key_exists('MonitorJob', $_POST) )
      $button = "MonitorJob";
} else {
   $ce_threshold = 0;
}


?>

<html>
<head>
<title>PortageLiveAPI</title>
<STYLE type="text/css">
   div.PRIME { width: 70%;}
   .PRIME {border-width: 1; border: solid; text-align: center; font-size:1.2em; background-color: #FAFAD2; padding: 1px; margin: 1px}
   .SUCCESS {color: green}
   .ERROR { color: red; text-decoration: blink; font-weight: bold; }
</STYLE>
</head>

<body>
<p align="center"><img src="/images/NRC_banner_e.jpg" /></p>

<h1>PortageLive SOAP API test</h1>

This page demonstrates how the appliance can be used as a web service.

Link to <a href="<?=$WSDL?>">the WSDL</a>.


<FORM action="" enctype="multipart/form-data" method="post" name="formulaire" target="_self">

<br/> Context:
<SELECT NAME = "context">
<?php
try {
   $client = new SoapClient($WSDL);
   $call = $client->getAllContexts(false);
   $tokens = preg_split("/;/", $call);
   foreach ($tokens as $token) {
      if ($context == $token) {
         print "<OPTION SELECTED=\"selected\" VALUE=\"$token\">$token</OPTION>";
      }
      else {
         print "<OPTION VALUE=\"$token\">$token</OPTION>";
      }
   }
}
catch (SoapFault $exception) {
   print "<BR/><SPAN class=\"ERROR\">SOAP Fault trying to list contexts: </SPAN>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}\n";
}
?>
<OPTION VALUE="InvalidContext">Invalid context for debugging</OPTION>
</SELECT>
<BR />

Prime:
<INPUT TYPE = "RADIO" NAME = "PrimeMode" VALUE="partial" CHECKED="checked" /> Partial
<INPUT TYPE = "RADIO" NAME = "PrimeMode" VALUE="full" /> Full
<INPUT TYPE = "RADIO" NAME = "PrimeMode" VALUE="bogus" /> Unsupported
<INPUT TYPE = "Submit" Name = "Prime"  VALUE = "Prime context"/>


<!-- INPUT TYPE = "TEXT"   Name = "context"       VALUE = "<?=$context?>" / -->
<hr/> Enter text here:
<INPUT TYPE = "TEXT"   Name = "to_translate" />
<INPUT TYPE = "Submit" Name = "TranslateBox"  VALUE = "Translate Text"/>

<INPUT TYPE = "hidden" Name = "MAX_FILE_SIZE" VALUE = "2000000" />
<hr/> Alternatively, use a TMX file:
<INPUT TYPE = "file"   Name = "tmx_filename"/>
<INPUT TYPE = "Submit" Name = "TranslateTMX"  VALUE = "Translate TMX File"/>
<br/> Alternatively, use a SDLXLIFF file:
<INPUT TYPE = "file"   Name = "sdlxliff_filename"/>
<INPUT TYPE = "Submit" Name = "TranslateSDLXLIFF"  VALUE = "Translate SDLXLIFF File"/>
<br/>CE threshold for filtering (between 0 and 1; 0.0 = no filter)
<INPUT TYPE = "TEXT"   Name = "ce_threshold"  VALUE = "<?=$ce_threshold?>" SIZE="4" />

</FORM>


<?php

try {
   $client = new SoapClient($WSDL);
   print "<hr/><b>Contexts: </b>" . $client->getAllContexts(false) . "</br>";
   print "<br/><b>Verbose contexts: </b>" . $client->getAllContexts(true) . "</br>";
}
catch (SoapFault $exception) {
   print "<br/><span class=\"ERROR\">SOAP Fault trying to list contexts: </span>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}";
}


function processFile($type, $file) {
   global $context;
   global $ce_threshold;
   $filename = $file["name"];
   print "<hr/><b>Translating $type file: </b> $filename <br/>";
   print "<b>Context: </b> $context <br/>";
   print "<b>Processed on: </b> " . `date` . "<br/>";

   if ( is_uploaded_file($file["tmp_name"]) ) {
      $tmp_file = $file["tmp_name"];
      //print "<br/><b>$type Upload OK.  Trace: </b> " . print_r($file, true);
      $tmp_contents = file_get_contents($tmp_file);
      $tmp_contents_base64 = base64_encode($tmp_contents);
      //print "<br/><b>file contents len: </b> " . strlen($tmp_contents) .
      //      " <b>base64 len: </b> " . strlen($tmp_contents_base64);

      try {
         global $WSDL;
         $client = new SoapClient($WSDL);

         $ce_threshold += 0;
         if ($type == "tmx") {
            $reply = $client->translateTMXCE($tmp_contents_base64, $filename, $context, $ce_threshold);
         }
         else
         if ($type == "sdlxliff") {
            $reply = $client->translateSDLXLIFFCE($tmp_contents_base64, $filename, $context, $ce_threshold);
         }
         else {
            print "<B>Unknown type: $type</B><BR/>\n";
         }
         print "<hr/><b>Portage replied: </b>$reply";
         print "<br/><a href=\"$reply\">Monitor job interactively</a>";
         global $monitor_token;
         $monitor_token=$reply;
         //print "<br/><b>Trace: </b>"; var_dump($client);
      } catch (SoapFault $exception) {
         print "<HR/><span class=\"ERROR\">SOAP Fault: </span>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}";
      }

   } else {
      print "<br/><b>$type Upload error.  Trace: </b> " . print_r($file, true);
      $file_error_codes = array(
            0=>"There is no error, the file uploaded with success",
            1=>"The uploaded file exceeds the upload_max_filesize directive in php.ini",
            2=>"The uploaded file exceeds the MAX_FILE_SIZE directive that was specified in the HTML form",
            3=>"The uploaded file was only partially uploaded",
            4=>"No file was uploaded",
            6=>"Missing a temporary folder",
            7=>"Failed to write file to disk",
            8=>"A PHP extension stopped the file upload",
            );
      print "<br/><b>Error code description: </b> {$file_error_codes[$file["error"]]}";
   }


   print "<hr/><b>Done processing on: </b>" . `date`;
}

if ( $button == "Prime"  && $_POST['Prime'] != "" ) {
   try {
      $PrimeMode = $_POST['PrimeMode'];
      $client = new SoapClient($WSDL);
      $rc = $client->primeModels($context, $PrimeMode);
      //print "Prime Models ($context, $PrimeMode) rc = $rc<br />";  // DEBUGGING
      print "<span class=\"PRIME SUCCESS\">Primed successfully!</span></br>";
      if ($rc != "OK")
         print "<div class=\"PRIME ERROR\">INVALID return code.</div>";
   }
   catch (SoapFault $exception) {
      print "<div class=\"PRIME ERROR\">Error with primeModels:<br/><b>{$exception->faultcode}</b><br/>{$exception->faultstring}</div>";
   }
}
else
if ( $button == "TranslateBox" && $_POST['to_translate'] != "") {
   $to_translate = $_POST['to_translate'];
   print "<hr/><b>Translating: </b> $to_translate <br/>";
   print "<b>Context: </b> $context <br/>";
   print "<b>Processed on: </b> " . `date` . "<br/>";
   $client = "";
   try {
      $client = new SoapClient($WSDL);
   }
   catch (SoapFault $exception) {
      print "<HR/><span class=\"ERROR\">SOAP Fault: </span>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}";
   }

   $start_time = microtime(true);
   try {
      print "<hr/><b>Portage getTranslation() replied: </b>";
      print $client->getTranslation($to_translate);
      //print "<br/><b>Trace: </b>"; var_dump($client);
   } catch (SoapFault $exception) {
      print "<span class=\"ERROR\">SOAP Fault: </span>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}";
   }
   $end_time = microtime(true);
   printf("<br/><b>Translating took: </b>%.2f seconds <br/>", $end_time-$start_time);
   print "<b>Finished at: </b>" . `date` . "<br/>";

   $start_time = microtime(true);
   try {
      print "<hr/><b>getTranslation2() replied: </b>";
      print $client->getTranslation2($to_translate, $context);
      //print "<br/><b>Trace: </b>"; var_dump($client);
   } catch (SoapFault $exception) {
      print "<span class=\"ERROR\">SOAP Fault: </span>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}";
   }
   $end_time = microtime(true);
   printf("<br/><b>Translating took: </b>%.2f seconds <br/>", $end_time-$start_time);
   print "<b>Finished at: </b>" . `date` . "<br/>";

   $start_time = microtime(true);
   try {
      print "<hr/><b>getTranslationCE() replied: </b>";
      print $client->getTranslationCE($to_translate, $context);
      //print "<br/><b>Trace: </b>"; var_dump($client);
   } catch (SoapFault $exception) {
      print "<span class=\"ERROR\">SOAP Fault: </span>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}";
   }
   $end_time = microtime(true);
   printf("<br/><b>Translating took: </b>%.2f seconds <br/>", $end_time-$start_time);
   print "<b>Finished at: </b>" . `date` . "<br/>";



   /*
      try {
      } catch (SoapFault $exception) {
      if ($exception->faultcode == "PortageContext") {
      print "<HR/><b>PortageContext error: </b>{$exception->faultstring}<br/>";
      } else {
      print "<HR/><b>Unknown SOAP Fault: </b>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}<BR/>";
      print "<br/><b>exception: </b>";
      var_dump($exception);
      print "<br/><b>client: </b>";
      var_dump($client);
      }
      }
    */
}
else
if ( $button == "TranslateTMX" && $_FILES["tmx_filename"]["name"] != "") {
   processFile("tmx", $_FILES["tmx_filename"]);
}
else
if ($button == "TranslateSDLXLIFF" && $_FILES["sdlxliff_filename"]["name"] != "") {
   processFile("sdlxliff", $_FILES["sdlxliff_filename"]);
}
else
if ( $button == "MonitorJob" && !empty($monitor_token) ) {
   try {
      $client = new SoapClient($WSDL);
      # TODO: Monitor SDLXLIFF Status.
      if ( $button == "TranslateTMX") {
         $reply = $client->translateTMXCE_Status($monitor_token);
      }
      else
      if ($button == "TranslateSDLXLIFF") {
         $reply = $client->translateSDLXLIFFCE_Status($monitor_token);
      }
      else {
         print "<B>Unknown type: $monitor_token</B><BR/>\n";
      }
      print "<hr/><b>Job status: </b> $reply";
      if ( preg_match("/^0 Done: (\S*)/", $reply, $matches) )
         print "<br/>Right click and save: <a href=\"$matches[1]\">Translated content</a>";
      print "<br/><a href=\"$monitor_token\">Switch to interactive job monitoring</a>";
   } catch (SoapFault $exception) {
      print "<HR/><span class=\"ERROR\">SOAP Fault: </span>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}";
   }
}


?>

<hr/>
<FORM action="" method="post" name="formulaire2" target="_self">
Job Token:
<INPUT TYPE = "TEXT"   Name = "monitor_token" VALUE = "<?=$monitor_token?>" />
<INPUT TYPE = "Submit" Name = "MonitorJob"    VALUE = "Monitor Job via SOAP"/>
</FORM>


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
	 <small>Technologies langagi&egrave;res interactives / Interactive Language Technologies <br /> Technologies de l’information et des communications / Information and Communications Technologies <br /> Conseil national de recherches Canada / National Research Council Canada <br /> Copyright 2004&ndash;2012, Sa Majest&eacute; la Reine du Chef du Canada /  Her Majesty in Right of Canada <br /> <a href="/portage_notices.html">Third party Copyright notices</a>
	 </small>
      </td>
   </tr>
</table>

</body>
</html>
