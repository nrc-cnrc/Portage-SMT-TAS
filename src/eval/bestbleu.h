/**
 * @author Aaron Tikuisis
 * @file bestbleu.h  Program that calculates the best BLEU score possible to achieve with a given source and nbest set.
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
 */

#ifndef BESTBLEU_H
#define BESTBLEU_H

#include <portage_defs.h>
#include <file_utils.h>
#include <argProcessor.h>
#include <errors.h>
#include <cassert>


namespace Portage
{
   /// Program bestbleu namespace.
   /// Prevents global namespace pollution in doxygen.
   namespace bestbleu
   {
      /// Program bestbleu usage.
      static char help_message[] = "\n\
bestbleu [-v][-ci][-bs][-bi][-ws][-distrib][-r][-dyn]\n\
         [-S <number of sentences>][-K <N-Best List Size>]\n\
         [-n nbest [-n nbest [ .. ]]]\n\
         nbest-file ref1 ... refR\n\
Outputs the best (oracle) BLEU score for the given data.\n\
The nbest-file should contain a set of K best translation candidates (for\n\
constant K, unless using the Dynamic N-Best List Size) for the n-th source\n\
sentence, and each ref1 ... refR should contain S reference translations, such\n\
that the n-th line corresponds to the n-th source sentence.\n\
\n\
Options:\n\
-v       Write progress reports to cerr (repeatable) [don't].\n\
-ci      Compute confidence intervals using bootstrap resampling.\n\
-bi      Displays best translation index.\n\
-bs      Displays best translation sentence.\n\
-ws      Displays worst translation sentence.\n\
-r       Calculates the worst score possible with the given translations.\n\
-n       Specify the number of translations to choose from; for each source,\n\
         only the first <nbest> lines will be used as candidates. -n may be\n\
         used many times with different values. [use the whole list]\n\
-distrib For files containing distributions.\n\
-dyn     Use the Dynamic N-Best List size.\n\
         You must specify your maximum N Best List size with -K\n\
-K:      Sets the maximum N-Best List size in Dynamic mode\n\
-S:      depricated number of sentences\n\
-y       maximum NGRAMS for calculating BLEUstats matches [4]\n\
-u       maximum NGRAMS for calculating BLEUstats score [y]\n\
         where 1 <= y, 1 <= u <= y\n\
";

      /// Program bestbleu command line switches.
      const char* const switches[] = {"v", "bs", "ws", "r", "ci", "bi",
                                      "K:", "S:", "n:", "distrib", "dyn", "y:", "u:"};
      /// Specific argument processing class for bestbleu program.
      class ARG : public argProcessor
      {
         private:
            Logging::logger  m_vLogger;
            Logging::logger  m_dLogger;

         public:
            bool bVerbose;        ///< is verbose activated
            bool bBestIndex;      ///< do we need to output the best index
            bool bBestSentence;   ///< do we need to output the best sentence
            bool bWorseSentence;  ///< do we need to output the worse sentence
            bool bWorseScore;     ///< do we need to output the worse score
            bool bCi;             ///< should we calculate the confidence interval
            bool bDistribution;  

            string sNBestFile;  ///< file containing the nbest

            vector<string> sRefFiles;  ///< list of reference files.

            Uint maxNumBest;       ///< maximum of hypotheses per nbest to use
            vector<Uint> numBest;  ///< limiting nbest size to < nbest.size

            Uint S;  ///< number of sources
            Uint R;  ///< number of references
            Uint K;  ///< number of hypotheses per source

            bool bDyn;  ///< dynamic sized nbest list provided.

            Uint           maxNgrams;         ///< holds the max ngrams size for the BLEUstats
            Uint           maxNgramsScore;    ///< holds the max ngrams size when BLEUstats::score

         public:
         /**
          * Default constructor.
          * @param argc  same as the main argc
          * @param argv  same as the main argv
          */
         ARG(const int argc, const char* const argv[])
            : argProcessor(ARRAY_SIZE(switches), switches, 2, -1, help_message, "-h", true)
            , m_vLogger(Logging::getLogger("verbose.main.arg"))
            , m_dLogger(Logging::getLogger("debug.main.arg"))
            , bVerbose(false)
            , bBestIndex(false)
            , bBestSentence(false)
            , bWorseSentence(false)
            , bWorseScore(false)
            , bCi(false)
            , bDistribution(false)
            , maxNumBest(0)
            , S(0), R(0), K(0)
            , bDyn(false)
            , maxNgrams(4)
            , maxNgramsScore(0)
         {
            argProcessor::processArgs(argc, argv);
         }

