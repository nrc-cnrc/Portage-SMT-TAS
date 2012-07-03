/**
 * @author Eric Joanis
 * @file test_your_class.h  Beginnings of a test suite for file_utils
 *
 * Only tests part of file_utils, because tests have not been written for all
 * methods that existed before we were using CxxTest.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "file_utils.h"

using namespace Portage;

namespace Portage {

class TestFileUtils : public CxxTest::TestSuite 
{
public:
   void testSwapLanguages() {
      string a, b, c, d;
      TS_ASSERT_EQUALS("prefix.l2_given_l1", swap_languages("prefix.l1_given_l2","_given_",&a,&b,&c,&d));
      TS_ASSERT_EQUALS("prefix;l2_given_l1:suffix", swap_languages("prefix;l1_given_l2:suffix","_given_",&c,&d));
      TS_ASSERT_EQUALS("l2_given_l1", swap_languages("l1_given_l2","_given_",&a,&a,&a,&a));
      TS_ASSERT_EQUALS("l2_given_l1&suffix", swap_languages("l1_given_l2&suffix","_given_",&a,&a,&a));
      TS_ASSERT_EQUALS("prefix+lb2la", swap_languages("prefix+la2lb","2",&a,&a));
      TS_ASSERT_EQUALS("prefix-l2twol1.gz", swap_languages("prefix-l1twol2.gz","two"));
      TS_ASSERT_EQUALS("stuff*French.isnot.English*nonsense", swap_languages("stuff*English.isnot.French*nonsense",".isnot.",&a,&b,&c,&d));
      TS_ASSERT_EQUALS("stuff*FrenchisnotEnglish*nonsense", swap_languages("stuff*EnglishisnotFrench*nonsense","isnot",&a,&b));
      TS_ASSERT_EQUALS("tm.fr2Sfr.gz", swap_languages("tm.Sfr2fr.gz","2"));
      TS_ASSERT_EQUALS("tm.f2S.gz", swap_languages("tm.S2f.gz","2",&a,&b,&c,&d));
      TS_ASSERT_EQUALS("S", a);
      TS_ASSERT_EQUALS("f", b);
      TS_ASSERT_EQUALS("tm.", c);
      TS_ASSERT_EQUALS(".gz", d);
      TS_ASSERT_EQUALS("tm.fren2S.gz", swap_languages("tm.S2fren.gz","2",&a));
      TS_ASSERT_EQUALS("tm.f2Swiss.gz", swap_languages("tm.Swiss2f.gz","2"));
      TS_ASSERT_EQUALS("", swap_languages("prefix.l__given_l2","_given_",&a,&b,&c,&d));
      TS_ASSERT_EQUALS("", swap_languages("prefix.l1_given_.2","_given_",&c));
      TS_ASSERT_EQUALS("", swap_languages("prefix.l1_given_","_given_"));
      TS_ASSERT_EQUALS("", swap_languages("_given_l2","_given_",&a,&b,&c));
      TS_ASSERT_EQUALS("", swap_languages("anything","",&a,&b));
   }

   void testFileSize() {
      TS_ASSERT_EQUALS(fileSize("/a/b/c"), 0ul);
      char tmp_file_name[] = "tmp_testFileSizeXXXXXX";
      int tmp_fd = mkstemp(tmp_file_name);
      oMagicStream os(tmp_fd, true);
      TS_ASSERT(os);
      TS_ASSERT_EQUALS(fileSize(tmp_file_name), 0ul);
      os << "Test\n" << flush;
      TS_ASSERT_EQUALS(fileSize(tmp_file_name), 5ul);
      os.close();
      unlink(tmp_file_name);
   }

   void test_is_directory() {
      TS_ASSERT(is_directory("."));
      char cwd[1024];
      TS_ASSERT(is_directory(getcwd(cwd, 1024)));
      char tmp_file_name[] = "./tmp_test_is_directoryXXXXXX";
      int tmp_fd = mkstemp(tmp_file_name);
      assert(tmp_fd >= 0);
      close(tmp_fd);
      TS_ASSERT(!is_directory(tmp_file_name));
      unlink(tmp_file_name);
      TS_ASSERT(!is_directory(tmp_file_name));
      TS_ASSERT(symlink(cwd, tmp_file_name) == 0);
      TS_ASSERT(is_directory(tmp_file_name));
      unlink(tmp_file_name);
      TS_ASSERT(!is_directory(tmp_file_name));
   }

   void testGetAppPath() {
      string my_app_path = GetAppPath();
      TS_ASSERT_EQUALS(BaseName(my_app_path).substr(0,5), "test_");
      TS_ASSERT_EQUALS(BaseName(DirName(my_app_path)), "tests");
      TS_ASSERT_EQUALS(BaseName(DirName(DirName(my_app_path))), "utils");
      TS_ASSERT_EQUALS(BaseName(DirName(DirName(DirName(my_app_path)))), "src");
   }

   void test_adjustRelativePath() {
      TS_ASSERT_EQUALS(adjustRelativePath("a/b/c", "test_file"), "a/b/c/test_file");
      TS_ASSERT_EQUALS(adjustRelativePath("a/b/c", "d/e/test_file"), "a/b/c/d/e/test_file");
      TS_ASSERT_EQUALS(adjustRelativePath("/a/b/c", "d/e/test_file"), "/a/b/c/d/e/test_file");
      TS_ASSERT_EQUALS(adjustRelativePath("/a/b/c", "/d/e/test_file"), "/d/e/test_file");
      TS_ASSERT_EQUALS(adjustRelativePath("a/b/c", ""), "");
      TS_ASSERT_EQUALS(adjustRelativePath("", "test_file"), "test_file");
      TS_ASSERT_EQUALS(adjustRelativePath("", ""), "");
   }
}; // TestFileUtils

} // Portage
