/**
 * @author George Foster
 * @file meanvar.cc  Calculate mean and variance.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#include <iostream>
#include <fstream>
#include <str_utils.h>
#include <bc_stats.h>
#include <bootstrap.h>
#include <arg_reader.h>
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
meanvar [-d][-c ci][-s ns] [in [out]]\n\
\n\
Calc mean and variance of a sample, possibly with bootstrap confidence intervals.\n\
Input is a column of numbers.\n\
\n\
Options:\n\
\n\
-d   Standard deviation instead of variance\n\
-c   Use a confidence interval of ci [.95]\n\
-s   Use ns resampling runs, 0 means don't calculate conf intervals [0]\n\
";

// globals

static string infile, outfile;
static bool do_sdev = false;
static double ci = .95;
static Uint ns = 0;
static ifstream ifs;
static ofstream ofs;
static istream* isp = &cin;
static ostream* osp = &cout;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "meanvar");
   getArgs(argc, argv);
   istream& is = *isp;
   ostream& os = *osp;

   // read raw file contents

   vector<double> numbers;
   string line;
   Uint ntot = 0;
   while (getline(is, line)) {
      ++ntot;
      double val;
      if (!conv(line, val))
	 error(ETFatal, "bad number on line %d", ntot);
      numbers.push_back(val);
   }

   double m = mean(numbers.begin(), numbers.end());
   double v = do_sdev ? 
      sdev(numbers.begin(), numbers.end(), m) :
      var(numbers.begin(), numbers.end(), m);

   typedef vector<double>::iterator NumIter;
   if (ns) {
      double delta_m = bootstrapConfInterval(numbers.begin(), numbers.end(), mean<NumIter>, ci, ns);
      double delta_v = do_sdev ? 
	 bootstrapConfInterval(numbers.begin(), numbers.end(), sdevp<NumIter>, ci, ns) :
	 bootstrapConfInterval(numbers.begin(), numbers.end(), varp<NumIter>, ci, ns);
      os << "mean = " << m << " +- " << delta_m 
	 << (do_sdev ? ", sdev = " : ", var = ") << v << " += " << delta_v << endl;
   } else
      os << "mean = " << m 
	 << (do_sdev ? ", sdev = " : ", var = ") << v << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   char* switches[] = {"d", "c:", "s:", "nce"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("d", do_sdev);
   arg_reader.testAndSet("c", ci);
   arg_reader.testAndSet("s", ns);
   
   arg_reader.testAndSet(0, "input", &isp, ifs);
   arg_reader.testAndSet(1, "output", &osp, ofs);

}   
