<?php
# @file soap.php 
# @brief Test web page for the SOAP API to PortageLive
# 
# @author Patrick Paul and Eric Joanis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, 2010, Her Majesty in Right of Canada

# This works if this file and the WSDL and PortageLive service are on the same
# machine and in the same directory:
$WSDL="PortageLiveAPI.wsdl";
# In the general case, uncomment and modify this statement to point to where
# your PortageLive server actually exists:
#$WSDL="http://__REPLACE_THIS_WITH_YOUR_IP__/PortageLiveAPI.wsdl";

$context = "";
$button = "";
$monitor_token = "";
if ( $_POST ) {
  if ( array_key_exists('context', $_POST) ) {
    $context = $_POST['context'];
    if ( array_key_exists('TranslateBox', $_POST) )
      $button = "TranslateBox";
    if ( array_key_exists('TranslateTMX', $_POST) )
      $button = "TranslateTMX";
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
</head>
<body>
<p align="center"><img src="/images/NRC_banner_e.jpg" /></p>

<h1>PortageLive SOAP API test</h1>

This page demonstrates how the appliance can be used as a web service.

Link to <a href="<?=$WSDL?>">the WSDL</a>.

<FORM action="" enctype="multipart/form-data" method="post" name="formulaire" target="_self">

<br/> Context:
<INPUT TYPE = "TEXT"   Name = "context"       VALUE = "<?=$context?>" />
<hr/> Enter text here:
<INPUT TYPE = "TEXT"   Name = "to_translate" />
<INPUT TYPE = "Submit" Name = "TranslateBox"  VALUE = "Translate Text"/>

<hr/> Alternatively, use a TMX file:
<INPUT TYPE = "hidden" Name = "MAX_FILE_SIZE" VALUE = "2000000" />
<INPUT TYPE = "file"   Name = "tmx_filename"/>
<INPUT TYPE = "Submit" Name = "TranslateTMX"  VALUE = "Translate TMX File"/>
<br/>CE threshold for filtering (between 0 and 1; 0.0 = no filter)
<INPUT TYPE = "TEXT"   Name = "ce_threshold"  VALUE = "<?=$ce_threshold?>" SIZE="4" />



</FORM>


<?php




if ( $button == "TranslateBox" && $_POST['to_translate'] != "") {
  $to_translate = $_POST['to_translate'];
  print "<hr/><b>Translating: </b> $to_translate <br/>";
  print "<b>Context: </b> $context <br/>";
  print "<b>Processed on: </b> " . `date` . "<br/>";
  $client = "";
  try {
    $client = new SoapClient($WSDL);
  } catch (SoapFault $exception) {  
    print "<HR/><b>SOAP Fault: </b>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}<BR/>";
  }

  try {
    print "<hr/><b>Portage getTranslation() replied: </b>";
    print $client->getTranslation($to_translate);
    //print "<br/><b>Trace: </b>"; var_dump($client);
  } catch (SoapFault $exception) {  
    print "<b>SOAP Fault: </b>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}<BR/>";
  }

  try {
    print "<hr/><b>getTranslation2() replied: </b>";
    print $client->getTranslation2($to_translate, $context);
    //print "<br/><b>Trace: </b>"; var_dump($client);
  } catch (SoapFault $exception) {  
    print "<b>SOAP Fault: </b>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}<BR/>";
  }

  try {
    print "<hr/><b>getTranslationCE() replied: </b>";
    print $client->getTranslationCE($to_translate, $context);
    //print "<br/><b>Trace: </b>"; var_dump($client);
  } catch (SoapFault $exception) {  
    print "<b>SOAP Fault: </b>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}<BR/>";
  }



  #try {
  #} catch (SoapFault $exception) {
  #  if ($exception->faultcode == "PortageContext") {
  #    print "<HR/><b>PortageContext error: </b>{$exception->faultstring}<br/>";
  #  } else {
  #    print "<HR/><b>Unknown SOAP Fault: </b>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}<BR/>";
  #    print "<br/><b>exception: </b>";
  #    var_dump($exception);
  #    print "<br/><b>client: </b>";
  #    var_dump($client);
  #  }
  #}
}
else
if ( $button == "TranslateTMX" && $_FILES["tmx_filename"]["name"] != "") {
  $tmx_filename = $_FILES["tmx_filename"]["name"];
  print "<hr/><b>Translating TMX file: </b> $tmx_filename <br/>";
  print "<b>Context: </b> $context <br/>";
  print "<b>Processed on: </b> " . `date` . "<br/>";

  if ( is_uploaded_file($_FILES["tmx_filename"]["tmp_name"]) ) {
    $tmp_file = $_FILES["tmx_filename"]["tmp_name"];
    #print "<br/><b>TMX Upload OK.  Trace: </b> " . print_r($_FILES["tmx_filename"], true);
    $tmx_contents = file_get_contents($tmp_file);
    $tmx_contents_base64 = base64_encode($tmx_contents);
    #print "<br/><b>file contents len: </b> " . strlen($tmx_contents) .
    #      " <b>base64 len: </b> " . strlen($tmx_contents_base64);
    
  try {
      $client = new SoapClient($WSDL);

      $ce_threshold += 0;
      $reply = $client->translateTMXCE($tmx_contents_base64, $tmx_filename, $context, $ce_threshold);
      print "<hr/><b>Portage replied: </b>$reply";
      print "<br/><a href=\"$reply\">Monitor job interactively</a>";
      $monitor_token=$reply;
      #print "<br/><b>Trace: </b>"; var_dump($client);
    } catch (SoapFault $exception) {  
      print "<HR/><b>SOAP Fault: </b>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}<BR/>";
  }

  } else {
    print "<br/><b>TMX Upload error.  Trace: </b> " . print_r($_FILES["tmx_filename"], true);
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
    print "<br/><b>Error code description: </b> {$file_error_codes[$_FILES["tmx_filename"]["error"]]}";
  }


  print "<hr/><b>Done processing on: </b>" . `date`;
}
else
if ( $button == "MonitorJob" && !empty($monitor_token) ) {
  try {
    $client = new SoapClient($WSDL);
    $reply = $client->translateTMXCE_Status($monitor_token);
    print "<hr/><b>Job status: </b> $reply";
    if ( preg_match("/^0 Done: (\S*)/", $reply, $matches) )
      print "<br/>Right click and save: <a href=\"$matches[1]\">Output TMX</a>";
    print "<br/><a href=\"$monitor_token\">Switch to interactive job monitoring</a>";
  } catch (SoapFault $exception) {  
    print "<HR/><b>SOAP Fault: </b>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}<BR/>";
  }
}


?>

<hr/>
<FORM action="" method="post" name="formulaire2" target="_self">
Job Token:
<INPUT TYPE = "TEXT"   Name = "monitor_token"       VALUE = "<?=$monitor_token?>" />
<INPUT TYPE = "Submit" Name = "MonitorJob"  VALUE = "Monitor Job via SOAP"/>
</FORM>


<hr/>
<table width="100%" cellspacing="0" cellpadding="0" border="0">
   <tr>
      <td width="20%" align="right" valign="bottom">
	 <img alt="NRC-IIT - Institute for Information Technology" src="/images/iit_sidenav_graphictop_e.gif" height="54" />
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
	 <img alt="NRC-IIT - Institute for Information Technology" src="/images/iit_sidenav_graphicbottom_e.gif" />
      </td> 
      <td align="center" valign="top">
	 <small>Technologies langagi&egrave;res interactives / Interactive Language Technologies <br /> Institut de technologie de l'information / Institute for Information Technology <br /> Conseil national de recherches Canada / National Research Council Canada <br /> Copyright 2004&ndash;2010, Sa Majest&eacute; la Reine du Chef du Canada /  Her Majesty in Right of Canada <br /> <a href="/portage_notices.html">Third party Copyright notices</a>
	 </small>
      </td>
   </tr>
</table>

</body>
</html>
