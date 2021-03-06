<?php
# @file PortageLiveLib.php
# @brief Implementation of the API to the PortageII SMT software suite.
#
# @author Samuel Larkin, Patrick Paul & Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009 - 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009 - 2017, Her Majesty in Right of Canada


# Server configuration options
if (php_sapi_name() == 'cli') {
   # We need to specify a full path and not simply "." because the working
   # directory has do be a absolute path since runCommand executes from /tmp.
   $base_web_dir = getcwd();
   $base_portage_dir = "$base_web_dir/tests";
   $error_output_dir = "$base_web_dir/plive";
}
elseif (php_sapi_name() == 'cli-server') {
   # We need to specify a full path and not simply "." because the working
   # directory has do be a absolute path since runCommand executes from /tmp.
   $base_web_dir = $_SERVER['DOCUMENT_ROOT'];
   $base_portage_dir = "$base_web_dir/tests";
   $error_output_dir = "$base_web_dir/plive";
}
else {
   $base_web_dir = "/var/www/html";
   $base_portage_dir = "/opt/PortageII";
   $error_output_dir = '/tmp';
}
$base_url = "/";

# produce debugging information
function debug($i)
{
   if (False) {
      error_log(print_r($i, true), 3, "$error_output_dir/PortageLiveAPI.debug.log");
      return " " . print_r($i, true);
   }
}

# http_response_code() only exists starting with php 5.4, but we can simulate
# it with older versions
if (!function_exists('http_response_code')) {
   function http_response_code($code) {
      header('X-PHP-Response-Code: '.$code, true, $code);
   }
}


class PortageLiveLib
{
   var $validLanguages = array('en' => 1, 'fr' => 1, 'es' => 1, 'da' =>1);

   private function validLanguagesToString()
   {
      return '{' . implode(", ", array_keys($this->validLanguages)) . '}';
   }

   # Gather all relevant information about translation context $context
   public function getContextInfo($context)
   {
      $info = array();
      $context_parts = explode("/", $context, 2);
      global $base_portage_dir;
      if (count($context_parts) == 2) {
         # We have a static context plus a document ID
         $info["context"] = $context_parts[0];
         $info["document_id"] = $context_parts[1];
         $potential_workdir = $this->getDocumentModelWorkDir($info["context"], $info["document_id"]);
         # If the initial incrAddSentence is immediately followed by a
         # translate request, there is a potential race condition that can be
         # prevented by making sure the canoe.ini.cow also exists which will
         # trigger a fallback on the context while the document_model gets
         # finalized.
         if (is_dir($potential_workdir) and is_file("$potential_workdir/canoe.ini.cow")) {
            $info["context_dir"] = $potential_workdir;
         } else {
            $info["context_dir"] = "$base_portage_dir/models/" . $info['context'];
         }
      } else {
         $info["context"] = $context;
         $info["context_dir"] = "$base_portage_dir/models/" . $info['context'];
      }
      $info["script"] = "$info[context_dir]/soap-translate.sh";
      $info["canoe_ini"] = "$info[context_dir]/canoe.ini.cow";
      $info['is_incremental'] = is_file("$info[context_dir]/incremental.config");
      if (is_file($info["script"])) {
         $info["good"] = true;
         $cmdline = `tail -n -1 < $info[script]`;
         if (preg_match('/(-decode-only|-with-ce|-with-rescoring)/', $cmdline, $matches)) {
            $info["good"] = false;
            $info["label"] = "$context: context from a previous, incompatible version of PortageII";
         } else {
            $src = "";
            if (preg_match('/-xsrc=([-a-zA-Z]+)/', $cmdline, $matches))
               $src = $matches[1];
            else if (preg_match('/-src=(\w+)/', $cmdline, $matches))
               $src = $matches[1];
            $info["source"] = $src;
            $tgt = "";
            if (preg_match('/-xtgt=([-a-zA-Z]+)/', $cmdline, $matches))
               $tgt = $matches[1];
            else if (preg_match('/-tgt=(\w+)/', $cmdline, $matches))
               $tgt = $matches[1];
            $info["target"] = $tgt;
            #if (is_file("$info[context_dir]/canoe.ini.cow"))
            #   $info["canoe_ini"] = "$info[context_dir]/canoe.ini.cow";
            #if (is_file("$info[context_dir]/rescore.ini"))
            #   $info["rescore_ini"] = "$info[context_dir]/rescore.ini";
            if (is_file("$info[context_dir]/ce_model.cem"))
               $info["ce_model"] = "$info[context_dir]/ce_model.cem";
            $info["label"] = "$context ($src --> $tgt)"
               . (empty($info["ce_model"]) ? "" : " with CE")
               . ($info['is_incremental'] ? ' [Incr]' : '');
         }
      } else {
         $info["good"] = false;
         $info["label"] = "$context: bad context";
      }
      return $info;
   }

