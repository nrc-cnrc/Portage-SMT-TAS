<?php
require 'PortageLiveLib.php';

# Let's change the web workdir to a local workdir.  It needs to be an absolute
# path.
$base_web_dir = getcwd();

class PortageLiveLib_incrAddSentence_Test extends PHPUnit_Framework_TestCase
{
   var $document_level_model_ID;
   var $unique_id;
   var $source;
   var $target;
   var $witnessFileName;
   var $queueFileName;
   var $corporaFileName;

   protected function setUp()
   {
      $this->document_level_model_ID = PortageLiveLib::MAGIC_UNITTEST_DOCUMENT_ID;
      $this->unique_id = time() . rand(0, 100000);
      $this->source = 'A_' . $this->unique_id;
      $this->target = 'B_' . $this->unique_id;

      global $base_web_dir;
      #$workdir = $service->incrClearDocumentLevelModelWorkdir($document_level_model_ID);
      $this->witnessFileName = $base_web_dir . "/plive/DOCUMENT_LEVEL_MODEL_{$this->document_level_model_ID}/witness";
      if (is_file($this->witnessFileName)) {
         $this->assertTrue(unlink($this->witnessFileName), 'Unable to delete the witness file.');
      }

      $this->queueFileName = $base_web_dir . "/plive/DOCUMENT_LEVEL_MODEL_{$this->document_level_model_ID}/queue";
      if (is_file($this->queueFileName)) {
         $this->assertTrue(unlink($this->queueFileName), 'Unable to delete the queue file.');
      }

      $this->corporaFileName = $base_web_dir . "/plive/DOCUMENT_LEVEL_MODEL_{$this->document_level_model_ID}/corpora";
      if (is_file($this->corporaFileName)) {
         $this->assertTrue(unlink($this->corporaFileName), 'Unable to delete the corpora file.');
      }
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage You must provide a valid document_level_model_ID.
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
    * @expectedExceptionMessage You must provide a source sentence.
    */
   public function test_no_source()
   {
      $service = new PortageLiveLib();

      # We need to make the source and target unique if we want to test for
      # their presence in the corpora.
      $result = $service->incrAddSentence($this->document_level_model_ID);

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
      $result = $service->incrAddSentence($this->document_level_model_ID, $this->source);

      $this->assertFalse($result);
   }


   public function test_basic_valid_usage()
   {
      $service = new PortageLiveLib();

      # We need to make the source and target unique if we want to test for
      # their presence in the corpora.
      $result = $service->incrAddSentence($this->document_level_model_ID, $this->source, $this->target);

      # Assert
      $this->assertEquals(True, $result, "Unable to add sentence pair to the queue.");

      # Let incremental training finish.
      sleep(3);

      $this->assertFileExists($this->queueFileName, "There should be a queue file.");
      $this->assertEquals(filesize($this->queueFileName), 0, 'The queue should be empty.');

      $this->assertFileExists($this->corporaFileName, "There should be a corpora file.");
      $corpora = file_get_contents($this->corporaFileName);
      #2017-06-07 15:24:19\tsource_sentence\ttranslation_sentence
      $this->assertRegExp("/[0-p: -]+\t$this->source\t$this->target/",
         trim($corpora),
         "The corpora file should contain our sentence pair.");

      $this->assertFileExists($this->witnessFileName,
         "There should be a witness that attests that training was completed.");
      $witness = file_get_contents($this->witnessFileName);
      $this->assertEquals('Training is done',
         trim($witness),
         "We were expecting that training would've been done.");
   }
}
?>
