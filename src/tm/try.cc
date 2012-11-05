/**
 * @author George Foster
 * @file tm/try.cc  Program template.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <iomanip>
#include <arg_reader.h>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
try [-v] [-n nl] [infile [outfile]]\n\
\n\
Shell program to do misc compiler / code behaviour experiments.\n\
\n\
Options:\n\
\n\
-v  Dummy verbose switch\n\
-n  Dummy max line switch\n\
";

// globals

static bool verbose = false;
static Uint num_lines = 0;
static ifstream ifs;
static ofstream ofs;
static istream* isp = &cin;
static ostream* osp = &cout;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* const switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("n", num_lines);
   
   arg_reader.testAndSet(0, "infile", &isp, ifs);
   arg_reader.testAndSet(1, "outfile", &osp, ofs);
}   
