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
# Copyright 2009 - 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009 - 2015, Her Majesty in Right of Canada


$base_web_dir = "/var/www/html";
$base_portage_dir = "/opt/PortageII";

# produce debugging information
function debug($i) {
   if ( 0 ) {
      error_log($i, 3, '/tmp/PortageLiveAPI.debug.log');
      return " " . print_r($i, true);
   }
}

class PortageLiveAPI {

   # Load models in memory
   public function primeModels($context, $PrimeMode) {
      $rc = 0;
      $i = $this->getContextInfo($context);
      $this->validateContext($i);
      $command = "prime.sh $PrimeMode";
      $this->runCommand($command, "", $i, $rc, false);

      if ( $rc != 0 )
         throw new SoapFault("PortagePrimeError", "Failed to prime, something went wrong in prime.sh.\nrc=$rc; Command=$command");

      return true;
   }

   # Gather all relevant information about translation context $context
   public function getContextInfo($context) {
      $info = array();
      $info["context"] = $context;
      global $base_portage_dir;
      $info["context_dir"] = "$base_portage_dir/models/$context";
      $info["script"] = "$info[context_dir]/soap-translate.sh";
      if ( is_file($info["script"]) ) {
         $info["good"] = true;
         $cmdline = `tail -n -1 < $info[script]`;
         if ( preg_match('/(-decode-only|-with-ce|-with-rescoring)/', $cmdline, $matches) ) {
            $info["good"] = false;
            $info["label"] = "$context: context from a previous, incompatible version of PortageII";
         }
         else {
            $src = "";
            if ( preg_match('/-xsrc=([-a-zA-Z]+)/', $cmdline, $matches) )
               $src = $matches[1];
            else if ( preg_match('/-src=(\w+)/', $cmdline, $matches) )
               $src = $matches[1];
            $tgt = "";
            if ( preg_match('/-xtgt=([-a-zA-Z]+)/', $cmdline, $matches) )
               $tgt = $matches[1];
            else if ( preg_match('/-tgt=(\w+)/', $cmdline, $matches) )
               $tgt = $matches[1];
            #if ( is_file("$info[context_dir]/canoe.ini.cow") )
            #   $info["canoe_ini"] = "$info[context_dir]/canoe.ini.cow";
            #if ( is_file("$info[context_dir]/rescore.ini") )
            #   $info["rescore_ini"] = "$info[context_dir]/rescore.ini";
            if ( is_file("$info[context_dir]/ce_model.cem") )
               $info["ce_model"] = "$info[context_dir]/ce_model.cem";
            $info["label"] = "$context ($src --> $tgt)" .
                           (empty($info["ce_model"]) ? "" : " with CE");
         }
      }
      else {
         $info["good"] = false;
         $info["label"] = "$context: bad context";
      }
      return $info;
   }

   # Enumerate all installed contexts
   public function getAllContexts($verbose = false) {
      $contexts = array();
      global $base_portage_dir;
      $dirs = scandir("$base_portage_dir/models");
      foreach ($dirs as $dir) {
         $info = $this->getContextInfo($dir);
         if ($info["good"]) {
            if ( $verbose )
               $contexts[] = $info['label'];
            else
               $contexts[] = $dir;
         }
      }
      return join(";",$contexts);
   }

   # Validate the context described in context info $i, throwing SoapFault if
   # there are errors.
   # If $need_ce is true, also check that the context provides confidence estimation.
   private function validateContext(&$i, $need_ce = false) {
      $context = $i["context"];
      if ( ! $i["good"] ) {
         if (!file_exists($i["context_dir"])) {
            throw new SoapFault("PortageContext", "Context \"$context\" does not exist.\n" . debug($i));
         }
         else {
            throw new SoapFault("PortageContext", "Context \"$context\" is broken.\n" . debug($i));
         }
      }
      if ( $need_ce && empty($i["ce_model"]) ) {
         throw new SoapFault("PortageContext", "Context \"$context\" does not support confidence estimation.\n" . debug($i));
      }
   }

