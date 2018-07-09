<?php
# @file soap.php
# @brief Test web page for the SOAP API to PortageLive
#
# @author Patrick Paul, Eric Joanis and Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009 - 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009 - 2015, Her Majesty in Right of Canada

# This works if this file and the WSDL and PortageLive service are on the same
# machine and in the same directory:
$WSDL="PortageLiveAPI.wsdl";
# In the general case, uncomment and modify this statement to point to where
# your PortageLive server actually exists:
$WSDL="http://__REPLACE_THIS_WITH_YOUR_IP__/PortageLiveAPI.wsdl";

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
      if ( array_key_exists('TranslatePlainText', $_POST) )
         $button = "TranslatePlainText";
      if ( array_key_exists('updateFixedTerms', $_POST) )
         $button = "updateFixedTerms";
      if ( array_key_exists('getFixedTerms', $_POST) )
         $button = "getFixedTerms";
      if ( array_key_exists('removeFixedTerms', $_POST) )
         $button = "removeFixedTerms";
      if ( array_key_exists('Prime', $_POST) )
         $button = "Prime";
      if ( array_key_exists('IncrAddSentence', $_POST) )
         $button = "IncrAddSentence";
      if ( array_key_exists('IncrStatus', $_POST) )
         $button = "IncrStatus";
   }

   if ( array_key_exists('MonitorJob', $_POST) )
      $button = "MonitorJob";

   if ( array_key_exists('ce_threshold', $_POST) )
      $ce_threshold = $_POST['ce_threshold'] + 0;
   else
      $ce_threshold = 0;

   if ( array_key_exists('monitor_token', $_POST) )
      $monitor_token = $_POST['monitor_token'];
}
else {
   $ce_threshold = 0;
}

function displayFault($exception, $title = "SOAP Fault:") {
   print "<div class='displayFault'>\n";
   print "<header class='ERROR'>$title</header>\n";
   echo "<pre>";
   echo "Fault code: " . $exception->faultcode . "\n";
   echo "Fault string: " . $exception->faultstring . "\n";
   echo "Fault message: " . $exception->getMessage() . "\n";
   echo "Fault trace: " . var_dump($exception->getTrace()) . "\n";
   echo "Fault line: " . $exception->getLine() . "\n";
   echo "</pre>";
   print "</div>\n";
}

function listContext($WSDL, $context) {
   try {
      $client = new SoapClient($WSDL);
      $call = $client->getAllContexts(false);
      $tokens = preg_split("/;/", $call);

      print "<SELECT NAME = \"context\">\n";
      foreach ($tokens as $token) {
         if ($context == $token) {
            print "<OPTION SELECTED=\"selected\" VALUE=\"$token\">$token</OPTION>\n";
         }
         else {
            print "<OPTION VALUE=\"$token\">$token</OPTION>\n";
         }
      }
      print "<OPTION VALUE=\"InvalidContext\">Invalid context for debugging</OPTION>\n";
      print "</SELECT>\n";
   }
   catch (SoapFault $exception) {
      displayFault($exception, "SOAP Fault trying to list contexts:");
   }
}

function getAllContexts($verbose) {
   try {
      global $WSDL;
      $client = new SoapClient($WSDL);
      return $client->getAllContexts($verbose);
   }
   catch (SoapFault $exception) {
      displayFault($exception, "SOAP Fault trying to list contexts:");
   }
}

function getVersion() {
   try {
      global $WSDL;
      $client = new SoapClient($WSDL);
      return $client->getVersion();
   }
   catch (SoapFault $exception) {
      displayFault($exception, "SOAP Fault trying to get version:");
   }
}

