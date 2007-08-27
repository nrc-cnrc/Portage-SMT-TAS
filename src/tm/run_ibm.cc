/**
 * @author George Foster
 * @file run_ibm.cc  Program that calculates perplexity of given IBM model for
 * p(lang2|lang2) on list of line-aligned files.
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
#include <cmath>
#include <file_utils.h>
#include <printCopyright.h>
#include "tm_io.h"
#include "arg_reader.h"
#include "ibm.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
run_ibm [-vr][-m i][-s smooth] model\n\
   file1_lang1 file1_lang2 ... fileN_lang1 fileN_lang2\n\
\n\
Calculate perplexity of given IBM model for p(lang2|lang2) on list of\n\
line-aligned files.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-r  Do a pairwise reversal of the input file list, turning it into:\n\
    file1_lang2 file1_lang1 ... fileN_lang2 fileN_lang1\n\
-m  Use model <i>, 1 or 2 [2]\n\
-s  Replace 0 probabilities for target tokens with <smooth> [1e-50]\n\
";

// globals

static bool verbose = false;
static bool reverse_dir = false;
static Uint modelno = 2;
static double smooth = 1e-06;
static string model;


const char* const switches[] = {"v", "r", "m:", "s:"};
static ArgReader arg_reader(ARRAY_SIZE(switches), switches, 
			    1, -1, help_message, "-h", true);

static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "run_ibm");
   getArgs(argc, argv);
   IBM1* ibm = modelno == 1 ? new IBM1(model) : new IBM2(model);

   string in_f1, in_f2;

   double logpr = 0.0;
   Uint num_files = 0, num_segments = 0, num_toks = 0;

   for (Uint arg = 1; arg+1 < arg_reader.numVars(); arg += 2) {
	 
      Uint a1 = reverse_dir ? 1 : 0, a2 = reverse_dir ? 0 : 1;
      string file1 = arg_reader.getVar(arg+a1), file2 = arg_reader.getVar(arg+a2);
      if (verbose)
	 cerr << "reading " << file1 << "/" << file2 << endl;
      arg_reader.testAndSet(arg+a1, "file1", in_f1);
      arg_reader.testAndSet(arg+a2, "file2", in_f2);
      IMagicStream in1(in_f1);
      IMagicStream in2(in_f2);

      string line1, line2;
      vector<string> toks1, toks2;

      while (getline(in1, line1)) {
	 if (!getline(in2, line2)) {
	    error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
		  file1.c_str(), file2.c_str());
	    break;
	 }
	 
	 toks1.clear(); toks2.clear();
	 toks1.push_back(ibm->nullWord());
	 TMIO::getTokens(line1,toks1);
	 TMIO::getTokens(line2,toks2);

	 logpr += ibm->logpr(toks1, toks2, smooth);
	 ++num_segments;
	 num_toks += toks2.size();

      }
	 
      if (getline(in2, line2))
	 error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
	       file1.c_str(), file2.c_str());

      ++num_files;
   }

   cout << "ppx = " << exp(-logpr/num_toks) << ", using model " << "[" << model << "] "
	<< "on " << num_files << " files, " << num_segments << " segments, " << num_toks << " tokens" 
	<< endl;

}

// arg processing

void getArgs(int argc, char* argv[])
{
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("r", reverse_dir);
   arg_reader.testAndSet("m", modelno);
   arg_reader.testAndSet("s", smooth);

   arg_reader.testAndSet(0, "model", model);
}   
