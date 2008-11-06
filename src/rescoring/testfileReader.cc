/*
 * @author Samuel Larkin
 * @file testfileReader.cc  Program test unit for testing class
 * FileReader::{FixReader,DynamicReader}
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
*/
            
#include <iostream>
#include <fileReader.h>

using namespace std;

void sentenceBased(Portage::FileReader::FileReaderBase<string>& reader)
{
   cout << "SENTENCE BASED" << endl;
   unsigned int n(0);
   string s;
   while (reader.pollable())
   {
      cout << "LOOP" << endl;
      while (reader.poll(s, &n))
      {
         cout << n << " : " << s << endl;
      }
      cout << n << " : " << s << endl;
   }
   cout << endl;
}

void nbestListBased(Portage::FileReader::FileReaderBase<string>& reader)
{
   cout << "NBESTLIST BASED" << endl;
   Portage::FileReader::FileReaderBase<string>::Group s;
   while (reader.pollable())
   {
      cout << "LOOP" << endl;
      reader.poll(s);
      cout << s.size() << endl;
   }
   cout << endl;
}

int main(const int argc, const char * const argv[])
{
   typedef unsigned int Uint;
   if (argc > 1 && !strcmp(argv[1], "-h")) {
      cerr << "Usage: testfileReader <F1> <F2>" << endl;
      cerr << "Unit test for file reader." << endl;
      cerr << "<F1> dynamic nbest list file." << endl;
      cerr << "<F2> fix nbest list file." << endl;
      exit(1);
   }

   if (argc < 3) return -1;

   if (true) {
      Portage::FileReader::DynamicReader<string> reader(argv[1], 10);
      sentenceBased(reader);
   }

   if (true) {
      Portage::FileReader::DynamicReader<string> reader(argv[1], 10);
      nbestListBased(reader);
   }

   if (true)
   {
      Portage::FileReader::FixReader<string> reader(argv[2], 3);
      sentenceBased(reader);
   }

   if (true) {
      Portage::FileReader::FixReader<string> reader(argv[2], 3);
      nbestListBased(reader);
   }

   /*if (true)
     {
     cout << "CORPUS BASED" << endl;
     Portage::FileReader::matrixCandidate s;
     Portage::FileReader::DynamicReader<string> dr(argv[1], 10);
     dr.poll(s);
     cout << s.size() << endl;
     for (Uint i(0); i<s.size(); ++i)
     {
     cout << i << "->" << s[i].size() << endl;
     }

     if (dr.pollable())
     cout << "OUI" << endl;
     else
     cout << "NON" << endl;
     }

     if (true)  
     {
     cout << "CORPUS BASED" << endl;
     Portage::FileReader::matrixCandidate s;
     Portage::FileReader::FixReader<string> dr(argv[2], 3);
     dr.poll(s);
     cout << s.size() << endl;
     for (Uint i(0); i<s.size(); ++i)
     {
     cout << i << "->" << s[i].size() << endl;
     }

     if (dr.pollable())
     cout << "OUI" << endl;
     else
     cout << "NON" << endl;
     }*/

   return 0;
}
