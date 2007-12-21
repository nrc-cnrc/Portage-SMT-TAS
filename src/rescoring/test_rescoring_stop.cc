/**
 * @author George Foster
 * @file test_rescoring_stop.cc 
 * 
 * 
 * COMMENTS: 
 *
 * Test rescoring stop condition strategy
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <queue>
#include <stack>
#include <algorithm>
#include <cmath>
#include <gfstats.h>
#include "docid.h"
#include "arg_reader.h"
#include "file_utils.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
test_rescoring_stop [-v][-s strategy] [infile]\n\
\n\
Read a sequence of bleu scores from <infile>, and determine the stopping point according to\n\
the given strategy.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-s  Use <strategy> to determine stopping point, on of:\n\
    opt     - choose best score in list\n\
    std     - standard rescore_train doubling strategy\n\
    norm:n  - use normal approximation to estimate expected\n\
              number of iters required to exceed current max;\n\
              stop if total iters required for this is > n\n\
    [opt]\n\
";

// globals

static bool verbose = false;
static string strategy = "opt";
static string infile = "-";

static void getArgs(int argc, char* argv[]);

static const unsigned int NUM_INIT_RUNS(7);
static const double FTOL(0.01);
static const double BLEUTOL(0.0001);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   double best_score(-INFINITY);
   double extend_thresh(0.0);   // bleu threshold for extending total_runs
   Uint num_runs(0);
   Uint total_runs = NUM_INIT_RUNS+2;
   Uint best_run = 0;

   if (isPrefix("norm", strategy.c_str())) {
      vector<string> toks;
      if (split(strategy, toks, " :") != 2)
         error(ETFatal, "expecting norm strategy in format norm:n, where n is max iters");
      total_runs = conv<Uint>(toks[1]);
   }

   vector<double> bleu_history;

   IMagicStream instr(infile);

   while (num_runs < total_runs) {

      string line;
      if (!getline(instr, line))
         break;

      double fret = log(conv<double>(line));
      ++num_runs;
      bleu_history.push_back(exp(fret));

      if (fret > best_score) {
         best_score = fret;
         best_run = num_runs;
      }

      if (strategy == "opt") {
         total_runs = num_runs+1;
      } else if (strategy == "std") {
         if (exp(fret) > extend_thresh) {
            total_runs = max(num_runs+NUM_INIT_RUNS+2, 2 * num_runs); // bizarre for bkw compat
            extend_thresh = exp(fret) + BLEUTOL;
         }
      } else if (isPrefix("norm", strategy.c_str())) {
         if (bleu_history.size() > NUM_INIT_RUNS+2) {
            double m = mean(bleu_history.begin(), bleu_history.end());
            double s = sdev(bleu_history.begin(), bleu_history.end(), m);
            Uint expected_iters = Uint(1.0 / (1.0 - normalCDF(exp(best_score), m, s)));
//             cerr << "mean = " << m 
//                  << ", sdev = " << s 
//                  << ", curr max = " << exp(best_score)
//                  << ", expiters = " << expected_iters << endl;

            if (num_runs + expected_iters > total_runs)
               total_runs = num_runs;
         }
      } else
         error(ETFatal, "unknown strategy: " + strategy);
   }

   if (strategy == "opt")
      total_runs = best_run;
   
   cout << "runs = " << total_runs << ", score = " << exp(best_score) << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "s:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("s", strategy);

   arg_reader.testAndSet(0, "infile", infile);
}
