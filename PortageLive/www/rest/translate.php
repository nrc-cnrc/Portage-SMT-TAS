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
 * Copyright 2016, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2016, Her Majesty in Right of Canada
 */

// curl --get http://132.246.128.219/language/translate/v3.php --data q=hello  --data q=tree  --data q=car
// curl --get http://132.246.128.219/language/translate/v3.php --data q=hello  --data q=tree  --data q=car --data target=fr  --data key=toy-regress-
// curl --get http://132.246.128.219/language/translate/translate.php --data target=fr --data q=hello  --data q='tree+%26amp%3B+lake'  --data q=car --data target=fr  --data key=toy-regress- --data prettyprint=false

// QUERY_STRING="q=tree&target=fr&key=toy-regress-&prettyprint=false" php translate.php
// QUERY_STRING="target=fr&q=hello&q=tree+%26amp%3B+lake&q=car&target=fr&key=toy-regress-&prettyprint=false" php translate.php

/*
 * This is a sample of the expected output format.
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

or

{
   "data": {
      "translations": [
         {
            "translatedText": "hello"
         },
         {
            "translatedText": "tree & Lake"
         },
         {
            "translatedText": "car"
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
   protected $q = array();


   /**
    * This function is necessary in order to properly extract multiple q(ueries) from the url.
    * Simply relying on $_GET['q'] is not sufficient since it will only return
    * the last q(uery) value.
    */
   public function parseRequest() {
      //print_r($_SERVER['QUERY_STRING']."\n");
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


   public function bundleTranslations($translations) {
      /**
       * This is a help function that wraps Portage's translations into the
       * appropriate google spec format.
       */
      $wrapTranslation = function ($t) {
         return array("translatedText" => $t);
      };

      // We want one translation per line.
      $translations = split("\n", $translations);
      // Remove empty translations.
      $translations = array_filter($translations);
      // Prepare the json structure.
      // Transform the translations into the proper JSON format.
      $translations = array_map($wrapTranslation, $translations);
      // Let's bundle the translations into an object.
      $translations = array("translations" => $translations);
      // One last wrapping layer to reproduce google's return json format.
      $translations = array("data" => $translations);

      return json_encode($translations, $this->prettyprint ? JSON_PRETTY_PRINT : 0);
   }


   public function translate() {
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
         $translations = parent::translate($q, $context, $newline, $performTagTransfer, $useConfidenceEstimation);

         print $this->bundleTranslations($translations) . PHP_EOL;
      }
      else {
         throw new Exception(json_encode(array("ERROR" => array("message" => "You need to provide a query using q=X."))));
      }
   }

}



try {
   $translator = new RestTranlator();
   $translator->translate();
}
catch (SoapFault $exception) {
   print json_encode(array("ERROR" => array("SOAPfault" => $exception))) . "\n";
}
catch (Exception $exception) {
   print $exception->getMessage() . "\n";
}

?>
