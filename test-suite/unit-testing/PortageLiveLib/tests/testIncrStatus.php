<?php
require 'PortageLiveLib.php';

# Let's change the web workdir to a local workdir.  It needs to be an absolute
# path.
$base_web_dir = getcwd();
$base_portage_dir = getcwd() . '/tests/';

class PortageLiveLib_incrStatus_Test extends PHPUnit_Framework_TestCase
{
   var $document_model_id;
   var $document_model_dir;
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
                            . $this->document_model_id;
      $this->document_model_dir = $document_model_dir;

      $files = glob("$document_model_dir/*");
      foreach($files as $file){
         if(is_file($file))
            $this->assertTrue(unlink($file), "Unable to delete $file.");
      }
   }

   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage You must provide a valid document_model_id.
    */
   public function test_no_document_model_id()
   {
      $service = new PortageLiveLib();

      $result = $service->incrStatus();
      $this->assertFalse($result);
   }

   public function test_nonexistent_document_model_id()
   {
      $service = new PortageLiveLib();

      $result = $service->incrStatus($this->document_model_dir);
      $this->assertEquals($result, "N/A", "Should get 'N/A' for nonexistent document model.");
   }

   public function test_empty_document_model()
   {
      @mkdir($this->document_model_dir, 0755, true);

      $service = new PortageLiveLib();

      $result = $service->incrStatus($this->document_model_id);
      $this->assertEquals($result, "N/A", "Should get 'N/A' for empty document model.");
   }

   public function test_one_incr_training()
   {
      $service = new PortageLiveLib();

      $result = $service->incrAddSentence($this->context, $this->document_model_id,
                                          $this->source, $this->target);
      $this->assertEquals(True, $result, "Unable to add sentence pair to the queue.");

      $result = $service->incrStatus($this->document_model_id);
      $expected = "Update in_progress, N/A, corpus: 1, queue: 0";
      $this->assertEquals($result, $expected, "Should get '$expected' for training in progress.");

      # Let incremental training finish.
      sleep(3);

      $result = $service->incrStatus($this->document_model_id);
      $expected = "Update complete, 0 success, corpus: 1, queue: 0";
      $this->assertEquals($result, $expected, "Should get '$expected' after training is completed.");
   }

   public function test_two_incr_training()
   {
      $service = new PortageLiveLib();

      $result = $service->incrAddSentence($this->context, $this->document_model_id,
            $this->source, $this->target);
      $this->assertEquals(True, $result, "Unable to add sentence pair to the queue.");

      $result = $service->incrStatus($this->document_model_id);
      $expected = "Update in_progress, N/A, corpus: 1, queue: 0";
      $this->assertEquals($result, $expected, "Should get '$expected' after first sentence pair added.");

      $result = $service->incrAddSentence($this->context, $this->document_model_id,
            $this->source, $this->target);
      $this->assertEquals(True, $result, "Unable to add sentence pair to the queue.");

      $result = $service->incrStatus($this->document_model_id);
      $expected = "Update pending+in_progress, N/A, corpus: 1, queue: 1";
      $this->assertEquals($result, $expected, "Should get '$expected' after second sentence pair added.");

      # Let first incremental training finish.
      sleep(2);

      $result = $service->incrStatus($this->document_model_id);
      $expected = "Update in_progress, 0 success, corpus: 2, queue: 0";
      $this->assertEquals($result, $expected, "Should get '$expected' after first training completed.");

      # Let second incremental training finish.
      sleep(2);

      $result = $service->incrStatus($this->document_model_id);
      $expected = "Update complete, 0 success, corpus: 2, queue: 0";
      $this->assertEquals($result, $expected, "Should get '$expected' after second training completed.");
   }

   public function test_failure_status()
   {
      $service = new PortageLiveLib();

      $result = $service->incrAddSentence($this->context, $this->document_model_id,
            $this->source, $this->target);
      $this->assertEquals(True, $result, "Unable to add sentence pair to the queue.");

      $result = $service->incrStatus($this->document_model_id);
      $expected = "Update in_progress, N/A, corpus: 1, queue: 0";
      $this->assertEquals($result, $expected, "Should get '$expected' for training in progress.");

      # Let incremental training finish.
      sleep(3);

      # Fake a failure.
      file_put_contents("$this->document_model_dir/incr-update.status", "1");

      $result = $service->incrStatus($this->document_model_id);
      $expected = "Update complete, 1 failure, corpus: 1, queue: 0";
      $this->assertEquals($result, $expected, "Should get '$expected' after training is completed.");
   }
}
?>
