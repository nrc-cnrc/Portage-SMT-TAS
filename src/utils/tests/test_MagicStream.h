/**
 * @author Samuel Larkin
 * @file test_MagicStream.h  Test suite for MagicStream.
 *
 * This test suite is intended for testing all features of MagicStream.
 * 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "MagicStream.h"
#include "binio.h"
#include "file_utils.h"
//#include <bits/functexcept.h> // __throw_ios_failure
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>

template <class T>
inline std::string to_string (const T& t)
{
   std::stringstream ss;
   ss << t;
   return ss.str();
}

using namespace Portage;

// Debug helper :D
// To activate use -DTEST_MAGIC_STREAM_DEBUG flag at compile time
inline void log(const string& msg)
{
#ifdef TEST_MAGIC_STREAM_DEBUG
   cerr << "\tTEST_MAGIC_STREAM_DEBUG LOG:\t" << msg << endl;
#endif
}

namespace Portage {

class TestMagicStream : public CxxTest::TestSuite 
{
private:
   string m_read_msg;
   const string base_name_test;

   /**
   * Prints a matrix of size mI x mJ to out.  Each line is composed of msg i j
   * @param out  output stream
   * @param msg  a SINGLE word to be printed on each line
   * @param mI   maximum number of lines
   * @param mJ   maximum number of columns
   */
   void printMatrice(ostream& out, const string& msg, unsigned mI, unsigned mJ) throw() {
      if (msg.find(" ") != string::npos) __throw_ios_failure("Message can't have spaces");
      if (!out.good()) __throw_ios_failure("Bad stream before writing");
      for (unsigned i(0); i<mI; ++i)
         for (unsigned j(0); j<mJ; ++j) {
            out << msg << " " << i << " " << j << endl;
            if (!out.good()) __throw_ios_failure("Bad stream after writing matrix data point");
         }
      if (!out.good()) __throw_ios_failure("Bad stream after writing");
   }

   /**
   * Asserts that a matrix of size mI x mJ from in is well formed.  Each line should be composed of msg i j
   * @param in   input stream
   * @param msg  a SINGLE word that should be on each line
   * @param mI   maximum number of lines
   * @param mJ   maximum number of columns
   */
   void checkMatrice(istream& in, const string& msg, unsigned mI, unsigned mJ) {
      if (msg.find(" ") != string::npos) __throw_ios_failure("Message can't have spaces");
      if (!in.good()) __throw_ios_failure("Bad stream before reading");
      string mot;
      unsigned a = 0xBAD;
      unsigned b = 0XBAD;
      for (unsigned i(0); i<mI; ++i)
         for (unsigned j(0); j<mJ; ++j) {
            in >> mot >> a >> b;
            if (!in.good()) __throw_ios_failure("Bad stream while reading matrix data point");
            if (mot != msg) __throw_ios_failure("Wrong message");
            if (a != i) __throw_ios_failure("Bad i argument");
            if (b != j) __throw_ios_failure("Bad j argument");
         }
      if (!in.good()) __throw_ios_failure("Bad stream after reading");
   }

