/**
 * @author George Foster
 * @file asciify.cc   Program that converts text to ascii
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include "arg_reader.h"
#include <printCopyright.h>

using namespace Portage;
using namespace std;

/// Program asciify usage
static char help_message[] = "\n\
asciify [-v] [infile [outfile]]\n\
\n\
Remove any non-ascii (8th bit set) characters in <infile> by mapping them to hex\n\
codes according to the RFC2396 scheme.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;  ///< Should we print verbose information
static Uint num_lines = 0;    ///< 
static ifstream ifs;          ///< input file
static ofstream ofs;          ///< output file
static istream* isp = &cin;
static ostream* osp = &cout;

/**
 * Parses the command line arguments.
 * @param argc  number of command line arguments
 * @param argv  vector of command line arguments
 */
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2006, "asciify");
   getArgs(argc, argv);
   istream& is = *isp;
   ostream& os = *osp;
   
   string line;
   while (getline(is, line)) {

      for (Uint i = 0; i < line.length(); ++i) {
	 if (line[i] & (unsigned char)0x80)
	    os << '%' << hex << (int)(unsigned char)line[i];
	 else
	    os << line[i];
      }
      os << endl;
   }
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