   # Run $command, giving it $src_string as input, using context info $i.
   # Returns the STDOUT of $command.
   # Throws SoapFault (faultcode="PortageServer") if there is a problem.
   # If $command has a non-zero exit status:
   #   If $exit_status is NULL, throws SoapFault(faultcode="PortageServer")
   #   If $exit_status is not NULL, set $exit_status to $command's exit status.
   # If $wantoutput is false, STDERR and STDOUT are discarded; required to
   # launch a background job using &, for example.
   private function runCommand($command, $src_string, &$i, &$exit_status = NULL, $wantoutput = true) {
      $cwd = "/tmp";
      global $base_portage_dir;
      $context_dir = $i["context_dir"];
      $env = array(
         'PORTAGE'         => "$base_portage_dir",
         'LD_LIBRARY_PATH' => "$context_dir/lib:$base_portage_dir/lib:/lib:/usr/lib",
         'PATH'            => "$context_dir/bin:$i[context_dir]:$base_portage_dir/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
         'PERL5LIB'        => "$context_dir/lib:$base_portage_dir/lib",
         'PYTHONPATH'      => "$context_dir/lib:$base_portage_dir/lib",
         'PORTAGE_INTERNAL_CALL' => 1
      );

      #error_log(print_r($env, true), 3, '/tmp/PortageLiveAPI.debug.log');

      $descriptorspec = array(
         0 => array("pipe", "r"),  // stdin is a pipe that the child will read from
      );
      if ( $wantoutput ) {
         // stdout is a pipe that the child will write to
         $descriptorspec[1] = array("pipe", "w");
         // stderr is a file to write to
         $descriptorspec[2] = array("file", "/tmp/error-output.txt", "a");
      }
      else {
         $descriptorspec[1] = array("file", "/dev/null", "a");
         $descriptorspec[2] = array("file", "/dev/null", "a");
      }
      $process = proc_open($command, $descriptorspec, $pipes, $cwd, $env);

      if (is_resource($process)) {
         // $pipes now looks like this:
         // 0 => writeable handle connected to child stdin
         // 1 => readable handle connected to child stdout
         // Any error output will be appended to /tmp/error-output.txt

         fwrite($pipes[0], $src_string);
         fclose($pipes[0]);

         $my_retval = "";
         if ( $wantoutput ) {
            $my_retval = stream_get_contents($pipes[1]);
            fclose($pipes[1]);
         }

         // It is important that you close any pipes before calling
         // proc_close in order to avoid a deadlock
         $return_value = proc_close($process);
         if ( $return_value != 0 ) {
            if ( is_null($exit_status) )
               throw new SoapFault("PortageServer",
                  "non-zero return code from $command: $return_value\n".debug($i),
                  "PortageLiveAPI");
            else
               $exit_status = $return_value;
         }
         else {
            if (!is_null($exit_status))
               $exit_status = 0;
         }
      }
      else {
         throw new SoapFault("PortageServer", "failed to run $translate_script: $!\n".debug($i));
      }

      return $my_retval . debug($i);
   }

   # Create a working directory based on $filename.
   # Throws SoapFault (faultcode=PortageServer) in case of error.
   # Returns the name of the directory created.
   private function makeWorkDir($filename) {
      date_default_timezone_set("UTC");
      $timestamp = date("Ymd\THis\Z");
      $base = $this->normalizeName("SOAP_{$filename}_{$timestamp}");
      global $base_web_dir;
      $work_path = "$base_web_dir/plive/";
      $dir = `mktemp -d $work_path{$base}_XXXXXX 2>&1`;
      if ( strpos($dir, "$work_path$base") === 0 )
	 return rtrim($dir);
      else
	 throw new SoapFault("PortageServer", "can't create temp work dir for $filename: $dir : $base");
   }

   # Normalize a name to keep only alphanumeric, dash, underscore, dot and plus
   private function normalizeName($filename) {
      return preg_replace(array("/[^-_.+a-zA-Z0-9]/", '/[ .]$/', '/^[ .]/'), array("", '', ''), $filename);
   }

