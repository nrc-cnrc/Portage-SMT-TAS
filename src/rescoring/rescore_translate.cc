/**
 * @author Samuel Larkin based on George Foster, based on "rescore" by Aaron Tikuisis
 * @file rescore_translate.cc  Program rescore_translate which picks the best
 * translation for a source given an nbest list a feature functions weights
 * vector.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */


#include "rescore_translate.h"
#include "featurefunction_set.h"
#include "rescore_io.h"

#include "exception_dump.h"
#include "rescoring_general.h"
#include "printCopyright.h"
#include "bleu.h"

#include <queue>
#include <stack>
#include <sstream>


using namespace Portage;
using namespace std;

// Assuming v is a vector of logprobs, return log of the sum of the
// probabilities in the vector.
static double logNorm(const uVector& v)
{
   double sum = 0.0;
   double max = 0.0;
   for (Uint i = 0; i < v.size(); ++i)
      if (v[i] < max) max = v[i];
   for (Uint i = 0; i < v.size(); ++i)
      sum += exp(v[i] - max);
   return log(sum) + max;
}

////////////////////////////////////////////////////////////////////////////////
// main
int MAIN(argc, argv)
{
   printCopyright(2004, "rescore_translate");
   using namespace rescore_translate;
   ARG arg(argc, argv);

   ////////////////////////////////////////
   // SOURCE
   LOG_VERBOSE2(verboseLogger, "Reading source file");
   Sentences  sources;
   const Uint S(RescoreIO::readSource(arg.src_file, sources));
   if (S == 0) error(ETFatal, "empty source file: %s", arg.src_file.c_str());


   ////////////////////////////////////////
   // FEATURE FUNCTION SET
   LOG_VERBOSE2(verboseLogger, "Reading ffset");
   FeatureFunctionSet ffset;
   ffset.read(arg.model, arg.bVerbose, arg.ff_pref.c_str(), arg.bIsDynamic);
   ffset.createTgtVocab(sources, FileReader::create<Translation>(arg.nbest_file, arg.K));
   uVector wts(ffset.M());
   ffset.getWeights(wts);
   {
      ostringstream oss;
      oss << wts;
      LOG_VERBOSE2(verboseLogger, "Rescoring using weights: %s", oss.str().c_str());
   }

   LOG_VERBOSE2(verboseLogger, "Initializing FF matrix with %d source sentences", S);
   ffset.initFFMatrix(sources);


   ////////////////////////////////////////
   // ALIGNMENT
   ifstream astr;
   const bool bNeedsAlignment = ffset.requires() & FF_NEEDS_ALIGNMENT;
   if (bNeedsAlignment) {
      if (arg.alignment_file.empty())
         error(ETFatal, "At least one of feature function requires the alignment");
      LOG_VERBOSE2(verboseLogger, "Reading alignments from %s", arg.alignment_file.c_str());
      astr.open(arg.alignment_file.c_str());
      if (!astr) error(ETFatal, "unable to open alignment file %s", arg.alignment_file.c_str());
   }

   LOG_VERBOSE2(verboseLogger, "Processing Nbest lists");
   NbestReader  pfrN(FileReader::create<Translation>(arg.nbest_file, arg.K));
   Uint s(0);
   for (; pfrN->pollable(); ++s)
   {
      Nbest  nbest;
      pfrN->poll(nbest);
      Uint K(nbest.size());

      vector<Alignment> alignments(K);
      Uint k(0);
      for (; bNeedsAlignment && k < K && alignments[k].read(astr); ++k)
      {
         nbest[k].alignment = &alignments[k];
      }
      if (bNeedsAlignment && (k != K))
         error(ETFatal, "unexpected end of nbests file after %d lines (expected %dx%d=%d lines)", s*K+k, S, K, S*K);

      LOG_VERBOSE3(verboseLogger, "Computing FF matrix");
      uMatrix H;
      ffset.computeFFMatrix(H, s, nbest);
      K = nbest.size();  // IMPORTANT K might change if computeFFMatrix detects empty lines

      if (arg.kout == 1) {      // output single best hypothesis
         if (K==0) {
            cout << endl;
         } else {
            const uVector Scores = boost::numeric::ublas::prec_prod(H, wts);
            double logz = arg.conf_scores ? logNorm(Scores) : 0.0;
            const Uint bestIndex = my_vector_max_index(Scores);         // k = sentence which is scored highest
            if (arg.bPrintRank)cout << bestIndex+1 << " ";
            if (arg.print_scores) cout << Scores[bestIndex] - logz << " ";
            cout << nbest.at(bestIndex) << endl;  // DISPLAY best hypothesis
         }
      } else {                  // ouptput k best hypotheses
         const uVector Scores = boost::numeric::ublas::prec_prod(H, wts);
         double logz = arg.conf_scores ? logNorm(Scores) : 0.0;
         priority_queue< pair<float, Uint> > best_scores;
         Uint kout = (arg.kout == 0) ? K : min(arg.kout, K);
         for (Uint i = 0; i < Scores.size(); ++i) { // put best kout into pq
            best_scores.push(make_pair(-Scores[i], i));
            if (best_scores.size() > kout) best_scores.pop();
         }
         stack < pair<float, Uint> > best_scores_stack;
         while (!best_scores.empty()) { // reverse order of queue
            best_scores_stack.push(best_scores.top());
            best_scores.pop();
         }
         while (!best_scores_stack.empty()) { // output
            pair<float, Uint>& p = best_scores_stack.top();
            if (arg.bPrintRank) cout << p.second+1 << " ";
            if (arg.print_scores) cout << Scores[p.second] - logz << " ";
            cout << nbest.at(p.second) << endl;  // DISPLAY best hypothesis
            best_scores_stack.pop();
         }
         if (arg.K)             // pad with blank lines if necessary
            for (; kout < arg.kout; ++kout)  cout << endl;
      }
   }

   astr.close();
   if (s != S) error(ETFatal, "File inconsistency s=%d, S=%d", s, S);

} END_MAIN
