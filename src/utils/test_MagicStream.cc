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
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
               
#include <file_utils.h>
#include <printCopyright.h>


using namespace Portage;
using namespace std;

const unsigned maxI(4);
const unsigned maxJ(5);

void coutModificationTest();
void magicStreamTest();
void linkTest(const string& icmd = "head -n 5 link.txt |tail -n2 |");
void fileHandleTest(const string& test, const string& header, const string& ext);
void fileDescriptorTest(const string& test, const string& header, const string& ext);
void OpenCloseTest();
void fileSeek();
void fileIgnore();
void MagicStreamDotWrite();
void NoneExistingZip();
void fileExist();

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
   //(argc <= 1) ? linkTest():  linkTest(argv[1]);
   //fileHandleTest("Using a FILE*", "file_handle", ".txt");
   //fileDescriptorTest("Using a file descriptor", "file_descriptor", ".txt");
   //OpenCloseTest();
   //fileSeek();
   //fileIgnore();
   //MagicStreamDotWrite();
   //NoneExistingZip();
   fileExist();
   
   cout << "END OF TESTS" << endl;
   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
void endOfTestMarker(const bool success = true) {
   cout << "TEST " << (success ? "SUCCESSFUL" : "FAILED") << endl;
   cout << "====================" << endl << endl;
}
void printMatrice(ostream& out, const string& msg, const unsigned mI = maxI, const unsigned mJ = maxJ);
void checkMatrice(istream& in, const string& msg, const unsigned mI = maxI, const unsigned mJ = maxJ);


/**
 * Prints a matrix of size mI x mJ to out.  Each line is composed of msg i j
 * @param out  output stream
 * @param msg  message to be printed on each line
 * @param mI   maximum number of lines
 * @param mJ   maximum number of columns
 */
