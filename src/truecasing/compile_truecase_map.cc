/**
 * @author George Foster
 * @file compile_truecase_map.cc
 * @brief Compile a truecase map in the format expected by truecase.pl.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <string_hash.h>
#include <map>
#include <file_utils.h>
#include <str_utils.h>
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
compile_truecase_map [options] TFILES LFILES\n\
\n\
Compile a truecase map in the format expected by truecase.pl. TFILES is a list\n\
of files having the desired (true) casing; LFILES is a list of the same files\n\
converted to lowercase. The two lists must match line for line and token for\n\
token. The resulting map is written to stdout in the format required for the\n\
SRILM disambig program.\n\
\n\
Options:\n\
\n\
-v   Write progress reports to cerr.\n\
-a   Convert tokens in TFILES to lowercase internally; LFILES is empty.\n\
     Warning: may not work properly for characters outside 7-bit ASCII.\n\
";

// globals

static bool verbose = false;
static bool auto_lc = false;


static string infile("-");
static string outfile("-");
static vector<string> tfiles;
static vector<string> lfiles;

static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2008);
   getArgs(argc, argv);

   // lc -> (uc,count), (uc, count) ...
   unordered_map< string, map<string,Uint> > vocmap;

   for (Uint i = 0; i < tfiles.size(); ++i) {
      iSafeMagicStream tin(tfiles[i]);
      iSafeMagicStream* lin = NULL;
      if (!auto_lc) lin = new iSafeMagicStream(lfiles[i]);

      Uint line_num = 0;
      string line;
      vector<string> ttoks, ltoks;

      while (getline(tin, line)) {

         ++line_num;
         splitZ(line, ttoks);
         
         if (!auto_lc) {
            if (!getline((*lin), line))
               error(ETFatal, "lowercase file %s too short", lfiles[i].c_str());
            splitZ(line, ltoks);
            if (ltoks.size() != ttoks.size())
               error(ETFatal, "line length mismatch at line %d in %s", 
                     line_num, tfiles[i].c_str());
         } else {
            ltoks.resize(ttoks.size());
            for (Uint j = 0; j < ttoks.size(); ++j)
               toLower(ttoks[j], ltoks[j]);
         }
            
         for (Uint j = 0; j < ltoks.size(); ++j) {
            vocmap[ltoks[j]][ttoks[j]]++;
         }
      }
   }

   unordered_map< string, map<string,Uint> >::const_iterator p;
   for (p = vocmap.begin(); p != vocmap.end(); ++p) {
      Uint total = 0;
      for (map<string,Uint>::const_iterator pw = p->second.begin(); pw != p->second.end(); ++pw) 
         total += pw->second;
      cout << p->first;

      for (map<string,Uint>::const_iterator pw = p->second.begin(); pw != p->second.end(); ++pw)
         cout << '\t' << pw->first << '\t' << pw->second / (double) total;
      cout << endl;
   }
   
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "a"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("a", auto_lc);

   arg_reader.getVars(0, tfiles);

   if (!auto_lc) {
      if (tfiles.size() % 2)
         error(ETFatal, "TFILES and LFILES lists must contain same number of files");
      lfiles.assign(tfiles.begin() + tfiles.size() / 2, tfiles.end());
      tfiles.erase(tfiles.begin() + tfiles.size() / 2, tfiles.end());
   }
}
