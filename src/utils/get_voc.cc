/**
 * @author George Foster
 * @file get_voc.cc  Get vocabulary (unique words) from tokenized text on stdin.
 * 
 * 
 * COMMENTS: 
 *
 * Get vocabulary (unique words) from tokenized text on stdin.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include "file_utils.h"
#include "voc.h"
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
get_voc [-vc] [infile [outfile]]\n\
\n\
Get vocabulary (unique words) from tokenized text on stdin.\n\
This is the same as piping through:\n\
\n\
   tr ' ' '\\n' | egrep -v '^[ ]*$' | sort -u\n\
\n\
except that it's faster and not locale dependent (except for blanks and newlines).\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-c  Write count for each word\n\
";

// globals

static bool verbose = false;
static bool countwords = false;
static Uint num_lines = 0;
static iMagicStream ifs;
static oMagicStream ofs;
static istream* isp = &cin;
static ostream* osp = &cout;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "get_voc");
   getArgs(argc, argv);
   istream& is = *isp;
   ostream& os = *osp;

   CountingVoc voc;
   
   Uint lineno = 0;
   string line;
   vector<string> toks;

   while (getline(is, line) && (lineno++ < num_lines || num_lines == 0)) {
      if(line.length()==0)
         continue;
      char buf[line.length()+1];
      strcpy(buf, line.c_str());
      char* strtok_state; // state variable for strtok_r
      char* tok = strtok_r(buf, " ", &strtok_state);
      while (tok != NULL) {
         voc.add(tok);
         tok = strtok_r(NULL, " ", &strtok_state); 
      }
   }

   for (Uint i = 0; i < voc.size(); ++i) {
      os << voc.word(i);
      if (countwords) os << " " << voc.freq(i);
      os << endl;
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   char* switches[] = {"v", "c"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("c", countwords);
   
   arg_reader.testAndSet(0, "infile", &isp, ifs);
   arg_reader.testAndSet(1, "outfile", &osp, ofs);
}   
