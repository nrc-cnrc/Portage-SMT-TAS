<?php
/**
 * @author Samuel Larkin
 * @file getAllContexts.php
 * @brief A REST API to get PortageLive's Contexts.
 *
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Technologies de l'information et des communications /
 *   Information and Communications Technologies
 *   Conseil national de recherches Canada / National Research Council Canada
 *   Copyright 2017, Sa Majeste la Reine du Chef du Canada
 *   Copyright 2017, Her Majesty in Right of Canada
 */


/*
 * Replace this stub and create a class to handle and parse getAllContexts() requests.
 */

try {
   header('Content-Type: application/json');
   $response = array(
      array ('name' => 'generic-model', 'fullname' => 'generic-model.en-fr', 'source' => 'en', 'target' => 'en'),
      array ('name' => 'generic-model', 'fullname' => 'generic-model.fr-en', 'source' => 'fr', 'target' => 'fr')
   );
   print json_encode($response);
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
