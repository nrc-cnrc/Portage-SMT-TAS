<?php
# @file incrAddSentence.php
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

require 'incrAddSentence.php';

# Let's change the web workdir to a local workdir.  It needs to be an absolute
# path.
$base_web_dir = getcwd();
$base_portage_dir = getcwd() . '/tests/';

class IncrAddSentence_Test extends PHPUnit_Framework_TestCase
{
   protected $basicRequest;

   public function __construct() {
      $this->basicRequest = array(
         'context' => 'unittest.rev.en-fr',
         'document_level_model_ID' => 'rest_local',
         'source' => 'Source',
         'target' => 'Target'
         );
   }


   /**
    * Helper function to recursively remove a directory.
    */
   public static function deleteDir($dirPath) {
      if (! is_dir($dirPath)) {
         return;
      }
      if (substr($dirPath, strlen($dirPath) - 1, 1) != '/') {
         $dirPath .= '/';
      }
      $files = glob($dirPath . '*', GLOB_MARK);
      foreach ($files as $file) {
         if (is_dir($file)) {
            self::deleteDir($file);
         }
         else {
            unlink($file);
         }
      }
      rmdir($dirPath);
   }


   protected function setUp() {
      if (! is_dir('plive')) {
         mkdir('plive');
      }
      chmod('plive', 0755);
   }


   /**
    * @test
    * @expectedException Exception
    * @expectedExceptionMessage There is no query.
    */
   public function noArgument() {
      $service = new IncrementalTrainor();

      $_REQUEST = array();

      $service->addSentencePair();
   }


   /**
    * @test 
    * @expectedException Exception
    * @expectedExceptionMessage There is no query.
    */
   public function emptyArgument() {
      $service = new IncrementalTrainor();

      $_REQUEST = array();

      $service->addSentencePair();
   }


   /**
    * @test
    * @expectedException Exception
    * @expectedExceptionMessage You must provide a valid document_model_id.
    */
   public function noDocumentLevelModelID() {
      $service = new IncrementalTrainor();

      $_REQUEST = $this->basicRequest;
      unset($_REQUEST['document_level_model_ID']);

      $service->addSentencePair();
   }


   /**
    * @test
    * @expectedException Exception
    * @expectedExceptionMessage You must provide a source sentence.
    */
   public function noSourceSentence() {
      $service = new IncrementalTrainor();

      $_REQUEST = $this->basicRequest;
      unset($_REQUEST['source']);

      $service->addSentencePair();
   }


   /**
    * @test
    * @expectedException Exception
    * @expectedExceptionMessage You must provide a target sentence.
    */
   public function noTargetSentence() {
      $service = new IncrementalTrainor();

      $_REQUEST = $this->basicRequest;
      unset($_REQUEST['target']);

      $service->addSentencePair();
   }


   /**
    * @test
    */
   public function basicUsage() {
      $service = new IncrementalTrainor();

      $_REQUEST = $this->basicRequest;

      $result = $service->addSentencePair();

      $this->assertTrue(isset($result), 'Adding a sentence pair is supposed to return us something.');
      $this->assertTrue(isset($result['result']), 'There should be a result field.');
      $this->assertTrue($result['result'], 'The returned result should indicate success.');

      $this->assertFalse(isset($result['warnings']), 'There should be absolutely no warnings.');
   }


   /**
    * @test
    */
   public function pliveDirectoryMissing() {
      IncrAddSentence_Test::deleteDir('plive');
      $this->assertFalse(is_dir('plive'));

      $service = new IncrementalTrainor();

      $_REQUEST = $this->basicRequest;

      $result = $service->addSentencePair();

      $this->assertTrue(isset($result), 'Adding a sentence pair is supposed to return us something.');
      $this->assertTrue(isset($result['result']), 'There should be a result field.');
      $this->assertTrue($result['result'], 'The returned result should indicate success.');

      $this->assertFalse(isset($result['warnings']), 'There should be absolutely no warnings.');
   }


   /**
    * @test
    */
   public function usingInvalidArguments() {
      $service = new IncrementalTrainor();

      $_REQUEST = $this->basicRequest;
      $_REQUEST['bad']   = 'invalid';
      $_REQUEST['wrong'] = 'unsupported';

      $result = $service->addSentencePair();

      $this->assertTrue(isset($result), 'Adding a sentence pair is supposed to return us something.');
      $this->assertTrue(isset($result['result']), 'There should be a result field.');
      $this->assertTrue($result['result'], 'The returned result should indicate success.');

      $this->assertJsonStringEqualsJsonString(
         '{"result":true,"warnings":{"InvalidArgument":{"message":"You used invalid options","options":["bad=invalid","wrong=unsupported"]}}}',
         json_encode($result),
         'In the returned message, we should see a warning about invalid arguments and we should also see the two invalid arguments.');
   }


   /**
    * @test
    */
   public function pliveDirectoryNotWritable() {
      $this->markTestSkipped( 'Unable to properly handle read-only plive folder.');

      IncrAddSentence_Test::deleteDir('plive');
      $this->assertFalse(is_dir('plive'));
      mkdir('plive', 0500);
      $this->assertTrue(is_dir('plive'));

      $_REQUEST = $this->basicRequest;
      $_REQUEST['source'] .= __FUNCTION__;
      $_REQUEST['target'] .= __FUNCTION__;

      $service = new IncrementalTrainor();

      $result = $service->addSentencePair();

      $this->assertTrue(isset($result), 'Adding a sentence pair is supposed to return us something.');
      $this->assertTrue(isset($result['result']), 'There should be a result field.');
      $this->assertFalse($result['result'], 'The returned result should indicate success.');

      $this->assertFalse(isset($result['warnings']), 'There should be absolutely no warnings.');
   }

};