# @param type  either translateTMXCE or translateSDLXLIFFCE which represent
#              what function to call depending on what is the type of the file
#              argument.
# @paran file  $_FILES["sdlxliff_filename"] or $_FILES["tmx_filename"]
function processFile($type, $file) {
   global $context;
   global $ce_threshold;

   $filename = $file["name"];
   $file_xtags = array_key_exists('file_xtags', $_POST);
   $use_Confidence_Estimation = array_key_exists('use_Confidence_Estimation', $_POST);

   print "<section id='file_translation'>\n";
   print "<header>$type</header>\n";
   print "<b>Translating using $type and file: </b> $filename <br/>";
   print "<b>Context: </b> $context <br/>";
   print "<b>CE: </b> $use_Confidence_Estimation <b>CE thr: " . (0 + $ce_threshold) . " <b>xtags: </b> $file_xtags <br/>";
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
         if ($type == "translatePlainText") {
            $reply = $client->$type($tmp_contents_base64, $filename, $context, $use_Confidence_Estimation, $file_xtags);
         } else {
            $reply = $client->$type($tmp_contents_base64, $filename, $context, $use_Confidence_Estimation, $ce_threshold, $file_xtags);
         }

         print "<b>Portage replied: </b>$reply";
         print "<br/><a href=\"$reply\">Monitor job interactively</a>";
         global $monitor_token;
         $monitor_token = $reply;
         //print "<br/><b>Trace: </b>"; var_dump($client);
      }
      catch (SoapFault $exception) {
         displayFault($exception);
      }

   }
   else {
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
   print "</section>\n";
}

function primeTestCase($WSDL, $context, $PrimeMode) {
   print "<section id='prime'>\n";
   print "<header>Prime</header>\n";
   try {
      $PrimeMode = $_POST['PrimeMode'];
      $client = new SoapClient($WSDL);
      $rc = $client->primeModels($context, $PrimeMode);
      print "Prime Models ($context, $PrimeMode) rc = $rc<br />";  // DEBUGGING
      # For backwards-compatibility, test the old ("OK") and new (true) return value
      if ($rc === true || $rc == "OK")
         print "<div class=\"PRIME SUCCESS\">Primed successfully!</div>";
      else {
         // This case should never happen since primeModels either returns true or a soapFault.
         print "<div class=\"PRIME ERROR\">INVALID return code.</div>";
      }
   }
   catch (SoapFault $exception) {
      print "<div class=\"PRIME ERROR\">\n";
      displayFault($exception, "Error with primeModels");
      print "</div>\n";
   }
   print "</section\n>";
}