   # Validate the context described in context info $i, throwing SoapFault if
   # there are errors.
   # If $need_ce is true, also check that the context provides confidence estimation.
   protected function validateContext(&$i, $need_ce = false)
   {
      $context = $i["context"];
      if (! $i["good"]) {
         if (!file_exists($i["context_dir"])) {
            if (isset($i['document_id'])) {
               $context .= '/' . $i['document_id'];
            }
            throw new SoapFault("PortageContext",
                                "Context \"$context\" does not exist.\n" . debug($i),
                                "PortageLiveAPI");
         } else {
            throw new SoapFault("PortageContext", "Context \"$context\" is broken.\n" . debug($i));
         }
      }
      if ($need_ce && empty($i["ce_model"])) {
         throw new SoapFault("PortageContext",
                             "Context \"$context\" does not support confidence estimation.\n" . debug($i));
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
   # $work_dir is not NULL, STDERR will be sent to $work_dir/trace.
   protected function runCommand($command, $src_string, &$i,
      &$exit_status = NULL, $wantoutput = true,
      $work_dir = NULL)
   {
      global $error_output_dir;
      $cwd = "/tmp";
      global $base_portage_dir;
      $context_dir = $i["context_dir"];
      if (php_sapi_name() === 'cli' or php_sapi_name() === 'cli-server')
         $env = NULL;
      else
         $env = array(
            'PORTAGE'         => "$base_portage_dir",
            'LD_LIBRARY_PATH' => "$context_dir/lib:$base_portage_dir/lib:/lib:/usr/lib",
            'PATH'            => "$context_dir/bin:$i[context_dir]:$base_portage_dir/bin"
                                 . ":/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
            'PERL5LIB'        => "$context_dir/lib:$base_portage_dir/lib",
            'PYTHONPATH'      => "$context_dir/lib:$base_portage_dir/lib",
            'PORTAGE_INTERNAL_CALL' => 1
         );

      #error_log($command . "\n", 3, "$error_output_dir/PortageLiveAPI.debug.log");
      #error_log(print_r($i, true), 3, "$error_output_dir/PortageLiveAPI.debug.log");
      #error_log(print_r($env, true), 3, "$error_output_dir/PortageLiveAPI.debug.log");

      $descriptorspec = array(
         0 => array("pipe", "r"),  # stdin is a pipe that the child will read from
      );
      if ($wantoutput) {
         # stdout is a pipe that the child will write to
         $descriptorspec[1] = array("pipe", "w");
      } else {
         $descriptorspec[1] = array("file", "/dev/null", "a");
      }
      # stderr is a file to write to
      if ($work_dir != NULL) {
         $descriptorspec[2] = array("file", "$work_dir/trace", "a");
      }
      else {
         $descriptorspec[2] = array("file", "$error_output_dir/error-output.txt", "a");
      }

      $process = proc_open($command, $descriptorspec, $pipes, $cwd, $env);

      if (is_resource($process)) {
         # $pipes now looks like this:
         # 0 => writeable handle connected to child stdin
         # 1 => readable handle connected to child stdout
         # Any error output will be appended to $error_output_dir/error-output.txt

         fwrite($pipes[0], $src_string);
         fclose($pipes[0]);

         $my_retval = "";
         if ($wantoutput) {
            $my_retval = stream_get_contents($pipes[1]);
            fclose($pipes[1]);
         }

         # It is important that you close any pipes before calling
         # proc_close in order to avoid a deadlock
         $return_value = proc_close($process);
         if ($return_value != 0) {
            if (is_null($exit_status))
               throw new SoapFault("PortageServer",
                  "non-zero return code from $command: $return_value\n".debug($i),
                  "PortageLiveAPI");
            else
               $exit_status = $return_value;
         } else {
            if (!is_null($exit_status))
               $exit_status = 0;
         }
      } else {
         throw new SoapFault("PortageServer", "failed to run $command: $!\n".debug($i));
      }

      return $my_retval . debug($i);
   }

   # Create a working directory based on $filename.
   # Throws SoapFault (faultcode=PortageServer) in case of error.
   # Returns the name of the directory created.
   protected function makeWorkDir($filename)
   {
      date_default_timezone_set("UTC");
      $timestamp = date("Ymd\THis\Z");
      $base = $this->normalizeName("SOAP_{$filename}_{$timestamp}");
      global $base_web_dir;
      $work_path = "$base_web_dir/plive/";
      $dir = `mktemp -d $work_path{$base}_XXXXXX 2>&1`;
      if (strpos($dir, "$work_path$base") === 0)
         return rtrim($dir);
      else
         throw new SoapFault("PortageServer",
                             "can't create temp work dir for $filename: $dir : $base");
   }

   # Create a working directory based on $filename.
   # Throws SoapFault (faultcode=PortageServer) in case of error.
   # Returns the name of the directory created.
   protected function makeDocumentModelWorkDir($context, $document_model_id)
   {
      if (!isset($context) || empty($context)) {
         throw new SoapFault("PortageBadArgs",
                             "You must provide a valid context.");
      }

      if (!isset($document_model_id) || empty($document_model_id)) {
         throw new SoapFault("PortageBadArgs",
                             "You must provide a valid document_model_id.");
      }

      $work_dir = $this->getDocumentModelWorkDir($context, $document_model_id);
      if (is_dir($work_dir) || @mkdir($work_dir, 0700, true))
         return $work_dir;
      else if (is_dir($work_dir))   # handle potential race condition
         return $work_dir;
      else
         throw new SoapFault( "MkdirError",
            "Can't create temp work dir for document $document_model_id",
            "PortageServer",
            "mkdir was unable to create $work_dir.\nProbable causes: "
               . dirname($work_dir) . " doesn't exist or is read-only, or "
               . basename($work_dir) . " exists as a file not a directory."
         );
   }

   # Return the document model workdir path for a document model ID.
   protected function getDocumentModelWorkDir($context, $document_model_id)
   {
      if (!isset($context) || empty($context)) {
         throw new SoapFault("PortageBadArgs",
                             "You must provide a valid context.");
      }

      if (!isset($document_model_id) || empty($document_model_id)) {
         throw new SoapFault("PortageBadArgs",
                             "You must provide a valid document_model_id.");
      }

      global $base_web_dir;
      $work_dir = $this->normalizeName("DOCUMENT_MODEL_{$context}_{$document_model_id}");
      $work_dir = "$base_web_dir/plive/$work_dir";
      return $work_dir;
   }

   # Normalize a name to keep only alphanumeric, dash, underscore, dot and plus
   protected function normalizeName($filename)
   {
      return preg_replace(array("/[^-_.+a-zA-Z0-9]/", '/[ .]$/', '/^[ .]/'),
                          array("", '', ''), $filename);
   }



   # Enumerate all installed contexts
   public function getAllContexts($verbose = false, $json = false)
   {
      $contexts = array();
      global $base_portage_dir;
      $dirs = scandir("$base_portage_dir/models");
      foreach ($dirs as $dir) {
         $info = $this->getContextInfo($dir);
         if ($info["good"]) {
            if ($json)
               $contexts[] = array(
                  'name' => $dir,
                  'description' => $info['label'],
                  'source' => $info['source'],
                  'target' => $info['target'],
                  'is_incremental' => $info['is_incremental'],
                  'as_ce' => !empty($info['ce_model']),
               );
            else if ($verbose)
               $contexts[] = $info['label'];
            else
               $contexts[] = $dir;
         }
      }
      if ($json)
         return json_encode(array('contexts' => $contexts), JSON_PRETTY_PRINT);
      else
         return join(";",$contexts);
   }

   # Load models in memory
   public function primeModels($context, $PrimeMode)
   {
      $rc = 0;
      $i = $this->getContextInfo($context);
      $this->validateContext($i);
      $command = "prime.sh $PrimeMode";
      $this->runCommand($command, "", $i, $rc, false);

      if ($rc != 0)
         throw new SoapFault("PortagePrimeError",
                             "Failed to prime, something went wrong in prime.sh.\n"
                             . "rc=$rc; Command=$command");

      return true;
   }

   # param src_string:  input to translate
   # param context:  model to use for translation
   # param newline:  what is the interpretation of newline in the input
   # param xtags:  Transfer tags
   # param useCE:  Should we use confidence estimation?
   public function translate($src_string, $context, $newline, $xtags, $useCE)
   {
      #$this->checkIsThisXML($src_string);
      if (!isset($newline))
         throw new SoapFault("PortageBadArgs", "You must defined newline.\n"
                             . "Allowed newline types are: s, p or w");

      if (!($newline == "s" or $newline == "p" or $newline == "w"))
         throw new SoapFault("PortageBadArgs", "Illegal newline type " . $newline
                             . "\nAllowed newline types are: s, p or w");

      $i = $this->getContextInfo($context);
      $this->validateContext($i, $useCE);

      $options = " -verbose";
      $options .= ($useCE ? " -with-ce" : " -decode-only");
      $options .= ($xtags ? " -xtags" : "");
      $options .= " -nl=" . $newline;

      # PTGSH-197
      $work_dir = $this->makeWorkDir("{$context}_srcString");
      $options .= " -dir=\"$work_dir\"";

      $exit_status = NULL;
      $translations = $this->runCommand($i["script"] . $options, $src_string, $i, $exit_status, true, $work_dir);
      if ($src_string != '' and $translations == '')
         throw new SoapFault("PortageServer",
            "Server error while translating your input.");
      global $base_web_dir;
      return Array('Result' => $translations, 'workdir' => str_replace($base_web_dir, "", $work_dir));
   }

   # Translate a file using model $context and the confidence threshold
   # $ce_threshold.  A threshold of 0 means keep everything.
   # $contents_base64 is a string containing the full content of the xml file, in Base64 encoding.
   # $filename is the name of the XML file.
   # If $xtags is true, transfer tags from the source-language segments to the translations.
   # $type indicates the type of XML file we're translating
   private function translateFileCE($contents_base64, $filename, $context,
                                    $useCE, $ce_threshold, $xtags, $type)
   {
      global $error_output_dir;
      #error_log("translateFileCE with $type\n", 3, "$error_output_dir/PortageLiveAPI.debug.log");

      #if ($type === "plaintext" and !$useCE and $ce_threshold != 0) {
      #   # TODO send a meaningful SoapFault
      #   throw new SoapFault("Client", "Translating plain text file");
      #}

      # preg_match() returns 1 if the pattern matches given subject, 0 if it 
      # does not, or FALSE if an error occurred.
      if (preg_match("/.(?:txt|tmx|sdlxliff|xliff)$/i", $filename) == 0) {
         throw new SoapFault("PortageServer",
            "Your filename must end with either '.txt' or '.tmx' or '.sdlxliff' or '.xliff'");
      }

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
      if (!is_resource($local_file))
         throw new SoapFault("PortageServer", "failed to write $type to local file");
      $bytes_written = fwrite($local_file, $contents);
      if ($bytes_written != strlen($contents))
         throw new SoapFault("PortageServer", "incomplete write of $type to local file "
                             . "(wrote $bytes_written; expected ".strlen($contents).")");
      fclose($local_file);
      $contents = "";
      $contents_base64 = "";

      if ($is_xml) {
         $xml_check_rc = "";
         $xml_check_log = $this->runCommand("ce_tmx.pl check $work_dir/Q.in 2>&1",
               "", $i, $xml_check_rc);
         #return "TMX check log: $xml_check_log; xml_check_rc: $xml_check_rc";
         if ($xml_check_rc != 0)
            throw new SoapFault("Client", "$type check failed for $filename: "
                                . "$xml_check_log\n$work_dir/Q.in");
      }

      #$xml_lang = array("fr" => "FR-CA", "en" => "EN-CA"); # add more languages here as needed
      $command = "$i[script] ";  # Requires that last space.
      $command .= ($is_xml ? " -xml " : "");
      if ($useCE and !empty($i["ce_model"])) {
         $command .= "-with-ce ";
         if ($ce_threshold > 0) {
            $command .= "-filter=$ce_threshold ";
         }
      } else {
         $command .= "-decode-only ";
      }
      $command .= ($is_xml ? " -nl=s " : " -nl=p ") .
                  ($xtags ? " -xtags " : "") .
                  " -dir=\"$work_dir\" -out=\"$work_dir/P.out\" " .
                  " \"$work_dir/Q.in\" >& \"$work_dir/trace\" ";
      if ($is_xml)
         $command = "(if ($command); then ln -s QP.xml $work_dir/PLive-$work_name; fi; "
                    . "touch $work_dir/done)& disown %1";
      else
         $command = "(if ($command); then ln -s $work_dir/P.out $work_dir/PLive-$work_name; fi; "
                    . "touch $work_dir/done)& disown %1";

      $start_time = time();
      $result = $this->runCommand($command, "", $i, $exit_status, false, $work_dir);
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

   private function guardFilename($filename, $expected_extension) {
      $possible_extensions = $expected_extension === 'sdlxliff' ? 'sdlxliff|xliff' : $expected_extension;
      if (preg_match('/.(?:' . $possible_extensions . ')$/i', $filename) == 0) {
         $filename .= '.' . $expected_extension;
      }
      return $filename;
   }

   public function translateTMX($TMX_contents_base64, $TMX_filename, $context,
                                $useCE, $ce_threshold, $xtags)
   {
      return $this->translateFileCE($TMX_contents_base64,
         $this->guardFilename($TMX_filename, 'tmx'),
         $context,
         $useCE,
         $ce_threshold,
         $xtags,
         'tmx');
   }

   public function translateSDLXLIFF($SDLXLIFF_contents_base64, $SDLXLIFF_filename,
                                     $context, $useCE, $ce_threshold, $xtags)
   {
      return $this->translateFileCE($SDLXLIFF_contents_base64,
         $this->guardFilename($SDLXLIFF_filename, 'sdlxliff'),
         $context,
         $useCE,
         $ce_threshold,
         $xtags,
         'sdlxliff');
   }

   public function translatePlainText($PlainText_contents_base64, $PlainText_filename,
                                      $context, $useCE, $xtags)
   {
      $ce_threshold = 0;
      return $this->translateFileCE($PlainText_contents_base64,
         $this->guardFilename($PlainText_filename, 'txt'),
         $context,
         $useCE,
         $ce_threshold,
         $xtags,
         'plaintext');
   }

   public function translateFileStatus($monitor_token)
   {
      if (!isset($monitor_token)) {
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
               } else {
                  return "2 Failed - no sentences to translate : $info[dir]/trace";
               }
            } else {
               return "2 Failed".debug($info).": $info[dir]/trace";
            }
         } else {
            $linestodo = `cat $dir/q.tok 2> /dev/null | wc --lines 2> /dev/null`;
            $linesdone = 0;
            $outputs = " $dir/canoe-parallel*/out*  $dir/run-p.*/out.worker-*  $dir/p.raw";

            $linesdone = `cat $outputs 2> /dev/null | wc --lines 2> /dev/null`;

            $progress = 0;
            if (intval($linestodo) > 0) {
               if (intval($linesdone) < intval($linestodo))
                  $progress = intval(($linesdone / $linestodo) * 100);
               else
                  $progress = 90;
            }
            return "1 In progress ($progress% done)".debug($info);
         }
      } else {
         return "3 Dir not found".debug($info);
      }
   }

