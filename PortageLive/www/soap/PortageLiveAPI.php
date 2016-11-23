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


require 'basicTranslator.php';

class PortageLiveAPI extends BasicTranslator {

   var $validLanguages = array('en' => 1, 'fr' => 1, 'es' => 1, 'da' =>1);

   private function validLanguagesToString() {
      return '{' . implode(", ", array_keys($this->validLanguages)) . '}';
   }

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
      $command .= ($is_xml ? " -nl=s " : " -nl=p ") .
                  ($xtags ? " -xtags " : "") .
                  " -dir=\"$work_dir\" -out=\"$work_dir/P.out\" " .
                  " \"$work_dir/Q.in\" >& \"$work_dir/trace\" ";
      if ($is_xml)
         $command = "(if ($command); then ln -s QP.xml $work_dir/PLive-$work_name; fi; touch $work_dir/done)& disown %1";
      else
         $command = "(if ($command); then ln -s $work_dir/P.out $work_dir/PLive-$work_name; fi; touch $work_dir/done)& disown %1";

      $start_time = time();
      $result = $this->runCommand($command, "", $i, $exit_status, false);
      $info2 = "result len: " . strlen($result) . "<br/>";
      global $base_url;
      $monitor = "http://" . $_SERVER['SERVER_NAME'] .
                 "${base_url}cgi-bin/plive-monitor.cgi?" .
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
      if (! isset($monitor_token)) {
         return "3 Invalid job token (): missing monitor token";
      }

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
               if (filesize("$dir/P.txt") > 0) {
                  return "0 Done: $info[dir]/$info[file]";
               }
               else {
                  return "2 Failed - no sentences to translate : $info[dir]/trace";
               }
            }
            else {
               return "2 Failed".debug($info).": $info[dir]/trace";
            }
         }
         else {
            $linestodo = `cat $dir/q.tok 2> /dev/null | wc --lines 2> /dev/null`;
            $linesdone = 0;
            $outputs = " $dir/canoe-parallel*/out*  $dir/run-p.*/out.worker-* ";
            if ( $info['ce'] )
               $outputs .= "$dir/p.raw";
            else
               $outputs .= "$dir/p.dec";

            $linesdone = `cat $outputs 2> /dev/null | wc --lines 2> /dev/null`;

            $progress = 0;
            if ( intval($linestodo) > 0 ) {
               if ( intval($linesdone) < intval($linestodo) )
                  $progress = intval(($linesdone / $linestodo) * 100);
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
      if ($sourceLanguage === '') {
         throw new SoapFault("PortageBadArgs", "sourceLanguage cannot be empty it must be one of " . $this->validLanguagesToString());
      }
      if (!array_key_exists($sourceLanguage, $this->validLanguages)) {
         throw new SoapFault("PortageBadArgs", "sourceLanguage($sourceLanguage) must be one of " . $this->validLanguagesToString());
      }

      $targetLanguage = strtolower($targetLanguage);
      if ($targetLanguage === '') {
         throw new SoapFault("PortageBadArgs", "targetLanguage cannot be empty it must be one of " . $this->validLanguagesToString());
      }
      if (!array_key_exists($targetLanguage, $this->validLanguages)) {
         throw new SoapFault("PortageBadArgs", "targetLanguage($targetLanguage) must be one of " . $this->validLanguagesToString());
      }

      $contextInfo = $this->getContextInfo($context);
      $this->validateContext($contextInfo);

      if (!file_exists($contextInfo["context_dir"] . "/plugins/fixedTerms")) {
         throw new SoapFault("PortageContext", "This context($context) doesn't support fixed terms.");
      }

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
         throw new SoapFault("PortageServer", "incomplete read of fixed terms local file ($fixedTerms).", "PortageLiveAPI");

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
      return "PortageII-3.0.1";
   }

   # ==========================================================
   # Deprecated functions kept for backwards compatibility only
   # ==========================================================
   function getTranslation($src_string, $newline, $xtags) {
      if (!isset($newline)) $newline = "p"; # newline arg was added in PortageII 2.1
      if (!isset($xtags)) $xtags = false; # xtags arg was added in PortageII 2.1
      return $this->translate($src_string, "context", $newline, $xtags, false);
   }
   function getTranslation2($src_string, $context, $newline, $xtags) {
      if (!isset($newline)) $newline = "p"; # newline arg was added in PortageII 2.1
      if (!isset($xtags)) $xtags = false; # xtags arg was added in PortageII 2.1
      return $this->translate($src_string, $context, $newline, $xtags, false);
   }
   function getTranslationCE($src_string, $context, $newline, $xtags) {
      if (!isset($newline)) $newline = "p"; # newline arg was added in PortageII 2.1
      if (!isset($xtags)) $xtags = false; # xtags arg was added in PortageII 2.1
      return $this->translate($src_string, $context, $newline, $xtags, true);
   }
   function translateTMXCE($TMX_contents_base64, $TMX_filename, $context, $ce_threshold, $xtags) {
      if (!isset($xtags)) $xtags = false; # xtags arg was added in PortageII 2.0
      return $this->translateFileCE($TMX_contents_base64, $TMX_filename, $context, true, $ce_threshold, $xtags, "tmx");
   }
   function translateSDLXLIFFCE($SDLXLIFF_contents_base64, $SDLXLIFF_filename, $context, $ce_threshold, $xtags) {
      return $this->translateFileCE($SDLXLIFF_contents_base64, $SDLXLIFF_filename, $context, true, $ce_threshold, $xtags, "sdlxliff");
   }
   function translatePlainTextCE($PlainText_contents_base64, $PlainText_filename, $context, $ce_threshold, $xtags) {
      return $this->translateFileCE($PlainText_contents_base64, $PlainText_filename, $context, true, $ce_threshold, $xtags, "plaintext");
   }
   function translateTMXCE_Status($monitor_token) {
      return $this->translateFileStatus($monitor_token);
   }
   function translateSDLXLIFFCE_Status($monitor_token) {
      return $this->translateFileStatus($monitor_token);
   }
   function translatePlainTextCE_Status($monitor_token) {
      return $this->translateFileStatus($monitor_token);
   }
   # ==============================================================
   # End deprecated functions kept for backwards compatibility only
   # ==============================================================


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
