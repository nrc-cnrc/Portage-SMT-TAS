<?php
require 'PortageLiveLib.php';

# Let's change the web workdir to a local workdir.  It needs to be an absolute
# path.
$base_web_dir = getcwd();
$base_portage_dir = getcwd() . '/tests/';

class PortageLiveLib_incrAddSentence_Test extends PHPUnit_Framework_TestCase
{
   var $document_model_id;
   var $unique_id;
   var $source;
   var $target;
   var $witnessFileName;
   var $queueFileName;
   var $corporaFileName;

   protected function setUp()
   {
      $this->context = 'unittest.rev.en-fr';
      $this->document_model_id = 'cli_php';
      $this->unique_id = time() . rand(0, 100000);
      $this->source = 'A_' . $this->unique_id;
      $this->target = 'B_' . $this->unique_id;

      global $base_web_dir;
      $document_model_dir = $base_web_dir . "/plive/DOCUMENT_MODEL_"
                            . $this->context
                            . "_"
                            . "{$this->document_model_id}";
      #$workdir = $service->incrClearDocumentModelWorkdir($document_model_id);
      $this->witnessFileName = "$document_model_dir/witness";
      if (is_file($this->witnessFileName)) {
         $this->assertTrue(unlink($this->witnessFileName),
                           'Unable to delete the witness file.');
      }

      $this->queueFileName = "$document_model_dir/queue";
      if (is_file($this->queueFileName)) {
         $this->assertTrue(unlink($this->queueFileName),
                           'Unable to delete the queue file.');
      }

      $this->corporaFileName = "$document_model_dir/corpora";
      if (is_file($this->corporaFileName)) {
         $this->assertTrue(unlink($this->corporaFileName),
                           'Unable to delete the corpora file.');
      }
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage You must provide a valid context.
    */
   public function test_no_argument()
   {
      $service = new PortageLiveLib();

      # We need to make the source and target unique if we want to test for
      # their presence in the corpora.
      $result = $service->incrAddSentence();

      $this->assertFalse($result);
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage You must provide a valid document_model_id.
    */
   public function test_document_level_id()
   {
      $service = new PortageLiveLib();

      # We need to make the source and target unique if we want to test for
      # their presence in the corpora.
      $result = $service->incrAddSentence($this->context);

      $this->assertFalse($result);
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage You must provide a source sentence.
    */
   public function test_no_source()
   {
      $service = new PortageLiveLib();

      # We need to make the source and target unique if we want to test for
      # their presence in the corpora.
      $result = $service->incrAddSentence($this->context, $this->document_model_id);

      $this->assertFalse($result);
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage You must provide a target sentence.
    */
   public function test_no_target()
   {
      $service = new PortageLiveLib();

      # We need to make the source and target unique if we want to test for
      # their presence in the corpora.
      $result = $service->incrAddSentence($this->context, $this->document_model_id,
                                          $this->source);

      $this->assertFalse($result);
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage Can't create temp work dir for document
    */
   public function test_bad_document_model_workdir()
   {
      $bad_id = "cli_php_bad_1";

      # Create a bad document model workdir by creating it as a file.
      global $base_web_dir;
      $document_model_workdir = $base_web_dir . "/plive/DOCUMENT_MODEL_" . $this->context . "_$bad_id";
      file_put_contents($document_model_workdir, "");

      $service = new PortageLiveLib();

      # We need to make the source and target unique if we want to test for
      # their presence in the corpora.
      $result = $service->incrAddSentence($this->context, $bad_id, $this->source,
                                          $this->target);

      $this->assertFalse($result);
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage Can't create temp work dir for document
    */
   public function test_read_only_plive()
   {
      $bad_id = "cli_php_bad_2";

      # Change the plive directory to be read-only.
      global $base_web_dir;
      $plive_dir = $base_web_dir . "/plive";
      $this->assertTrue(chmod($plive_dir, 0555));

      $service = new PortageLiveLib();

      # We need to make the source and target unique if we want to test for
      # their presence in the corpora.
      try {
         $result = $service->incrAddSentence($this->context, $bad_id,
                                             $this->source, $this->target);
      } finally {
         # Change the plive directory back to read-write.
         $this->assertTrue(chmod($plive_dir, 0775));
      }

      $this->assertFalse($result);
   }


   public function test_basic_valid_usage()
   {
      $service = new PortageLiveLib();

      # We need to make the source and target unique if we want to test for
      # their presence in the corpora.
      $result = $service->incrAddSentence($this->context, $this->document_model_id,
                                          $this->source, $this->target);

      # Assert
      $this->assertEquals(True, $result, "Unable to add sentence pair to the queue.");

      # Let incremental training finish.
      sleep(3);

      $this->assertFileExists($this->queueFileName, "There should be a queue file.");
      $this->assertEquals(filesize($this->queueFileName), 0, 'The queue should be empty.');

      $this->assertFileExists($this->corporaFileName, "There should be a corpora file.");
      $corpora = file_get_contents($this->corporaFileName);
      #2017-06-07 15:24:19\tsource_sentence\ttranslation_sentence
      $this->assertRegExp("/[0-p: -]+\t$this->source\t$this->target/", trim($corpora),
                          "The corpora file should contain our sentence pair.");

      $this->assertFileExists($this->witnessFileName,
         "There should be a witness that attests that training was completed.");
      $witness = file_get_contents($this->witnessFileName);
      $this->assertEquals('Training is done', trim($witness),
                          "We were expecting that training would've been done.");
   }
}
?>
