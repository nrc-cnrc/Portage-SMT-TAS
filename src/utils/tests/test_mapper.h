/**
 * @author Samuel Larkin
 * @file test_mapper.h  Test suite for Mappers.
 *
 * This file, test_your_class.h, written by Eric Joanis, is intended as a
 * template for your own unit test suites.  Copy it to tests/test_<YOUR_CLASS>.h,
 * remove all the irrelevant content (such as this paragraph and all the sample
 * test cases below), substitute all instances of "your class" with appropriate
 * text describing yours, and you're ready to go: "make test" will now run
 * your test suite among the rest.
 * Full documentation of CxxTest is here:
 *      http://cxxtest.sourceforge.net/guide.html
 * and the list of assertions is here:
 *      http://cxxtest.sourceforge.net/guide.html#TOC8
 * And lots of examples exist in our own tests/test*.h files.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "mapper.h"
#include <fstream>

using namespace Portage;

namespace Portage {

class TestMapper : public CxxTest::TestSuite 
{
   const string unknown;
   const string wordClassFilename;

public:
   TestMapper() 
      : unknown("NOT FOUND")
      , wordClassFilename("tests/test_mapper")
   {
      ostringstream classMap;
      classMap << "DDD" << '\t' << 1 << endl;
      classMap << "A" << '\t' << 34 << endl;
      classMap << "E" << '\t' << 420 << endl;
      classMap << "C" << '\t' << 0 << endl;
      classMap << "BB" << '\t' << 1024 << endl;

      oMagicStream text(wordClassFilename + ".txt");
      text << classMap.str();

      oMagicStream mmmap(wordClassFilename + ".MMmap");
      istringstream is(classMap.str());
      mkMemoryMappedMap(is, mmmap);
   }

   void testWordClassMapper() {
      WordClassesMapper mapper(wordClassFilename + ".txt", unknown);
      string test;

      test = "DDD";
      TS_ASSERT_EQUALS(mapper(test), string("1"));

      test = "A";
      TS_ASSERT_EQUALS(mapper(test), string("34"));

      test = "E";
      TS_ASSERT_EQUALS(mapper(test), string("420"));

      test = "C";
      TS_ASSERT_EQUALS(mapper(test), string("0"));

      test = "BB";
      TS_ASSERT_EQUALS(mapper(test), string("1024"));

      test = "not part of the map";
      TS_ASSERT_EQUALS(mapper(test), unknown);

      test = "1";
      TS_ASSERT_EQUALS(mapper(test), unknown);
   }

   void testWordClassMapper_MemoryMapped_Map() {
      WordClassesMapper_MemoryMappedMap mapper(wordClassFilename + ".MMmap", unknown);
      string test;

      test = "DDD";
      TS_ASSERT_EQUALS(mapper(test), string("1"));

      test = "A";
      TS_ASSERT_EQUALS(mapper(test), string("34"));

      test = "E";
      TS_ASSERT_EQUALS(mapper(test), string("420"));

      test = "C";
      TS_ASSERT_EQUALS(mapper(test), string("0"));

      test = "BB";
      TS_ASSERT_EQUALS(mapper(test), string("1024"));

      test = "not part of the map";
      TS_ASSERT_EQUALS(mapper(test), unknown);

      test = "1";
      TS_ASSERT_EQUALS(mapper(test), unknown);
   }

}; // TestMapper

} // Portage
