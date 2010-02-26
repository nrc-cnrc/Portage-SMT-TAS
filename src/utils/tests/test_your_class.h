/**
 * @author INSERT YOUR NAME HERE
 * @file test_your_class.h  Test suite for YourClass
 *
 * This file, test_prog.h, written by Eric Joanis, is intended as a template
 * for your own unit test suites.  Copy it to tests/test_your_class.h, remove
 * all the irrelevant content (such as this paragraph and all the sample test
 * cases below), substitute all instances of "your class" with appropriate
 * text describing yours, and you're ready to go: "make test" will now run
 * your test suite among the rest.
 * Full documentation of CxxTest is here:
 *      http://cxxtest.sourceforge.net/guide.html
 * and the list of assertions is here:
 *      http://cxxtest.sourceforge.net/guide.html#TOC8
 * And several examples exist in our own tests/test*.h files.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
//#include "your_class.h"

using namespace Portage;

namespace Portage {

class TestYourClass : public CxxTest::TestSuite 
{
public:
   void setUp() {
      // Put here any code that should be run once before each test*() method.
      // This is intended to let you factor out code that you'd otherwise have
      // to repeat in each test case.
      // If you don't need any such code, leave this method out.
   }
   void tearDown() {
      // this is for code that needs to be run after each test*() method.
      // Again, this is intended for factoring out duplicated code, and can be
      // left out if you don't need it.
   }
   void testSomeFunction() {
      // A test case is any void-void method (i.e., takes no arguments, and
      // has void return type), and whose name starts with test.
      // In the test case, setup some stuff here, then assert what should be,
      // using any of the TS_ASSERT* assertions provided by CxxText.
      // Full list here: http://cxxtest.sourceforge.net/guide.html#TOC8
      TS_ASSERT_EQUALS(4u, Uint(2 + 2));
   }
   void testSomeOtherFunction() {
      // setup different stuff and assert what should be true
      TS_ASSERT(1 != 2);
   }
   void testSomethingElse() {
      // more different setup
      TS_ASSERT_DELTA(sqrt(4.0f), 2.0f, 0.00001);
   }
   void testConstructor() {
      // Maybe you want to know that construction and destruction don't
      // thow exceptions
      char* w(NULL);
      TS_ASSERT_THROWS_NOTHING(w = new char[10]);
      TS_ASSERT_THROWS_NOTHING(delete[] w);
   }
}; // TestYourClass

} // Portage
