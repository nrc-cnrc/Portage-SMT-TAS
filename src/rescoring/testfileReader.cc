/*
 * @author Samuel Larkin
 * @file testfileReader.cc  Program test unit for testing class
 * FileReader::{FixReader,DynamicReader}
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
*/
            
#include <iostream>
#include <fileReader.h>

using namespace std;
int main(const int argc, const char * const argv[])
{
   typedef unsigned int Uint;
   if (argc < 3)
   {
      return -1;
   }
   
   /*if (true)
   {
      cout << "SENTENCE BASED" << endl;
      unsigned int n(0);
      string s;
      Portage::FileReader::DynamicReader dr(argv[1], 10, 10);
      while (dr.pollable())
      {
         cout << "LOOP" << endl;
         while (dr.poll(n, s))
         {
            cout << n << " : " << s << endl;
         }
         cout << n << " : " << s << endl;
      }
   }
   
   if (true)
   {
      cout << "NBESTLIST BASED" << endl;
      Portage::FileReader::groupCandidate s;
      Portage::FileReader::DynamicReader dr(argv[1], 10, 10);
      while (dr.pollable())
      {
         cout << "LOOP" << endl;
         dr.poll(s);
         cout << s.size() << endl;
      }
   }
   
   if (true)
   {
      cout << "CORPUS BASED" << endl;
      Portage::FileReader::matrixCandidate s;
      Portage::FileReader::DynamicReader dr(argv[1], 10, 10);
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
      Portage::FileReader::FixReader dr(argv[2], 3, 3);
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
   
   return 0;*/
}