   # This function would be nice to use but we have case aka "a<1>b</1>" is not
   # valid xml since tag aren't allowed to start with digits.
   private function checkIsThisXML($string)
   {
      $test = "<?xml version='1.0'" . "?" . ">\n<document>" . $string . "</document>";
      #$test = "<document>" . $string . "</document>";
      if (simplexml_load_string($test) == FALSE) {
         throw new SoapFault("PortageNotXML", "This is invalid xml.\n"
                             . htmlspecialchars($string) . htmlspecialchars($test));
      }
   }


   protected function fixedTermsEnabled($contextInfo) {
      $context        = $contextInfo['context'];
      $fixedTerms_dir = $contextInfo["context_dir"] . "/plugins/fixedTerms";
      $tm             = $fixedTerms_dir . "/tm";
      if (!file_exists($fixedTerms_dir)) {
         throw new SoapFault("PortageContext",
                             "This context ($context) doesn't support fixed terms.");
      }

      if (!is_writable($fixedTerms_dir) or !is_writable($tm)) {
         throw new SoapFault("PortageContext",
                             "This context ($context) has its fixedTerms disabled.  Ask your administrator to enabled fixedTerms.");
      }

      return true;
   }


   # param  ContentsBase64 is the contents of the fixed term file in base64 encoding.
   # param  Filename is the fixed term file name.
   # param  encoding is the fixed term file's encoding and can be either UTF-8 or CP-1252.
   # param  context must be a valid context identifier as returned by getAllContexts().
   # param  sourceColumnIndex is the 1-based index of the source column. {1 or 2}
   # param  sourceLanguage is the source term's language code {en, fr, es, da}
   # param  targetLanguage is the target term's language code {en, fr, es, da}
   public function updateFixedTerms($content, $filename, $encoding, $context,
                      $sourceColumnIndex, $sourceLanguage, $targetLanguage)
   {
      global $error_output_dir;
      #error_log(func_get_args(), 3, "$error_output_dir/PortageLiveAPI.debug.log");
      $encoding = strtolower($encoding);
      if (!($encoding == 'cp-1252' or $encoding == 'utf-8')) {
         throw new SoapFault("PortageBadArgs", "Unsupported encoding ($encoding): "
                             . "use either UTF-8 or CP-1252.");
      }

      $sourceLanguage = strtolower($sourceLanguage);
      if ($sourceLanguage === '') {
         throw new SoapFault("PortageBadArgs", "sourceLanguage cannot be empty. "
                             . "It must be one of " . $this->validLanguagesToString());
      }
      if (!array_key_exists($sourceLanguage, $this->validLanguages)) {
         throw new SoapFault("PortageBadArgs", "sourceLanguage ($sourceLanguage) "
                             . "must be one of " . $this->validLanguagesToString());
      }

      $targetLanguage = strtolower($targetLanguage);
      if ($targetLanguage === '') {
         throw new SoapFault("PortageBadArgs", "targetLanguage cannot be empty. "
                             . "It must be one of " . $this->validLanguagesToString());
      }
      if (!array_key_exists($targetLanguage, $this->validLanguages)) {
         throw new SoapFault("PortageBadArgs", "targetLanguage ($targetLanguage) "
                             . "must be one of " . $this->validLanguagesToString());
      }

      if ($content === '') {
         throw new SoapFault("PortageBadArgs", "There is no file content ($filename).");
      }

      $contextInfo = $this->getContextInfo($context);
      $this->validateContext($contextInfo);

      $this->fixedTermsEnabled($contextInfo);

      $work_dir = $this->makeWorkDir("fixedTermUpdate_{$context}_$filename");
      $localFilename = "$work_dir/fixedTerms.in";
      $local_file = fopen($localFilename, "w");
      if (!is_resource($local_file))
         throw new SoapFault("PortageServer", "failed to write to local file");
      $bytes_written = fwrite($local_file, $content);
      if ($bytes_written != strlen($content))
         throw new SoapFault("PortageServer", "incomplete write to local file " .
                             "(wrote $bytes_written; expected ".strlen($content).")");
      fclose($local_file);
      $content = "";

      if ($encoding == 'cp-1252') {
         $this->runCommand("iconv -f cp1252 -t UTF-8 < $localFilename > $localFilename.utf8",
                           "", $contextInfo, $exit_status, false, $work_dir);
         if ($exit_status != 0) {
            throw new SoapFault("PortageServer", "Error converting fixed terms to UTF-8");
         }
         $localFilename = $localFilename . ".utf8";
      }

      $fixedTerms_dir = $contextInfo["context_dir"] . "/plugins/fixedTerms";
      $tm             = $fixedTerms_dir . "/tm";
      $fixedTerms     = $fixedTerms_dir . "/fixedTerms";
      $command = "flock $tm.lock --command \"set -o pipefail;";
      $command .= " cp $localFilename $fixedTerms";
      $command .= " && fixed_term2tm.pl -source_column=$sourceColumnIndex "
                  . "-source=$sourceLanguage -target=$targetLanguage $fixedTerms";
      $command .= " | sort --unique > $tm\"";
      #error_log($command . "\n", 3, "$error_output_dir/PortageLiveAPI.debug.log");

      $exit_status = NULL;
      $result = $this->runCommand($command, "", $contextInfo, $exit_status, true, $work_dir);
      if ($exit_status != 0)
         throw new SoapFault("PortageServer",
            "non-zero return code from $command: $exit_status\n" . $result);

      if (is_file($tm) === FALSE)
         throw new SoapFault("PortageServer", "Phrase table not properly generated for $context");

      # Maybe it would be wiser to return the number of fixed term pairs that
      # were processed by fixed_term2tm.pl?
      return true;
   }

