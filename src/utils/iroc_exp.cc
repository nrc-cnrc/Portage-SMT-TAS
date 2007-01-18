/**
 * @author George Foster
 * @file iroc_exp.cc  Application.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <algorithm>
#include <gfstats.h>
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
iroc_exp [-va][-n num][-n1 npos][-i niter] \n\
\n\
Simulate expected IROC values across randomly drawn orderings of positive and\n\
negative examples.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-a  Use absolute IROC: if IROC < .5, set IROC = 1-IROC [don't]\n\
-n  Assume <num> total examples [10]\n\
-n1 Assume <n1> positive examples [1]\n\
-i  Use <niter> iterations [100]\n\
";

// globals

// static bool verbose = false;
static bool abs_iroc = false;
static Uint num = 10;
static Uint n1 = 1;
static Uint niter = 100;
static void getArgs(int argc, char* argv[]);

static void random_sample(const vector<Uint>& ranks, vector<Uint>& pos_examples)
{
   vector<Uint> ranks_copy(ranks.begin(), ranks.end());
   Uint e = ranks.size();
   for (Uint i = 0; i < pos_examples.size(); ++i) {
      Uint v = rand(e--);	// v in 0..e-1
      pos_examples[i] = ranks_copy[v];
      swap(ranks_copy[v], ranks_copy[e]);
   }
//    for (Uint i = 0; i < pos_examples.size(); ++i)
//       cerr << pos_examples[i] << ' ';
//    cerr << endl;

}

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "iroc_exp");
   getArgs(argc, argv);

   vector<Uint> pos_examples(n1);
   vector<Uint> ranks(num);
   for (Uint i = 0; i < ranks.size(); ++i)
      ranks[i] = i+1;
   
   double iroc_total = 0.0;
   for (Uint i = 0; i < niter; ++i) {
      random_sample(ranks, pos_examples);
      double iroc = 0.0;
      for (Uint j = 0; j < pos_examples.size(); ++j) {
	 int s = pos_examples[j]; // note: signed int!
	 iroc += (s - int(j) - 1) / (double) n1;
      }
      iroc /= (num - n1);
      
      iroc_total += abs_iroc && iroc < .5 ? (1.0-iroc) : iroc;
   }

   cout << iroc_total / niter << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   char* switches[] = {"v", "a", "n:", "n1:", "i:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   // arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("a", abs_iroc);
   arg_reader.testAndSet("n", num);
   arg_reader.testAndSet("n1", n1);
   arg_reader.testAndSet("i", niter);
}   
