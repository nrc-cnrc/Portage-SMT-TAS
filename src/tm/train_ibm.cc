/**
 * @author George Foster
 * @file train_ibm.cc  Program that trains IBM models 1 and 2.
 * 
 * 
 * COMMENTS: 
 *
 * TODO: add ppx-based break condition
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <file_utils.h>
#include <unistd.h>
#include <arg_reader.h>
#include <printCopyright.h>
#include "tm_io.h"
#include "ibm.h"

using namespace Portage;
using namespace std;

static const char help_message[] = "\n\
train_ibm [-vrm][-i init_model][-s ibm1_model][-n1 iters][-n2 iters][-p thresh]\n\
          [-slen len][-tlen len][-bksize size][-speed s][-beg line][-end line]\n\
   model file1_lang1 file1_lang2 ... fileN_lang1 fileN_lang2\n\
\n\
Train IBM1 and IBM2 models for p(lang2|lang1) from given list of line-aligned\n\
files. Write results to file <model> (ttable) and <model>.pos (posit params).\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-r  Do a pairwise reversal of the input file list, turning it into:\n\
    file1_lang2 file1_lang1 ... fileN_lang2 fileN_lang1\n\
-m  Ignore markup: treat all whitespace-separated strings as tokens\n\
-i  Initialize ttable from <init_model>, rather than compiling it from corpus.\n\
    This means that only the word pairs in <init_model> will be considered as\n\
    potential translations, and their starting probs will come from <init_model>.\n\
    If <init_model>.pos exists, IBM2 params will also be read in from this,\n\
    otherwise they will be initialized from the slen, tlen, and bksize params.\n\
-s  Save IBM1 model to <ibm_model> before IBM2 training starts\n\
-n1 Number of initial IBM1 iterations [5]\n\
-n2 Number of final IBM2 iterations [5]\n\
-p  Prune IBM1 probs < thresh after each IBM1 iteration [1e-06]\n\
-slen   Max source length for standard IBM2 pos table [50]\n\
-tlen   Max target length for standard IBM2 pos table [50]\n\
-bksize Size of IBM2 backoff pos table [50] \n\
-speed  For initial pass: 1 (slower, smaller) or 2 (faster, bigger) [2]\n\
-beg Beginning line in each file [1]\n\
-end End line in each file [0 = final]\n\
";

// globals

static const char* const switches[] = {"v", "r", "-m", "n1:", "n2:", "i:", "s:", "p:",
			   "slen:", "tlen:", "bksize:", "speed:", "beg:", "end:"};
static ArgReader arg_reader(ARRAY_SIZE(switches), switches, 
			    1, -1, help_message, "-h", true);

static bool verbose = false;
static bool reverse_dir = false;
static bool ignore_markup = false;
static Uint num_iters1 = 5;
static Uint num_iters2 = 5;
static string init_model;
static string ibm1_model;
static string model;
static double pruning_thresh = 1e-06;
static Uint slen = 50;
static Uint tlen = 50;
static Uint bksize = 50;
static Uint speed = 2;
static Uint begline = 1;
static Uint endline = 0;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "train_ibm");
   getArgs(argc, argv);

   string model_pos = IBM2::posParamFileName(model);
   string init_model_pos = IBM2::posParamFileName(init_model);

   if (access(model.c_str(), F_OK) == 0)
      error(ETFatal, "ttable file <%s> exists - won't overwrite", model.c_str());
   if (access(model_pos.c_str(), F_OK) == 0)
      error(ETFatal, "pos file <%s> exists - won't overwrite", model_pos.c_str());

   IBM2* ibm2;
   if (init_model == "")	// completely new model
      ibm2 = new IBM2(slen, tlen, bksize);
   else if (access(model_pos.c_str(), F_OK) != 0) // read IBM1 but not IBM2
      ibm2 = new IBM2(init_model, 0, slen, tlen, bksize);
   else				// read both IBM1 and IBM2
      ibm2 = new IBM2(init_model);
   IBM1* ibm1 = (IBM1*)ibm2;

   ibm1->getTTable().setSpeed(speed);
   
   if (verbose) 
      cerr << "initializing from " << 
	 (init_model == "" ? "corpus" : init_model.c_str()) << ":" << endl;

   string in_f1, in_f2;

   for (Uint iter = 0; iter <= num_iters1 + num_iters2; ++iter) {

      if (iter == 0 && init_model != "") 
	 continue; // skip ttable init if there is an input model

      if (iter == 1)
	 if (verbose) cerr << "beginning training:" << endl;

      if (iter > 0)
	 ibm2->initCounts();

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

	 Uint lineno = 0;
	 while (getline(in1, line1)) {
	    if (!getline(in2, line2)) {
	       error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
		     file1.c_str(), file2.c_str());
	       break;
	    }
	    ++lineno;
	    if (lineno < begline) continue;
	    if (endline != 0 && lineno > endline) break;
	 
	    toks1.clear(); toks2.clear();
	    toks1.push_back(ibm2->nullWord());

	    TMIO::getTokens(line1, toks1, ignore_markup);
	    TMIO::getTokens(line2, toks2, ignore_markup);

	    if (iter == 0)
	       ibm2->add(toks1, toks2);
	    else if (iter <= num_iters1)
	       ibm1->count(toks1, toks2);
	    else
	       ibm2->count(toks1, toks2);

	    if (verbose && lineno % 100000 == 0) 
	       cerr << "line " << lineno << " in " << file1 << "/" << file2 << endl;
	 }
	 
	 if (endline == 0 && getline(in2, line2))
	    error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
		  file1.c_str(), file2.c_str());
      }
      if (iter == 0) {
	 ibm2->compile();
      } else {
	 pair<double,Uint> r = iter <= num_iters1 ? 
	    ibm1->estimate(pruning_thresh) : ibm2->estimate(0.0);
	 if (verbose) 
	    cerr << "iter " << iter << (iter <= num_iters1 ? " (IBM1)" : " (IBM2)") <<
	       ": prev ppx = " << r.first << ", size = " << r.second << " word pairs" << endl;
	 if (ibm1_model != "" && iter == num_iters1)
	    ibm1->write(ibm1_model);

	 // test for break condition here
      }
   }
   ibm2->write(model);
}

// arg processing

void getArgs(int argc, char* argv[])
{
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("r", reverse_dir);
   arg_reader.testAndSet("m", ignore_markup);
   arg_reader.testAndSet("i", init_model);
   arg_reader.testAndSet("s", ibm1_model);
   arg_reader.testAndSet("n1", num_iters1);
   arg_reader.testAndSet("n2", num_iters2);
   arg_reader.testAndSet("p", pruning_thresh);
   arg_reader.testAndSet("slen", slen);
   arg_reader.testAndSet("tlen", tlen);
   arg_reader.testAndSet("bksize", bksize);
   arg_reader.testAndSet("speed", speed);
   arg_reader.testAndSet("beg", begline);
   arg_reader.testAndSet("end", endline);
   
   arg_reader.testAndSet(0, "model", model);
}   
