/**
 * @author George Foster
 * @file nnjm.cc
 * @brief NNJM testing
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"
#include "nnjm_abstract.h"
#include "timer.h"

#include <boost/scoped_ptr.hpp>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
nnjm [options] model [INFILE [OUTFILE]]\n\
\n\
Test NNJM model on data output by nnjm-genex.py, and write scores.\n\
\n\
\n\
Options:\n\
-s n  Use src window size of n [11].\n\
-n n  Use tgt order of n [4].\n\
-f f  Model format: nrc or udem [nrc]\n\
-native    Switch to native, assumes plain txt model\n\
-selfnorm  Skip normalization, only works with native\n\
-v    Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static bool native = false;
static bool selfnorm = false;
static Uint swin_size = 11;
static Uint ng_size = 4;
static string format = "nrc";
static string modelfile;
static string infile("-");
static string outfile("-");
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2014, "prog");
   getArgs(argc, argv);

   Uint fmt = 0;
   if (format == "nrc")
      fmt = 0;
   else if (format == "udem")
      fmt = 1;
   else
      error(ETFatal, "unknown format string: %s", format.c_str());

   iSafeMagicStream istr(infile);
   oSafeMagicStream ostr(outfile);

   Timer timer;
   boost::scoped_ptr<NNJMAbstract> nn;
   const bool useLookup = true;
   if(!native)
      error(ETFatal, "The PyWrap version of NNJM is not in PortageII yet. -native is required.");
      //nn.reset(NNJMs::new_PyWrap(modelfile, swin_size, ng_size, fmt));
   else
      nn.reset(NNJMs::new_Native(modelfile, selfnorm, useLookup));
   cerr << "Loading model took " << timer.secsElapsed(true) << " seconds." << endl;
   

   string line;
   vector<string> toks;
   vector<Uint> words(swin_size+ng_size+2);
   timer.reset();
   while (getline(istr, line)) {
      splitZ(line, toks);
      for (Uint i = 0; i < toks.size(); ++i)
         if (toks[i][0] != '/')
            words[i] = conv<Uint>(toks[i]);
      vector<Uint>::const_iterator s = words.begin();
      vector<Uint>::const_iterator h = words.begin()+swin_size+1;
      ostr << nn->logprob(s, s+swin_size, h, h+ng_size-1, words.back()) << endl;
   }
   cerr << "Calculating probs took " << timer.secsElapsed(true) << " seconds." << endl;

}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "n:", "s:", "f:", "native", "selfnorm"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("s", swin_size);
   arg_reader.testAndSet("n", ng_size);
   arg_reader.testAndSet("f", format);
   arg_reader.testAndSet("native", native);
   arg_reader.testAndSet("selfnorm", selfnorm);
   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "modelfile", modelfile);
   arg_reader.testAndSet(1, "infile", infile);
   arg_reader.testAndSet(2, "outfile", outfile);
}
