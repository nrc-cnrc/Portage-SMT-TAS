/**
 * @author Eric Joanis
 * @file test_line_index_file.h  Test suite for LineIndexedFile
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "line_indexed_file.h"
#include <stdlib.h>
#include "file_utils.h"

using namespace Portage;

namespace Portage {

class TestLineIndexedFile : public CxxTest::TestSuite 
{
   string tmpfile;

public:
   void setUp() {
      char tmpfilename[] = "/tmp/testLineIndex.XXXXXX";
      int fd = mkstemp(tmpfilename);
      FOR_ASSERT(fd);
      tmpfile = tmpfilename;
      assert(fd != -1);

      oSafeMagicStream out(tmpfile);
      out << "qwer" << endl << "asdf" << endl << "" << endl << "zxcv" << endl << "nofinaleol";
      out.close();
   }
   void tearDown() {
      unlink(tmpfile.c_str());
   }
   void testIndexedAccess() {
      LineIndexedFile f(tmpfile);
      TS_ASSERT_EQUALS(f.size(), 5u);
      TS_ASSERT_EQUALS(f.get(0), "qwer");
      TS_ASSERT_EQUALS(f.get(1), "asdf");
      TS_ASSERT_EQUALS(f.get(2), "");
      TS_ASSERT_EQUALS(f.get(3), "zxcv");
      TS_ASSERT_EQUALS(f.get(4), "nofinaleol");
   }
}; // TestLineIndexedFile

} // Portage
