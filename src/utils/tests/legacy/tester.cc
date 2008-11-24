/**
 * @author George Foster
 * @file utils/tester.cc
 * @brief Program to run class tests.
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
}

// arg processing

/*
void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 0, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
}   
*/
