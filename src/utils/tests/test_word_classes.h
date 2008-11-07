/**
 * @author Eric Joanis
 * @file test_word_classes.h Test suite for WordClasses.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include <stdlib.h>
#include <file_utils.h>
#include "word_classes.h"

using namespace Portage;

namespace Portage {

class TestWordClasses : public CxxTest::TestSuite 
{
public:
   void testCreate() {
      WordClasses* wc = new WordClasses();
      TS_ASSERT(wc != NULL);
      TS_ASSERT_THROWS_NOTHING(delete wc);
   }
   void testAdd() {
      WordClasses wc;
      wc.add("asdf", 3);
      TS_ASSERT_EQUALS(wc.size(), 1u);
      wc.add("qwer", 10);
      TS_ASSERT_EQUALS(wc.size(), 2u);
      wc.add("answer", 42);
      TS_ASSERT_EQUALS(wc.size(), 3u);
   }
   void testAddAgain() {
      WordClasses wc;
      wc.add("asdf", 3);
      wc.add("qwer", 10);
      wc.add("answer", 42);
      TS_ASSERT_EQUALS(wc.add("asdf", 5), false);
      TS_ASSERT_EQUALS(wc.add("qwer", 10), true);
      TS_ASSERT_EQUALS(wc.size(), 3u);
   }
   void testClassOf() {
      WordClasses wc;
      wc.add("asdf", 3u);
      wc.add("qwer", 10u);
      wc.add("answer", 42u);
      TS_ASSERT_EQUALS(wc.classOf("asdf"), 3u);
      TS_ASSERT_EQUALS(wc.classOf("qwer"), 10u);
      TS_ASSERT_EQUALS(wc.classOf("answer"), 42u);
      TS_ASSERT_EQUALS(wc.classOf("life"), WordClasses::NoClass);
   }
   void testHighestClassId() {
      WordClasses wc;
      TS_ASSERT_EQUALS(wc.getHighestClassId(), 0u);
      wc.add("asdf", 3);
      TS_ASSERT_EQUALS(wc.getHighestClassId(), 3u);
      wc.add("answer", 42);
      TS_ASSERT_EQUALS(wc.getHighestClassId(), 42u);
      wc.add("qwer", 10);
      TS_ASSERT_EQUALS(wc.getHighestClassId(), 42u);
      WordClasses wc2(wc);
      TS_ASSERT_EQUALS(wc2.getHighestClassId(), 42u);
   }
   void testClear() {
      WordClasses wc;
      wc.add("asdf", 3);
      wc.add("qwer", 10);
      wc.add("answer", 42);
      TS_ASSERT_EQUALS(wc.size(), 3u);
      wc.clear();
      TS_ASSERT_EQUALS(wc.size(), 0u);
   }
   void testRead() {
      char tmp_file_name[] = "WordClassTestReadXXXXXX";
      int tmp_fd = mkstemp(tmp_file_name);
      oMagicStream os(tmp_fd, true);
      TS_ASSERT(os);
      os << "asdf\t3\nqwer\t10\nanswer\t42" << endl;
      os.close();
      WordClasses wc;
      wc.read(tmp_file_name);
      unlink(tmp_file_name);
      TS_ASSERT_EQUALS(wc.size(), 3u);
      TS_ASSERT_EQUALS(wc.classOf("asdf"), 3u);
      TS_ASSERT_EQUALS(wc.classOf("qwer"), 10u);
      TS_ASSERT_EQUALS(wc.classOf("answer"), 42u);
      TS_ASSERT_EQUALS(wc.classOf("life"), WordClasses::NoClass);
   }
   void testDeepCopy() {
      WordClasses wc;
      wc.add("asdf", 3);
      wc.add("qwer", 10);
      wc.add("answer", 42);
      WordClasses* wc2 = new WordClasses(wc);
      WordClasses* wc3 = new WordClasses(wc);
      TS_ASSERT_EQUALS(wc.classOf("qwer"), 10u);
      TS_ASSERT_EQUALS(wc3->classOf("qwer"), 10u);
      TS_ASSERT_THROWS_NOTHING(wc.clear());
      TS_ASSERT_THROWS_NOTHING(wc3->clear());
      TS_ASSERT_EQUALS(wc2->size(), 3u);
      TS_ASSERT_EQUALS(wc2->classOf("qwer"), 10u);
      TS_ASSERT_THROWS_NOTHING(wc2->clear());
      TS_ASSERT_THROWS_NOTHING(delete wc2);
      TS_ASSERT_THROWS_NOTHING(delete wc3);
   }
   void testNoLeak() {
      WordClasses wc;
      wc.add("a string longer than the 128 character limit for "
             "allocation in the block_storage, so that long_storage will "
             "be needed.  This is a deep implementation detail, but we still "
             "want to be able to show there are no memory leaks.", 42);
   }

}; // TestWordClasses

} // Portage


