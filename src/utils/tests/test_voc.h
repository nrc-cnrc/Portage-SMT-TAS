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
public:

   void test_copy_const() {
      Voc v;
      v.add("asdf");
      v.add("qwer");
      v.add("zxcv");
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

   void test_read_write_stream() {
      Voc v;
      v.add("asdf");
      v.add("qwer");
      v.add("zxcv");

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
}; // TestYourClass

} // Portage
