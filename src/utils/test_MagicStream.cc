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
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
               
#include <MagicStream.h>
#include <file_utils.h>


using namespace Portage;
using namespace std;

const unsigned maxI(4);
const unsigned maxJ(5);

void printMatrice(ostream& out, const string& msg, const unsigned mI = maxI, const unsigned mJ = maxJ);
void checkMatrice(istream& in, const string& msg, const unsigned mI = maxI, const unsigned mJ = maxJ);
void coutModificationTest();
void magicStreamTest();
void linkTest(const string& icmd = "head -n 5 link.txt |tail -n2 |");
void OpenCloseTest();
void fileHandleTest(const string& test, const string& header, const string& ext);
void fileDescriptorTest(const string& test, const string& header, const string& ext);

////////////////////////////////////////////////////////////////////////////////
// MAIN
//usage is : executable "optional input pipe command => cmd2 |"
int main(int argc, char* argv[])
{
   if (true)
   {
      cout << (check_if_exists("existing.file") ? "TRUE" : "FALSE") << endl;
      IMagicStream is("unexisting.file");
      return 0;
   }
   
   for (int i(0); i<argc; ++i) {
      cout << "arguments[" << i << "]: " << argv[i] << endl;
   }
   cout << endl;
/*   
   coutModificationTest(); cout << "====================" << endl;
   magicStreamTest(); cout << "====================" << endl;
   if (argc <= 1)
      linkTest();
   else
      linkTest(argv[1]);
   cout << "====================" << endl;

   OpenCloseTest(); cout << "====================" << endl;
   */
   fileHandleTest("Using a FILE*", "file_handle", "txt"); cout << "====================" << endl;
   fileDescriptorTest("Using a file descriptor", "file_descriptor", "txt"); cout << "====================" << endl;
   
   cout << "END OF TESTS" << endl;
   return 0;
}



void printMatrice(ostream& out, const string& msg, const unsigned mI, const unsigned mJ)
{
   for (unsigned i(0); i<mI; ++i)
      for (unsigned j(0); j<mJ; ++j)
         out << msg << " " << i << " " << j << endl;
}

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

void PrintStatus(oMagicStream& ms)
{
   if (ms.is_open()) cout << "\tMagicStream is opened" << endl;
   if (ms.fail()) cout << "\tMagicStream fail" << endl;
   if (ms.good()) cout << "\tMagicStream good" << endl;
}

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

void fileHandleTest(const string& test, const string& header, const string& ext)
{
   cerr << "Testing file handle" << endl;
   const string file(header + ext);
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
   cout << endl;
}

void fileDescriptorTest(const string& test, const string& header, const string& ext)
{
   cerr << "Testing file descriptor" << endl;
   const string file(header + ext);
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
   cout << endl;
   if (fclose(f)) cerr << "file descriptor already closed" << endl;
   else cerr << "file descriptor closed" << endl;
}

void coutModificationTest()
{
   cout << "Modifying the cout.rdbuf" << endl;
   
   filebuf fb;
   streambuf* tmp = cout.rdbuf();
   fb.open("cout.filebuf.txt", ios::out);
   cout.rdbuf(&fb);
   cout << "Using cout to write to a file" << endl;
   fb.close();
   cout.rdbuf(tmp);
   cout << "cout rdbuf has succesfully been reattached" << endl;
   cout << endl;
}

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
   cout << endl;
}

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
}

void OpenCloseTest()
{
   const string header("status");
   cout << "Beginning open and close test" << endl;
   
   cout << "open close testing" << endl;
   {
      string test;
      oMagicStream ms;
      cout << "oMagicStream declaration: " << endl;
      PrintStatus(ms);

      ms.open(header + ".txt");
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
      oMagicStream ms;
      for (unsigned t(0); t<10; ++t)
      {
         ms.open(header + ".txt");
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
         ms.open(header + ".txt");
         getline(ms, test);
         cout << t << "Read: " << test << endl;
         ms.close();
      }
   }
   cout << endl;
}