   # param context must be a valid context identifier as returned by getAllContexts().
   public function getFixedTerms($context)
   {
      $contextInfo = $this->getContextInfo($context);
      $this->validateContext($contextInfo);

      $fixedTerms = $contextInfo["context_dir"] . "/plugins/fixedTerms/fixedTerms";
      if (is_file($fixedTerms) === FALSE)
         throw new SoapFault("PortageServer", "$context doesn't have fixed terms.",
                             "PortageLiveAPI");

      $tm = $contextInfo["context_dir"] . "/plugins/fixedTerms/tm";
      if (is_file($tm) === FALSE)
         throw new SoapFault("PortageServer", "$context has incorrectly installed fixed terms.",
                             "PortageLiveAPI");

      $content = file_get_contents($fixedTerms);
      if ($content === FALSE)
         throw new SoapFault("PortageServer",
                             "incomplete read of fixed terms local file ($fixedTerms).",
                             "PortageLiveAPI");

      return $content;
   }

   # param context must be a valid context identifier as returned by getAllContexts().
   public function removeFixedTerms($context)
   {
      $contextInfo = $this->getContextInfo($context);
      $this->validateContext($contextInfo);

      $this->fixedTermsEnabled($contextInfo);

      $fixedTerms_dir = $contextInfo["context_dir"] . "/plugins/fixedTerms";
      $tm             = $fixedTerms_dir . "/tm";
      $fixedTerms     = $fixedTerms_dir . "/fixedTerms";

      if (is_file($fixedTerms)) {
         if (!unlink($fixedTerms)) {
            throw new SoapFault("PortageServer",
                                "Unable to delete fixed terms' list ($fixedTerms).");
         }
      }

      if (is_file($tm)) {
         if (!unlink($tm)) {
            throw new SoapFault("PortageServer",
               "Unable to delete fixed terms' translation model ($fixedTerms).");
         }
      }

      return true;
   }

