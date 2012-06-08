/**
 * @author  George Foster
 * @file dmestm.cc 
 * @brief  Estimate distortion model probabilities from counts
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include <arg_reader.h>
#include <phrase_table.h>
#include "dmstruct.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
dmestm [options] [dmc]\n\
\n\
Estimate a distortion model from a distortion count file <dmc> produced by\n\
dmcount (read from stdin if <dmc> is omitted). <dmc> may be one or more count\n\
files cat'd together. Output is written to stdout, in same format as the input,\n\
but with probabilities instead of counts.\n\
\n\
Options:\n\
\n\
-v          Write progress reports to cerr.\n\
-s          Sort output by source phrase [don't]\n\
-prune1 n   Prune so that each language1 phrase has at most n translations.\n\
            Based on summed joint freqs; done right after reading in counts.\n\
-prune1w n  Same as prune1, but multiply n by the number of words in the\n\
            current source phrase. Only one of prune1 or prune1w may be used.\n\
-wtu w      Scale uniform prior by w to get prior counts [30]\n\
-wtg w      Scale global prior (sum over all phrase pair freqs) by w [30]\n\
-wt1 w      Scale l1 prior (rel freq for each l1 phrase) by w [15]\n\
-wt2 w      Scale l2 prior (rel freq for each l2 phrase) by w [15]\n\
-eval cf    Evaluate the model on counts file <cf>. Write perplexity to stderr.\n\
-g gf       Write the smoothed global distribution to file <gf>.\n\
";


// globals

static Uint verbose = 0;
static bool sorting = false;
static Uint prune1 = 0;
static Uint prune1w = 0;
static string dmc;
static string gf;

static double uniform_wt = 30;  // wt on uniform prior
static double global_wt = 30;   // wt on global (sum over all counts) prior
static double l1_wt = 15;       // wt on l1 (sum over all linked l2 phrases) prior
static double l2_wt = 15;       // wt on l2 (sum over all linked l1 phrases) prior
static string eval;             // counts file for evaluation

static void getArgs(int argc, char* argv[]);

// Primitives for MAP smoothing: scale prior distn, add prior counts to 
// empirical counts, and normalize. These operate on arrays rather than 
// vectors in case we eventually want to precompute and store priors for
// l1 and l2 phrases.

float* scale(float* counts, double w)
{
   for (Uint i = 0; i < DistortionCount::size(); ++i)
      counts[i] *= w;
   return counts;
}

float* add(float* prior, const DistortionCount& empc)
{
   float* p = prior;
   for (Uint i = 0; i < empc.size(); ++i)
      *p++ += empc.val(i);
   return prior;
}

float* norm(float* counts)
{
   Uint n = DistortionCount::size() / 2;
   float sum1 = 0.0, sum2 = 0.0;
   for (Uint i = 0; i < n; ++i) {
      sum1 += counts[i];
      sum2 += counts[i+n];
   }
   for (Uint i = 0; i < n; ++i) {
      counts[i] /= sum1;
      counts[i+n] /= sum2;
   }
   return counts;
}

// Calculate the log likelihood incr due to the counts in <dc> under <distn>.
double logLikelihood(float* distn, const DistortionCount& dc)
{
   double like = 0.0;
   for (Uint i = 0; i < dc.size(); ++i)
      if (dc.val(i))
         like += dc.val(i) * log(distn[i]);
   return like;
}

// main

int main(int argc, char* argv[])
{
   printCopyright(2009, "dmestm");
   getArgs(argc, argv);

   string line;
   vector<string> toks;
   PhraseTableBase::ToksIter b1, e1, b2, e2, v, a;
   DistortionCount dc;

   // read eval counts into pteval if required

   Uint nphrases_eval_tot = 0;
   PhraseTableGen<DistortionCount> pteval;
   if (eval.size()) {
      iSafeMagicStream in(eval);
      while (getline(in, line)) {
         pteval.extractTokens(line, toks, b1, e1, b2, e2, v, a, true);
         dc.read(v, toks.end());
         pteval.addPhrasePair(b1, e1, b2, e2, dc);
         ++nphrases_eval_tot;
      }
   }

   // read main counts into pt

   iSafeMagicStream in(dmc.size() ? dmc : "-");
   PhraseTableGen<DistortionCount> pt;
   while (getline(in, line)) {
      pt.extractTokens(line, toks, b1, e1, b2, e2, v, a, true);
      dc.read(v, toks.end());
      pt.addPhrasePair(b1, e1, b2, e2, dc);
   }
   if (verbose)
      cerr << "read counts: " 
           << pt.numLang1Phrases() << " l1 phrases, "
           << pt.numLang2Phrases() << " l2 phrases" << endl;

   // prune based on total freqs (sum across m,s,d conditions) if called for

   if (prune1 || prune1w) {
      if (verbose)
         cerr << "pruning to best " << (prune1 ? prune1 : prune1w) 
              << " translations" << (prune1w ? " per word" : "") 
              << ", using total freqs" << endl;
      pt.pruneLang2GivenLang1(prune1 ? prune1 : prune1w, prune1w != 0);
   }

   // Make global & marginal counts for each language, for smoothing.

   DistortionCount global_counts;
   vector<DistortionCount> l1_marginals(pt.numLang1Phrases());
   vector<DistortionCount> l2_marginals(pt.numLang2Phrases());

   for (PhraseTableGen<DistortionCount>::iterator p = pt.begin(); p != pt.end(); ++p) {
      global_counts += p.getJointFreqRef();
      l1_marginals[p.getPhraseIndex(1)] += p.getJointFreqRef();
      l2_marginals[p.getPhraseIndex(2)] += p.getJointFreqRef();
   }      

   // set up global prior smoothing counts

   vector<float> global_prior(DistortionCount::size(), 1.0);
   norm(add(scale(norm(&global_prior[0]), uniform_wt), global_counts));
   scale(&global_prior[0], global_wt);   // global prior counts (unnormalized)

   // Estimate and output. (This could be sped up by pre-computing and storing
   // the smoothed prior counts for each l1 and l2 phrase.)

   string outname("-");
   if (sorting) 
      outname = "| LC_ALL=C TMPDIR=. sort";
   oSafeMagicStream os(outname);

   vector<float> l1_prior(DistortionCount::size());
   vector<float> l2_prior(DistortionCount::size());
   string p1, p2;
   vector<string> toks1, toks2;
   double ll = 0.0, ll_eval = 0.0;
   Uint totfreq = 0, totfreq_eval = 0.0;
   Uint nphrases = 0, nphrases_eval = 0.0;

   for (PhraseTableGen<DistortionCount>::iterator p = pt.begin(); p != pt.end(); ++p) {

      l1_prior = global_prior;
      l2_prior = global_prior;

      scale(norm(add(&l1_prior[0], l1_marginals[p.getPhraseIndex(1)])), l1_wt);
      scale(norm(add(&l2_prior[0], l2_marginals[p.getPhraseIndex(2)])), l2_wt);
      for (Uint i = 0; i < DistortionCount::size(); ++i) 
         l1_prior[i] += l2_prior[i];
      norm(add(&l1_prior[0], p.getJointFreqRef()));
      ll += logLikelihood(&l1_prior[0], p.getJointFreqRef());
      totfreq += p.getJointFreqRef().freq();
      ++nphrases;

      if (eval.size()) {
         p.getPhrase(1, toks1);
         p.getPhrase(2, toks2);
         if (pteval.exists(toks1.begin(), toks1.end(), toks2.begin(), toks2.end(), dc)) {
            ll_eval += logLikelihood(&l1_prior[0], dc);
            totfreq_eval += dc.freq();
            ++nphrases_eval;
         }
      }

      p.getPhrase(1, p1);
      p.getPhrase(2, p2);
      pt.writePhrasePair(os, p1.c_str(), p2.c_str(), NULL, l1_prior, false, 0.0f);
   }

   if (gf.size()) {
      oSafeMagicStream os(gf);
      // recalculate global_prior from counts, in case global_wt is 0
      global_prior.assign(DistortionCount::size(), 1.0);
      norm(add(scale(norm(&global_prior[0]), uniform_wt), global_counts));
      for (Uint i = 0; i < global_prior.size(); ++i)	
         os << global_prior[i] << ' ';
      os << endl;
   }

   if (verbose) {
      cerr << nphrases << " phrases written, total count = " << totfreq
           << ", ppx = " << exp(-ll / (2 * nphrases)) << endl;
   }
   if (eval.size()) {
      cerr << "eval file " << eval << ": " 
           << nphrases_eval << "/" << nphrases_eval_tot << " phrases used, total count = "
           << totfreq_eval << ", ppx = " << exp(-ll_eval / (2 * nphrases_eval)) << endl;
   }
}

// Arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "s", "prune1:", "prune1w:", 
                             "wtu:", "wtg:", "wt1:", "wt2:", "eval:", "g:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 1, help_message);
   arg_reader.read(argc-1, argv+1);

   vector<string> verboses;

   arg_reader.testAndSet("v", verboses);
   arg_reader.testAndSet("s", sorting);
   arg_reader.testAndSet("prune1", prune1);
   arg_reader.testAndSet("prune1w", prune1w);
   arg_reader.testAndSet("wtu", uniform_wt);
   arg_reader.testAndSet("wtg", global_wt);
   arg_reader.testAndSet("wt1", l1_wt);
   arg_reader.testAndSet("wt2", l2_wt);
   arg_reader.testAndSet("eval", eval);
   arg_reader.testAndSet("g", gf);
   arg_reader.testAndSet(0, "dmc", dmc);

   verbose = verboses.size();
}
