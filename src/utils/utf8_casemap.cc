/**
 * @author George Foster
 * @file utf8_casemap.cc 
 * @brief Case conversion for UTF8 text.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"
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

#ifndef NOICU
/**
 * Convert the casing for an input file.
 * @arg convet  What UTF8Utils function to use to convert the casing.
 */
void process(string& (UTF8Utils::*convert)(const string&, string&)) {
   iSafeMagicStream istr(infile);
   oSafeMagicStream ostr(outfile);

   UTF8Utils u8;

   string line, msg;
   Uint lineno = 0;
   while (getline(istr, line)) {
      ++lineno;
      ostr << (u8.*convert)(line, line) << endl;

      if (verbose && !u8.status(&msg))
         error(ETWarn, "Line %d not converted, error code is %s", lineno, msg.c_str());
   }
}
#endif

// main
int main(int argc, char* argv[])
{
   printCopyright(2007, "utf8_casemap");
   getArgs(argc, argv);

#ifdef NOICU

   error(ETFatal,
      "Compilation with ICU was disabled at build time.\n"
      "To use this program, install ICU, edit the ICU variable in\n"
      "PORTAGEshared/src/Makefile.user-conf and recompile.");

#else

   switch (what) {
   case 'l':
      process(&UTF8Utils::toLower);
      break;
   case 'u':
      process(&UTF8Utils::toUpper);
      break;
   case 'd':
      process(&UTF8Utils::decapitalize);
      break;
   case 'c':
      process(&UTF8Utils::capitalize);
      break;
   default:
      assert(0);
      break;
   }

#endif // NOICU

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
