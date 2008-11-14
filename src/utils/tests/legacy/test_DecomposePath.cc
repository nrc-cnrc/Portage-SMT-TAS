/**
 * @author Samuel Larkin
 * @file test_DecomposePath.cc  Program that test the DirName BaseName functionalities.
 *
 * COMMENTS: DirName and BaseName should output the same things as man basename and dirname example

 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
               
#include "file_utils.h"

using namespace Portage;
using namespace std;

void go(const char* fn)
{
   string p, f;
   DecomposePath(fn, &p, &f);
   cout << fn << " | " << p << " | " << f << endl;
   
   cout << fn << " | " << DirName(fn) << " | " << BaseName(fn) << endl << endl;
}

int main()
{
   go("/usr/test");
   go("/usr/");
   go("/");
   go(".");
   go("..");
   go("usr");
   go("./usr");

   return 0;
}
