/**
 * @author George Foster
 * @file bin_class_stats.cc  Generate statistics for binary classification results.
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
#include <str_utils.h>
#include <bc_stats.h>
#include <bootstrap.h>
#include <arg_reader.h>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
bin_class_stats [-r][-c ci][-s ns][-nce][-pr][-acc][-roc] [raw_data [stats]]\n\
\n\
Compute summary stats (CER, IROC, NCE), with error bounds, on binary classification\n\
results. Input should be in two columns: the 1st is the model output (probability\n\
of correctness or score monotonic with it); and the 2nd is the true class of the\n\
example, 0 for incorrect or 1 for correct. (If there are 3 cols, a la JHU CE wkshp,\n\
the middle one is ignored).\n\
\n\
Options:\n\
\n\
-v   Write progress reports on stderr.\n\
-r   Scores in input are regression, ie, lower are better and don't\n\
     calculate nce [higher are better, and do]\n\
-c   Use a confidence interval of ci [.95]\n\
-s   Use ns resampling runs, 0 means don't calculate conf intervals [10]\n\
-nce Calculate only the NCE measure (not compatible with -r) [do all]\n\
     Note that this measure is nominally in [0,1], but may also be < 0.\n\
-pr|-acc|-roc  Write pointwise precision/recall, accuracy, or ROC stats\n\
     to stdout, instead of normal output (only one of these flags may be\n\
     on at a time).\n\
";

// globals

static bool verbose = false;
static bool regr = false;
static string infile, outfile;
static double ci = 0.95;
static Uint ns = 10;
static bool nce_only;
static bool pr = false;
static bool acc = false;
static bool roc = false;
static ifstream ifs;
static ofstream ofs;
static istream* isp = &cin;
static ostream* osp = &cout;
static void getArgs(int argc, char* argv[]);

typedef pair<double,Uint> ScoreClass;
typedef vector<ScoreClass>::iterator SCIter;

vector<double> bcstatsAll(SCIter beg, SCIter end) 
{
   BCStats bcs(beg, end, !regr);
   vector<double> res;
   res.push_back(bcs.minCer());
   res.push_back(bcs.iroc());
   res.push_back(bcs.maxF());
   if (!regr)
      res.push_back(bcs.nce());
   return res;
}

double bcstatsNCE(SCIter beg, SCIter end) 
{
   BCStats bcs(beg, end, !regr);
   return bcs.nce();
}

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);
   istream& is = *isp;
   ostream& os = *osp;

   // read raw file contents

   vector<ScoreClass> results;
   vector<string> toks;
   string line;
   Uint ntot = 0;
   while (getline(is, line)) {
      ++ntot;
      Uint num_fields = split(line, toks);
      if (num_fields < 2 || num_fields > 3)
	 error(ETFatal, "line %d does not contain 2 or 3 entries", ntot);
      string& corr = num_fields == 2 ? toks[1] : toks[2];
      ScoreClass p;
      if (!conv(toks[0], p.first) || !conv(corr, p.second))
	 error(ETFatal, "bad entries on line %d", ntot);
      if (regr) p.first = -p.first; // regression is on error measures
      results.push_back(p);
      toks.clear();
   }

   if (results.size() == 0)
      return 0;

   // calculate stats

   BCStats bcs(results.begin(), results.end(), !regr);

   if (pr) bcs.precrecs();
   if (acc) bcs.accs();
   if (roc) bcs.rocs();
   if (pr || acc || roc)
      exit(0);

   vector<double> res(4);	// cer, iroc, maxf, optionally nce
   if (ns) {			// do resampling for conf intervals
      if (!nce_only)
	 bootstrapConfInterval(results.begin(), results.end(), 
			       bcstatsAll, res, ci, ns);
      else
	 res[3] = bootstrapConfInterval(results.begin(), results.end(), 
					bcstatsNCE, ci, ns);
   }

   if (!nce_only) {
      os << "sample size = " << results.size() << ", " << bcs.n1 << " correct"
	 << " (" << 100.0 * bcs.p1 << "%)" << endl;
      os << "CER  = " << 100.0 * bcs.minCer() << "% +- " << 100.0 * res[0] << endl;
      os << "IROC = " << bcs.iroc() << " +- " << res[1] << endl;
      os << "F    = " << bcs.maxF() << " +- " << res[2] << endl;
   }
   if (!regr)
      os << "NCE  = " << 100.0 * bcs.nce() << "% +- " << 100.0 * res[3] << endl;

}

// arg processing

void getArgs(int argc, char* argv[])
{
   char* switches[] = {"v", "r", "c:", "s:", "nce", "pr", "acc", "roc"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("r", regr);
   arg_reader.testAndSet("c", ci);
   arg_reader.testAndSet("s", ns);
   arg_reader.testAndSet("nce", nce_only);
   arg_reader.testAndSet("pr", pr);
   arg_reader.testAndSet("acc", acc);
   arg_reader.testAndSet("roc", roc);

   if ((pr || acc || roc) && (pr && acc || pr && roc || acc && roc))
      error(ETFatal, "only one of -pr, -acc, or -roc can be specified at a time");
   
   arg_reader.testAndSet(0, "raw_data", &isp, ifs);
   arg_reader.testAndSet(1, "stats", &osp, ofs);

   if (nce_only && regr) {
      error(ETWarn, "ignoring -r flag because -nce is set");
      regr = false;
   }

}   
