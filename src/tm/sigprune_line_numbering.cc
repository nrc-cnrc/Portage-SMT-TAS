// $Id$
/**
 * @author Samuel Larkin
 * @file prog.cc
 * @brief Numbers lines according to Howard's numbering scheme for sigpruning.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include "file_utils.h"
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
sigprune_line_numbering [options] [INFILE [OUTFILE]]\n\
\n\
  Numbers line from INFILE to OUTFILE using a base64 encoding\n\
  (default stdin to stdout).\n\
\n\
Options:\n\
\n\
  -t    Add a tab delimiter after the line number. [don't]\n\
  -v    Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static bool add_tab = false;
static string infile("-");
static string outfile("-");
static void getArgs(int argc, char* argv[]);

// main

static string write_b64( double i )
{
   string result;
   int nd = 0;
   int digit;
   int d[ 20 ];
   if ( i > 0 ) {
      while ( nd < 20 && i > 0 ) {
         digit = fmod( i, 64.0 );
         d[ nd++ ] = digit;
         i = ( i - digit ) / 64.0;
      }
      result.push_back( 'P' + nd );
      while ( nd > 0 ) {
         result.push_back( '0' + d[ --nd ] );
      }
   } else {
      i = -i;
      while ( nd < 20 && i > 0 ) {
         digit = fmod( i, 64.0 );
         d[ nd++ ] = digit;
         i = ( i - digit ) / 64.0;
      }
      result.push_back( 'O' - nd );
      while ( nd > 0 ) {
         result.push_back( 'o' - d[ --nd ] );
      }
   }
   return result;
}


int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   iSafeMagicStream istr(infile);
   oSafeMagicStream ostr(outfile);

   Uint lineno = 0;
   string line;
   while (getline(istr, line))
      ostr << write_b64(lineno++) << (add_tab ? "\t" : "") << line << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "t"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("t", add_tab);

   arg_reader.testAndSet(0, "infile", infile);
   arg_reader.testAndSet(1, "outfile", outfile);
}
