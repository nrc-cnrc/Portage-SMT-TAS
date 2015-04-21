/**
 * @author George Foster
 * @file merge_ttables.cc
 * @brief Do a count-based merge of two or more ttables.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include "ttable.h"
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
merge_ttables [options] tt1 [tt2 ...]\n\
\n\
Merge ttables tt1, tt2, ... by adding their probabilities, then renormalizing.\n\
Input ttables may be in Portage text or bin format; the output table is written\n\
to stdout in text format.\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static string countfile;
static vector<string> tt_files;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2013, "prog");
   getArgs(argc, argv);

   Voc voc;  // word <-> index
   map< Uint,map<Uint,float> > ttable;  // s -> t -> prob

   // Use ttable's machinery for reading, but dispense with it thereafter (it's
   // way too finicky, and we're not particularly worried about efficiency
   // here.

   for (Uint i = 0; i < tt_files.size(); ++i) {
      TTable tmp(tt_files[i]);
      for (TTable::WordMapIter p = tmp.beginSrcVoc(); p != tmp.endSrcVoc(); ++p) {
         const TTable::SrcDistn& dist = tmp.getSourceDistn(p->second);
         for (Uint k = 0; k < dist.size(); ++k) {
            const string& tw = tmp.targetWord(dist[k].first);
            ttable[voc.add(p->first.c_str())][voc.add(tw.c_str())] += dist[k].second;
         }
      }
   }
   // normalize and output

   map< Uint,map<Uint,float> >::iterator s;
   for (s = ttable.begin(); s != ttable.end(); ++s) {
      double sum = 0.0;
      map<Uint,float>::iterator t;
      for (t = s->second.begin(); t != s->second.end(); ++t) sum += t->second;
      for (t = s->second.begin(); t != s->second.end(); ++t)
         cout << voc.word(s->first) << ' ' << voc.word(t->first) << ' ' 
              << t->second / sum << endl;
   }

}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, -1, help_message);
   arg_reader.read(argc-1, argv+1);
   arg_reader.testAndSet("v", verbose);
   arg_reader.getVars(0, tt_files);
}
