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

public:
   // default constructor overrides error's callback to be able to catch it.
   TestCaseMapStrings() : tmp(Current::errorCallback, countErrorCallBack) {}

   void setUp() {
      ErrorCounts::last_msg.clear();
      ErrorCounts::Total = 0;
   }

   void testLatin1() {
      CaseMapStrings cms("en_CA.iso-8859-1");
      if (ErrorCounts::Total > 0) {
         TS_WARN(ErrorCounts::last_msg + " Skipping remainder of test. You may ignore" +
                 " this warning if you do not plan to use this locale.");
         return;
      }
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xc0")); // a/A accent grave
      TS_ASSERT_EQUALS(cms.toUpper("\xbd"), string("\xbd")); // 1/2 in latin1 (oe in latin9)
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }

   void testDefault() {
      CaseMapStrings cms("en_CA"); // defaults to latin1
      if (ErrorCounts::Total > 0) {
         TS_WARN(ErrorCounts::last_msg + " Skipping remainder of test. You may ignore" +
                 " this warning if you do not plan to use this locale.");
         return;
      }
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xc0")); // a/A accent grave
      TS_ASSERT_EQUALS(cms.toUpper("\xbd"), string("\xbd")); // 1/2 in latin1 (oe in latin9)
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }

   void testLatin9() {
      CaseMapStrings cms("en_GB.iso885915");
      if (ErrorCounts::Total > 0) {
         TS_WARN(ErrorCounts::last_msg + " Skipping remainder of test. You may ignore" +
                 " this warning if you do not plan to use this locale.");
         return;
      }
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xc0")); // a/A accent grave
      TS_ASSERT_EQUALS(cms.toUpper("\xbd"), string("\xbc")); // oe/OE
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }

   void testCP1252() {
      CaseMapStrings cms("en_CA.CP1252");
      if (ErrorCounts::Total > 0) {
         TS_WARN(ErrorCounts::last_msg + " Skipping remainder of test. You may ignore" +
                 " this warning if you do not plan to use this locale.");
         return;
      }
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xc0")); // a/A accent grave
      TS_ASSERT_EQUALS(cms.toUpper("\x9c"), string("\x8c")); // oe/OE
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }

   void testCLocale() {
      CaseMapStrings cms("C");
      TS_ASSERT_EQUALS(ErrorCounts::Total, 0);
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xe0")); // a accent grave not mapped in C/POSIX
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }

   void testUtf8() {
      CaseMapStrings cms("en_CA.utf-8");
      #ifdef NOICU
      TS_ASSERT_EQUALS(ErrorCounts::Total, 1);
      #else
      if (ErrorCounts::Total > 0) {
         TS_WARN(ErrorCounts::last_msg + " Skipping remainder of test. You may ignore" +
                 " this warning if you do not plan to use this locale.");
         return;
      }
      TS_ASSERT_EQUALS(cms.toUpper("\u00e0"), string("\u00c0")); // a/A accent grave
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
      #endif
   }

   void testEmptyLocale() {
      CaseMapStrings cms("");
      TS_ASSERT_EQUALS(ErrorCounts::Total, 0);
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xe0")); // latin-1 a accent grave not mapped in C/POSIX
      TS_ASSERT_EQUALS(cms.toUpper("\u00e0"), string("\u00e0")); // utf-8 a/A accent grave not mapped in C/POSIX
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }

   void testNullLocale() {
      CaseMapStrings cms(NULL);
      TS_ASSERT_EQUALS(ErrorCounts::Total, 0);
      TS_ASSERT_EQUALS(cms.toUpper("\xe0"), string("\xe0")); // latin-1 a accent grave not mapped in C/POSIX
      TS_ASSERT_EQUALS(cms.toUpper("\u00e0"), string("\u00e0")); // utf-8 a/A accent grave not mapped in C/POSIX
      TS_ASSERT_EQUALS(cms.toUpper("a"), string("A"));
   }

   void testBadLocale() {
      TS_ASSERT_EQUALS(ErrorCounts::last_msg, "");
      TS_ASSERT_EQUALS(ErrorCounts::Total, 0);
      CaseMapStrings cms("foobar.unknown");
      TS_ASSERT(!ErrorCounts::last_msg.empty());
      TS_ASSERT_EQUALS(ErrorCounts::Total, 1);
   }
}; // TestCaseMapStrings

} // Portage
