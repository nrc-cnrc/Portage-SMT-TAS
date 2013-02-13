// $Id$
/**
 * @author Samuel Larkin
 * @file merge_sigprune_counts.cc
 * @brief Inner merge function for sigprune.sh.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include "file_utils.h"
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
merge_sigprune_counts [options] jpt sigprune_count [sigprune_count ..]\n\
\n\
  Tally significance pruning counts and prepends them to jpt\n\
  and outputs the result to stdout.\n\
\n\
Options:\n\
\n\
  -v    Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static string jptfilename("-");
static vector<string> countfilenames;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   iSafeMagicStream jpt(jptfilename);
   vector<iSafeMagicStream*> counts(countfilenames.size(), NULL);
   for (Uint i(0); i<counts.size(); ++i) {
      counts[i] = new iSafeMagicStream(countfilenames[i]);
      if (counts[i] == NULL)
         error(ETFatal, "Unable to open file %s", countfilenames[i].c_str());
   }
   if (verbose) {
      cerr << "Tallying and prepending "
           << join(countfilenames)
           << " to " << jptfilename << endl;
   }


   string jpt_line;
   while (getline(jpt, jpt_line)) {
      static bool JptFormatNotValidated = true;
      if (JptFormatNotValidated) {
         JptFormatNotValidated = false;
         const char* PHRASE_TABLE_SEP = " ||| ";
         const Uint sep_len = strlen(PHRASE_TABLE_SEP);
         // Look for two or three occurrences of |||.
         const string::size_type index1 = jpt_line.find(PHRASE_TABLE_SEP, 0);
         if (index1 == string::npos)
            error(ETFatal, "%s is not a joint phrase table", jptfilename.c_str());
         const string::size_type index2 = jpt_line.find(PHRASE_TABLE_SEP, index1 + sep_len);
         if (index2 == string::npos)
            error(ETFatal, "%s is not a joint phrase table", jptfilename.c_str());
      }

      if (counts.size() == 1) {
         // There is no merging to be done other than pasting together the counts with the jpt.
         string counts_line;
         if (!getline(*counts[0], counts_line)) {
            error(ETFatal, "%s is too short!", countfilenames[0].c_str());
         }
         cout << counts_line << '\t' << jpt_line << '\n';
      }
      else {
         vector<Uint> total(4, 0);
         for (Uint i(0); i<counts.size(); ++i) {
            string counts_line;
            if (!getline(*counts[i], counts_line)) {
               error(ETFatal, "%s is too short!", countfilenames[i].c_str());
            }
            vector<Uint> data;
            split(counts_line, data);
            if (data.size() != 4) 
               error(ETFatal, "Invalid format in file ", countfilenames[i].c_str());

            for (Uint i(0); i<total.size(); ++i)
               total[i] += data[i];
         }
         cout << '\t' << join(total, "\t") << '\t' << jpt_line << '\n';
      }
   }

   for (Uint i(0); i<counts.size(); ++i)
      if (getline(*counts[i], jpt_line))
         error(ETFatal, "%s is too long!", countfilenames[i].c_str());
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "jpt", jptfilename);
   arg_reader.getVars(1, countfilenames);
}
