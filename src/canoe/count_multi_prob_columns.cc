/**
 * @author George Foster
 * @file count_multi_prob_columns.cc
 * @brief Count the total number of probability columns in one or more
 * multi-column conditional phrasetables.
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include <arg_reader.h>
#include "phrasetable.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
count_multi_prob_columns cpt1 [cpt2 ...]\n\
\n\
Count the total number of probability columns in one or more multi-column\n\
conditional phrasetables.\n\
\n\
Options:\n\
\n\
-w  Write output as a string of canoe.ini weights, eg 1.0:1.0:1.0 for a\n\
    six-column phrasetable.\n\
-a  Count the adirectional feature columns instead of the probability ones.\n\
";

// globals

static bool verbose = false;
static vector<string> infiles;
static bool weights = false;
static bool adir = false;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   Uint n = 0;
   for (Uint i = 0; i < infiles.size(); ++i)
      n += adir ? PhraseTable::countAdirScoreColumns(infiles[i].c_str())
                : PhraseTable::countProbColumns(infiles[i].c_str());

   if (weights) {
      const Uint end = adir ? n : n / 2;
      for (Uint i = 0; i < end; ++i) {
         cout << "1.0";
         if (i+1 < end) cout << ":";
      }
      cout << endl;
   } else
      cout << n << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "w", "a"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("w", weights);
   arg_reader.testAndSet("a", adir);

   arg_reader.getVars(0, infiles);
}