   # Returns the current API's version.
   public function getVersion()
   {
      return "PortageII-4.0";
   }

   public function incrClearDocumentModelWorkdir($context, $document_model_id = NULL)
   {
      error_log('Not yet properly implemented');
      assert(False);

      if (!isset($context) || empty($context)) {
         throw new SoapFault("PortageBadArgs",
                             "You must provide a valid context.");
      }

      if (!isset($document_model_id) || empty($document_model_id)) {
         throw new SoapFault("PortageBadArgs",
                             "You must provide a valid document_model_id.");
      }

      $work_dir = $this->getDocumentModelWorkDir($context, $document_model_id);

      if (! is_dir($work_dir))
         throw new SoapFault("PortageServer", "$document_model_id doesn't have "
                             . "a workdir: $work_dir");

      if (! rmdir($work_dir))
         throw new SoapFault("PortageServer", "can't remove temp work dir for "
                             . "$document_model_id: $work_dir");

      return $work_dir;
   }



   public function incrAddTextBlock($context = NULL,
                                    $document_model_id = NULL,
                                    $source_block = NULL,
                                    $target_block = NULL,
                                    $extra_data = NULL)
   {
      if (!isset($context) || empty($context)) {
         throw new SoapFault("PortageBadArgs", "You must provide a valid context.");
      }

      # TODO: Validate that the document_model_id is a valid one.
      if (!isset($document_model_id) || empty($document_model_id)) {
         throw new SoapFault("PortageBadArgs", "You must provide a valid document_model_id.");
      }

      if (!isset($source_block) || empty($source_block)) {
         throw new SoapFault("PortageBadArgs", "You must provide a source text block.");
      }

      if (!isset($target_block) || empty($target_block)) {
         throw new SoapFault("PortageBadArgs", "You must provide a target text block.");
      }

      $i = $this->getContextInfo($context);
      $this->validateContext($i);

      global $error_output_dir;

      # We need to set LC_ALL or else escapeshellarg will strip out unicode.
      # http://stackoverflow.com/questions/8734510/escapeshellarg-with-utf-8-only-works-every-other-time
      # http://positon.org/php-escapeshellarg-function-utf8-and-locales
      # http://markushedlund.com/dev/php-escapeshellarg-with-unicodeutf-8-support
      setlocale(LC_ALL, 'en_US.utf8');
      $source_block = escapeshellarg($source_block);
      $target_block = escapeshellarg($target_block);
      $work_dir = $this->makeDocumentModelWorkDir($context, $document_model_id);

      $command = "cd $work_dir && ";
      $command .= "incr-add-sentence.sh -block";

      if (isset($extra_data) && ! empty($extra_data)) {
         $extra_data = escapeshellarg($extra_data);
         $command .= " -extra-data " . $extra_data;
      }
      $command .= " -c " . $i["canoe_ini"];
      $command .= " -- $source_block $target_block";
      #error_log($command . "\n", 3, "$error_output_dir/PortageLiveAPI.debug.log");

      $dummy_context_info = array( 'context_dir' => '' );
      $exit_status = False;
      # Set $wantoutput to true for debugging and look at $error_output_dir/error-output.txt,
      # but then updates will no longer happen in the background, the soap client
      # will have to wait on them, so do so only for debugging!
      $wantoutput = False;
      $result = $this->runCommand($command, "'$source_block'\t'$target_block'",
                                  $dummy_context_info, $exit_status, $wantoutput);

      return $exit_status == 0 ? True : False;
   }



