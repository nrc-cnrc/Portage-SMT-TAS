/**
 * @author George Foster
 * @file utils/tester.cc Program to run class tests.
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
#include <fstream>
#include "arg_reader.h"
#include "quick_set.h"
#include "voc.h"
#include "gfmath.h"

using namespace Portage;
using namespace std;

/*
static char help_message[] = "\n\
tester\n\
\n\
Run class tests.\n\
\n\
Options:\n\
\n\
";
*/

// globals

//static bool verbose = false;
//static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   cerr << "TESTING QuickSet: " << endl;
   cerr << (QuickSet::test() ? "ok" : "failed") << endl;
   cerr << "TESTING Voc: " << endl;
   cerr << (Voc::test() ? "ok" : "failed") << endl;
   cerr << "TESTING CountingVoc: " << endl;
   cerr << (testCountingVoc() ? "ok" : "failed") << endl;

   // Ok, this is terrible, but we still don't have an easy test harness system
   // yet, so I'm cheating and putting this here for manual inspection of the
   // output...  EJJ
   cerr << "TESTING next_power_of_2:" << endl;
   for ( Uint i = 0; i < 18; ++i )
      cerr << i << "->" << next_power_of_2(i) << " ";
   cerr << endl;
   for ( Uint i = Uint(1024)*1024*1024*2 - 1; i < Uint(1024)*1024*1024*2 + 2; ++i )
      cerr << i << "->" << next_power_of_2(i) << " ";
   cerr << endl;

   // and I do it again...
   cerr << "TESTING trim(char*):" << endl;
   char test_str[] = " \t asd qwe\t asdf \tasdf \t ";
   char test_str_answer[] = "asd qwe\t asdf \tasdf";
   cerr << ( strcmp(trim(test_str), test_str_answer) == 0 ? "ok" : "failed") << " ";
   char test_str2[] = "";
   cerr << ( strcmp(trim(test_str2), "") == 0 ? "ok" : "failed") << " ";
   char test_str3[] = "asdf";
   cerr << ( strcmp(trim(test_str3), "asdf") == 0 ? "ok" : "failed") << endl;
      
}

// arg processing

/*
void getArgs(int argc, char* argv[])
{
   const char* const switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 0, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
}   
*/
