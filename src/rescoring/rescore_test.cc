/**
 * @author George Foster, based on "rescore" by Aaron Tikuisis
 * @file rescore_test.cc 
 * @brief Program rescore_test which tests a rescoring model on given source,
 * nbest and reference texts.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "rescore_test.h"
#include "exception_dump.h"
#include "bleu.h"
#include "featurefunction_set.h"
#include "powell.h"
#include "rescore_io.h"
#include "referencesReader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;


////////////////////////////////////////////////////////////////////////////////
// main
int MAIN(argc, argv)
{
   printCopyright(2004, "rescore_test");
   const Uint TESTS_FOR_AVG(100);
   RescoreTest::ARG arg(argc, argv);

   BLEUstats::setMaxNgrams(arg.maxNgrams);
   BLEUstats::setMaxNgramsScore(arg.maxNgramsScore);


   LOG_VERBOSE2(verboseLogger, "Reading source sentences");
   Sentences  sources;
   const Uint S = RescoreIO::readSource(arg.src_file, sources);
   if (S == 0) error(ETFatal, "empty source file: %s", arg.src_file.c_str());
   

   LOG_VERBOSE2(verboseLogger, "Creating feature functions set");
   FeatureFunctionSet ffset;
   const Uint M = ffset.read(arg.model, arg.bVerbose, arg.ff_pref.c_str(), arg.bIsDynamic);
   ffset.createTgtVocab(sources, FileReader::create<Translation>(arg.nbest_file, arg.K));
   uVector modelP(M);
   ffset.getWeights(modelP);


   LOG_VERBOSE2(verboseLogger, "Initializing FF matrix with %d source sentences", S);
   ffset.initFFMatrix(sources);


   LOG_VERBOSE2(verboseLogger, "Creating references reader");
   referencesReader  rReader(arg.refs_file);


   LOG_VERBOSE2(verboseLogger, "Rescoring with S=%d, R=%d, M=%d", S, rReader.getR(), M);
   MatrixBLEUstats  bleu(S);
   BLEUstats total;
   BLEUstats best;


   LOG_VERBOSE2(verboseLogger, "Processing Nbest lists");
   NbestReader  pfr(FileReader::create<Translation>(arg.nbest_file, arg.K));
   Uint s(0);
   for (; pfr->pollable(); ++s)
   {
      // READING NBEST
      Nbest nbest;
      pfr->poll(nbest);
      Uint K(nbest.size());

      // READING REFERENCES
      References refs;
      rReader.poll(refs);
      
      LOG_VERBOSE3(verboseLogger, "Computing FF matrix (s=%d, K=%d, R=%d)", s, K, rReader.getR());
      uMatrix H;
      ffset.computeFFMatrix(H, s, nbest);
      K = nbest.size();    // IMPORTANT K might change if computeFFMatrix finds empty lines

      LOG_VERBOSE3(verboseLogger, "Computing BLEU (s=%d, K=%d, R=%d)", s, K, rReader.getR());
      computeArrayRow(bleu[s], nbest, refs);

      // Determine which translation is chosen by the model, and add its stats to total
      const uVector scores = boost::numeric::ublas::prec_prod(H, modelP);
      Uint k = my_vector_max_index(scores); // k = sentence which is scored highest
      total  = total + bleu[s][k];

      // Choose the translation with the most n-gram matches for the greatest n (ie., only look at
      // n = 3 if there is a tie for n = 4, etc.).  Use these stats in the calculation of the "best"
      // BLEU score.
      int bestIndex(0);
      for (k = 1; k<bleu[s].size(); ++k) {
         for (int n = BLEUstats::getMaxNgrams()-1; n >= 0; --n) {
            if (bleu[s][bestIndex].match[n] > bleu[s][k].match[n]) {
               break;
            } else if (bleu[s][k].match[n] > bleu[s][bestIndex].match[n]) {
               bestIndex = k;
               break;
            }
         }
      }
      best = best + bleu[s][bestIndex];
   }
   if (s != S) error(ETFatal, "File inconsistency s=%d, S=%d", s, S);
   rReader.integrityCheck();

   cout << "Model score: " << exp(total.score()) << endl;;
   cout << "(Estimated) best score: " << exp(best.score()) << endl;

   // Randomly choose TESTS_FOR_AVG different translations and compute the score for each;
   // average these to get an estimate of the overall average.

   double avgScore(0);
   for (Uint i(0); i<TESTS_FOR_AVG; ++i) {
      BLEUstats cur;
      for (Uint s(0); s<S; ++s)  {
         const int k = rand() % bleu[s].size();
         cur = cur + bleu[s][k];
      }
      avgScore += cur.score();
   }
   avgScore /= TESTS_FOR_AVG;
   cout << "(Estimated) average score: " << exp(avgScore) << endl;
} END_MAIN
