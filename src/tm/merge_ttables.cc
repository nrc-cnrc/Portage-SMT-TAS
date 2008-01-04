/**
 * @author George Foster
 * @file merge_ttables.cc  Program that merges ttables tt2 and tt2, and write
 * merged table to stdout.
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
#include <file_utils.h>
#include <arg_reader.h>
#include <ttable.h>
#include <printCopyright.h>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
merge_ttables [-v][-n nl] tt1 tt2 \n\
\n\
Merge ttables tt2 and tt2, and write merged table to stdout.\n\
The resulting distributions are uniform.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static string tt1_file;
static string tt2_file;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "merge_ttables");
   getArgs(argc, argv);

   TTable tt1(tt1_file);

   if (verbose)
      cerr << "loaded tt1" << endl;

   iMagicStream istr(tt2_file);
   if (!istr)
      error(ETFatal, "unable to open tt2 file %s", tt2_file.c_str());

   string line;
   vector<string> toks;
   while (getline(istr, line)) {
      split(line, toks);
      tt1.add(toks[0], toks[1]);
      toks.clear();
   }
   tt1.makeDistnsUniform();

   if (verbose)
      cerr << "tables merged" << endl;

   tt1.write(cout);
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* const switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   
   arg_reader.testAndSet(0, "tt1", tt1_file);
   arg_reader.testAndSet(1, "tt2", tt2_file);
}   