   public function incrAddSentence($context = NULL,
                                   $document_model_id = NULL,
                                   $source_sentence = NULL,
                                   $target_sentence = NULL,
                                   $extra_data = NULL)
   {
      if (!isset($source_sentence) || empty($source_sentence)) {
         throw new SoapFault("PortageBadArgs", "You must provide a source sentence.");
      }

      if (!isset($target_sentence) || empty($target_sentence)) {
         throw new SoapFault("PortageBadArgs", "You must provide a target sentence.");
      }

      if (!isset($context) || empty($context)) {
         throw new SoapFault("PortageBadArgs", "You must provide a valid context.");
      }

      # TODO: Validate that the document_model_id is a valid one.
      if (!isset($document_model_id) || empty($document_model_id)) {
         throw new SoapFault("PortageBadArgs", "You must provide a valid document_model_id.");
      }

      $i = $this->getContextInfo($context);
      $this->validateContext($i);

      global $error_output_dir;

      # We need to set LC_ALL or else escapeshellarg will strip out unicode.
      # http://stackoverflow.com/questions/8734510/escapeshellarg-with-utf-8-only-works-every-other-time
      # http://positon.org/php-escapeshellarg-function-utf8-and-locales
      # http://markushedlund.com/dev/php-escapeshellarg-with-unicodeutf-8-support
      setlocale(LC_ALL, 'en_US.utf8');
      $source_sentence = escapeshellarg($source_sentence);
      $target_sentence = escapeshellarg($target_sentence);
      $work_dir = $this->makeDocumentModelWorkDir($context, $document_model_id);

      $command = "cd $work_dir && ";
      $command .= "incr-add-sentence.sh";

      if (isset($extra_data) && ! empty($extra_data)) {
         $extra_data = escapeshellarg($extra_data);
         $command .= " -extra-data " . $extra_data;
      }
      $command .= " -c " . $i["canoe_ini"];
      $command .= " -- $source_sentence $target_sentence";
      #error_log($command . "\n", 3, "$error_output_dir/PortageLiveAPI.debug.log");

      $dummy_context_info = array( 'context_dir' => '' );
      $exit_status = False;
      # Set $wantoutput to true for debugging and look at $error_output_dir/error-output.txt,
      # but then updates will no longer happen in the background, the soap client
      # will have to wait on them, so do so only for debugging!
      $wantoutput = False;
      $result = $this->runCommand($command, "'$source_sentence'\t'$target_sentence'",
                                  $dummy_context_info, $exit_status, $wantoutput);

      return $exit_status == 0 ? True : False;
   }