function updateFixedTermsTestCase($WSDL) {
   #print_r($_POST);  // Nice for debug POST's values.
   #print_r($_FILES);  // Nice for debug POST's values.
   global $context;
   $file = $_FILES["fixedTermsFilename"];
   $filename = $file["name"];
   print "<section id='updateFixedTermesResponse'>\n";
   print "<header>Update Fixed Terms</header>\n";
   print "<b>Updating fixed terms using file: </b> $filename <br/>";
   print "<b>Context: </b> $context <br/>";
   print "<b>Processed on: </b> " . `date` . "<br/>";

   $sourceColumnIndex = -1;
   if ( array_key_exists('fixedTermsSourceColumn', $_POST) ) {
      $sourceColumnIndex = $_POST['fixedTermsSourceColumn'];
   }
   else {
      print "<div class=\"ERROR\">Error updating fixed terms.  Missing source column.</div>";
      return;
   }

   $sourceLanguage = 'UNDEF';
   if ( array_key_exists('fixedTermsSourceLanguage', $_POST) ) {
      $sourceLanguage = $_POST['fixedTermsSourceLanguage'];
   }
   else {
      print "<div class=\"ERROR\">Error updating fixed terms.  Missing source language.</div>";
      return;
   }

   $targetLanguage = 'UNDEF';
   if ( array_key_exists('fixedTermsTargetLanguage', $_POST) ) {
      $targetLanguage = $_POST['fixedTermsTargetLanguage'];
   }
   else {
      print "<div class=\"ERROR\">Error updating fixed terms.  Missing target language.</div>";
      return;
   }

   $encoding = 'UNDEF';
   if ( array_key_exists('encoding', $_POST) ) {
      $encoding = $_POST['encoding'];
   }
   else {
      print "<div class=\"ERROR\">Error updating fixed terms.  Missing encoding.</div>";
      return;
   }

   if ( is_uploaded_file($file["tmp_name"]) ) {
      $tmp_file = $file["tmp_name"];
      //print "<br/><b>$type Upload OK.  Trace: </b> " . print_r($file, true);
      $tmp_contents = file_get_contents($tmp_file);
      //print "<br/><b>file contents len: </b> " . strlen($tmp_contents) .
      //      " <b>base64 len: </b> " . strlen($tmp_contents);

      try {
         $client = new SoapClient($WSDL);

         $reply = $client->updateFixedTerms($tmp_contents, $filename, $encoding, $context, $sourceColumnIndex, $sourceLanguage, $targetLanguage);

         if ($reply)
            print "<div class=\"SUCCESS\">Updated fixed terms successfully!</div>";
         else
            print "<div class=\"ERROR\">Error updating fixed terms.</div>";
      }
      catch (SoapFault $exception) {
         displayFault($exception);
      }
   }
   else {
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
   print "</section>\n";
}

function getFixedTermsTestCase($WSDL, $context) {
   print "<section id='getFixedTermsResponse'>\n";
   print "<header>Get Fixed Terms List</header>\n";
   try {
      $client = new SoapClient($WSDL);

      $reply = $client->getFixedTerms($context);

      if ($reply) {
         print "<div class=\"SUCCESS\">Get Fixed terms successfully!</div>";
         print "<textarea name=\"fixed terms list\" rows=\"10\" cols=\"50\">$reply</textarea>\n";
      }
      else
         print "<div class=\"ERROR\">Error getting fixed terms.</div>";
   }
   catch (SoapFault $exception) {
      displayFault($exception);
   }
   print "</section>\n";
}

function removeFixedTermsTestCase($WSDL, $context) {
   print "<section id='removeFixedTermsResponse'>\n";
   print "<header>Remove Fixed Terms List</header>\n";
   try {
      $client = new SoapClient($WSDL);

      $reply = $client->removeFixedTerms($context);

      if ($reply) {
         print "<div class=\"SUCCESS\">Remove fixed terms successfully!</div>";
      }
      else
         print "<div class=\"ERROR\">Error removing fixed terms.</div>";
   }
   catch (SoapFault $exception) {
      displayFault($exception);
   }
   print "</section>\n";
}

function monitorJobTestCase($WSDL, $button, $monitor_token) {
   try {
      $client = new SoapClient($WSDL);
      $reply = $client->translateFileStatus($monitor_token);
      $reply = htmlentities($reply);
      print "<hr/><b>Job status: </b> $reply";
      if ( preg_match("/^0 Done: (\S*)/", $reply, $matches) )
         print "<br/>Right click and save: <a href=\"$matches[1]\">Translated content</a>";
      print "<br/><a href=\"$reply\">Switch to interactive job monitoring</a>";
   }
   catch (SoapFault $exception) {
      displayFault($exception);
   }
}

function translateTestCase($WSDL, $context) {
   $newline = "p";
   if ( array_key_exists('newline', $_POST) )
      $newline = $_POST['newline'];

   $to_translate_xtags = array_key_exists('to_translate_xtags', $_POST);
   $to_translate = $_POST['to_translate'];

   print "<section id='source_text'>\n";
   print "<header>Source text</header>\n";
   print "<b>Translating: </b>";
   print "<div><code>" . nl2br(htmlspecialchars($to_translate)) . "</code></div>";
   print "<b>Context:</b> $context <br/>";
   print "<b>newline:</b> $newline <br/>";
   echo "<b>xtags?:</b> ", ($to_translate_xtags ? "TRUE" : "FALSE"), "<br/>";
   print "<b>Processed on: </b> " . `date` . "<br/>";
   print "</section>\n";

   $client = "";
   try {
      $client = new SoapClient($WSDL);
   }
   catch (SoapFault $exception) {
      displayFault($exception);
   }


   print "<section id='translateWithDefaultContext'>\n";
   $start_time = microtime(true);
   try {
      print "<header>translate() with default context</header>\n";
      print "<b>Portage translate() replied: </b>";
      $reply = nl2br(htmlspecialchars($client->translate($to_translate, "context", $newline, $to_translate_xtags, false)));
      print "<div><code>\n";
      print "$reply\n";
      print "</code></div>\n";
      //print "<br/><b>Trace: </b>"; var_dump($client);
   }
   catch (SoapFault $exception) {
      displayFault($exception);
   }
   $end_time = microtime(true);
   printf("<b>Translating took: </b>%.2f seconds <br/>", $end_time-$start_time);
   print "<b>Finished at: </b>" . `date` . "<br/>";
   print "</section>\n";


   print "<section id='translateWithoutConfidenceEstimation'>\n";
   $start_time = microtime(true);
   try {
      print "<header>translate() without Confidence Estimation</header>\n";
      print "<b>translate() replied: </b>";
      $reply = nl2br(htmlspecialchars($client->translate($to_translate, $context, $newline, $to_translate_xtags, false)));
      print "<div><code>\n";
      print "$reply\n";
      print "</code></div>\n";
      //print "<br/><b>Trace: </b>"; var_dump($client);
   }
   catch (SoapFault $exception) {
      displayFault($exception);
   }
   $end_time = microtime(true);
   printf("<b>Translating took: </b>%.2f seconds <br/>", $end_time-$start_time);
   print "<b>Finished at: </b>" . `date` . "<br/>";
   print "</section>\n";


   print "<section id='translateWithConfidenceEstimation'>\n";
   $start_time = microtime(true);
   try {
      print "<header>translate() with Confidence Estimation</header>\n";
      print "<b>translate() replied: </b>";
      $reply = nl2br(htmlspecialchars($client->translate($to_translate, $context, $newline, $to_translate_xtags, true)));
      print "<div><code>\n";
      print "$reply\n";
      print "</code></div>\n";
      //print "<br/><b>Trace: </b>"; var_dump($client);
   }
   catch (SoapFault $exception) {
      displayFault($exception);
   }
   $end_time = microtime(true);
   printf("<b>Translating took: </b>%.2f seconds <br/>", $end_time-$start_time);
   print "<b>Finished at: </b>" . `date` . "<br/>";
   print "</section>\n";



   /*
      try {
      }
      catch (SoapFault $exception) {
      if ($exception->faultcode == "PortageContext") {
      print "<HR/><b>PortageContext error: </b>{$exception->faultstring}<br/>";
      }
      else {
      print "<HR/><b>Unknown SOAP Fault: </b>faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring}<BR/>";
      print "<br/><b>exception: </b>";
      var_dump($exception);
      print "<br/><b>client: </b>";
      var_dump($client);
      }
      }
    */
}

function incrAddSentenceTestCase($WSDL) {
   print "<section id='incrAddSentenceResponse'>\n";
   print "<header>Incremental Add Sentence Pair</header>\n";
   global $context;
   $doc_id = $_POST['incrAddSentence_document_id'];
   $source_sent = $_POST['incrAddSentence_source_sent'];
   $target_sent = $_POST['incrAddSentence_target_sent'];
	$context = htmlentities($context);
	$doc_id = htmlentities($doc_id);
	$source_sent = htmlentities($source_sent);
	$target_sent = htmlentities($target_sent);
   print "<b>Context: </b> $context<br>\n";
   print "<b>Document ID: </b> $doc_id<br>\n";
   print "<b>Source sentence: </b> $source_sent<br>\n";
   print "<b>Target sentence: </b> $target_sent<br>\n";
   try {
      $client = new SoapClient($WSDL);
      $reply = $client->incrAddSentence($context, $doc_id, $source_sent, $target_sent, "");
		$reply = htmlentities($reply);
      print "<b>Reply: </b> $reply";
   }
   catch (SoapFault $exception) {
      displayFault($exception);
   }
   print "</section>\n";
}

function incrStatusTestCase($WSDL) {
   print "<section id='incrStatusResponse'>\n";
   print "<header>Incremental Training Status</header>\n";
   global $context;
   $doc_id = $_POST['incrStatus_document_id'];
	$context = htmlentities($context);
   $doc_id = htmlentities($doc_id);
   print "<b>Context: </b> $context<br>\n";
   print "<b>Document ID: </b> $doc_id<br>\n";
   try {
      $client = new SoapClient($WSDL);
      $reply = $client->incrStatus($context, $doc_id);
		$reply = htmlentities($reply);
      print "<b>Reply: </b> $reply";
   }
   catch (SoapFault $exception) {
      displayFault($exception);
   }
   print "</section>\n";
}


function testSuite($WSDL) {
   global $button;
   global $context;
   global $PrimeMode;
   global $monitor_token;

   if ( $button != "" ) {
      print "<a name='testcase_output_anchor'>\n";
   }

   if ( $button == "Prime"  && $_POST['Prime'] != "" ) {
      primeTestCase($WSDL, $context, $PrimeMode);
   }
   else
   if ( $button == "updateFixedTerms" && $_FILES['fixedTermsFilename']['name'] != "") {
      updateFixedTermsTestCase($WSDL);
   }
   else
   if ( $button == "getFixedTerms") {
      getFixedTermsTestCase($WSDL, $context);
   }
   else
   if ( $button == "removeFixedTerms") {
      removeFixedTermsTestCase($WSDL, $context);
   }
   else
   if ( $button == "TranslateBox" && $_POST['to_translate'] != "") {
      $expanded_context = $_POST['translate_document_id']
         ? $context . "/" . $_POST['translate_document_id']
         : $context;
      #print "expanded_context = $expanded_context<br/>";
      translateTestCase($WSDL, $expanded_context);
   }
   else
   if ( $button == "TranslateTMX" && $_FILES["tmx_filename"]["name"] != "") {
      processFile("translateTMX", $_FILES["tmx_filename"]);
   }
   else
   if ($button == "TranslateSDLXLIFF" && $_FILES["sdlxliff_filename"]["name"] != "") {
      processFile("translateSDLXLIFF", $_FILES["sdlxliff_filename"]);
   }
   else
   if ($button == "TranslatePlainText" && $_FILES["plain_text_filename"]["name"] != "") {
      processFile("translatePlainText", $_FILES["plain_text_filename"]);
   }
   else
   if ( $button == "MonitorJob" && !empty($monitor_token) ) {
      monitorJobTestCase($WSDL, $button, $monitor_token);
   }
   else
   if ( $button == "IncrAddSentence" ) {
      incrAddSentenceTestCase($WSDL);
   }
   else
   if ( $button == "IncrStatus" ) {
      incrStatusTestCase($WSDL);
   }
}

?>

<!DOCTYPE HTML>
<html>
<head>
<title>PortageLiveAPI</title>
<STYLE type="text/css">
   div.PRIME {
      width: 70%;
      margin-left: auto;
      margin-right: auto;
   }
   .PRIME {
      border-width: 1;
      border: solid;
      //text-align: center;
      font-size: 1.2em;
      background-color: #FAFAD2;
      padding: 1px;
      margin: 1px
   }
   .SUCCESS {color: green}
   .ERROR { color: red; text-decoration: blink; font-weight: bold; }
   section {
      background-color: #89CCFF;
      /*border: solid;*/
      padding: 1px;
      margin: 3px
   }
   section header {color: #FF2130; background-color:#002253; font-size: 1.2em}
   #Portage_main {width:60%; margin-left:auto; margin-right:auto;}
   div.displayFault header {
      background-color: white;
   }
   div.displayFault header.ERROR ul li {
      text-align: left;
   }
</STYLE>
</head>

<body onload=' location.href="#testcase_output_anchor" '>
<header>
<p align="center"><img src="images/NRC_banner_e.jpg" /></p>
</header>

<div id="Portage_main">
<header>
<h1>PortageLive SOAP API test</h1>
</header>

This page demonstrates how the appliance can be used as a web service.

Link to <a href="<?php echo $WSDL;?>">the WSDL</a> and its
<a href="<?php echo $WSDL;?>.xml">auto-generated documentation</a>
(requires an XSLT interpreter, e.g., IE or Chrome).

<FORM action="" enctype="multipart/form-data" method="post" name="formulaire" target="_self">

<section id='context'>
<header>Context</header>
<br/> Context:
<?php listContext($WSDL, $context) ?>
</section>

<section id='prime'>
<header>Prime</header>
Prime:
<INPUT TYPE = "RADIO" NAME = "PrimeMode" VALUE="partial" CHECKED="checked" /> Partial
<INPUT TYPE = "RADIO" NAME = "PrimeMode" VALUE="full" /> Full
<INPUT TYPE = "RADIO" NAME = "PrimeMode" VALUE="bogus" /> Unsupported
<INPUT TYPE = "Submit" Name = "Prime"  VALUE = "Prime context"/>
</section>


<section id='updateFixedTermsRequest'>
<header>Update Fixed Terms</header>
<label for="fixedTermsSourceColumn">Source Column Index (1-base): </label>
<INPUT TYPE = "TEXT"   Name = "fixedTermsSourceColumn"  VALUE = "1" SIZE="4" />
<label for="fixedTermsSourceLanguage">Source Language: </label>
<INPUT TYPE = "TEXT"   Name = "fixedTermsSourceLanguage"  VALUE = "en" SIZE="4" />
<label for="fixedTermsTargetLanguage">Target Language: </label>
<INPUT TYPE = "TEXT"   Name = "fixedTermsTargetLanguage"  VALUE = "fr" SIZE="4" />
<label for="fixedTermsFilename">Fixed terms file: </label>
<SELECT NAME = "encoding">
   <OPTION SELECTED="selected" VALUE="UTF-8">UTF-8</OPTION>;
   <OPTION VALUE="cp-1252">cp-1252</OPTION>;
</SELECT>
<INPUT TYPE = "file"   Name = "fixedTermsFilename"/>
<INPUT TYPE = "Submit" Name = "updateFixedTerms"  VALUE = "Update Fixed Terms"/>
</section>


<section id='getFixedTermsRequest'>
<header>Get Fixed Terms</header>
<INPUT TYPE = "Submit" Name = "getFixedTerms"  VALUE = "Get Fixed Terms"/>
</section>


<section id='removeFixedTermsRequest'>
<header>Remove Fixed Terms</header>
<INPUT TYPE = "Submit" Name = "removeFixedTerms"  VALUE = "Remove Fixed Terms"/>
</section>


<section id="plain_text">
<header>Translate</header>
<!-- INPUT TYPE = "TEXT"   Name = "context"       VALUE = "<?php echo $context;?>" / -->
 Enter text here:
<table width="60%" border="0">
<tr>
<td>
<textarea name="to_translate" rows="10" cols="50"></textarea>
</td>
</tr>
<td>
<INPUT TYPE = "Submit" Name = "TranslateBox"  VALUE = "Translate Text"/>
</td>
</tr>
</table>

<div>
<hr/>
<i>Advanced options</i>
<table>
<tr valign="top">
<td>
<INPUT TYPE = "checkbox" Name = "to_translate_xtags" VALUE = "Process and transfer tags."/>
</td>
<td>
<b>xtags</b>
-- Check this box if input text contains tags and you want to process & transfer them.
</td>
</tr>
<tr valign="top">
<td>
<INPUT TYPE = "radio" Name = "newline" VALUE = "s"/>
</td>
<td>
<b>one sentence per line</b>
-- Check this box if input text has one sentence per line.
</td>
</tr>
<tr valign="top">
<td>
<INPUT TYPE = "radio" Name = "newline" VALUE = "p" checked = "checked"/>
</td>
<td>
<b>one paragraph per line</b>
-- Check this box if input text has one paragraph per line.
</td>
</tr>
<tr valign="top">
<td>
<INPUT TYPE = "radio" Name = "newline" VALUE = "w"/>
</td>
<td>
<b>blank lines mark paragraphs</b>
-- Check this box if input text has two consecutive newlines mark the end of a paragraph, otherwise newline is just whitespace.
</td>
</tr>
<tr valign="top"><td></td><td>
Document ID: <INPUT TYPE = "TEXT" Name = "translate_document_id" SIZE = 30 />
</td> </tr>
</table>
</div>
</section>

<section id='translating_xml_file'>
<header>Translate a file</header>
<INPUT TYPE = "hidden" Name = "MAX_FILE_SIZE" VALUE = "2000000" />
<table>
<tr>
<td>
 Translate a plain text file:
<INPUT TYPE = "file"   Name = "plain_text_filename"/>
</td>
<td>
 Translate a TMX file:
<INPUT TYPE = "file"   Name = "tmx_filename"/>
</td>
<td>
 Translate an SDLXLIFF file:
<INPUT TYPE = "file"   Name = "sdlxliff_filename"/>
</td>
</tr>
<tr>
<td>
<INPUT TYPE = "Submit" Name = "TranslatePlainText"  VALUE = "Translate plain text File"/>
</td>
<td>
<INPUT TYPE = "Submit" Name = "TranslateTMX"  VALUE = "Translate TMX File"/>
</td>
<td>
<INPUT TYPE = "Submit" Name = "TranslateSDLXLIFF"  VALUE = "Translate SDLXLIFF File"/>
</td>
</tr>
</table>

<div>
<hr/>
<i>Advanced options</i>
<table>
<tbody>
<tr>
<td>
<INPUT TYPE = "checkbox" Name = "file_xtags" VALUE = "Process and transfer tags."/>
</td>
<td>
<b>xtags</b>
-- Check this box if input text contains tags and you want to process &amp; transfer them.
</td>
</tr>
<tr valign="top">
<td>
<INPUT TYPE = "checkbox" Name = "use_Confidence_Estimation" VALUE = "Use Confidence Estimation if available."/>
</td>
<td>
<b>Confidence Estimation</b>
-- Check this box if you want to use Confidence Estimation when a system provides Confidence Estimation.
</td>
</tr>
<tr>
<td>
<INPUT TYPE = "TEXT"   Name = "ce_threshold"  VALUE = "<?php echo $ce_threshold;?>" SIZE="4" />
</td>
<td>
CE threshold for filtering (between 0 and 1; 0.0 = no filter)
</td>
</tbody>
</tr>
</table>
</div>
</section>

<section id='incrAddSentence'>
<header>incrAddSentence()</header>
Source sentence: <INPUT TYPE = "TEXT" Name = "incrAddSentence_source_sent" SIZE = 100 /> <br/>
Target sentence: <INPUT TYPE = "TEXT" Name = "incrAddSentence_target_sent" SIZE = 100 /> <br/>
Document ID: <INPUT TYPE = "TEXT" Name = "incrAddSentence_document_id" SIZE = 30 />
<INPUT TYPE = "Submit" Name = "IncrAddSentence"    VALUE = "incrAddSentence()"/>
</section>

<section id='incrStatus'>
<header>incrStatus()</header>
Document ID: <INPUT TYPE = "TEXT" Name = "incrStatus_document_id" SIZE = 30 />
<INPUT TYPE = "Submit" Name = "IncrStatus"    VALUE = "incrStatus()"/>
</section>

</FORM>


<section id='getAllContexts'>
<header>getAllContexts()</header>
<b>Contexts: </b> <?php print getAllContexts(false) ?> <br/>
<br/>
<b>Verbose contexts: </b> <?php print getAllContexts(true) ?> <br/>
</section>

<section id='getVersion'>
<header>getVersion()</header>
<b>Version: </b> <?php print getVersion() ?> <br/>
</section>

<?php testSuite($WSDL); ?>

<section id='monitor_job'>
<header>Monitor a job</header>
<FORM action="" method="post" name="formulaire2" target="_self">
Job Token:
<INPUT TYPE = "TEXT"   Name = "monitor_token" VALUE = "<?php echo $monitor_token;?>" />
<INPUT TYPE = "Submit" Name = "MonitorJob"    VALUE = "Monitor Job via SOAP"/>
</FORM>
</section>
</div>


<footer>
<table width="100%" cellspacing="0" cellpadding="0" border="0">
   <tr>
      <td width="20%" align="right" valign="center">
         <img alt="NRC-ICT" src="images/sidenav_graphic.png" height="44" />
      </td>
      <td width="60%" align="center" valign="bottom">
         <img width="286" alt="National Research Council Canada" src="images/mainf1.gif" height="44" />
      </td>
      <td width="20%" align="left" valign="center">
         <img width="93" alt="Government of Canada" src="images/mainWordmark.gif" height="44" />
      </td>
   </tr>
   <tr>
      <td>
      </td>
      <td align="center" valign="top">
         <small>Traitement multilingue de textes / Multilingual Text Processing <br />
            Technologies de l'information et des communications / Information and Communications Technologies <br />
            Conseil national de recherches Canada / National Research Council Canada <br />
            Copyright 2004&ndash;2014, Sa Majest&eacute; la Reine du Chef du Canada /  Her Majesty in Right of Canada <br />
            <a href="/portage_notices.php">Third party Copyright notices</a>
         </small>
      </td>
   </tr>
</table>
</footer>

</body>
</html>
