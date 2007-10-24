/**
 * @author George Foster
 * @file tm/tester.cc  Program that runs regression tests for Portage TM module.
 * 
 * 
 * COMMENTS: 
 *
 * Run all tests for this module.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <arg_reader.h>
#include "tm_io.h"

using namespace Portage;
using namespace std;

static const char help_message[] = "\n\
tester\n\
\n\
Run regression tests for Portage TM module.\n\
\n\
";

static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   //TMIO::test();
}

// arg processing

static const char* switches[] = {"v"};
static ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, -1, help_message, "-h", true);

void getArgs(int argc, char* argv[])
{
   arg_reader.read(argc-1, argv+1);
}   
