/**
 * @author Samuel Larkin
 * @file test_split.cc  Simple test pgm for split
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include <str_utils.h>
#include <voc.h>
#include <vector>
#include <string>
#include <iostream>


using namespace std;
using namespace Portage;

void print(const Voc& voc)
{
   cout << "voc.size: " << voc.size() << " => ";
   voc.write(cout, ", ");
   cout << endl;
}

void print(const vector<string>& v)
{
   typedef vector<string>::const_iterator IT;
   for (IT it(v.begin()); it!=v.end(); ++it)
      printf("[%d]%s ", (Uint)it->size(), it->c_str());
   cout << endl;
}

template<class T>
void print(const vector<T>& v)
{
   for (Uint i(0); i<v.size(); ++i) 
      cout << v[i] << " ";
   cout << endl;
}

int main()
{
   // STRINGS
   cout << "Testing string" << endl;
   const string my_string("Samuel   Larkin est grand");
   vector<string> fields;
   cout << "char: " << split(my_string.c_str(), fields) << endl;
   print(fields);
   cout << "string: " << split(my_string, fields) << endl;
   print(fields);
   cout << "string: " << splitZ(my_string, fields) << endl;
   print(fields);
   cout << "string: " << splitZ(my_string, fields) << endl;
   print(fields);
   cout << "string: " << splitZ(my_string, fields) << endl;
   print(fields);
   cout << "string: " << splitCheckZ(my_string, fields, " \t\n", 3) << endl;
   print(fields);
   cout << endl;

   // FLOATS
   cout << "Testing float" << endl;
   string my_float("12   4 -2.5");
   vector<float> fieldf;
   cout << "float: " << split(my_float, fieldf) << endl;
   print(fieldf);
   cout << endl;

   // VOCABULARY
   cout << "Testing vocabulary" << endl;
   Voc voc;
   Voc::addConverter aConverter(voc);
   vector<Uint> fieldi;
   cout << "voc: " << split(my_string.c_str(), fieldi, aConverter) << endl;
   print(fieldi);
   print(voc);
   cout << "Voc2: " << split("Samuel   mange des fruits pour dejeuner", fieldi, aConverter) << endl;
   print(fieldi);
   print(voc);
   cout << endl;

   return 0;
}
