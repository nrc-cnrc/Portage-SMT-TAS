<?php
/**
 * @author Darlene Stewart
 * @file incrStatus.php
 * @brief A REST API to get the status of incremental training.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Technologies de l'information et des communications /
 *   Information and Communications Technologies
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2017, Sa Majeste la Reine du Chef du Canada
 * Copyright 2017, Her Majesty in Right of Canada
 */


require 'PortageLiveLib.php';

class Warnings {
   protected $warningIssued = False;
   protected $invalidArgument = array(
         "message" => "You used invalid options",
         "options" => array());

   public function addInvalidArgument($argument) {
      $this->warningIssued = True;
      array_push($this->invalidArgument['options'], $argument);
   }

   public function isWarningIssued() {
      return $this->warningIssued;
   }

   public function jsonSerialize() {
      $warnings = get_object_vars($this);
      # We don't want warningIssued to be serialized.
      unset($warnings['warningIssued']);
      sort($warnings['invalidArgument']['options'], SORT_STRING);  # to have consistent unittesting.
      return $warnings;
   }
}

/**
 * Properly decode the entities in `value` of the query string making sure
 * we handle the ampersand correctly.
 */
function decodeArgument($v) {
   $v = urldecode($v);
   $v = html_entity_decode($v);
   return $v;
}

/**
 * Verify the arguments and record those that are not part of the
 * interface for error reporting them back to the caller.
 */
function parseRequest($request) {
   if (!isset($request) || empty($request)) {
      throw new Exception("There is no query.");
   }

   $warnings = new Warnings();

   foreach ($request as $k => $v) {
      switch($k) {
         case 'context':
            $context = decodeArgument($v);
            break;
         case 'document_model_ID':
            $document_model_ID = decodeArgument($v);
            break;
         default:
            # Report back to the user any invalid arguments in the query.
            $warnings->addInvalidArgument("$k=$v");
      }
   }

   return array('context' => $context,
                'document_model_ID' => $document_model_ID,
                'warnings' => $warnings);
}

/**
 * Package the response from incrStatus into an Object.
 * @param String $response
 */
function packageResponse($response) {
   $fields = preg_split('/, */', $response, 4);
   $object = array();
   if (count($fields) < 2) {
      $object['update'] = $fields[0];
   } else {
      $update = explode(' ', $fields[0], 2);
      if ($update[0] == 'Update')
         $object['update'] = $update[1];
      else
         $object['update'] = $fields[0];
      if (count($fields) > 1) {
         $object['exit_status'] = $fields[1];
      }
      if (count($fields) > 2) {
         $corpus_size = explode(' ', $fields[2], 2);
         if ($corpus_size[0] == 'corpus:')
            $object['corpus_size'] = (int)$corpus_size[1];
         else
            $object['corpus_size'] = $fields[2];
      }
      if (count($fields) > 3) {
         $queue_size = explode(' ', $fields[3], 2);
         if ($queue_size[0] == 'queue:')
            $object['queue_size'] = (int)$queue_size[1];
         else
            $object['queue_size'] = $fields[3];
      }
   }
   return $object;
}

try {
   header('content-type: application/json');

   # GET and POST both populate $_REQUEST.
   $request = parseRequest(@$_REQUEST);

   $portageLiveLib = new PortageLiveLib();
   $response = $portageLiveLib->incrStatus($request['context'], $request['document_model_ID']);
   $incr_status = packageResponse($response);
   $message = array('incr_status' => $incr_status);

   # Invalid argument warnings shouldn't cause a failure but should be reported
   # to the user.
   if ($request['warnings']->isWarningIssued()) {
      $message['warnings'] = $request['warnings']->jsonSerialize();
   }

   print json_encode($message);
}
catch (SoapFault $exception) {
   http_response_code(404);
   $error = array(
      "error" => array(
         "message" => $exception->getMessage(),
         "type" => $exception->faultcode,
         "code" => -3
      )
   );
   print json_encode($error);
}
catch (Exception $exception) {
   http_response_code(404);
   $error = array(
      "error" => array(
         "message" => $exception->getMessage(),
         "code" => -3
      )
   );
   print json_encode($error);
}

?>
