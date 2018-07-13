<?php
require 'PortageLiveLib.php';

# Let's change the web workdir to a local workdir.  It needs to be an absolute
# path.
$base_web_dir = getcwd();
$base_portage_dir = getcwd() . '/tests/';

//base64_encode("fr\ten\na\tb"), //ContentsBase64

class PortageLiveLib_FixedTerms_Test extends PHPUnit_Framework_TestCase
{
   var $fixedTerms_dir = 'tests/models/fixedTerms/plugins/fixedTerms/';
   var $tm             = 'tests/models/fixedTerms/plugins/fixedTerms/tm';
   var $fixedTerms     = 'tests/models/fixedTerms/plugins/fixedTerms/fixedTerms';
   var $context        = 'fixedTerms';

   protected function setUp()
   {
      if (!file_exists('tests/models/fixedTerms/plugins')) {
         mkdir('tests/models/fixedTerms/plugins', 0770, TRUE);
      }
      if (!file_exists($this->fixedTerms_dir)) {
         mkdir($this->fixedTerms_dir);
      }
      // By default, we want the fixedTerms to be enabled.
      chmod($this->fixedTerms_dir, 0777);

      if (file_exists($this->tm)) {
         unlink($this->tm);
      }
      touch($this->tm);

      // By default, we want the fixedTerms to be enabled.
      chmod($this->tm, 0666);
   }


