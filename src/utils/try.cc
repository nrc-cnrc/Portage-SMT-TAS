/**
 * @author George Foster
 * @file utils/try.cc  Test program.
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
#include <algorithm>
#include "arg_reader.h"
#include "file_utils.h"
#include "bootstrap.h"
#include "gfstats.h"
#include "gfmath.h"
#include "trie.h"
#include "voc.h"
#include "ls_poly_fit.h"
#include "histogram_fit.h"
#include "parse_xmlish_markup.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
try [-v] m\n\
\n\
Just a test...\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static Uint n = 5;
static void getArgs(int argc, char* argv[]);

string line;

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   Uint beg, end;
   XMLishTag tag;

   while (getline(cin, line)) {
      
      const char* p = line.c_str();
      
      while (parseXMLishTag(p, tag, &beg, &end)) {
         cout << tag.toString() << endl;
         p += end;
      }
      if (*(p + beg))
         error(ETFatal, "runaway tag!!");
   }

//       if (parseXMLishTag(line.c_str(), tag, &beg, &end)) {
//          cout << "found at: [" << line.substr(beg, end-beg) << "]" << endl;
//          cout << tag.toString() << endl;
//       } else
//         cout << "no!" << endl;
      
      // string newline;
      // cout << XMLescape(line.c_str(), newline) << endl;
      // cout << XMLunescape(line.c_str(), newline) << endl;

}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* const switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);
   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "n", n);
   
}   
