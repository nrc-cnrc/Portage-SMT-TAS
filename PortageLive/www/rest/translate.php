<?php
/**
 * @author Samuel Larkin
 * @file translate.php
 * @brief restful json api for Portage translate.
 *
 * This rest api mimics Google's translation api and is used in Matecat's integration.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

/*
 * TODO:
 * - The following functions were extracted from PortageLiveAPI.php's class
 *    methods and should be factorized out with the original into a library.
 *      - getContextInfo
 *      - validateContext
 *      - normalizeName
 *      - makeWorkDir
 *      - runCommand
 *      - translate
 * - This was hack rather quickly and should be revised for robustness.
 */

// curl -G http://132.246.128.219/language/translate/v3.php -d q=hello  -d q=tree  -d q=car
// curl -G http://132.246.128.219/language/translate/v3.php -d q=hello  -d q=tree  -d q=car -d target=fr  -d key=toy-regress-
// QUERY_STRING="q=tree&target=fr&key=toy-regress-&prettyprint=false" php translate.php
// QUERY_STRING="q=tree&target=fr&key=toy-regress-&prettyprint=false" php -e -r 'parse_str($_SERVER["QUERY_STRING"], $_GET); include "translate.php";'

/*
{
    "data": {
        "translations": [
            {
                "translatedText": "Hallo Welt",
                "detectedSourceLanguage": "en"
            }
        ]
    }
}
*/

require 'basicTranslator.php';

class RestTranlator extends BasicTranslator {

   protected $source = '';
   protected $target = '';
   protected $prettyprint = true;
   protected $key = '';
   protected $q;


   public function parseRequest() {
      //print_r($_SERVER['QUERY_STRING']."\n");
      $this->q = array();
      foreach (split('&', $_SERVER['QUERY_STRING']) as $a) {
         list($k, $v) = split('=', $a, 2);
         switch($k) {
            case "key":
               $this->key = $v;
               break;
            case "prettyprint":
               $this->prettyprint = filter_var($v, FILTER_VALIDATE_BOOLEAN, FILTER_NULL_ON_FAILURE);
               break;
            case "q":
               $v = urldecode($v);
               $v = html_entity_decode($v);
               array_push($this->q, $v);
               break;
            case "source":
               $this->source = $v;
               break;
            case "target":
               $this->target = $v;
               break;
            default:
         }
      }
   }


   public function processQuery() {
      $prep = function ($t) {
         return array("translatedText" => $t);
      };

      if (!isset($_SERVER['QUERY_STRING']) || empty($_SERVER['QUERY_STRING'])) {
         // There are no query_string, should it be an error or should we return some documentation?
         throw new Exception( json_encode(array("ERROR" => array("message" => "There is no query."))));
      }

      $this->parseRequest();

      if (!isset($this->target) || empty($this->target)) {
         throw new Exception(json_encode(array("ERROR" => array("message" => "You need to provide a target using target=X."))));
      }

      // Validate that source is a valid source language / supported source language.

      // Deduce and/or validate the target language.
      if (!isset($this->source) || empty($this->source)) {
         if ($this->target === "fr") $this->source = "en";
         if ($this->target === "en") $this->source = "fr";
      }

      $performTagTransfer = false;
      $useConfidenceEstimation = false;
      $newline = "p";
      $context = $this->key . $this->source . "-" . $this->target;

      if ((int)$this->q > 0) {
         // For efficiency, let's glue all queries into a single request for Portage.
         $q = join("\n", $this->q);
         // Translate the queries.
         $translations = $this->translate($q, $context, $newline, $performTagTransfer, $useConfidenceEstimation);
         // Divide the translations into the original queries.
         $translations = split("\n", $translations);
         // Remove empty translations.
         $translations = array_filter($translations);
         // Transform the translations into the proper JSON format.
         $translations = array_map($prep, $translations);

         print json_encode(array("data" => array("translations" => $translations)), $this->prettyprint ? JSON_PRETTY_PRINT : 0);
      }
      else {
         throw new Exception(json_encode(array("ERROR" => array("message" => "You need to provide a query using q=X."))));
      }
   }

}



try {
   $translator = new RestTranlator();
   $translator -> processQuery();
}
catch (SoapFault $exception) {
   print json_encode(array("ERROR" => array("SOAPfault" => $exception))) . "\n";
}
catch (Exception $exception) {
   print $exception->getMessage() . "\n";
}

?>