   /////////////////////////////////////////////////////////
   // Update Fixed Terms

   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage This context (fixedTerms) has its fixedTerms disabled.  Ask your administrator to enabled fixedTerms.
    */
   public function test_updateFixedTerms_disabled()
   {
      chmod($this->fixedTerms_dir, 0555);
      chmod($this->tm, 0444);

      $service = new PortageLiveLib();

      $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         $this->context,  // context
         1,  // sourceColumnIndex
         'fr',  // sourceLanguage
         'en'  // targetLanguage
      );
   }


   public function test_updateFixedTerms_enabled()
   {
      chmod($this->fixedTerms_dir, 0777);
      chmod($this->tm, 0666);

      $service = new PortageLiveLib();

      $response = $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         $this->context,  // context
         1,  // sourceColumnIndex
         'fr',  // sourceLanguage
         'en'  // targetLanguage
      );

      $this->assertTrue($response, "Unable to add fixedTerms.");
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage There is no file content (Filename).
    */
   public function test_updateFixedTerms_noContent()
   {
      $service = new PortageLiveLib();

      $service->updateFixedTerms(
         '', //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         $this->context,  // context
         1,  // sourceColumnIndex
         'fr',  // sourceLanguage
         'en'  // targetLanguage
      );
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage Unsupported encoding (bad encoding): use either UTF-8 or CP-1252.
    */
   public function test_updateFixedTerms_badEncoding()
   {
      $service = new PortageLiveLib();

      $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'Bad Encoding',  // encoding
         $this->context,  // context
         1,  // sourceColumnIndex
         'fr',  // sourceLanguage
         'en'  // targetLanguage
      );
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage Context "badContext" does not exist.
    */
   public function test_updateFixedTerms_badContext()
   {
      $service = new PortageLiveLib();

      $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         'badContext',  // context
         1,  // sourceColumnIndex
         'fr',  // sourceLanguage
         'en'  // targetLanguage
      );
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage sourceLanguage cannot be empty. It must be one of {en, fr, es, da}
    */
   public function test_updateFixedTerms_noSourceLanguage()
   {
      $service = new PortageLiveLib();

      $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         $this->context,  // context
         1,  // sourceColumnIndex
         '',  // sourceLanguage
         'en'  // targetLanguage
      );
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage sourceLanguage (bad) must be one of {en, fr, es, da}
    */
   public function test_updateFixedTerms_badSourceLanguage()
   {
      $service = new PortageLiveLib();

      $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         $this->context,  // context
         1,  // sourceColumnIndex
         'bad',  // sourceLanguage
         'en'  // targetLanguage
      );
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage targetLanguage cannot be empty. It must be one of {en, fr, es, da}
    */
   public function test_updateFixedTerms_noTargetLanguage()
   {
      $service = new PortageLiveLib();

      $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         $this->context,  // context
         1,  // sourceColumnIndex
         'fr',  // sourceLanguage
         ''  // targetLanguage
      );
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage targetLanguage (bad) must be one of {en, fr, es, da}
    */
   public function test_updateFixedTerms_badTargetLanguage()
   {
      $service = new PortageLiveLib();

      $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         $this->context,  // context
         1,  // sourceColumnIndex
         'fr',  // sourceLanguage
         'bad'  // targetLanguage
      );
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage non-zero return code from flock
    */
   public function test_updateFixedTerms_badColumn()
   {
      $service = new PortageLiveLib();

      $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         $this->context,  // context
         0,  // sourceColumnIndex
         'fr',  // sourceLanguage
         'en'  // targetLanguage
      );
   }


   // TODO: public function test_updateFixedTerms_badCP1252()


   /////////////////////////////////////////////////////////
   // Get Fixed Terms

   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage Context "badContext" does not exist.
    */
   public function test_getFixedTerms_badContext()
   {
      $service = new PortageLiveLib();

      $response = $service->getFixedTerms(
         'badContext'  // context
      );

   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage fixedTerms doesn't have fixed terms.
    */
   public function test_getFixedTerms_noFixedTerms()
   {
      if (is_file($this->fixedTerms)) {
         unlink($this->fixedTerms);
      }

      $service = new PortageLiveLib();

      $response = $service->getFixedTerms(
         $this->context// context
      );

   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage fixedTerms has incorrectly installed fixed terms.
    */
   public function test_getFixedTerms_noTm()
   {
      if (!is_file($this->fixedTerms)) {
         touch($this->fixedTerms);
      }

      if (is_file($this->tm)) {
         unlink($this->tm);
      }

      $service = new PortageLiveLib();

      $response = $service->getFixedTerms(
         $this->context  // context
      );

   }


   /////////////////////////////////////////////////////////
   // Remove Fixed Terms

   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage This context (fixedTerms) has its fixedTerms disabled.  Ask your administrator to enabled fixedTerms.
    */
   public function test_removeFixedTerms_disabled()
   {
      chmod($this->fixedTerms_dir, 0555);
      chmod($this->tm, 0444);

      $service = new PortageLiveLib();

      $service->removeFixedTerms(
         $this->context  // context
      );
   }


   public function test_removeFixedTerms_enabled()
   {
      chmod($this->fixedTerms_dir, 0777);
      chmod($this->tm, 0666);

      $service = new PortageLiveLib();

      $response = $service->removeFixedTerms(
         $this->context  // context
      );

      $this->assertTrue($response, "Unable to remove fixedTerms.");
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage Context "badContext" does not exist.
    */
   public function test_removeFixedTerms_badContext()
   {
      $service = new PortageLiveLib();

      $response = $service->getFixedTerms(
         'badContext'  // context
      );

   }



   /////////////////////////////////////////////////////////
   // Combo Fixed Terms

   // The successful case when after we have tested all possible failures.
   public function test_updateFixedTerms_getFixedTerms()
   {
      if (is_file($this->fixedTerms)) {
         unlink($this->fixedTerms);
      }

      if (!is_file($this->tm)) {
         touch($this->tm);
      }

      $service = new PortageLiveLib();

      $response = $service->updateFixedTerms(
	 "fr\ten\na\tb", //ContentsBase64
         'Filename',  //Filename
         'UTF-8',  // encoding
         $this->context,  // context
         1,  // sourceColumnIndex
         'fr',  // sourceLanguage
         'en'  // targetLanguage
      );

      $this->assertTrue($response, "Unable to add fixedTerms.");

      $content = file_get_contents($this->fixedTerms);
      $this->assertEquals($content, "fr\ten\na\tb", "FixedTerms aren't what we've expected.");

      $response = $service->getFixedTerms(
         $this->context  // context
      );

      $this->assertEquals($response, "fr\ten\na\tb", "FixedTerms aren't what we've expected.");

      $response = $service->removeFixedTerms(
         $this->context  // context
      );

      $this->assertTrue($response, "Unable to remove fixedTerms.");

      $this->assertTrue(!is_file($this->fixedTerms), "Unable to remove fixedTerms.");
      $this->assertTrue(!is_file($this->tm), "Unable to remove fixedTerms.");
   }


}
?>