public:
   TestMagicStream()
   : base_name_test("MagicStreamTest")
   , filename_first(base_name_test + "First")
   , msg_first("Making_sure_ts_assertions_work_with_MagicStream")
   , filename_plain(base_name_test + "File")
   , msg_plain("Testing_file_MagicStream")
   , filename_gzip("MagicStreamTestFile.gz")
   , msg_gzip("Testing_gz_file_MagicStream")
   , filename_bzip2("MagicStreamTestFile.bz2")
   , msg_bzip2("Testing_bzip2_file_MagicStream")
   , filename_lzma("MagicStreamTestFile.lzma")
   , msg_lzma("Testing_lzma_file_MagicStream")
   , msg_fd("Testing_file_descriptor_MagicStream")
   , filename_fh(base_name_test + "FileHandle")
   , msg_fh("Testing_file_handle_MagicStream")
   , filename_pipes(base_name_test + "Pipes")
   , msg_pipes("Testing_pipes_MagicStream")
   , filename_multiple(base_name_test + "Multiple")
   , msg_multiple("open close test ")
   , filename_binary(base_name_test + "Binary")
   , msg_binary("Writing_a_vector_in_binary_form")
   , vSize(300)
   , vValue(23)
   , filename_fallback(base_name_test + "Fallback")
   , msg_fallback("Fallback")
   , filename_zlib_fallback(base_name_test + "ZlibFallback" + ".gz")
   , msg_zlib_fallback("Testing zlib fallback for gzip!")
   {}
   ~TestMagicStream() {
      TS_ASSERT(system("rm -f MagicStreamTest*") == 0);
   }

   void setUp() {
      // Reset the message to detect bad reads.
      m_read_msg = "Beginning of test";
   }
   void tearDown() {
      // this is for code that needs to be run after each test*() method.
      // Again, this is intended for factoring out duplicated code, and can be
      // left out if you don't need it.
   }

   // THIS MUST BE THE FIRST TEST.
   // Make sure the TS_ASSERT does what is expected on Streams.
   const string filename_first;
   const string msg_first;
   void testAbsoluteFirstWriteTest4MagicStream() {
      // This test should failed since the stream is not opened.
      oMagicStream os_unopened;
      TS_ASSERT(!(os_unopened << msg_first));
      // No assertion should occur for the following test.
      oMagicStream os_opened(filename_first);
      TS_ASSERT(os_opened << msg_first);
   }
   void testAbsoluteFirstReadTest4MagicStream() {
      // This test should failed since the stream is not opened.
      iMagicStream is_unopened;
      TS_ASSERT(!(is_unopened >> m_read_msg));
      // No assertion should occur for the following test.
      iMagicStream is_opened(filename_first);
      TS_ASSERT(is_opened >> m_read_msg);
      TS_ASSERT_EQUALS(m_read_msg, msg_first);
   }

   // Testing File
   const string filename_plain;
   const string msg_plain;
   void testWritingFile() {
      oMagicStream os(filename_plain);
      TS_ASSERT(os);
      TS_ASSERT(os << msg_plain << endl);
      TS_ASSERT_THROWS_NOTHING(printMatrice(os, msg_plain, 3u, 5u));
      os.close();
      TS_ASSERT(os);
   }
   void testReadingFile() {
      iMagicStream is(filename_plain);
      TS_ASSERT(is);
      TS_ASSERT(getline(is, m_read_msg));
      TS_ASSERT_EQUALS(m_read_msg, msg_plain);
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_plain, 3u, 5u));
      is.close();
      TS_ASSERT(is);
   }

   // Testing Gzip File
   const string filename_gzip;
   const string msg_gzip;
   void testWritingGzipFile() {
      oMagicStream os(filename_gzip);
      TS_ASSERT(os);
      TS_ASSERT(os << msg_gzip << endl);
      TS_ASSERT_THROWS_NOTHING(printMatrice(os, msg_gzip, 3u, 5u));
      os.close();
      TS_ASSERT(os);
   }
   void testReadingGzipFile() {
      iMagicStream is(filename_gzip);
      TS_ASSERT(is);
      TS_ASSERT(getline(is, m_read_msg));
      TS_ASSERT_EQUALS(m_read_msg, msg_gzip);
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_gzip, 3u, 5u));
      is.close();
      TS_ASSERT(is);
   }

   // Testing Bzip2 File
   const string filename_bzip2;
   const string msg_bzip2;
   void testWritingBzip2File() {
      oMagicStream os(filename_bzip2);
      TS_ASSERT(os);
      TS_ASSERT(os << msg_bzip2 << endl);
      TS_ASSERT_THROWS_NOTHING(printMatrice(os, msg_bzip2, 3u, 5u));
      os.close();
      TS_ASSERT(os);
   }
   void testReadingBzip2File() {
      iMagicStream is(filename_bzip2);
      TS_ASSERT(is);
      TS_ASSERT(getline(is, m_read_msg));
      TS_ASSERT_EQUALS(m_read_msg, msg_bzip2);
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_bzip2, 3u, 5u));
      is.close();
      TS_ASSERT(is);
   }

   // Testing lzma File
   const string filename_lzma;
   const string msg_lzma;
   void testWritingLzmaFile() {
      oMagicStream os(filename_lzma);
      TS_ASSERT(os);
      TS_ASSERT(os << msg_lzma << endl);
      TS_ASSERT_THROWS_NOTHING(printMatrice(os, msg_lzma, 3u, 5u));
      os.close();
      TS_ASSERT(os);
   }
   void testReadingLzmaFile() {
      iMagicStream is(filename_lzma);
      TS_ASSERT(is);
      TS_ASSERT(getline(is, m_read_msg));
      TS_ASSERT_EQUALS(m_read_msg, msg_lzma);
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_lzma, 3u, 5u));
      is.close();
      TS_ASSERT(is);
   }

   // Testing File Descriptor
   string filename_fd;
   const string msg_fd;
   void testWritingWithFileDescriptor() {
      char tmp_file_name[] = "MagicStreamTestFileDescriptor_XXXXXX";
      int tmp_fd = mkstemp(tmp_file_name);
      filename_fd = tmp_file_name;
      oMagicStream os(tmp_fd, true);
      TS_ASSERT(os);
      TS_ASSERT(os << msg_fd << endl);
      TS_ASSERT_THROWS_NOTHING(printMatrice(os, msg_fd, 3u, 5u));
      os.close();
      TS_ASSERT(os);
   }
   void testReadingWithFileDescriptor() {
      int tmp_fd = open(filename_fd.c_str(), O_RDONLY);
      iMagicStream is(tmp_fd, true);\
      TS_ASSERT(is);
      TS_ASSERT(getline(is, m_read_msg));
      TS_ASSERT_EQUALS(m_read_msg, msg_fd);
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_fd, 3u, 5u));
      is.close();
      TS_ASSERT(is);
   }

   // Test File Handle
   string filename_fh;
   const string msg_fh;
   void testWritingWithFileHandle() {
      FILE* f(0);
      f = fopen(filename_fh.c_str(), "w");
      TS_ASSERT(f != NULL);
      oMagicStream os(f, true);
      TS_ASSERT(os);
      TS_ASSERT(os << msg_fh << endl);
      TS_ASSERT_THROWS_NOTHING(printMatrice(os, msg_fh, 3u, 5u));
      os.close();
      TS_ASSERT(os);
      // We've asked MagicStream to close the file handle, make sure it did
      //TS_ASSERT(fclose(f) != 0);
   }
   void testReadingWithFileHandle() {
      FILE* f(0);
      f = fopen(filename_fh.c_str(), "r");
      TS_ASSERT(f != NULL);
      iMagicStream is(f, false);
      TS_ASSERT(is);
      TS_ASSERT(getline(is, m_read_msg));
      TS_ASSERT_EQUALS(m_read_msg, msg_fh);
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_fh, 3u, 5u));
      is.close();
      TS_ASSERT(is);
      // We've asked MagicStream to NOT close the file handle, make sure it did
      TS_ASSERT(fclose(f) == 0);
   }

   // Test Pipes
   string filename_pipes;
   const string msg_pipes;
   void testWritingPipes() {
      oMagicStream os("| tail -5 > " + filename_pipes);
      TS_ASSERT_THROWS_NOTHING(printMatrice(os, msg_pipes, 3u, 5u));
   }
   void testReadingPipes() {
      iMagicStream is("head -2 " + filename_pipes + " |");
      TS_ASSERT(is);
      string dummy;
      int cnt(0);
      // First line must be pipes 2 0
      TS_ASSERT(getline(is, dummy));
      TS_ASSERT_EQUALS(dummy, msg_pipes + " 2 0");
      ++cnt;
      // Second line must be pipes 2 1
      TS_ASSERT(getline(is, dummy));
      TS_ASSERT_EQUALS(dummy, msg_pipes + " 2 1");
      ++cnt;
      // There should be no other lines but try to read them to make sure.
      while (getline(is, dummy)) {
         TS_FAIL("Shouldn't have read this from a pipe: " + dummy);
         ++cnt;
      }
      TS_ASSERT_EQUALS(cnt, 2);
   }

   // Multiple open/close.
   const string filename_multiple;
   const string msg_multiple;
   void testMultipleWrite() {
      oMagicStream ms;
      for (unsigned t(0); t<10; ++t) {
         ms.open(filename_multiple);
         TS_ASSERT(ms);
         TS_ASSERT(ms << msg_multiple << t << endl);
         ms.close();
      }
   }
   void testMultipleRead() {
      iMagicStream ms;
      for (unsigned t(0); t<10; ++t) {
         ms.open(filename_multiple);
         TS_ASSERT(ms);
         TS_ASSERT(getline(ms, m_read_msg));
         TS_ASSERT_EQUALS(m_read_msg, msg_multiple + "9");
         ms.close();
      }
   }

   // Test binary io
   const string filename_binary;
   const string msg_binary;
   const Uint vSize;
   const Uint vValue;
   void testBinaryWrite() {
      vector<Uint> vecteur(vSize, vValue);
      oMagicStream oms(filename_binary);
      TS_ASSERT(oms);
      TS_ASSERT(oms << msg_binary << endl);
      BinIO::writebin(oms, vecteur);
      TS_ASSERT(oms);
      bool last(false);
      oms.write((char*)&last, sizeof(last));
      TS_ASSERT(oms);
   }
   void testBinaryRead() {
      iMagicStream ims(filename_binary);
      TS_ASSERT(ims);
      vector<Uint> vecteur(1, 1);
      TS_ASSERT(getline(ims, m_read_msg));
      TS_ASSERT_EQUALS(m_read_msg, msg_binary);
      BinIO::readbin(ims,vecteur);
      TS_ASSERT(ims);
      TS_ASSERT_EQUALS(vecteur.size(), vSize);
      for (vector<Uint>::const_iterator value(vecteur.begin()); value!=vecteur.end(); ++value) {
         TS_ASSERT_EQUALS(*value, vValue);
      }
      bool last(true);
      ims.read((char*)&last, sizeof(last));
      TS_ASSERT(ims);
      TS_ASSERT_EQUALS(last, false);
      //copy(vecteur.begin(), vecteur.end(), ostream_iterator<int>(cerr, " "));
   }

   // Test gz fallback when user gives a none gz filename and the file on
   // disk is gz.
   const string filename_fallback;
   const string msg_fallback;
   void testFallbackOnGz() {
      {
         oMagicStream os(filename_fallback + ".gz");
         TS_ASSERT(os);
         TS_ASSERT(os << msg_fallback << endl);
         TS_ASSERT_THROWS_NOTHING(printMatrice(os, msg_fallback, 3u, 5u));
      }
      {
         iMagicStream is(filename_fallback);
         TS_ASSERT(is);
         TS_ASSERT(getline(is, m_read_msg));
         TS_ASSERT_EQUALS(m_read_msg, msg_fallback);
         TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_fallback, 3u, 5u));
      }
   }

   // Testing ignore, this is mainly for binlm.
   // Note: reusing plain text file generated by testWritingFile.
   void testFileIgnorePlainText() {
      iMagicStream is(filename_plain);
      TS_ASSERT(is.ignore(msg_plain.size()));
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_plain, 3u, 5u));
   }
   void testFileIgnoreGzip() {
      iMagicStream is(filename_gzip);
      TS_ASSERT(is.ignore(msg_gzip.size()));
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_gzip, 3u, 5u));
   }
   void testFileIgnoreBzip2() {
      iMagicStream is(filename_bzip2);
      TS_ASSERT(is.ignore(msg_bzip2.size()));
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_bzip2, 3u, 5u));
   }
   void testFileIgnoreLzma() {
      iMagicStream is(filename_lzma);
      TS_ASSERT(is.ignore(msg_lzma.size()));
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_lzma, 3u, 5u));
   }

   void testFileSeekPlainText() {
      iMagicStream is(filename_plain);
      TS_ASSERT(is.seekg(ios_base::beg + msg_plain.size()));
      TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_plain, 3u, 5u));
   }
   void testFileSeekGzip() {
      iMagicStream is(filename_gzip);
      TS_ASSERT(!is.seekg(ios_base::beg + msg_gzip.size()));
      //TS_ASSERT(getline(is, m_read_msg));
      //TS_ASSERT_EQUALS(m_read_msg, msg_gzip);
      //TS_ASSERT_THROWS_NOTHING(checkMatrice(is, msg_gzip, 3u, 5u));
   }

   // Test all detectable extension by MagicStream.
   void testExtensionDetectionGzip() {
      TS_ASSERT(MagicStreamBase::isZip("file.gz"));
      TS_ASSERT(MagicStreamBase::isZip("file.z"));
      TS_ASSERT(MagicStreamBase::isZip("file.Z"));
      TS_ASSERT(!MagicStreamBase::isZip("file-gz"));
      TS_ASSERT(!MagicStreamBase::isZip("file-z"));
      TS_ASSERT(!MagicStreamBase::isZip("file_z"));
      TS_ASSERT(!MagicStreamBase::isZip("file.gzip"));
      TS_ASSERT(!MagicStreamBase::isZip("file.gz.txt"));
   }
   void testExtensionDetectionBzip2() {
      TS_ASSERT(MagicStreamBase::isBzip2("file.bz"));
      TS_ASSERT(MagicStreamBase::isBzip2("file.bz2"));
      TS_ASSERT(MagicStreamBase::isBzip2("file.bzip2"));
      TS_ASSERT(!MagicStreamBase::isBzip2("file.bzip2.txt"));
   }
   void testExtensionDetectionLzma() {
      TS_ASSERT(MagicStreamBase::isLzma("file.lzma"));
      TS_ASSERT(!MagicStreamBase::isLzma("file.LZMA"));
      TS_ASSERT(!MagicStreamBase::isLzma("file.lzma.txt"));
   }

   // Test if a file exists
   void testFileExists() {
      TS_ASSERT(!check_if_exists(""));
      TS_ASSERT(check_if_exists(filename_plain));
      TS_ASSERT(check_if_exists(filename_gzip));
      TS_ASSERT(check_if_exists(filename_bzip2));
      TS_ASSERT(check_if_exists("-"));
      TS_ASSERT(check_if_exists("ls |"));
      TS_ASSERT(check_if_exists("| cat"));
      //iSafeMagicStream is("unexisting.file");
      TS_ASSERT(!check_if_exists("unexisting.file.txt"));
      TS_ASSERT(!check_if_exists("unexisting.file.gz"));
      TS_ASSERT(!check_if_exists("unexisting.file.bzip2"));
   }

   // Test opening a file that does not exist
   void testOpenNotExists() {
      TS_ASSERT(!iMagicStream("unexisting.file.gz"));
      TS_ASSERT(!iMagicStream("unexisting.file.txt"));
      TS_ASSERT(!iMagicStream("unexisting.file.bzip2"));
   }


   // Special case that only works on a node.
   // Here we are trying to take all the memory and when the memory is full, we
   // try to open a gzip file which forks and at that precis moment the os
   // needs twice as much memory which will fail.
   const string filename_zlib_fallback;
   const string msg_zlib_fallback;
   void testZlibFallback() {
      const unsigned int num_write(800);
      vector<char*> v;
      // Bloat the memory.
      do {
         try {
            v.push_back(new char[2000000000]);
         } catch (std::bad_alloc& e) {
            v.back() = NULL;
         }
      } while (v.back() != NULL);

      log("Memory blocks: " + to_string(v.size()));

      // Delete at least one block from memory so we don't crash someone's job.
      delete v.back();
      v.pop_back();

      // Write a file using the backup system.
      {
         oMagicStream oms(filename_zlib_fallback);
         TS_ASSERT(oms.rdbuf() != NULL);
         for (unsigned int i(0); i<num_write; ++i)
            oms << msg_zlib_fallback << endl;
      }

      // Verify the content of the file.
      {
         iMagicStream ims(filename_zlib_fallback);
         TS_ASSERT(ims.rdbuf() != NULL);
         unsigned int cpt = 0;
         string line;
         while (getline(ims, line)) {
            TS_ASSERT(line == msg_zlib_fallback.c_str());
            ++cpt;
         }
         TS_ASSERT_EQUALS(cpt, num_write);
      }

      // Lets free up have the memory that way MagicStream won't use the zlib
      // fallback.
      const unsigned int Max = v.size() / 2;
      for (unsigned int i=0; i<Max; ++i) {
         delete v.back();
         v.pop_back();
      }

      log("Memory blocks: " + to_string(v.size()));

      // Write a file using the backup system.
      {
         oMagicStream oms(filename_zlib_fallback);
         TS_ASSERT(oms.rdbuf() != NULL);
         for (unsigned int i(0); i<num_write; ++i)
            oms << msg_zlib_fallback << endl;
      }

      // Verify the content of the file.
      {
         iMagicStream ims(filename_zlib_fallback);
         TS_ASSERT(ims.rdbuf() != NULL);
         unsigned int cpt = 0;
         string line;
         while (getline(ims, line)) {
            TS_ASSERT(line == msg_zlib_fallback.c_str());
            ++cpt;
         }
         TS_ASSERT_EQUALS(cpt, num_write);
      }

      // Clean up the memory
      for (unsigned int i=0; i<v.size(); ++i)
         delete v[i];
   }

}; // TestMagicStream

} // Portage
