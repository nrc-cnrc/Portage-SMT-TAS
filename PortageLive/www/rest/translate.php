<?php
/**
 * @author Samuel Larkin
 * @file translate.php
 * @brief restful json api for Portage translate.
 *
 * This rest api mimics Google's translation api and is used in Matecat's integration.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Technologies de l'information et des communications /
 *   Information and Communications Technologies
 *   Conseil national de recherches Canada / National Research Council Canada
 *   Copyright 2017, Sa Majeste la Reine du Chef du Canada
 *   Copyright 2017, Her Majesty in Right of Canada
 */

# curl --get http://132.246.128.219/language/translate/v3.php --data q=hello  --data q=tree  --data q=car
# curl --get http://132.246.128.219/language/translate/v3.php --data q=hello  --data q=tree  --data q=car --data target=fr  --data key=toy-regress-
# curl --get http://132.246.128.219/language/translate/translate.php --data target=fr --data q=hello  --data q='tree+%26amp%3B+lake'  --data q='home+%26+hardware' --data target=fr  --data key=toy-regress- --data prettyprint=false

# QUERY_STRING="q=tree&target=fr&key=toy-regress-&prettyprint=false" php translate.php
# QUERY_STRING="target=fr&q=hello&q=tree+%26amp%3B+lake&q=car&target=fr&key=toy-regress-&prettyprint=false" php translate.php

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

require 'PortageLiveLib.php';

class RestTranlator extends PortageLiveLib {

   protected $source = '';
   protected $target = '';
   protected $prettyprint = true;
   protected $key = '';
   protected $q = array();

   protected $invalid_url_arguments = array();


   /**
    * This actually performs per argument check.
    */
   protected function parseRequest($request) {
      #var_dump($request);
      if (!isset($request) || empty($request)) {
         throw new Exception("There is no query.");
      }

      foreach ($request as $k => $v) {
         switch($k) {
            case "key":
               $this->key = $v;
               break;
            case "prettyprint":
               $this->prettyprint = filter_var($v, FILTER_VALIDATE_BOOLEAN, FILTER_NULL_ON_FAILURE);
               break;
            case "q":
               if (!is_array($v)) {
                  $v = array($v);
               }
               $this->q = array_map(function($q) {
                     $q = urldecode($q);
                     $q = html_entity_decode($q);
                     return $q;
                  },
                  $v);
               break;
            case "source":
               $this->source = $v;
               break;
            case "target":
               $this->target = $v;
               break;
            default:
               array_push($this->invalid_url_arguments, "$k=$v");
         }
      }
   }


   /**
    * Assembles the translations into the expected JSON structure.
    */
   protected function bundleTranslations($translations) {
      /**
       * This is a help function that wraps Portage's translations into the
       * appropriate google spec format.
       */
      $wrapTranslation = function ($t) {
         return array("translatedText" => $t);
      };

      # We want one translation per line.
      $translations = split("\n", $translations);
      # Remove empty translations.
      $translations = array_filter($translations);
      # Prepare the json structure.
      # Transform the translations into the proper JSON format.
      $translations = array_map($wrapTranslation, $translations);
      # Let's bundle the translations into an object.
      $translations = array("translations" => $translations);
      # One last wrapping layer to reproduce google's return json format.
      $translations = array("data" => $translations);

      if (count($this->invalid_url_arguments) > 0) {
         $invalid_url_arguments_warning = array(
            'message' => 'You used invalid argument(s)',
            'arguments' => $this->invalid_url_arguments
         );
         if (!isset($translations['warnings'])) {
            $translations['warnings'] = array();
         }
         array_push($translations['warnings'], $invalid_url_arguments_warning);
      }

      return json_encode($translations, $this->prettyprint ? JSON_PRETTY_PRINT : 0);
   }


   public function translate() {
      $this->parseRequest(@$_REQUEST);

      if (!isset($this->target) || empty($this->target)) {
         throw new Exception("You need to provide a target using target=X.");
      }

      # Validate that source is a valid source language / supported source
      # language.

      # Deduce and/or validate the target language.
      if (!isset($this->source) || empty($this->source)) {
         if ($this->target === "fr") $this->source = "en";
         if ($this->target === "en") $this->source = "fr";
      }

      $performTagTransfer = false;
      $useConfidenceEstimation = false;
      $newline = "p";
      # System's format:
      #   <key> dot <source> dash <target>
      if (substr($this->key, -1) !== '.') {
         $this->key .= '.';
      }
      $context = $this->key . $this->source . "-" . $this->target;

      if (count($this->q) > 0) {
         # For efficiency, let's glue all queries into a single request for Portage.
         $q = join("\n", $this->q);
         # Translate the queries.
         $translations = parent::translate($q, $context, $newline, $performTagTransfer, $useConfidenceEstimation);

         return $this->bundleTranslations($translations);
      }
      else {
         throw new Exception("You need to provide a query using q=X.");
      }
   }

}



try {
   header('Content-Type: application/json');
   $translator = new RestTranlator();
   print $translator->translate();
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
