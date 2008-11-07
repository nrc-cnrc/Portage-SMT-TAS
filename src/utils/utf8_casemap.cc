/**
 * @author George Foster
 * @file utf8_casemap.cc   Case conversion for UTF8 text.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include "arg_reader.h"
#include "utf8_utils.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
utf8_casemap [-v][-c m] [infile [outfile]]\n\
\n\
Perform letter case conversion of UTF8-encoded <infile>, and write results to\n\
<outfile>. This is locale- and language independent. Any lines that contain\n\
invalid UTF8 are just copied verbatim to <outfile>.\n\
\n\
Options:\n\
\n\
-v  Write warning messages about coding problems to cerr.\n\
-c  The conversion to apply, one of [l]:\n\
    l - lowercase\n\
    u - uppercase\n\
    d - lowercase only the 1st letter on each line\n\
    c - uppercase only the 1st letter on each line\n\
";

// globals

static bool verbose = false;
static char what = 'l';
static string infile("-");
static string outfile("-");
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   iSafeMagicStream istr(infile);
   oSafeMagicStream ostr(outfile);

   UTF8Utils u8;

   string line, msg;
   Uint lineno = 0;
   while (getline(istr, line)) {
      ++lineno;
      switch (what) {
      case 'l':
         cout << u8.toLower(line, line) << endl;
         break;
      case 'u':
         cout << u8.toUpper(line, line) << endl;
         break;
      case 'd':
         cout << u8.decapitalize(line, line) << endl;
         break;
      case 'c':
         cout << u8.capitalize(line, line) << endl;
         break;
      default:
         assert(0);
         break;
      }
      if (verbose && !u8.status(&msg))
         error(ETWarn, "Line %d not converted, error code is %s", lineno, msg.c_str());
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "c:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("c", what);

   if (what != 'l' && what != 'u' && what != 'd' && what != 'c')
      error(ETFatal, "-c value must be one of l, u, d, or c");

   arg_reader.testAndSet(0, "infile", infile);
   arg_reader.testAndSet(1, "outfile", outfile);
}
