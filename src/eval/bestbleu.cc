/**
 * @author Aaron Tikuisis / Samuel Larkin
 * @file bestbleu.cc
 * @brief Program that finds the highest achievable score with a given set of
 * source and nbest.
 *
 * $Id$
 *
 * Evaluation Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 *
 * This file contains a main program that computes (probably) the best BLEU
 * score obtainable by taking one target sentence from each of a set of N-best
 * lists.
 */

#include <bleu.h>
#include <bestbleu.h>
#include <bestbleucompute.h>
#include <exception_dump.h>
#include <errors.h>
#include <file_utils.h>
#include <str_utils.h>
#include <bootstrap.h>
#include <fileReader.h>
#include <referencesReader.h>
#include <printCopyright.h>



// adding BLEUcomputer to Portage::bestbleu namespace so we don't pollute the global namespace
namespace Portage {
namespace bestbleu {
/// callable entity for booststrap confidence interval.
class BLEUcomputer
{
   private:
      const Uint K;
   public:
      /// Constructor.
      /// @param K  Number of hypotheses to be used for each source.
      BLEUcomputer(Uint K): K(K) {}
      /**
       * Calculates the BLEU score over [begin end)
       * @param begin  start iterator
       * @param end    end iterator
       * @return Returns the BLEU score calculated over [begin end)
       */
      double operator()(mbIT begin, mbIT end)
      {
         return exp(Oracle::computeBestBLEU(begin, end, K).score());
      }
};
} // ends namespace bestbleu
} // ends namespace Protage


using namespace std;
using namespace Portage;
using namespace Portage::bestbleu;


////////////////////////////////////////////////////////////////////////////////////
// MAIN
//
int MAIN(argc, argv)
{
    printCopyright(2004, "bestbleu");
    ARG arg(argc, argv);

    BLEUstats::setMaxNgrams(arg.maxNgrams);
    BLEUstats::setMaxNgramsScore(arg.maxNgramsScore);

    LOG_VERBOSE3(verboseLogger, "Allocating values placeholders");
    MatrixBLEUstats  bleu(arg.S);
    AllNbests        nbests(arg.S);

    LOG_VERBOSE3(verboseLogger, "Creating file readers");
    NbestReader  pfr(FileReader::create<Translation>(arg.sNBestFile, arg.bDyn ? 0 : arg.K));
    referencesReader  rReader(arg.sRefFiles);

    LOG_VERBOSE3(verboseLogger, "Reading files");
    Uint s(0);
    for (; pfr->pollable(); ++s)
    {
        LOG_DEBUG(debugLogger, "s:%d, K:%d, nMax=%d, R=%d", s, arg.S, arg.maxNumBest, arg.R);
        pfr->poll(nbests[s]);
        References  refs(arg.R);
        rReader.poll(refs);

        computeBLEUArrayRow(bleu[s], nbests[s], refs, arg.maxNumBest);
        if (!(arg.bBestSentence || arg.bWorseSentence || arg.bDistribution || arg.bVerbose))
           nbests[s].clear();
    }

    LOG_VERBOSE3(verboseLogger, "Integrity checking of nbest file and reference files");
    if (s != arg.S) error(ETFatal, "File inconsistency s=%d, S=%d", s, arg.S);
    rReader.integrityCheck();


    LOG_INFO(verboseLogger, "Calculating best score");
    vector<Uint> bestIndex(arg.S, 0);
    vector<Uint> worseIndex(arg.S, 0);
    typedef vector<Uint>::const_iterator vUcit;
    for (vUcit it(arg.numBest.begin()); it!=arg.numBest.end(); ++it) {
        if (arg.bVerbose) {
            if (*it > 1) {
                LOG_INFO(verboseLogger, "Finding best BLEU using %d top candidates", *it);
            } else {
                LOG_INFO(verboseLogger, "Finding best BLEU using 1 top candidate");
            }
        }

        BLEUstats best = Oracle::computeBestBLEU(bleu.begin(), bleu.end(), *it, &bestIndex, arg.bVerbose, true);
        BLEUstats worse;
        if (arg.bWorseScore)
            worse = Oracle::computeBestBLEU(bleu.begin(), bleu.end(), *it, &worseIndex, arg.bVerbose, false);

        char szFilename[256];
        for (Uint s(0); (arg.bBestIndex || arg.bVerbose || arg.bBestSentence || arg.bWorseScore || arg.bDistribution) && s < arg.S; ++s) {
            if (arg.bBestIndex)
               cerr << bestIndex[s] << endl;
            if (arg.bBestSentence)
               fprintf(stderr, "Best Sentence index : %d : %s\n", bestIndex[s], nbests.at(s)[bestIndex[s]].c_str());
            if (arg.bWorseSentence)
               fprintf(stderr, "Worse Sentence index : %d : %s\n", worseIndex[s], nbests[s][worseIndex[s]].c_str());
            LOG_VERBOSE3(verboseLogger, "Best match for sentence %d (index=%d):\n\t%s", s, bestIndex[s], nbests[s][bestIndex[s]].c_str());

            if (arg.bDistribution) {
               snprintf(szFilename, 256-2, "output/Sentence%4.4d.dist", s+1);
               oSafeMagicStream of(szFilename);
               for (Uint k(0); k<arg.maxNumBest; ++k) {
                   of << k << ": " << nbests[s][k] << endl;
                   bleu[s][k].output(of);
                   of << "BLEU score: " << exp(bleu[s][k].score()) << endl;
                   of << endl;
               }
            }
        }
        cout << endl;

        if (*it == arg.K && arg.numBest.size() == 1) {
            cout << "Result:" << endl;
        } else if (*it > 1) {
            cout << "Result using " << *it << " candidates:" << endl;
        } else {
            cout << "Result using 1 candidate:" << endl;
        }


        // Display Results
        //
        best.output();
        cout << "BLEU score: " << exp(best.score()) << endl;
        if (arg.bWorseScore) {
           cout << "Worse Score possible with those translations" << endl;
           worse.output();
           cout << "Worse BLEU score: " << exp(worse.score()) << endl;
        }


        // Confidence interval
        //
        if (arg.bCi) {
            double score = exp(best.score());
            double gamma = bootstrapConfInterval(bleu.begin(), bleu.end(), BLEUcomputer(*it), 0.9, 100);
            cout << "Confidence interval: [" << score - gamma << ", " << score + gamma << "]" << endl;
        } // if
    } // for
} END_MAIN
