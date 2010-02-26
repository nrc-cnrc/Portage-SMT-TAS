/**
 * @author Eric Joanis
 * @file test_casemap_strings.h test suite for CaseMapStrings
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "casemap_strings.h"
#include "tmp_val.h"
#include "errors.h"

using namespace Portage;
using namespace Portage::Error_ns;

namespace Portage {

class TestCaseMapStrings : public CxxTest::TestSuite 
{
private:
   // Temporarily mute the the function error used by arg_reader for all tests.
   tmp_val<Error_ns::ErrorCallback> tmp;
   static unsigned int error_message_count;
   static string error_message;

public:
   // default constructor overrides error's callback to be able to catch it.
   TestCaseMapStrings() : tmp(dummy::errorCallback, countErrorCallBack) {}

   /**
    * Stub function replacement for function error that prevents printing to
    * stderr but that keeps track of the last error message string.
    */
   static void countErrorCallBack(ErrorType et, const string& msg) {
      ++error_message_count;
      error_message = msg;
   }

   void setUp() {
      error_message = "";
      error_message_count = 0;
   }

   void testLatin1() {
      CaseMapStrings cms("en_CA.iso-8859-1");
      TS_ASSERT_EQUALS(error_message_count, 0);
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xc0")); // a/A accent grave
      TS_ASSERT_EQUALS(cms.toUpper("\xbd"), string("\xbd")); // 1/2 in latin1 (oe in latin9)
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }
   void testDefault() {
      CaseMapStrings cms("en_CA"); // defaults to latin1
      TS_ASSERT_EQUALS(error_message_count, 0);
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xc0")); // a/A accent grave
      TS_ASSERT_EQUALS(cms.toUpper("\xbd"), string("\xbd")); // 1/2 in latin1 (oe in latin9)
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }
   void testLatin9() {
      CaseMapStrings cms("en_GB.iso885915");
      TS_ASSERT_EQUALS(error_message_count, 0);
      if ( error_message_count == 0 ) {
         // if the en_GB.iso885915 locale happens not to be installed, don't
         // perform these tests.
         TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xc0")); // a/A accent grave
         TS_ASSERT_EQUALS(cms.toUpper("\xbd"), string("\xbc")); // oe/OE
         TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
      }
   }
   void testCP1252() {
      CaseMapStrings cms("en_CA.CP1252");
      //TS_ASSERT_EQUALS(error_message_count, 0);
      // CP1252 typically not supported on Linux systems, so don't depend on it.
      if ( error_message_count == 0 ) {
         TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xc0")); // a/A accent grave
         TS_ASSERT_EQUALS(cms.toUpper("\x9c"), string("\x8c")); // oe/OE
         TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
      }
   }
   void testCLocale() {
      CaseMapStrings cms("C");
      TS_ASSERT_EQUALS(error_message_count, 0);
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xe0")); // a accent grave not mapped in C/POSIX
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }
   void testUtf8() {
      CaseMapStrings cms("en_CA.utf-8");
      #ifdef NOICU
      TS_ASSERT_EQUALS(error_message_count, 1);
      #else
      TS_ASSERT_EQUALS(error_message_count, 0);
      TS_ASSERT_EQUALS(cms.toUpper("\u00e0"), string("\u00c0")); // a/A accent grave
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
      #endif
   }
   void testBadLocale() {
      TS_ASSERT_EQUALS(error_message, "");
      TS_ASSERT_EQUALS(error_message_count, 0);
      CaseMapStrings cms("foobar.unknown");
      TS_ASSERT(!error_message.empty());
      TS_ASSERT_EQUALS(error_message_count, 1);
   }
}; // TestCaseMapStrings

unsigned int TestCaseMapStrings::error_message_count = 0;
string TestCaseMapStrings::error_message;

} // Portage
