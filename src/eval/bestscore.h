/**
 * @author Aaron Tikuisis
 * @file bestbleu.h  Program that calculates the best BLEU score possible to achieve with a given source and nbest set.
 *
 * Evaluation Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef __BEST_SCORE_H__
#define __BEST_SCORE_H__

#include <portage_defs.h>
#include <bestscorecompute.h>
#include <errors.h>
#include <file_utils.h>
#include <str_utils.h>
#include <bootstrap.h>
#include <fileReader.h>
#include <referencesReader.h>
#include <errors.h>
#include <cassert>

namespace Portage
{

   /// Program bestscore namespace.
   /// Prevents global namespace pollution in doxygen.
   namespace bestscore
   {
      using namespace std;

      /// Generic arguments processing class for the oracle programs.
      struct ARG
      {
	 bool bVerbose;
	 bool rev;             ///< reverse (worst) oracle
	 bool conf;            ///< calculate conf interval
	 bool sentscale;       ///< scale sent-level bleu
	 bool bDyn;            ///< dynamic nbests
	 Uint maxn;            ///< max desired nbest length for processing
	 int f;                ///< scaling factor for oracle-substitution BLEU
	 string outopt;        ///< what to output

	 string sNBestFile;           ///< file containing the nbest
         vector<string> sRefFiles;    ///< list of reference files.

	 Uint S;  ///< number of sources
	 Uint R;  ///< number of references
	 Uint K;  ///< true number of hypotheses per source (0 if dynamic)

	 ARG()
	    : bVerbose(false)
	    , rev(false)
	    , conf(false)
	    , sentscale(false)
	    , bDyn(false)
	    , maxn(0)
	    , f(0)
	    , outopt("std")
	    , S(0)
	    , R(0)
	    , K(0)
	 { }
      };

      /// callable entity for booststrap confidence interval.
      template <class ScoreMetric>
      class ScoreComputer
      {
         private:
            const Uint K;
            vector<Uint> indexes;

         public:
            /// Constructor.
            /// @param K  Number of hypotheses to be used for each source.
            ScoreComputer(Uint K): K(K) {}
            /**
             * Calculates the BLEU score over [begin end)
             * @param begin  start iterator
             * @param end    end iterator
             * @return Returns the BLEU score calculated over [begin end)
             */
            double operator()(typename vector<vector<ScoreMetric> >::const_iterator begin,
               typename vector<vector<ScoreMetric> >::const_iterator end)
            {
               indexes.clear();
               return ScoreMetric::convertToDisplay(computeBestScore<ScoreMetric>(begin, end, K, indexes).score());
            }
      };

      template<class ScoreMetric>
      void bestScore(const ARG& arg, referencesReader& rReader, NbestReader nbestReader)
      {
	 //LOG_VERBOSE3(verboseLogger, "Allocating values placeholders");
	 typedef vector<vector<ScoreMetric> > Matrix;
         Matrix     scores(arg.S);
	 AllNbests  nbests(arg.S);

	 //LOG_VERBOSE3(verboseLogger, "Reading files");
	 Uint s(0);
	 for (; nbestReader->pollable(); ++s) {
	    //LOG_DEBUG(debugLogger, "s:%d, K:%d, nMax=%d, R=%d", s, arg.S, arg.maxn, arg.R);
	    nbestReader->poll(nbests[s]);

	    References  refs(arg.R);
	    rReader.poll(refs);

	    computeArrayRow(scores[s], nbests[s], refs, 
		  arg.maxn ? arg.maxn : numeric_limits<Uint>::max());
	    if (arg.outopt == "std") nbests[s].clear();
	 }

	 //LOG_VERBOSE3(verboseLogger, "Integrity checking of nbest file and reference files");
	 nbestReader.reset();
	 rReader.integrityCheck();
	 if (s != arg.S) error(ETFatal, "File inconsistency s=%d, S=%d", s, arg.S);

	 //LOG_INFO(verboseLogger, "Calculating best score");
	 vector<Uint> best_indexes(arg.S, 0);

	 // do the real work

	 ScoreMetric best = computeBestScore<ScoreMetric>(scores.begin(), scores.end(), 
	       arg.maxn, best_indexes, arg.bVerbose, !arg.rev);

	 // output results in selected format

	 if (arg.outopt == "std") {
	    best.output();
	    printf("%s score: %f\n", ScoreMetric::name(), ScoreMetric::convertToDisplay(best.score()));
	    if (arg.conf) {
	       const double score = ScoreMetric::convertToDisplay(best.score());
	       const double gamma = bootstrapConfInterval(scores.begin(), scores.end(), 
		     ScoreComputer<ScoreMetric>(arg.maxn), 0.9, 100);
	       cout << "Confidence interval: [" 
		  << score - gamma << ", " << score + gamma << "]" << endl;
	    }
	 }
         else if (arg.outopt == "oracle") {
	    for (Uint s = 0; s < arg.S; ++s)
	       cout << best_indexes[s] << ' ' 
		  << nbests[s][best_indexes[s]].c_str() << endl;
	 }
         else if (arg.outopt == "nbest") {
	    const int scale = arg.f <= 0 ? 1 : arg.S / arg.f;
	    for (Uint s = 0; s < arg.S; ++s) {
	       best -= scores[s][best_indexes[s]];
	       const double sc = arg.sentscale ? 
		  min(scores[s][best_indexes[s]].getBestMatchLength()/100.0, 1.0) : 1.0;
	       for (Uint k = 0; k < nbests[s].size(); ++k) {
		  const ScoreMetric t = scores[s][k] * scale;
		  best += t;
		  cout << s << ' ' 
                     << ScoreMetric::convertToDisplay(scores[s][k].score()) * sc << ' '
                     << ScoreMetric::convertToDisplay(best.score()) << endl;
		  best -= t;
	       }
	       best += scores[s][best_indexes[s]];
	    }
	 }
      }  // ends bestScore
   } // ends namespace bestscore
} // ends namespace Portage

#endif // __BEST_SCORE_H__
