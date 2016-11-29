<?php
# @file basicTranslator.php
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


# Server configuration options
$base_web_dir = "/var/www/html";
$base_portage_dir = "/opt/PortageII";
$base_url = "/";



# produce debugging information
function debug($i) {
   if ( 0 ) {
      error_log($i, 3, '/tmp/PortageLiveAPI.debug.log');
      return " " . print_r($i, true);
   }
}

class BasicTranslator {

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

   # Validate the context described in context info $i, throwing SoapFault if
   # there are errors.
   # If $need_ce is true, also check that the context provides confidence estimation.
   protected function validateContext(&$i, $need_ce = false) {
      $context = $i["context"];
      if ( ! $i["good"] ) {
         if (!file_exists($i["context_dir"])) {
            throw new SoapFault("PortageContext", "Context \"$context\" does not exist.\n" . debug($i), "PortageLiveAPI");
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
   protected function runCommand($command, $src_string, &$i, &$exit_status = NULL, $wantoutput = true) {
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
         throw new SoapFault("PortageServer", "failed to run $command: $!\n".debug($i));
      }

      return $my_retval . debug($i);
   }

   # Create a working directory based on $filename.
   # Throws SoapFault (faultcode=PortageServer) in case of error.
   # Returns the name of the directory created.
   protected function makeWorkDir($filename) {
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
   protected function normalizeName($filename) {
      return preg_replace(array("/[^-_.+a-zA-Z0-9]/", '/[ .]$/', '/^[ .]/'), array("", '', ''), $filename);
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

      # PTGSH-197
      $work_dir = $this->makeWorkDir("{$context}_srcString");
      $options .= " -dir=\"$work_dir\"";

      return $this->runCommand($i["script"] . $options, $src_string, $i);
   }

}
