/**
 * @author George Foster
 * @file train_lm_mixture.cc
 * @brief Train a mixture of ngram language models using EM on a given text file.
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include "file_utils.h"
#include "arg_reader.h"
#include "lm.h"
#include "vocab_filter.h"
#include "em.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
train_lm_mixture [options] ngram-models textfile\n\
\n\
  Train a mixture of ngram language models using EM on a given text file.\n\
  <ngram-models> is a file containing a list of the ngram models to use, one\n\
  per line, with optional space-separated initial weights. The results are a\n\
  set of normalized weights written to stdout.\n\
\n\
  This does the same thing as the script train-lm-mixture, but without using\n\
  SRILM. Results are slightly different.\n\
\n\
Options:\n\
\n\
  -v          Write progress reports to cerr.\n\
  -order n    Set ngram order [0 = use order of models]\n\
  -sent-filt  Do per-sentence vocab filtering for models [whole text filt]\n\
  -oov meth   OOV handling: do 'lmeval -h' for description [SimpleAutoVoc]\n\
              NB: any tokens assigned prob 0 by all models are ignored.\n\
  -n maxiter  Max number of EM iterations [100]\n\
  -prec p     Precision: stop when max change in any weight is < p [0.001]\n\
";

// globals

static bool verbose = false;
static Uint order = 0;
static bool per_sent_filt = false;
static bool oov_ok;
static PLM::OOVHandling oov("SimpleAutoVoc", oov_ok);
static Uint num_iters = 100;
static double prec = 0.001;
static string modelsfilename;
static string textfilename;

static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   // set up vocabulary, for filtering models

   if (verbose) cerr << "setting up vocabulary " << endl;
   time_t start = time(NULL);
   Uint line_count = countFileLines(textfilename);
   VocabFilter vocab(per_sent_filt ? line_count : 0);
   VocabFilter::addConverter  aConverter(vocab, per_sent_filt);
   string line;
   {
      vector<Uint> dummy;
      iSafeMagicStream textfile(textfilename);

      while (getline(textfile, line)) {
         if (line.empty()) continue;
         dummy.clear();
         split(line.c_str(), dummy, aConverter);
         aConverter.next();
      }
      if (verbose)
         cerr << "voc read in " << (time(NULL)-start) << " secs" << endl;
   }

   // load lms

   if (verbose) {
      cerr << "loading models - using " << oov.toString() << endl;
   }
   start = time(NULL);
   vector<PLM*> models;
   vector<double> init_weights;
   iSafeMagicStream modelsfile(modelsfilename);
   vector<string> toks;
   Uint max_order = 0;
   while (getline(modelsfile, line)) {
      if (line.empty()) continue;
      splitZ(line, toks);       // allow other tokens following model name
      models.push_back(PLM::Create(toks[0], &vocab, oov, -INFINITY,
                                   true, order, NULL, !verbose));
      if (toks.size() > 1) init_weights.push_back(conv<double>(toks[1]));
      max_order = max(max_order, models.back()->getOrder());
   }
   if (init_weights.size() && init_weights.size() != models.size())
      error(ETFatal, "not all models seem to have a weight in %s", modelsfilename.c_str());
   if (order == 0) order = max_order;
   if (verbose) {
      cerr << "models loaded in " << (time(NULL)-start) << " secs" << endl;
      cerr << "native max order = " << max_order << "; using order = " << order << endl;
      cerr << (init_weights.size() ? "using supplied init weights" : "using uniform init weights") 
           << endl;
   }

   // em

   start = time(NULL);
   EM em(models.size());
   if (init_weights.size()) em.getWeights() = init_weights;
   vector<double> probs(models.size());
   string sent_end(PLM::SentEnd);

   for (Uint iter = 0; iter < num_iters; ++iter) {

      Uint lineno = 0;
      iSafeMagicStream textfile(textfilename);
      Uint context[order - 1];
      double logprob = 0.0;
      Uint ntoks = 0, noovs = 0;

      while (getline(textfile, line)) {

         for (Uint j = 0; j < order - 1; ++j) 
            context[j] = vocab.index(PLM::SentStart);

         splitZ(line, toks);
         toks.push_back(sent_end); // predict eos as well
         for (Uint i = 0; i < toks.size(); ++i) {
            Uint w = vocab.index(toks[i].c_str());
            for (Uint m = 0; m < models.size(); ++m)
               probs[m] = exp10(models[m]->wordProb(w, context, order-1));
            double p = em.count(probs);
            if (p) {
               logprob += log10(p);
               ++ntoks;
            } else
               ++noovs;
            for (Uint j = order-1; j > 0; --j) 
               context[j] = context[j-1];
            context[0] = w;
         }
         ++lineno;
      }
      if (verbose) {
         if (iter == 0)
            cerr << lineno << " lines read, " 
                 << ntoks << " tokens counted (includes eos), " 
                 << noovs << " tokens ignored" << endl;
         cerr << "iter " << iter+1 << " done: ppx = " << exp10(-logprob / ntoks) << endl;
      }
      if (em.estimate() < prec)
         break;
   }
   if (verbose)
      cerr << "done EM in " << (time(NULL)-start) << " secs" << endl;
   
   // output
   
   for (Uint i = 0; i < models.size(); ++i)
      cout << em.getWeights()[i] << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "order:", "per-sent-filt", "oov:", "n:", "prec:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("order", order);
   arg_reader.testAndSet("per-sent-filt", per_sent_filt);
   arg_reader.testAndSet("n", num_iters);
   arg_reader.testAndSet("prec", prec);

   string oov_string;
   arg_reader.testAndSet("oov", oov_string);
   if (!oov_string.empty()) {
      PLM::OOVHandling oov_arg(oov_string, oov_ok);
      if (!oov_ok)
         error(ETFatal, 
               "bad value for -oov switch: %s\n - see lm_eval -h for valid values",
               oov_string.c_str());
      oov = oov_arg;
   }

   arg_reader.testAndSet(0, "infile", modelsfilename);
   arg_reader.testAndSet(1, "outfile", textfilename);
}