   public function incrStatus($context, $document_model_id = NULL)
   {
      //error_log("incrStatus call: ($context , $document_model_id)");
      if (!isset($context) || empty($context)) {
         throw new SoapFault("PortageBadArgs",
                             "You must provide a valid context.");
      }

      if (!isset($document_model_id) || empty($document_model_id)) {
         throw new SoapFault("PortageBadArgs",
                             "You must provide a valid document_model_id.");
      }

      $work_dir = $this->getDocumentModelWorkDir($context, $document_model_id);

      if (! is_dir($work_dir))
         return "N/A";

      # Check if an update is in progress.
      $update_in_progress = false;
      # Lock the queue while we check the training lock to avoid a race
      # condition with incr-add-sentence.sh.
      $queue_lock_fp = @fopen("$work_dir/queue.lock", 'r');
      if ($queue_lock_fp) {
         flock($queue_lock_fp, LOCK_EX);
         $training_lock_fp = @fopen("$work_dir/istraining.lock", 'r');
         if ($training_lock_fp) {
            if(!flock($training_lock_fp, LOCK_EX | LOCK_NB))
               $update_in_progress = true;
            else
               flock($training_lock_fp, LOCK_UN);
            fclose($training_lock_fp);
         }
         flock($queue_lock_fp, LOCK_UN);
         fclose($queue_lock_fp);
      }

      # Check if an update is pending.
      $update_pending = @filesize("$work_dir/queue") != 0;

      $update_complete = !$update_pending && !$update_in_progress
                         && @filesize("$work_dir/corpora") != 0;

      if (!$update_pending && !$update_in_progress && !$update_complete)
         return "N/A";

      $status = "Update ";
      if ($update_pending) {
         $status .= "pending";
         if ($update_in_progress)
            $status .= "+in_progress";
      } elseif ($update_in_progress)
         $status .= "in_progress";
      elseif ($update_complete)
         $status .= "complete";

      $last_update_status = trim(@file_get_contents("$work_dir/incr-update.status"));
      if ($last_update_status == "")
         $last_update_status = "N/A";
      elseif ($last_update_status == "0")
         $last_update_status .= " success";
      else
         $last_update_status .= " failure";

      $corp_size = trim(`cat $work_dir/corpora 2> /dev/null | wc -l`);
      $queue_size = trim(`cat $work_dir/queue 2> /dev/null | wc -l`);

      return "$status, $last_update_status, corpus: $corp_size, queue: $queue_size";
   }


