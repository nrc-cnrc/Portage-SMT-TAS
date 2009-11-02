/**
 * @author Samuel Larkin
 * @file test_MagicStream.cc  Program that test the MagicStream functionalities.
 *
 *
 * COMMENTS: Testing unit for MagicStream which permits the following:
 *           - to read from standard in or to write to standard out
 *           - to read/write to a compress gzip file
 *           - to read/write to a plain text file
 *           - to read/write from a pipe
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include <MagicStream.h>
#include <file_utils.h>
#include <iostream>
#include <printCopyright.h>


using namespace Portage;
using namespace std;

const unsigned maxI(4);
const unsigned maxJ(5);

void coutModificationTest();
void fileSeek();
void NoneExistingZip();
void fileExist();
void multiplePipes();
void multipleFiles();

////////////////////////////////////////////////////////////////////////////////
// MAIN
//usage is : executable "optional input pipe command => cmd2 |"
int main(int argc, char* argv[])
{
   printCopyright(2006, "test_MagicStream");

   // Expliciting the command line argument
   for (int i(0); i<argc; ++i) {
      cout << "arguments[" << i << "]: " << argv[i] << endl;
   }
   cout << endl;

   //coutModificationTest();
   //magicStreamTest();
   //fileSeek();
   NoneExistingZip();
   fileExist();
   //multiplePipes();
   //multipleFiles();

   cout << "END OF TESTS" << endl;
   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
void endOfTestMarker(bool success = true) {
   cout << "TEST " << (success ? "SUCCESSFUL" : "FAILED") << endl;
   cout << "====================" << endl << endl;
}
void printMatrice(ostream& out, const string& msg, unsigned mI = maxI, unsigned mJ = maxJ);
void checkMatrice(istream& in, const string& msg, unsigned mI = maxI, unsigned mJ = maxJ);


/**
 * Prints a matrix of size mI x mJ to out.  Each line is composed of msg i j
 * @param out  output stream
 * @param msg  message to be printed on each line
 * @param mI   maximum number of lines
 * @param mJ   maximum number of columns
 */
void printMatrice(ostream& out, const string& msg, unsigned mI, unsigned mJ)
{
   for (unsigned i(0); i<mI; ++i)
      for (unsigned j(0); j<mJ; ++j)
         out << msg << " " << i << " " << j << endl;
}

/**
 * Asserts that a matrix of size mI x mJ from in is well formed.  Each line should be composed of msg i j
 * @param in   input stream
 * @param msg  message that should be on each line
 * @param mI   maximum number of lines
 * @param mJ   maximum number of columns
 */
void checkMatrice(istream& in, const string& msg, unsigned mI, unsigned mJ)
{
   string mot;
   unsigned a = 0xBAD;
   unsigned b = 0xBAD;
   for (unsigned i(0); i<mI; ++i)
      for (unsigned j(0); j<mJ; ++j)
      {
         in >> mot >> a >> b;
         assert(mot == msg);
         assert(a == i);
         assert(b == j);
      }
   cout << "Matrice was successfully read" << endl;
}

/**
 * Checks if a stream is open, has the fail bit set and has the good bet set.
 * @param ms  output stream to check
 */
void PrintStatus(oMagicStream& ms)
{
   if (ms.is_open()) cout << "\tMagicStream is opened" << endl;
   if (ms.fail()) cout << "\tMagicStream fail" << endl;
   if (ms.good()) cout << "\tMagicStream good" << endl;
}

/**
 * Checks if a stream is open, has the fail bit set and has the good bet set.
 * @param ms  input stream to check
 */
void PrintStatus(iMagicStream& ms)
{
   if (ms.is_open()) cout << "\tMagicStream is opened" << endl;
   if (ms.fail()) cout << "\tMagicStream fail" << endl;
   if (ms.good()) cout << "\tMagicStream good" << endl;
}

/**
 * Pretest to see the feasability of changing underlying buffer of stream for a different one.
 */
void coutModificationTest()
{
   cout << "Modifying the cout.rdbuf" << endl;

   const string msg("Using cout to write to a file");
   const string filename("cout.filebuf.txt");

   filebuf fb;
   streambuf* tmp = cout.rdbuf();
   fb.open(filename.c_str(), ios::out);
   cout.rdbuf(&fb);
   cout << msg << endl;
   fb.close();
   cout.rdbuf(tmp);
   cout << "cout rdbuf has succesfully been reattached" << endl;
   fstream os(filename.c_str());
   string line;
   getline(os, line);
   delete_if_exists(filename.c_str(), "Cleaning up");
   cout << endl;
   endOfTestMarker(line == msg);
}

/**
 * Checks for file existence
 */
