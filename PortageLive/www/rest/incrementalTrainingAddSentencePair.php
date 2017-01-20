<?php
# @file incrementalTrainingAddSentencePair.php
# @brief Incremental Training REST API
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada


# MATECAT API:
# https://www.matecat.com/api/docs
# https://github.com/matecat/MateCat-Filters/wiki/API-documentation

# CRUD:
# https://en.wikipedia.org/wiki/Create,_read,_update_and_delete
# Operation          SQL      HTTP                 DDS
# Create             INSERT   PUT / POST           write
# Read (Retrieve)    SELECT   GET                  read / take
# Update (Modify)    UPDATE   POST / PUT / PATCH   write
# Delete (Destroy)   DELETE   DELETE               dispose

# https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol#Request_methods
# POST
#   The POST method requests that the server accept the entity enclosed in the
#   request as a new subordinate of the web resource identified by the URI. The
#   data POSTed might be, for example, an annotation for existing resources; a
#   message for a bulletin board, newsgroup, mailing list, or comment thread; a
#   block of data that is the result of submitting a web form to a
#   data-handling process; or an item to add to a database.
# PUT
#   The PUT method requests that the enclosed entity be stored under the
#   supplied URI. If the URI refers to an already existing resource, it is
#   modified; if the URI does not point to an existing resource, then the
#   server can create the resource with that URI.
# PATCH
#   The PATCH method applies partial modifications to a resource.

/*
 * What should be the signature of this REST call?
 * Should the document level model id be in the URL or part of the arguments?
 *
 */

require 'PortageLiveLib.php';

class Warnings implements JsonSerializable {
   protected $warningIssued = False;
   protected $InvalidArgument = array(
      "message" => "You used invalid options",
      "options" => array());

   public function addInvalidArgument($argument) {
      $this->warningIssued = True;
      array_push($this->InvalidArgument['options'], $argument);
   }

   public function isWarningIssued() {
      return $this->warningIssued;
   }

   public function jsonSerialize()
   {
      $warnings = get_object_vars($this);
      # We don't want warningIssued to be serialized.
      unset($warnings['warningIssued']);
      return $warnings;
   }
};



class IncrementalTrainor extends PortageLiveLib {
   protected $document_level_model_ID = NULL;
   protected $source = NULL;
   protected $target = NULL;

   protected $warnings = NULL;  # This will keep track of warnings for us.

   public function __construct() {
      $this->warnings = new Warnings();
   }

   /**
    * Properly decode the entities in `value` of the query string making sure
    * we handle the ampersand correctly.
    */
   protected function decodeArgument($v) {
      $v = urldecode($v);
      $v = html_entity_decode($v);
      return $v;
   }


   /**
    * Given a query string, extract its argument aka key=value pairs making
    * sure to properly handle ampersand in value.
    */
   protected function parseRequest($request) {
      #print_r($_SERVER['QUERY_STRING']."\n");
      if (!isset($request) || empty($request)) {
         throw new Exception("There is no query.");
      }

      foreach (split('&', $request) as $a) {
         list($k, $v) = split('=', $a, 2);
         switch($k) {
            case "document_level_model_ID":
               $this->document_level_model_ID = $this->decodeArgument($v);
               break;
            case "source":
               $this->source = $this->decodeArgument($v);
               break;
            case "target":
               $this->target = $this->decodeArgument($v);
               break;
            default:
               $this->warnings->addInvalidArgument("$k=$v");
         }
      }
   }


   public function addSentencePair() {
      $this->parseRequest(@$_SERVER['QUERY_STRING']);

      # We will let PortageLiveLib::incrementalTrainingAddSentencePair()
      # handle its arguments errors.
      $result = $this->incrementalTrainingAddSentencePair(
         $this->document_level_model_ID,
         $this->source,
         $this->target
      );

      $message = array('result' => $result);

      # Warnings should not cause failure but should at the very least be
      # reported to the user.
      if ($this->warnings->isWarningIssued()) {
         $message['warnings'] = $this->warnings;
      }

      return $message;
   }
}



# if this wasn't loaded as a library by phpunit, execute a main function.
# This is the equivalent of Python's if __name__ == '__main__':
if (!count(debug_backtrace())) {
   try {
      header('content-type: application/json');
      $incrementalTrainor = new IncrementalTrainor();
      print json_encode($incrementalTrainor->addSentencePair());
   }
   catch (SoapFault $exception) {
      http_response_code(404);
      $error = array(
         "result" => False,
         "error" => array(
            "message" => $exception->getMessage(),
            "type" => $exception->faultcode
         )
      );
      print json_encode($error);
   }
   catch (Exception $exception) {
      http_response_code(404);
      $error = array(
         "result" => False,
         "error" => array(
            "message" => $exception->getMessage()
         )
      );
      print json_encode($error);
   }
}

?>