void printMatrice(ostream& out, const string& msg, const unsigned mI, const unsigned mJ)
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
void checkMatrice(istream& in, const string& msg, const unsigned mI, const unsigned mJ)
{
   string mot;
   unsigned a;
   unsigned b;
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
 * Using the default interface of MagicStream (default usage)
 * Creates a files, read it with getline/dumps it to screen and rereads the file checking it integrity.
 * @param test    A message to incorporate as the file header
 * @param header  head of filename
 * @param ext     filename extension
 */
void fileTest(const string& test, const string& header, const string& ext)
{
   const string file(header + ext);
   cout << "Writing matrix to file" << endl;
   {
      oMagicStream ms(file);
      ms << test << endl;
      printMatrice(ms, header);
   }
   cout << endl;
   
   cout << "Getline from file" << endl;
   {
      string line;
      iMagicStream ms(file);
      while(getline(ms, line))
         cout << line << endl;
   }
   cout << endl;
   
   cout << "testing integrity of file" << endl;
   {
      iMagicStream ms(file);
      string line;
      getline(ms, line);
      assert(line == test);
      checkMatrice(ms, header);
   }
   cout << endl;
}

/**
 * Using the file handle interface of MagicStream
 * Creates a files and reads the file checking it integrity.
 * @param test    A message to incorporate as the file header
 * @param header  head of filename
 * @param ext     filename extension
 */
void fileHandleTest(const string& test, const string& header, const string& ext)
{
   const string file(header + ext);
   cerr << "Testing file handle with: " << file << endl;
   FILE* f(0);
   cout << "Writing matrix to file" << endl;
   {
      f = fopen(file.c_str(), "w");
      oMagicStream ms(f, true);
      ms << test << endl;
      printMatrice(ms, header);
   }
   cout << endl;
   
   /*cout << "Getline from file" << endl;
   {
      string line;
      f = fopen(file.c_str(), "r");
      iMagicStream ms(f, true);
      while(getline(ms, line))
         cout << line << endl;
   }
   cout << endl;*/
   
   cout << "testing integrity of file" << endl;
   {
      f = fopen(file.c_str(), "r");
      iMagicStream ms(f, true);
      string line;
      getline(ms, line);
      assert(line == test);
      checkMatrice(ms, header);
   }
   
   delete_if_exists(file.c_str(), "Cleaning up");
   cout << endl;
   endOfTestMarker();
}

/**
 * Using the file descriptor interface of MagicStream
 * Creates a files and reads the file checking it integrity.
 * @param test    A message to incorporate as the file header
 * @param header  head of filename
 * @param ext     filename extension
 */
void fileDescriptorTest(const string& test, const string& header, const string& ext)
{
   const string file(header + ext);
   cerr << "Testing file descriptor" << endl;
   FILE* f(0);
   cout << "Writing matrix to file" << endl;
   {
      f = fopen(file.c_str(), "w");
      oMagicStream ms(fileno(f), false);
      ms << test << endl;
      printMatrice(ms, header);
   }
   cout << endl;
   if (fclose(f)) cerr << "file descriptor already closed" << endl;
   else cerr << "file descriptor closed" << endl;
   
   /*cout << "Getline from file" << endl;
   {
      string line;
      f = fopen(file.c_str(), "r");
      iMagicStream ms(fileno(f), true);
      while(getline(ms, line))
         cout << line << endl;
   }
   cout << endl;*/
   
   cout << "testing integrity of file" << endl;
   {
      f = fopen(file.c_str(), "r");
      iMagicStream ms(fileno(f), true);
      string line;
      getline(ms, line);
      assert(line == test);
      checkMatrice(ms, header);
   }
   if (fclose(f)) cerr << "file descriptor already closed" << endl;
   else cerr << "file descriptor closed" << endl;
   delete_if_exists(file.c_str(), "Cleaning up");
   cout << endl;
   endOfTestMarker();
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
 * Main set of tests for i/oMagicStream.
 * Tests text files, gz files and pipes
 */
void magicStreamTest()
{
   const string header("MagicStream");
   cout << "MagicStream testing" << endl;
   
   cout << "Trying standard out" << endl;
   {
      cout << "Writing ..." << endl;
      {
         oMagicStream ms("-");
         ms << "Writing to the magic stream stdout" << endl;
         ms << 330033 << endl;
         printMatrice(ms, "stdout");
      }
      cout << endl;
      
      cout << "Reading ..." << endl;
      {
         iMagicStream ms("-");
         int tst;
         cout << "Please input a number to test the MagicStream in standard input mode: ";
         ms >> tst;
         cout << "You inputed: " << tst << endl;
         
         cout << "Please input some random text and press (CTRL+D) when finished: ";
         string line;
         while(getline(ms, line))
            cout << line << endl;
      }
      cout << endl;
   }
   cout << endl;

   cout << "Trying plain text file" << endl;
   {
      fileTest("Writing in the magic stream TEXT file :)", header, ".txt");
   }
   cout << endl;


   cout << "Trying gz file" << endl;
   {
      const string test("Writing in the magic stream file ZIPPED :)");
      fileTest(test, header, ".gz");
   
      // This is only valid in input mode
      cout << "Testing indirect fallback to .gz files" << endl;
      {
         iMagicStream ms(header);
         string line;
         getline(ms, line);
         assert(line == test);
         checkMatrice(ms, header);
      }
   }
   cout << endl;
   

   cout << "Trying output piped to head" << endl;
   {
      oMagicStream ms("| head -n3");
      printMatrice(ms, "head");
   }

   delete_if_exists(string(header + ".txt").c_str(), "Cleaning up");
   delete_if_exists(string(header + ".gz").c_str(), "Cleaning up");
   cout << endl;
   endOfTestMarker();
}

/**
 * Can't remember the nature of this test :(
 * Something to do with transfering infos from input pipe to an output pipe
 */
void linkTest(const string& icmd)
{
   const string header("link");
   cout << "Beginning link test" << endl;
   
   cout << "Creating " << header << ".txt" << endl;
   {
      oMagicStream ms(header + ".txt");
      printMatrice(ms, header);
   }
   cout << endl;
   
   cout << "Trying custom linking test, look for results in:" << header << ".gz" << endl;
   {
      iMagicStream ims(icmd);
      oMagicStream oms(header + ".gz");
      string line;
      while (getline(ims, line))
         oms << line << endl;
   }
   cout << endl;
   endOfTestMarker();
}

/**
 * Basically testing the open and close of MagicStream
 */
void OpenCloseTest()
{
   const string header("status");
   cout << "Beginning open and close test" << endl;
   
   const string file(header + ".txt");

   cout << "open close testing" << endl;
   {
      string test;
      oMagicStream ms;
      cout << "oMagicStream declaration: " << endl;
      PrintStatus(ms);

      ms.open(file);
      cout << "oMagicStream open: " << endl;
      PrintStatus(ms);
      
      ms << header << endl;
      ms.close();
      cout << "oMagicStream close: " << endl;
      PrintStatus(ms);
   }
   cout << endl;

   cout << "Multiple open and close writing" << endl;
   {
      for (unsigned t(0); t<10; ++t)
      {
         oMagicStream ms;
         ms.open(file);
         PrintStatus(ms);
         ms << "open close test " << t << endl;
         ms.close();
      }
   }
   cout << endl;

   cout << "Multiple open and close reading" << endl;
   {
      iMagicStream ms;
      for (unsigned t(0); t<10; ++t)
      {
         string test;
         ms.open(file);
         getline(ms, test);
         cout << t << "Read: " << test << endl;
         ms.close();
      }
   }
   cout << endl;
   delete_if_exists(file.c_str(), "Cleaning up");
   endOfTestMarker();
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
   cout << "| echo" << (check_if_exists("| echo") ? "TRUE" : "FALSE") << endl;
   IMagicStream is("unexisting.file");
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
      OMagicStream ms(file_gz);
      ofstream os(file_txt.c_str());
      printMatrice(ms, msg, maxI, maxJ);
      printMatrice(os, msg, maxI, maxJ);
   }

   cout << "gz format" << endl;
   IMagicStream ms(file_gz);
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
 * Tests the file ignore capabilities of MagicStream.
 * This test was added for binlm
 */
void fileIgnore()
{
   cout << endl << "File Ignoring test" << endl;
   const unsigned int maxI(1);
   const unsigned int maxJ(3);
   const string msg("FileIgnore");
   const string file_gz("fileIgnore.gz");
   const string file_txt("fileIgnore.txt");
   {
      OMagicStream ms(file_gz);
      ofstream os(file_txt.c_str());
      printMatrice(ms, msg, maxI, maxJ);
      printMatrice(os, msg, maxI, maxJ);
   }

   cout << "gz format" << endl;
   IMagicStream ms(file_gz);
   if (!ms.ignore(msg.size()))
      cout << "Ignoring gz failed" << endl;
   else
      cout << "Succes\n" << ms.rdbuf();

   cout << "txt format" << endl;
   ifstream is(file_txt.c_str());
   if (!is.ignore(msg.size()))
      cout << "Ignoring txt failed" << endl;
   else
      cout << "Succes\n" << is.rdbuf();

   delete_if_exists(file_gz.c_str(), "Cleaning up");
   delete_if_exists(file_txt.c_str(), "Cleaning up");
   endOfTestMarker();
}

/**
 * Helper function to write a vector to a stream.
 */
template<typename T>
ostream& operator<<(ostream& os, const vector<T>& v) {
   unsigned int s(v.size());
   os.write((char*)&s, sizeof(unsigned int));
   if (s>0)
      os.write((char*)&v[0], s*sizeof(T));
   bool last(false);   
   os.write((char*)&last, sizeof(last));
   return os;
}

/**
 * Helper function to read a vector from a stream.
 */
template<typename T>
istream& operator>>(istream& is, vector<T>& v) {
   Uint s(0);
   is.read((char*)&s, sizeof(unsigned int));
   if (s>0) {
      v.resize(s);
      is.read((char*)&v[0], s*sizeof(T));
   }
   else {
      v.clear();
   }
   bool last(true);
   is.read((char*)&last, sizeof(last));
   assert(last == false);
   return is;
}

/**
 * Writing binairy .gz file and testing the integrity of the file.
 */
void MagicStreamDotWrite()
{
   cout << "Beginning MagicStreamDotWrite test" << endl;

   const string filename("delme.MagicStreamDotWrite.bin.gz");
   const string message("je fais un test pour debugger MagicStream.write");
   const Uint vSize(300);
   const Uint vValue(23);
   vector<Uint> vecteur(vSize, vValue);
   {
      oMagicStream oms(filename);
      oms << message << endl;
      oms << vecteur;
   }

   {
      iMagicStream ims(filename);
      string test;
      getline(ims, test);
      assert(test == message);
      ims >> vecteur;
      for (vector<Uint>::const_iterator value(vecteur.begin()); value!=vecteur.end(); ++value) {
         assert(*value == vValue);
      }
      //copy(vecteur.begin(), vecteur.end(), ostream_iterator<int>(cerr, " "));
   }

   delete_if_exists(filename.c_str(), "Cleaning up after MagicStreamDotWrite");
   cerr << endl;
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

