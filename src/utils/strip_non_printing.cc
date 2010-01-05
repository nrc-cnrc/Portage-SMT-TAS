// $Id$
/**
 * @author Samuel Larkin
 * @file strip_non_printing.cc
 * @brief Strip out non printing charaters except for new line.
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */
#include "MagicStream.h"
#include "errors.h"
#include "arg_reader.h"
#include <iostream>
#include <string>

using namespace Portage;
using namespace std;

/**
 STATES := S R C
 cntrl := ' ' & iscntrl  // Control characters and space aka the space.
 reg := !cntrl

 S > \n / \n > S
 S > !reg / > S
 S > reg / reg > R

 R > reg / reg > R
 R > \n / \n > S
 R > !reg / > C

 C > !reg / > C
 C > \n / \n > S
 C > reg / ' 'reg > R
*/

static char help_message[] = "\n\
strip_non_printing [options] [INFILE [OUTFILE]]\n\
\n\
  Copy INFILE to OUTFILE (default stdin to stdout) and remove non printing\n\
  characters except for newline.  Also collapse multiple space and/or non\n\
  printing characters to a single white space.\n\
\n\
Options:\n\
\n\
  none.\n\
";

// globals

static bool verbose = false;
static string infile("-");
static string outfile("-");

enum state { start, regular, control };

bool isRegular(const char c) {
   return !(c == ' ' || iscntrl(c) || c == '\n');
}

void stripNonPrinting(istream& in, ostream& out) {
   char c = 0;
   state etat = start;
   while (!in.get(c).eof()) {
      //cerr << etat << endl;  // DEBUGGING
      switch (etat) {
      case start:
         if (c == '\n') {
            out << c;
         }
         else if (isRegular(c)) {
            out << c;
            etat = regular;
         }
         break;
      case regular:
         if (c == '\n') {
            out << c;
            etat = start;
         }
         else if (!isRegular(c)) {
            etat = control;
         }
         else {
            out << c;
         }
         break;
      case control:
         if (c == '\n') {
            out << c;
            etat = start;
         }
         else if (isRegular(c)) {
            out << ' ' << c;
            etat = regular;
         }
         break;
      default:
         error(ETFatal, "Invalid state type: %d", etat);
      }
   }
}

static void getArgs(int argc, char* argv[]);

int main(int argc, char* argv[]) {
   getArgs(argc, argv);

   iMagicStream in(infile);
   oMagicStream out(outfile);

   stripNonPrinting(in, out);

   return 0;
}


void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "infile", infile);
   arg_reader.testAndSet(1, "outfile", outfile);
}