   # ==========================================================
   # Deprecated functions kept for backwards compatibility only
   # ==========================================================
   public function getTranslation($src_string, $newline, $xtags)
   {
      if (!isset($newline)) $newline = "p"; # newline arg was added in PortageII 2.1
      if (!isset($xtags)) $xtags = false; # xtags arg was added in PortageII 2.1
      return $this->translate($src_string, "context", $newline, $xtags, false);
   }
   public function getTranslation2($src_string, $context, $newline, $xtags)
   {
      if (!isset($newline)) $newline = "p"; # newline arg was added in PortageII 2.1
      if (!isset($xtags)) $xtags = false; # xtags arg was added in PortageII 2.1
      return $this->translate($src_string, $context, $newline, $xtags, false);
   }
   public function getTranslationCE($src_string, $context, $newline, $xtags)
   {
      if (!isset($newline)) $newline = "p"; # newline arg was added in PortageII 2.1
      if (!isset($xtags)) $xtags = false; # xtags arg was added in PortageII 2.1
      return $this->translate($src_string, $context, $newline, $xtags, true);
   }
   public function translateTMXCE($TMX_contents_base64, $TMX_filename, $context,
                                  $ce_threshold, $xtags)
   {
      if (!isset($xtags)) $xtags = false; # xtags arg was added in PortageII 2.0
      return $this->translateFileCE($TMX_contents_base64,
         $this->guardFilename($TMX_filename, 'tmx'),
         $context,
         true,
         $ce_threshold,
         $xtags,
         'tmx');
   }
   public function translateSDLXLIFFCE($SDLXLIFF_contents_base64, $SDLXLIFF_filename,
                                       $context, $ce_threshold, $xtags)
   {
      return $this->translateFileCE($SDLXLIFF_contents_base64,
         $this->guardFilename($SDLXLIFF_filename, 'sdlxliff'),
         $context,
         true,
         $ce_threshold,
         $xtags,
         'sdlxliff');
   }
   public function translatePlainTextCE($PlainText_contents_base64, $PlainText_filename,
                                        $context, $ce_threshold, $xtags)
   {
      return $this->translateFileCE($PlainText_contents_base64,
         $this->guardFilename($PlainText_filename, 'txt'),
         $context,
         true,
         $ce_threshold,
         $xtags,
         'plaintext');
   }
   public function translateTMXCE_Status($monitor_token)
   {
      return $this->translateFileStatus($monitor_token);
   }
   public function translateSDLXLIFFCE_Status($monitor_token)
   {
      return $this->translateFileStatus($monitor_token);
   }
   public function translatePlainTextCE_Status($monitor_token)
   {
      return $this->translateFileStatus($monitor_token);
   }
   # ==============================================================
   # End deprecated functions kept for backwards compatibility only
   # ==============================================================

}

?>
