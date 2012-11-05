/**
 * @author Darlene Stewart
 * @file test_utf8_utils.h test suite for UTF8Utils
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "utf8_utils.h"
#include "tmp_val.h"
#include "errors.h"

using namespace Portage;
using namespace Portage::Error_ns;

namespace Portage {

class TestUTF8Utils : public CxxTest::TestSuite
{
private:
   // Temporarily mute the the function error used by arg_reader for all tests.
   tmp_val<Error_ns::ErrorCallback> tmp;

public:
   // default constructor overrides error's callback to be able to catch it.
   TestUTF8Utils() : tmp(Current::errorCallback, countErrorCallBack) {}

   void setUp() {
      ErrorCounts::Total = 0;
   }

   void testToUpper() {
      UTF8Utils utf8;
      string out;
      #ifdef NOICU
      TS_ASSERT_EQUALS(ErrorCounts::Total, 1);
      #else
      TS_ASSERT_EQUALS(ErrorCounts::Total, 0);
      TS_ASSERT_EQUALS(utf8.toUpper("a", out), string("A"));
      TS_ASSERT_EQUALS(utf8.toUpper("\u00e0", out), string("\u00c0")); // a/A accent grave
      // Greek small letter upsilon with dialytika and tonos ->
      // should be Greek capital upsilon with dialytika (Unicode point 03AB), but could be:
      // Greek capital letter upsilon + combining diaeresis + combining acute accent (in ICU 3.4), or perhaps
      // Greek capital letter upsilon + combining dialytika tonos
      utf8.toUpper("\u03b0", out);
      switch (out.size()) {
         case 2: TS_ASSERT_EQUALS(out, string("\u03ab")); break;
         case 6: TS_ASSERT_EQUALS(out, string("\u03a5\u0308\u0301")); break;
         case 4: TS_ASSERT_EQUALS(out, string("\u03a5\u0344")); break;
         default: TS_FAIL("\u03b0 incorrectly uppercased as: " + out);
      }
      #endif
   }

   void testToLower() {
      UTF8Utils utf8;
      string out;
      #ifdef NOICU
      TS_ASSERT_EQUALS(ErrorCounts::Total, 1);
      #else
      TS_ASSERT_EQUALS(ErrorCounts::Total, 0);
      TS_ASSERT_EQUALS(utf8.toUpper("a", out), string("A"));
      TS_ASSERT_EQUALS(utf8.toUpper("\u00e0", out), string("\u00c0")); // a/A accent grave
      #endif
   }

   void testCapitalize() {
      UTF8Utils utf8;
      string out;
      #ifdef NOICU
      TS_ASSERT_EQUALS(ErrorCounts::Total, 1);
      #else
      TS_ASSERT_EQUALS(ErrorCounts::Total, 0);
      TS_ASSERT_EQUALS(utf8.capitalize("a", out), string("A"));
      TS_ASSERT_EQUALS(utf8.capitalize("abc", out), string("Abc"));
      TS_ASSERT_EQUALS(utf8.capitalize("\u00e0", out), string("\u00c0")); // a/A accent grave
      TS_ASSERT_EQUALS(utf8.capitalize("\u00e0bc", out), string("\u00c0bc")); // a/A accent grave as first character
      // Capitalize Greek small letter upsilon with dialytika and tonos
      string capped;
      switch(utf8.toUpper("\u03b0", out).size()) {
         case 2: capped = "\u03ab"; break;
         case 6: capped = "\u03a5\u0308\u0301"; break;
         case 4: capped = "\u03a5\u0344"; break;
         default: TS_FAIL("\u03b0 incorrectly uppercased as: " + out);
      }
      TS_ASSERT_EQUALS(utf8.capitalize("\u03b0", out), string(capped));
      TS_ASSERT_EQUALS(utf8.capitalize("\u03b0bc", out), string(capped + "bc"));
      #endif
   }

   void TestDecapitalize() {
      UTF8Utils utf8;
      string out;
      #ifdef NOICU
      TS_ASSERT_EQUALS(ErrorCounts::Total, 1);
      #else
      TS_ASSERT_EQUALS(ErrorCounts::Total, 0);
      TS_ASSERT_EQUALS(utf8.decapitalize("A", out), string("a"));
      TS_ASSERT_EQUALS(utf8.decapitalize("ABC", out), string("aBC"));
      TS_ASSERT_EQUALS(utf8.decapitalize("\u00c0", out), string("\u00e0")); // A/a accent grave
      TS_ASSERT_EQUALS(utf8.decapitalize("\u00c0BC", out), string("\u00e0BC")); // A/a accent grave as first character
      #endif
   }

}; // TestUTF8Utils

} // Portage
