/**
 * @author Samuel Larkin
 * @file test_arg_reader.h  Test suite for ArgReader.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "arg_reader.h"
#include "tmp_val.h"
#include <boost/optional.hpp>


using namespace Portage;
using namespace boost;
using namespace Portage::Error_ns;

namespace Portage {

class TestArgReader : public CxxTest::TestSuite 
{
private:
   // Temporarily mute the the function error used by arg_reader for all tests.
   tmp_val<Error_ns::ErrorCallback> tmp;

public:
   /**
    * Default constructor.
    * Important to note we are replacing error's callback since we want to mute
    * it and track some info.
    */
   TestArgReader()
   : tmp(Current::errorCallback, countErrorCallBack)
   {}

   /**
    * Checks if the last seen error message is what we are expecting.
    * @param expected_error_message  last expected error message.
    */
   void checkErrorMessage(const char* const expected_error_message) {
      TS_ASSERT(expected_error_message != NULL);
      if (expected_error_message)
         TS_ASSERT_EQUALS(ErrorCounts::last_msg, expected_error_message);
      ErrorCounts::Total = 0;
   }

   /**
    * Generic function to test bool command line arguments named x/no-x.
    * @param argc  how many arguments on the command line.
    * @param argv  command line argumenrs.
    * @param val  expected value of x.
    * @param expected_error_count  expected number of error messages displayed during this test.
    * @param expected_error_message  last expected error message displayed during this test.
    */
   void duplicatedArgumentX(Uint argc, const char* const argv[],
      optional<bool> val, unsigned int expected_error_count,
      const char* const expected_error_message = NULL)
   {
      const char* const switches[] = {"x", "no-x"};
      ArgReader arg(2, switches, 0, 0, "", "", false, "", "");
      arg.read(argc, argv);
      optional<bool> test_value;
      arg.testAndSetOrReset("x", "no-x", test_value);
      TS_ASSERT_EQUALS(test_value, val);
      TS_ASSERT_EQUALS(ErrorCounts::Total, expected_error_count);
      if (expected_error_count > 0) checkErrorMessage(expected_error_message);
   }

   void setUp() {
      ErrorCounts::Total = 0;
   }

   /////////////////////////////////////
   // Tests definition begins.

   // Simple test with no arguments.
   void testDuplicatedArgumentNoArgs() {
      // It should be neither true nor false.
      duplicatedArgumentX(0, NULL, optional<bool>(), 0);
   }
   // Simple test where only -x is provided.
   void testDuplicatedArgumentSet() {
      const char* const argv[] = {"-x"};
      duplicatedArgumentX(1, argv, true, 0);
   }
   // Simple test where only -no-x is provided.
   void testDuplicatedArgumentUnset() {
      const char* const argv[] = {"-no-x"};
      duplicatedArgumentX(1, argv, false, 0);
   }
   // Simple test where we specify x twice.
   void testDuplicatedArgumentSetSet() {
      const char* const argv[] = {"-x", "-x"};
      duplicatedArgumentX(2, argv, true, 0);
   }
   // Test where both -no-x and -x are provided.
   void testDuplicatedArgumentUnsetSet() {
      //cerr << "testDuplicatedArgumentUnsetSet" << endl;
      const char* const argv[] = {"-no-x", "-x"};
      duplicatedArgumentX(2, argv, true, 1, 
      "-x specified after -no-x; last instance prevails: -x");
   }
   // Test where both -x and -no-x are provided.
   void testDuplicatedArgumentSetUnset() {
      //cerr << "testDuplicatedArgumentSetUnset" << endl;
      const char* const argv[] = {"-x", "-no-x"};
      duplicatedArgumentX(2, argv, false, 1,
      "-no-x specified after -x; last instance prevails: -no-x");
   }
   // Test where -no-x is provided and where -x is provided twice.
   void testDuplicatedArgumentUnsetSetSet() {
      //cerr << "testDuplicatedArgumentUnsetSetSet" << endl;
      const char* const argv[] = {"-no-x", "-x", "-x"};
      duplicatedArgumentX(3, argv, true, 1,
      "-x specified after -no-x; last instance prevails: -x");
   }

   // Make sure the last value in is the retrieved value.
   void testDuplicatedArgumentValueValue() {
      //cerr << "A test" << endl;
      const char* const switches[] = {"a:"};
      ArgReader arg(1, switches, 0, 0, "", "", false, "", "");
      const char* const argv[] = {"-a", "1", "-a", "2"};
      arg.read(4, argv);
      Uint tmp;
      arg.testAndSet("a", tmp);
      TS_ASSERT_EQUALS(tmp, 2);
      TS_ASSERT_EQUALS(ErrorCounts::Total, 1);
      checkErrorMessage("-a specified multiple times; last value prevails: -a 2");
   }

   // Make sure non-duplicated values do not generate a warning.
   void testNonDuplicatedArgumentValueValue() {
      //cerr << "A test" << endl;
      const char* const switches[] = {"a:"};
      ArgReader arg(1, switches, 0, 0, "", "", false, "", "");
      const char* const argv[] = {"-a", "1"};
      arg.read(2, argv);
      Uint tmp;
      arg.testAndSet("a", tmp);
      TS_ASSERT_EQUALS(tmp, 1);
      TS_ASSERT_EQUALS(ErrorCounts::Total, 0);
   }
}; // TestArgReader

} // Portage