void fileExist()
{
   cout << "empty: " << (check_if_exists("") ? "TRUE" : "FALSE") << endl;
   cout << "existing.file: " << (check_if_exists("existing.file") ? "TRUE" : "FALSE") << endl;
   cout << "-: " << (check_if_exists("-") ? "TRUE" : "FALSE") << endl;
   cout << "ls |: " << (check_if_exists("ls |") ? "TRUE" : "FALSE") << endl;
   cout << "| echo: " << (check_if_exists("| echo") ? "TRUE" : "FALSE") << endl;
   iSafeMagicStream is("unexisting.file");
}

/**
 * Tests the file seeking capabilities of MagicStream.
 * This test was added for binlm
 */
void fileSeek()
{
   cout << endl << "File Seeking test" << endl;
   const unsigned int maxI(1);
   const unsigned int maxJ(3);
   const string msg("FileSeek");
   const string file_gz("fileSeek.gz");
   const string file_txt("fileSeek.txt");
   {
      oSafeMagicStream ms(file_gz);
      ofstream os(file_txt.c_str());
      printMatrice(ms, msg, maxI, maxJ);
      printMatrice(os, msg, maxI, maxJ);
   }

   cout << "gz format" << endl;
   iSafeMagicStream ms(file_gz);
   if (!ms.seekg(ios_base::beg+msg.size())) {
      cout << "Seeking gz failed" << endl;
      cout << ms.rdbuf();
      PrintStatus(ms);
   }
   else
      cout << "Succes\n" << ms.rdbuf();

   cout << "txt format" << endl;
   ifstream is(file_txt.c_str());
   if (!is.seekg(ios_base::beg+msg.size()))
      cout << "Seeking txt failed" << endl;
   else
      cout << "Succes\n" << is.rdbuf();

   delete_if_exists(file_gz.c_str(), "Cleaning up");
   delete_if_exists(file_txt.c_str(), "Cleaning up");
   endOfTestMarker();
}

/**
 * Testing opening in reading mode an .gz file that doesn't exist should raise an error.
 */
void NoneExistingZip()
{
   cout << "Beginning NoneExistingZip test" << endl;

   const string filename("NoneExistingZipFile.gz");
   iMagicStream ms(filename);
   if (ms.fail()) {
      printf("%s doesn't exist and can't be opened => test successful\n", filename.c_str());
   }
   else {
      printf("TEST FAILED: shouldn't be able to open %s\n", filename.c_str());
      assert(false);
   }

   cerr << endl;
   endOfTestMarker();
}

#ifdef _SC_STREAM_MAX
#define STREAM_MAX	sysconf(_SC_STREAM_MAX)
#else
#define STREAM_MAX	"unspecified"
#endif

/// What is the maximum number of pipes allow by the system.
void multiplePipes()
{
   const Uint max(sysconf(_SC_OPEN_MAX)+1);
   cout << "Multiple pipe opens" << endl;

   cout << "System's spec: " << endl;
   cerr << "Max stream: " << STREAM_MAX << endl;
   cerr << "Max file descriptor: " << sysconf(_SC_OPEN_MAX) << endl;

   cout << "One at a time" << endl;
   for (Uint i(0); i<max; ++i) {
      oMagicStream in("delme.what.gz");
      cout << ".";
   }
   cout << endl;

   cout << "All at once" << endl;
   oMagicStream* array[max];
   for (Uint i(0); i<max; ++i) {
      char buff[32];
      snprintf(buff, 30, "delme.%d.gz", i);
      array[i] = new oMagicStream(buff);
      cout << ".";
   }
   cout << endl;

   cout << "Closing pipes" << endl;
   for (Uint i(0); i<max; ++i) {
      if (array[i]) delete array[i];
   }
   cout << endl;
}

/// What is the maximum number of simultaneous opened file by the system.
void multipleFiles()
{
   cerr << "Max stream: " << STREAM_MAX << endl;
   cerr << "Max file descriptor: " << sysconf(_SC_OPEN_MAX) << endl;

   //const Uint max(sysconf(_SC_STREAM_MAX)+1);
   const Uint max(1030);
   ostream* array[max];
   cerr << "Testing ostream" << endl;
   for (Uint i(0); i<max; ++i) {
      char buff[32];
      snprintf(buff, 30, "delme.%d.gz", i);
      array[i] = new oMagicStream(buff);
      cout << ".";
   }
   cout << endl;

   oMagicStream* array2[max];
   cerr << "Testing oMagicStream" << endl;
   for (Uint i(0); i<max; ++i) {
      char buff[32];
      snprintf(buff, 30, "delme.%d.gz", i);
      array2[i] = new oMagicStream(buff);
      cout << ".";
   }
   cout << endl;

   for (Uint i(0); i<max; ++i) {
      if (array[i]) delete array[i];
   }
   cout << endl;
}

