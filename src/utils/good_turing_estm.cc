/**
 * @author George Foster
 * @file good_turing_estm.cc 
 * @brief Good-Turing frequency estimates.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <vector>
#include "good_turing.h"
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
good_turing_estm -c [infile [outfile]]\n\
\n\
Read frequencies from <infile> and write Good-Turing estimates of frequencies\n\
to <outfile> (output corresponds line-by-line with input).\n\
\n\
Options:\n\
-c  Input is <count freq> pairs rather than plain word frequencies, where count\n\
    gives the number of words having the corresponding frequency.\n\
\n\
";

// globals

static bool verbose = false;
static bool input_has_counts = false;
static Uint num_lines = 0;
static ifstream ifs;
static ofstream ofs;
static istream* isp = &cin;
static ostream* osp = &cout;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "good_turing_estm");
   getArgs(argc, argv);
   istream& is = *isp;
   ostream& os = *osp;

   vector<Uint> freqs, counts;

   Uint lineno = 0;
   string line;

   vector<Uint> count_freq;

   if (input_has_counts) 
      while (getline(is, line) && (lineno++ < num_lines || num_lines == 0)) {
         count_freq.clear();
         if (!split(line, count_freq) || count_freq.size() != 2)
            error(ETFatal, "format problem in line %d", lineno);
         counts.push_back(count_freq[0]);
         freqs.push_back(count_freq[1]);
      }
   else
      while (getline(is, line) && (lineno++ < num_lines || num_lines == 0))
         freqs.push_back(conv<Uint>(line));


   GoodTuring* gt;
   if (input_has_counts)
      gt = new GoodTuring(freqs.size(), &freqs[0], &counts[0]);
   else
      gt = new GoodTuring(freqs.size(), &freqs[0]);

   for (Uint i = 0; i < freqs.size(); ++i) 
      os << gt->smoothedFreq(freqs[i]) << endl;

   delete gt;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* const switches[] = {"v", "c", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("c", input_has_counts);
   arg_reader.testAndSet("n", num_lines);
   
   arg_reader.testAndSet(0, "infile", &isp, ifs);
   arg_reader.testAndSet(1, "outfile", &osp, ofs);
}   