   # Translate a file using model $context and the confidence threshold
   # $ce_threshold.  A threshold of 0 means keep everything.
   # $contents_base64 is a string containing the full content of the xml file, in Base64 encoding.
   # $filename is the name of the XML file.
   # If $xtags is true, transfer tags from the source-language segments to the translations.
   # $type indicates the type of XML file we're translating
   private function translateFileCE($contents_base64, $filename, $context, $useCE, $ce_threshold, $xtags, $type) {
      #error_log("translateFileCE with $type\n", 3, '/tmp/PortageLiveAPI.debug.log');

      #if ($type === "plaintext" and !$useCE and $ce_threshold != 0) {
      #   # TODO send a meaningful SoapFault
      #   throw new SoapFault("Client", "Translating plain text file");
      #}

      $i = $this->getContextInfo($context);
      $this->validateContext($i, $ce_threshold > 0);
      $is_xml = ($type === "tmx" or $type === "sdlxliff");

      $work_dir = $this->makeWorkDir("{$context}_$filename");
      $work_name = $this->normalizeName("{$context}_$filename");
      $contents = base64_decode($contents_base64);
      $info = "len: " . strlen($contents) . " base64 len: " .
              strlen($contents_base64) . "<br/>";
      #return $info;
      $local_file = fopen("$work_dir/Q.in", "w");
      if ( !is_resource($local_file) )
         throw new SoapFault("PortageServer", "failed to write $type to local file");
      $bytes_written = fwrite($local_file, $contents);
      if ( $bytes_written != strlen($contents) )
         throw new SoapFault("PortageServer", "incomplete write of $type to local file " .
                             "(wrote $bytes_written; expected ".strlen($contents).")");
      fclose($local_file);
      $contents="";
      $contents_base64="";

      if ($is_xml) {
         $xml_check_rc = "";
         $xml_check_log = $this->runCommand("ce_tmx.pl check $work_dir/Q.in 2>&1",
               "", $i, $xml_check_rc);
         #return "TMX check log: $xml_check_log; xml_check_rc: $xml_check_rc";
         if ( $xml_check_rc != 0 )
            throw new SoapFault("Client", "$type check failed for $filename: $xml_check_log\n$work_dir/Q.in");
      }

      #$xml_lang = array("fr" => "FR-CA", "en" => "EN-CA"); # add more languages here as needed
      $command = "$i[script] ";  # Requires that last space.
      $command .= ($is_xml ? " -xml " : "");
      if ($useCE and !empty($i["ce_model"])) {
         $command .= "-with-ce ";
         if ($ce_threshold > 0) {
            $command .= "-filter=$ce_threshold ";
         }
      }
      else {
         $command .= "-decode-only ";
      }
      $command .= "-nl=s -dir=\"$work_dir\" -out=\"$work_dir/P.out\" " .
                 ($xtags ? " -xtags " : "") .
                 "\"$work_dir/Q.in\" >& \"$work_dir/trace\" ";
      if ($is_xml)
         $command = "(if ($command); then ln -s QP.xml $work_dir/PLive-$work_name; fi; touch $work_dir/done)& disown %1";
      else
         $command = "(if ($command); then ln -s $work_dir/P.out $work_dir/PLive-$work_name; fi; touch $work_dir/done)& disown %1";

      $start_time = time();
      $result = $this->runCommand($command, "", $i, $exit_status, false);
      $info2 = "result len: " . strlen($result) . "<br/>";
      $monitor = "http://" . $_SERVER['SERVER_NAME'] .
                 "/cgi-bin/plive-monitor.cgi?" .
                 "time=$start_time&" .
                 "file=PLive-$work_name&" .
                 "context=$context&" .
                 "dir=plive/" . basename($work_dir) . "&" .
                 "ce=" . (!empty($i["ce_model"]) ? "1" : "0");
      return $monitor;
   }

   public function translateTMX($TMX_contents_base64, $TMX_filename, $context, $useCE, $ce_threshold, $xtags) {
      return $this->translateFileCE($TMX_contents_base64, $TMX_filename, $context, $useCE, $ce_threshold, $xtags, "tmx");
   }

   public function translateSDLXLIFF($SDLXLIFF_contents_base64, $SDLXLIFF_filename, $context, $useCE, $ce_threshold, $xtags) {
      return $this->translateFileCE($SDLXLIFF_contents_base64, $SDLXLIFF_filename, $context, $useCE, $ce_threshold, $xtags, "sdlxliff");
   }

   public function translatePlainText($PlainText_contents_base64, $PlainText_filename, $context, $useCE, $xtags) {
      $ce_threshold = 0;
      return $this->translateFileCE($PlainText_contents_base64, $PlainText_filename, $context, $useCE, $ce_threshold, $xtags, "plaintext");
   }

