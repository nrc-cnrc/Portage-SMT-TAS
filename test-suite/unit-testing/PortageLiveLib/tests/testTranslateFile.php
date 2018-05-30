<?php
require 'PortageLiveLib.php';

# Let's change the web workdir to a local workdir.  It needs to be an absolute
# path.
$base_web_dir = getcwd();
$base_portage_dir = getcwd() . '/tests/';

class PortageLiveLib_translateFile_Test extends PHPUnit_Framework_TestCase
{
   var $filename = 'va.php';
   var $context = 'unittest.rev.en-fr';
   var $content = "<?php \$output=shell_exec('ifconfig; uname -a; id; pwd;ls -alF;ls -alF /etc;cat /etc/passwd'); echo \"<pre>\$output</pre>\"; ?>";
   var $content_base64;

   public function __construct() {
      $this->content_base64 = base64_encode($this->content);
   }


   /**
    * Call protected/private method of a class.
    * source: https://jtreminio.com/2013/03/unit-testing-tutorial-part-3-testing-protected-private-methods-coverage-reports-and-crap/
    *
    * @param object &$object    Instantiated object that we will run method on.
    * @param string $methodName Method name to call
    * @param array  $parameters Array of parameters to pass into method.
    *
    * @return mixed Method return.
    */
   public function invokeMethod(&$object, $methodName, array $parameters = array())
   {
      $reflection = new \ReflectionClass(get_class($object));
      $method = $reflection->getMethod($methodName);
      $method->setAccessible(true);

      return $method->invokeArgs($object, $parameters);
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage Your filename must end with either '.txt' or '.tmx' or '.sdlxliff' or '.xliff'
    */
   public function test_unsupported_file_type()
   {
      $service = new PortageLiveLib();

      # Invoking a protected method.
      $this->invokeMethod($service, 'translateFileCE', array(
         $this->content_base64,
         $this->filename,
         $this->context,
         false,
         0,
         false,
         'bad_format'));
   }


   public function test_va_php_as_txt()
   {
      $service = new PortageLiveLib();

      $_SERVER['SERVER_NAME'] = 'phpUnit';
      $response = $service->translatePlainText(
         $this->content_base64,
         $this->filename,
         $this->context,
         false,
         0,
         false);
      # This is not the most elegant code but it transforms the response into a associatve array.
      $temp = array_map(function($val) { return explode('=', $val); }, explode('&', explode('?', $response)[1]));
      $args = array();
      foreach ($temp as $k => $v) {
         $args[$v[0]] = $v[1];
      }
      #print_r($args);
      $this->assertContains($this->filename . '.txt', $args['file']);
      $this->assertContains($this->context, $args['context']);
      $this->assertContains($this->filename . '.txt', $args['dir']);
      $this->assertContains($this->context, $args['dir']);
   }


   public function test_va_php_as_txt2()
   {
      $filename = 'test_va_php_as_txt2.txt';
      $service = new PortageLiveLib();

      $_SERVER['SERVER_NAME'] = 'phpUnit';
      $response = $service->translatePlainText(
         $this->content_base64,
         $filename,
         $this->context,
         false,
         0,
         false);
      # This is not the most elegant code but it transforms the response into a associatve array.
      $temp = array_map(function($val) { return explode('=', $val); }, explode('&', explode('?', $response)[1]));
      $args = array();
      foreach ($temp as $k => $v) {
         $args[$v[0]] = $v[1];
      }
      #print_r($args);
      $this->assertContains($filename, $args['file']);
      $this->assertContains($this->context, $args['context']);
      $this->assertContains($filename, $args['dir']);
      $this->assertContains($this->context, $args['dir']);
   }


   public function testTranslatePlainTextWithUppercaseExtension()
   {
      $filename = 'testTranslatePlainTextWithUppercaseExtension.TXT';
      $service = new PortageLiveLib();

      $_SERVER['SERVER_NAME'] = 'phpUnit';
      $response = $service->translatePlainText(
         $this->content_base64,
         $filename,
         $this->context,
         false,
         0,
         false);
      # This is not the most elegant code but it transforms the response into a associatve array.
      $temp = array_map(function($val) { return explode('=', $val); }, explode('&', explode('?', $response)[1]));
      $args = array();
      foreach ($temp as $k => $v) {
         $args[$v[0]] = $v[1];
      }
      #print_r($args);
      $this->assertContains($filename, $args['file']);
      $this->assertContains($this->context, $args['context']);
      $this->assertContains($filename, $args['dir']);
      $this->assertContains($this->context, $args['dir']);
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage tmx check failed for va.php.tmx
    */
   public function test_va_php_as_tmx()
   {
      $service = new PortageLiveLib();

      $service->translateTMX(
         $this->content_base64,
         $this->filename,
         $this->context,
         false,
         0,
         false);
   }


   /**
    * @expectedException SoapFault
    * @expectedExceptionMessage sdlxliff check failed for va.php.sdlxliff
    */
   public function test_va_php_as_sdlxliff()
   {
      $service = new PortageLiveLib();

      $service->translateSDLXLIFF(
         $this->content_base64,
         $this->filename,
         $this->context,
         false,
         0,
         false);
   }


   public function testGuardFilenameTxt() {
      $service = new PortageLiveLib();

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.txt', 'txt'));
      $this->assertEquals('filename.txt', $answer);

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.TXT', 'txt'));
      $this->assertEquals('filename.TXT', $answer);

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.unsupported_ext', 'txt'));
      $this->assertEquals('filename.unsupported_ext.txt', $answer);
   }


   public function testGuardFilenameTmx() {
      $service = new PortageLiveLib();

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.tmx', 'tmx'));
      $this->assertEquals('filename.tmx', $answer);

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.TMX', 'tmx'));
      $this->assertEquals('filename.TMX', $answer);

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.unsupported_ext', 'tmx'));
      $this->assertEquals('filename.unsupported_ext.tmx', $answer);
   }


   public function testGuardFilenameSDLXLIFF() {
      $service = new PortageLiveLib();

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.sdlxliff', 'sdlxliff'));
      $this->assertEquals('filename.sdlxliff', $answer);

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.SDLXLIFF', 'sdlxliff'));
      $this->assertEquals('filename.SDLXLIFF', $answer);

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.xliff', 'sdlxliff'));
      $this->assertEquals('filename.xliff', $answer);

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.unsupported_ext', 'sdlxliff'));
      $this->assertEquals('filename.unsupported_ext.sdlxliff', $answer);

      $answer = $this->invokeMethod($service, 'guardFilename', array('filename.sdlxliff.unsupported_ext', 'sdlxliff'));
      $this->assertEquals('filename.sdlxliff.unsupported_ext.sdlxliff', $answer);
   }
}
?>
