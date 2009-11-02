// $Id$
/**
 * @author Write your name here
 * @file prog.cc
 * @brief Briefly describe your program here.
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
prog [options] [INFILE [OUTFILE]]\n\
\n\
  Copy INFILE to OUTFILE (default stdin to stdout).\n\
\n\
Options:\n\
\n\
  -v    Write progress reports to cerr.\n\
  -n NL Copy only first NL lines [0 = all]\n\
";

// globals

static bool verbose = false;
static Uint num_lines = 0;
static string infile("-");
static string outfile("-");
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   iSafeMagicStream istr(infile);
   oSafeMagicStream ostr(outfile);

   Uint lineno = 0;
   string line;
   while (getline(istr, line) && (lineno++ < num_lines || num_lines == 0))
      ostr << line << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("n", num_lines);

   arg_reader.testAndSet(0, "infile", infile);
   arg_reader.testAndSet(1, "outfile", outfile);
}