   public function translateFileStatus($monitor_token) {
      $tokens = preg_split("/[?&]/", $monitor_token);
      $info = array();
      foreach ($tokens as $token) {
         if (preg_match("/(.*?)=(.*)/", $token, $matches)) {
            $info[$matches[1]] = $matches[2];
         }
      }
      #return debug($info);
      $required_keys = array("file", "dir");
      foreach ($required_keys as $key) {
         if (!array_key_exists($key, $info))
            return "3 Invalid job token ($monitor_token): missing \"$key\" field";
      }

      global $base_web_dir;
      $dir = "$base_web_dir/$info[dir]";
      if (is_dir($dir)) {
         if (is_file("$dir/done")) {
            if (is_file("$dir/$info[file]")) {
               return "0 Done: $info[dir]/$info[file]";
            }
            else {
               return "2 Failed".debug($info).": $info[dir]/trace";
            }
         }
         else {
            $linestodo = `cat $dir/q.tok 2> /dev/null | wc -l 2> /dev/null`;
            $linesdone = 0;
            if ( $info['ce'] )
               $linesdone = `cat $dir/p.raw 2> /dev/null | wc -l 2> /dev/null`;
            else
               $linesdone = `cat $dir/p.dec 2> /dev/null | wc -l 2> /dev/null`;
            $progress = 0;
            if ( $linestodo > 0 ) {
               if ( $linesdone < $linestodo )
                  $progress = intval(($linesdone / $linestodo) * 89);
               else
                  $progress = 90;
            }
            return "1 In progress ($progress% done)".debug($info);
         }
      }
      else {
         return "3 Dir not found".debug($info);
      }
   }

   # This function would be nice to use but we have case aka "a<1>b</1>" is not
   # valid xml since tag aren't allowed to start with digits.
   private function checkIsThisXML($string) {
      $test = "<?xml version='1.0'" . "?" . ">\n<document>" . $string . "</document>";
      #$test = "<document>" . $string . "</document>";
      if (simplexml_load_string($test) == FALSE) {
         throw new SoapFault("PortageNotXML", "This is invalid xml.\n" . htmlspecialchars($string) . htmlspecialchars($test));
      }
   }

   # param src_string:  input to translate
   # param context:  translate src_string using what context
   # param newline:  what is the interpretation of newline in the input
   # param xtags:  Transfer tags
   # param useCE:  Should we use confidence estimation?
   public function translate($src_string, $context, $newline, $xtags, $useCE) {
      #$this->checkIsThisXML($src_string);
      if (!isset($newline))
         throw new SoapFault("PortageBadArgs", "You must defined newline.\nAllowed newline types are: s, p or w");

      if (!($newline == "s" or $newline == "p" or $newline == "w"))
         throw new SoapFault("PortageBadArgs", "Illegal newline type " . $newline . "\nAllowed newline types are: s, p or w");

      $i = $this->getContextInfo($context);
      $this->validateContext($i, $useCE);

      $options = " -verbose";
      $options .= ($useCE ? " -with-ce" : " -decode-only");
      $options .= ($xtags ? " -xtags" : "");
      $options .= " -nl=" . $newline;
      #$options .= " -dir=/tmp/";  # SAM DEBUGGING

      return $this->runCommand($i["script"] . $options, $src_string, $i);
   }

