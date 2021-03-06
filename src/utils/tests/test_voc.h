/**
 * @author Eric Joanis
 * @file test_voc.h  Test suite for Voc
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "voc.h"
#include "file_utils.h"

using namespace Portage;

namespace Portage {

class TestVoc : public CxxTest::TestSuite 
{
private:
   Voc v;

public:
   void setUp() {
      v.add("asdf");
      v.add("qwer");
      v.add("zxcv");
   }
   void tearDown() { v.clear(); }

   void test_copy_const() {
      Voc v2(v);
      TS_ASSERT_DIFFERS(v.word(0),v2.word(0));
      TS_ASSERT_DIFFERS(v.word(1),v2.word(1));
      TS_ASSERT_DIFFERS(v.word(2),v2.word(2));
      TS_ASSERT_RELATION(str_equal, v.word(0), v2.word(0));
      TS_ASSERT_RELATION(str_equal, v.word(1), v2.word(1));
      TS_ASSERT_RELATION(str_equal, v.word(2), v2.word(2));
      TS_ASSERT_EQUALS(v2.size(),3u);
      TS_ASSERT_EQUALS(v2.index("asdf"),0u);
      TS_ASSERT_EQUALS(v2.index("qwer"),1u);
      TS_ASSERT_EQUALS(v2.index("zxcv"),2u);
      TS_ASSERT_EQUALS(v2.index("foo"),3u);
      TS_ASSERT_EQUALS(v2.add("bar"),3u);
      TS_ASSERT_EQUALS(v2.index("foo"),4u);
      TS_ASSERT_EQUALS(v2.index("bar"),3u);
   }

   void test_read_write() {
      ostringstream oss;
      v.write(oss);
      string ref="asdf\nqwer\nzxcv\n";
      TS_ASSERT_EQUALS(oss.str(), "asdf\nqwer\nzxcv\n");
      Voc v2;
      istringstream iss(ref);
      v2.read(iss);
      TS_ASSERT_RELATION(str_equal, v.word(0), v2.word(0));
      TS_ASSERT_RELATION(str_equal, v.word(1), v2.word(1));
      TS_ASSERT_RELATION(str_equal, v.word(2), v2.word(2));
   }

   void test_read_write_stream() {
      char tmp_file_name[] = "VocTestReadWriteXXXXXX";
      int tmp_fd = mkstemp(tmp_file_name);
      oMagicStream os(tmp_fd, true);
      TS_ASSERT(os);
      v.writeStream(os);
      os.close();

      Voc v2;
      iMagicStream is(tmp_file_name);
      v2.readStream(is, tmp_file_name);
      unlink(tmp_file_name);

      TS_ASSERT_DIFFERS(v.word(0),v2.word(0));
      TS_ASSERT_DIFFERS(v.word(1),v2.word(1));
      TS_ASSERT_DIFFERS(v.word(2),v2.word(2));
      TS_ASSERT_RELATION(str_equal, v.word(0), v2.word(0));
      TS_ASSERT_RELATION(str_equal, v.word(1), v2.word(1));
      TS_ASSERT_RELATION(str_equal, v.word(2), v2.word(2));
      TS_ASSERT_EQUALS(v2.size(),3u);
      TS_ASSERT_EQUALS(v2.index("asdf"),0u);
      TS_ASSERT_EQUALS(v2.index("qwer"),1u);
      TS_ASSERT_EQUALS(v2.index("zxcv"),2u);
      TS_ASSERT_EQUALS(v2.index("foo"),3u);
      TS_ASSERT_EQUALS(v2.add("bar"),3u);
      TS_ASSERT_EQUALS(v2.index("foo"),4u);
      TS_ASSERT_EQUALS(v2.index("bar"),3u);
   }

   // remapping old index.
   void test_remap1() {
      const char* const already  = "zxcv";
      const char* const oldToken = "qwer";
      const char* const newToken = "qaz";

      TS_ASSERT_EQUALS(v.size(), 3u);
      TS_ASSERT_EQUALS(v.index(oldToken), 1u);

      // Invalid index.
      TS_ASSERT(!v.remap(3u, "should fail"));
      // new token already part of voc.
      TS_ASSERT(!v.remap(0u, already));

      // change the old token for the new token.
      TS_ASSERT(v.remap(1u, newToken));
      TS_ASSERT_EQUALS(v.index(newToken), 3u); // EJJ remap is now partial: word(i), but not index(s)
      TS_ASSERT_RELATION(str_equal, v.word(1u), newToken);
      // Make sure no new words were added.
      TS_ASSERT_EQUALS(v.size(), 3u);
      // Make sure old token doesn't exists anymore.
      TS_ASSERT_EQUALS(v.index(oldToken), 1u); // EJJ remap is now partial: word(i), but not index(s)
   }

   // remapping old token.
   void test_remap2() {
      const char* const already  = "zxcv";
      const char* const oldToken = "qwer";
      const char* const newToken = "qaz";

      TS_ASSERT_EQUALS(v.size(), 3u);
      TS_ASSERT_EQUALS(v.index(oldToken), 1u);

      // Remapping something that is not in voc.
      TS_ASSERT(!v.remap(newToken, newToken));
      // new token already part of voc.
      TS_ASSERT(!v.remap(oldToken, already));

      // change the old token for new token.
      TS_ASSERT(v.remap(oldToken, newToken));
      TS_ASSERT_EQUALS(v.index(newToken), 3u); // EJJ remap is now partial: word(i), but not index(s)
      TS_ASSERT_RELATION(str_equal, v.word(1u), newToken);
      // Make sure no new words were added.
      TS_ASSERT_EQUALS(v.size(), 3u);
      // Make sure old token doesn't exists anymore.
      TS_ASSERT_EQUALS(v.index(oldToken), 1u); // EJJ remap is now partial: word(i), but not index(s)
   }
}; // TestVoc

class TestProbVoc : public CxxTest::TestSuite
{
private:
   ProbVoc v;

public:
   void setUp() {
      v.add("asdf", 0.1f);
      v.add("qwer", 0.2f);
      v.add("zxcv", 1.0f);
   }
   void tearDown() { v.clear(); }

   void test_read_write() {
      char tmp_file_name[] = "ProbVocTestReadWriteXXXXXX";
      int tmp_fd = mkstemp(tmp_file_name);
      oMagicStream os(tmp_fd, true);
      TS_ASSERT(os);
      v.write(os);
      os.close();

      ProbVoc v2;
      v2.read(tmp_file_name);
      unlink(tmp_file_name);

      TS_ASSERT_DIFFERS(v.word(0), v2.word(0));
      TS_ASSERT_DIFFERS(v.word(1), v2.word(1));
      TS_ASSERT_DIFFERS(v.word(2), v2.word(2));
      TS_ASSERT_RELATION(str_equal, v.word(0), v2.word(0));
      TS_ASSERT_RELATION(str_equal, v.word(1), v2.word(1));
      TS_ASSERT_RELATION(str_equal, v.word(2), v2.word(2));
      TS_ASSERT_EQUALS(v.freq((Uint)0), v2.freq((Uint)0));
      TS_ASSERT_EQUALS(v.freq(1), v2.freq(1));
      TS_ASSERT_EQUALS(v.freq(2), v2.freq(2));
      TS_ASSERT_EQUALS(v2.size(), 3u);
      TS_ASSERT_EQUALS(v2.index("asdf"), 0u);
      TS_ASSERT_EQUALS(v2.index("qwer"), 1u);
      TS_ASSERT_EQUALS(v2.index("zxcv"), 2u);
      TS_ASSERT_EQUALS(v2.freq("asdf"), 0.1f);
      TS_ASSERT_EQUALS(v2.freq("qwer"), 0.2f);
      TS_ASSERT_EQUALS(v2.freq("zxcv"), 1.0f);
   }

   void test_sort() {
      v.sortReverseFreq();
      TS_ASSERT_EQUALS(v.index("asdf"), 2u);
      TS_ASSERT_EQUALS(v.index("qwer"), 1u);
      TS_ASSERT_EQUALS(v.index("zxcv"), 0u);
      TS_ASSERT_EQUALS(v.word(0u), string("zxcv"));
      TS_ASSERT_EQUALS(v.word(1u), string("qwer"));
      TS_ASSERT_EQUALS(v.word(2u), string("asdf"));
      TS_ASSERT_EQUALS(v.freq(0u), 1.0f);
      TS_ASSERT_EQUALS(v.freq(1u), 0.2f);
      TS_ASSERT_EQUALS(v.freq(2u), 0.1f);
   }
   
   void test_clear() {
      v.clear();
      TS_ASSERT_EQUALS(v.size(), 0);
   }
}; // TestProbVoc



} // Portage
