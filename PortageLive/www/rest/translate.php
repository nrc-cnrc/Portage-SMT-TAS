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

   public function processQuery() {
      $prep = function ($t) {
         return array("translatedText" => $t);
      };

      if (!isset($_SERVER['QUERY_STRING']) || empty($_SERVER['QUERY_STRING'])) {
         // There are no query_string, should it be an error or should we return some documentation?
         throw new Exception( json_encode(array("ERROR" => array("message" => "There is no query."))));
      }
      $q = $_GET['q'];
      $q = html_entity_decode($q);


      $key = $_GET['key'];
      $target = $_GET['target'];

      if (isset($_GET['source'])){
         $source = $_GET['source'];
      }

      if (isset($_GET['prettyprint'])){
         $prettyprint = $_GET['prettyprint'];
         }

      if (!isset($_GET['prettyprint'])){
         $prettyprint = "false";
         }

      if (!isset($target) || empty($target)) {
         throw new Exception(json_encode(array("ERROR" => array("message" => "You need to provide a target using target=X."))));
      }

      // Validate that source is a valid source language / supported source language.

      // Deduce and/or validate the target language.
      if (!isset($source) || empty($source)) {
         if ($target === "fr") $source = "en";
         if ($target === "en") $source = "fr";
      }

      $performTagTransfer = false;
      $useConfidenceEstimation = false;
      $newline = "p";
      $context = "$key" . $source . "-" . $target;

      if ($q) {
         // Translate the queries.
         $translations = $this->translate($q, $context, $newline, $performTagTransfer, $useConfidenceEstimation);
         // Divide the translations into the original queries.
         $translations = split("\n", $translations);
         // Remove empty translations.
         $translations = array_filter($translations);
         // Transform the translations into the proper JSON format.
         $translations = array_map($prep, $translations);

         if ( $prettyprint === "true" ) {
            print json_encode(array("data" => array("translations" => $translations)), $prettyprint ? JSON_PRETTY_PRINT : 0);
         }
         if ( $prettyprint === "false" ) {
            print json_encode(array("data" => array("translations" => $translations)));
         }
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