   # param  ContentsBase64 is the contents of the fixed term file in base64 encoding.
   # param  Filename is the fixed term file name.
   # param  encoding is the fixed term file's encoding and can be either UTF-8 or CP-1252.
   # param  context must be a valid context identifier as returned by getAllContexts().
   # param  sourceColumnIndex is the 1-based index of the source column. {1 or 2}
   # param  sourceLanguage is the source term's language code {en, fr, es, da}
   # param  targetLanguage is the target term's language code {en, fr, es, da}
   public function updateFixedTerms($content, $filename, $encoding, $context, $sourceColumnIndex, $sourceLanguage, $targetLanguage) {
      #error_log(func_get_args(), 3, '/tmp/PortageLiveAPI.debug.log');
      $encoding = strtolower($encoding);
      if (!($encoding == 'cp-1252' or $encoding == 'utf-8')) {
         throw new SoapFault("PortageBadArgs", "Unsupported encoding ($encoding): use either UTF-8 or CP-1252.");
      }

      $sourceLanguage = strtolower($sourceLanguage);
      $targetLanguage = strtolower($targetLanguage);

      $contextInfo = $this->getContextInfo($context);
      $this->validateContext($contextInfo);

      if ($content == '') {
         throw new SoapFault("PortageBadArgs", "There is no file content ($filename).");
      }
      $work_dir = $this->makeWorkDir("fixedTermUpdate_{$context}_$filename");
      $localFilename = "$work_dir/fixedTerms.in";
      $local_file = fopen($localFilename, "w");
      if ( !is_resource($local_file) )
         throw new SoapFault("PortageServer", "failed to write to local file");
      $bytes_written = fwrite($local_file, $content);
      if ( $bytes_written != strlen($content) )
         throw new SoapFault("PortageServer", "incomplete write to local file " .
                             "(wrote $bytes_written; expected ".strlen($content).")");
      fclose($local_file);
      $content = "";

      if ($encoding == 'cp-1252') {
         $this->runCommand("iconv -f cp1252 -t UTF-8 < $localFilename > $localFilename" . ".utf8", "", $contextInfo, $exit_status, false);
         if ($exit_status != 0) {
            throw new SoapFault("PortageServer", "Error converting fixed terms to UTF-8");
         }
         $localFilename = $localFilename . ".utf8";
      }

      $tm = $contextInfo["context_dir"] . "/plugins/fixedTerms/tm";
      $fixedTerms = $contextInfo["context_dir"] . "/plugins/fixedTerms/fixedTerms";
      #$command = "PORTAGE_INTERNAL_CALL=1 flock " . $contextInfo["context_dir"] . "/plugins/fixedTerms/tm -c \"set -o pipefail; fixed_term2tm.pl -source_column=$sourceColumnIndex -source=$sourceLanguage -target=$targetLanguage $localFilename";
      $command = "flock $tm.lock --command \"set -o pipefail;";
      $command .= " cp $localFilename $fixedTerms";
      $command .= " && fixed_term2tm.pl -source_column=$sourceColumnIndex -source=$sourceLanguage -target=$targetLanguage $fixedTerms";
      $command .= " | sort --unique > $tm\"";
      #error_log($command . "\n", 3, '/tmp/PortageLiveAPI.debug.log');

      $exit_status = NULL;
      $result = $this->runCommand($command, "", $contextInfo, $exit_status, true);
      if ($exit_status != 0)
         throw new SoapFault("PortageServer",
            "non-zero return code from $command: $exit_status\n" . $result);

      if (is_file($tm) === FALSE)
         throw new SoapFault("PortageServer", "Phrase table not properly generated for $context");

      # May be it would be wiser to return the number of fixed term pairs that were processed by fixed_term2tm.pl?
      return true;
   }

   # param context must be a valid context identifier as returned by getAllContexts().
   public function getFixedTerms($context) {
      $contextInfo = $this->getContextInfo($context);
      $this->validateContext($contextInfo);

      $fixedTerms = $contextInfo["context_dir"] . "/plugins/fixedTerms/fixedTerms";
      if (is_file($fixedTerms) === FALSE)
         throw new SoapFault("PortageServer", "$context doesn't have fixed terms.", "PortageLiveAPI");

      $tm = $contextInfo["context_dir"] . "/plugins/fixedTerms/tm";
      if (is_file($tm) === FALSE)
         throw new SoapFault("PortageServer", "$context has incorrectly installed fixed terms.", "PortageLiveAPI");

      $content = file_get_contents($fixedTerms);
      if ( $content === FALSE)
         throw new SoapFault("PortageServer", "incomplete read of fixed terms local file ($fixedTerms).");

      return $content;
   }

   # param context must be a valid context identifier as returned by getAllContexts().
   public function removeFixedTerms($context) {
      $contextInfo = $this->getContextInfo($context);
      $this->validateContext($contextInfo);

      $fixedTerms = $contextInfo["context_dir"] . "/plugins/fixedTerms/fixedTerms";
      if (is_file($fixedTerms)) {
         if ( !unlink($fixedTerms) ) {
            throw new SoapFault("PortageServer", "Unable to delete fixed terms' list ($fixedTerms).");
         }
      }

      $tm = $contextInfo["context_dir"] . "/plugins/fixedTerms/tm";
      if (is_file($tm)) {
         if ( !unlink($tm) ) {
            throw new SoapFault("PortageServer", "Unable to delete fixed terms' translation model ($fixedTerms).");
         }
      }

      return true;
   }

   # Returns the current API's version.
   public function getVersion() {
      return "PortageII-3.0";
   }
}

try {
   $server = new SoapServer("PortageLiveAPI.wsdl");
   $server->setClass("PortageLiveAPI");
   $server->handle();
}
catch (SoapFault $exception) {
   print "SOAP Fault: (faultcode: {$exception->faultcode}, faultstring: {$exception->faultstring})<BR/>";
   var_dump($exception);
}
?>