         /// See argProcessor::printSwitchesValue()
         virtual void printSwitchesValue()
         {
            LOG_INFO(m_vLogger, "Printing arguments");
            if (m_dLogger->isDebugEnabled())
            {
               LOG_DEBUG(m_dLogger, "Verbose: %s", (bVerbose ? "ON" : "OFF"));
               LOG_DEBUG(m_dLogger, "Best Index: %s", (bBestIndex ? "ON" : "OFF"));
               LOG_DEBUG(m_dLogger, "Best Sentence: %s", (bBestSentence ? "ON" : "OFF"));
               LOG_DEBUG(m_dLogger, "Worse Sentence: %s", (bWorseSentence ? "ON" : "OFF"));
               LOG_DEBUG(m_dLogger, "Worse Score: %s", (bWorseScore ? "ON" : "OFF"));
               LOG_DEBUG(m_dLogger, "Confidence interval: %s", (bCi ? "ON" : "OFF"));
               LOG_DEBUG(m_dLogger, "S: %d", S);
               LOG_DEBUG(m_dLogger, "R: %d", R);
               LOG_DEBUG(m_dLogger, "K: %d", K);
               LOG_DEBUG(m_dLogger, "Best Translation file name: %s", sNBestFile.c_str());
               LOG_DEBUG(m_dLogger, "Maximum Ngrams size: %d", maxNgrams);
               LOG_DEBUG(m_dLogger, "Maximum Ngrams size for scoring BLEU: %d", maxNgramsScore);

               LOG_DEBUG(m_dLogger, "Number of reference files: %d", sRefFiles.size());
               std::stringstream oss1;
               for (Uint i(0); i<sRefFiles.size(); ++i)
               {
                  oss1 << "- " << sRefFiles[i].c_str() << " ";
               }
               LOG_DEBUG(m_dLogger, oss1.str().c_str());

               LOG_DEBUG(m_dLogger, "Maximum value for nBest size: %d", maxNumBest);
               std::stringstream oss2;
               for (Uint i(0); i<numBest.size(); ++i)
               {
                  oss2 << "-" << numBest[i] << " ";
               }
               LOG_DEBUG(m_dLogger, oss2.str().c_str());
            }
         }

         /// See argProcessor::processArgs()
         virtual void processArgs()
         {
            LOG_INFO(m_vLogger, "Processing arguments");

            // Taking care of general flags
            //
            mp_arg_reader->testAndSet("v", bVerbose);
            if ( getVerboseLevel() > 0 ) bVerbose = true;
            if ( bVerbose && getVerboseLevel() < 1 ) setVerboseLevel(1);
            mp_arg_reader->testAndSet("ci", bCi);
            mp_arg_reader->testAndSet("bi", bBestIndex);
            mp_arg_reader->testAndSet("bs", bBestSentence);
            mp_arg_reader->testAndSet("ws", bWorseSentence);
            mp_arg_reader->testAndSet("r", bWorseScore);
            mp_arg_reader->testAndSet("distrib", bDistribution);
            if (bWorseSentence)
            {
               bWorseScore = true;
            }

            // Taking care of the maximum values for BLEUSTATS
            //
            mp_arg_reader->testAndSet("y", maxNgrams);
            mp_arg_reader->testAndSet("u", maxNgramsScore);
            // Make sure we are not trying to compute the BLEU::score on some
            // higher ngrams than what was calculated.
            if (maxNgramsScore == 0) maxNgramsScore = maxNgrams;
            if (maxNgramsScore > maxNgrams) maxNgramsScore = maxNgrams;
            if (!(maxNgrams > 0) || !(maxNgramsScore))
               error(ETFatal, "You must specify value for y and u greater then 0!");
               
            // Taking care of the mandatory arguments
            //
            mp_arg_reader->testAndSet("S", S);
            mp_arg_reader->testAndSet("K", K);
            mp_arg_reader->testAndSet(0, "Nbest List File", sNBestFile);
            mp_arg_reader->getVars(1, sRefFiles);
            R = sRefFiles.size();

	    // Count the S and K arguments -- there's no need to require them!
	    //
            {
               assert (R > 0);
	       const Uint S0 = countFileLines(sRefFiles.front());
	       if ( S0 == 0 ) error(ETFatal, "Empty reference file: %s", sRefFiles.front().c_str());
	       if ( S != 0 && S0 != S )
	          error(ETFatal, "-S %d specified, but %d lines in ref %s",
		                   S, S0, sRefFiles.front().c_str());
	       S = S0;
            }

            mp_arg_reader->testAndSet("dyn", bDyn);
            if (bDyn)
            {
               if (K == 0) error(ETFatal, "When using dyn you must specify -K maxValue");
            }
            else
            {
		    const Uint SK = countFileLines(sNBestFile);
		    const Uint K0 = SK/S;
		    if ( S == 0 || SK == 0 || K0 * S != SK )
		       error(ETFatal, "Inconsistency between nbest and ref number of lines\n\tnbest: %d, ref %s: %d => K: %d",
				      SK, S, sRefFiles.front().c_str(), K0);

		    // If -S and/or -K are provided, use them as sanity checking (we
		    // keep them for backward compatibility anyway).
		    //
		    if ( K != 0 && K0 != K )
		       error(ETFatal, "-K %d specified, but %d inferred from ref and nbest files",
				      K, K0);
		    K = K0;
            }

            // Taking care of the multi arguments n switch
            //
            if (mp_arg_reader->getSwitch("n"))
            {
               vector<string>  snBest;
               mp_arg_reader->testAndSet("n", snBest);
               int next(0);
               numBest.reserve(snBest.size());
               for (vector<string>::const_iterator it(snBest.begin()); it!=snBest.end(); ++it)
               {
                  if (!conv(*it, next))
                  {
                     error(ETFatal, "%s is not a number", it->c_str());
                  }
                  if (next <= 0)
                  {
                     error(ETFatal, "Invalid value for nbest");
                  } // if
                  numBest.push_back((Uint)next);
                  maxNumBest = max(maxNumBest, (Uint)next);
               }
            }
            else
            {
               numBest.push_back(K);
               maxNumBest = K;
               LOG_WARN(m_vLogger, "Using single default value for n: K=%d", K);
            }

            // Checking integrety
            //
            if (S == 0) error(ETFatal, "We must be able to compute S or use -S to specify");
            if (K == 0) error(ETFatal, "Empty list of best translations");
            if (K < maxNumBest) error(ETFatal, "numBest is greater than the number of candidate translations");
         }
      };
   } // ends namespace bestbleu
} // ends namespace Portage

#endif // BESTBLEU_H
