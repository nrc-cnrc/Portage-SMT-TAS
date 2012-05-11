/**
 * @author Samuel Larkin
 * @file bleumain.h  Program that calculates the BLEU score of a given source and nbest set.
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

#ifndef __BLEUMAIN_H__
#define __BLEUMAIN_H__

#include "portage_defs.h"
#include "argProcessor.h"
#include "bleu.h"
#include "scoremain.h"


namespace Portage
{
   /// Program bleumain namespace.
   /// Prevents global namespace pollution in doxygen.
   namespace bleumain
   {
      /// bleumain help string describing its usage.
      static char help_message[] = "\n\
bleumain [-v][-c][-detail d][-smooth s] testfile ref1 ref2 ... refn\n\
\n\
Computes the BLEU score for the set of translations in testfile, using the\n\
reference files ref1, ... , refn. Each file should have one sentence per line,\n\
and the sentences in testfile should match line for line with the sentences in\n\
each reference file.\n\
\n\
Options:\n\
\n\
-v         Write progress reports to cerr [don't].\n\
-c         Compute a 95% confidence interval around the score using bootstrap\n\
           resampling.\n\
-detail d  d=1: Print the (smoothed) BLEU score for each single sentence.\n\
           d=2: Also print the n-gram statistics for each single sentence.\n\
-smooth s  Smoothing method: [1]\n\
           s=0: none\n\
           s=1: Replace 0 n-gram matches by fixed epsilon\n\
           s=2: Increase the count by 1 for all n-grams with n>1\n\
                (cf. Lin and Och, Coling 2004)\n\
-y         maximum NGRAMS for calculating BLEUstats matches [4]\n\
-u         maximum NGRAMS for calculating BLEUstats score [y]\n\
           where 1 <= y, 1 <= u <= y\n\
\n\
Hack:\n\
           To compute NIST-style BLEU scores, define the environment variable\n\
           PORTAGE_NIST_STYLE_BLEU.\n\
";

      /// Program bleumain command line switches.
      const char* const switches[] = {"c", "detail:", "smooth:", "y:", "u:", "v"};

      /// Specific argument processing class for bleumain program.
      class ARG : public argProcessor, public scoremain::ARG
      {
         private:
            Logging::logger  m_vLogger;
            Logging::logger  m_dLogger;

         public:
            int   iSmooth;
            Uint  maxNgrams;         ///< holds the max ngrams size for the BLEUstats
            Uint  maxNgramsScore;    ///< holds the max ngrams size when BLEUstats::score

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
            , iSmooth(DEFAULT_SMOOTHING_VALUE)
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
               LOG_DEBUG(m_dLogger, "Do conf: %s", (bDoConf? "ON" : "OFF"));
               LOG_DEBUG(m_dLogger, "Detailed output: %i", iDetail);
               LOG_DEBUG(m_dLogger, "Smoothing method: %i", iSmooth);
               LOG_DEBUG(m_dLogger, "Test file name: %s", sTestFile.c_str());
               LOG_DEBUG(m_dLogger, "Maximum Ngrams size: %d", maxNgrams);
               LOG_DEBUG(m_dLogger, "Maximum Ngrams size for scoring BLEU: %d", maxNgramsScore);

               LOG_DEBUG(m_dLogger, "Number of reference files: %d", sRefFiles.size());
               std::stringstream oss1;
               for (Uint i(0); i<sRefFiles.size(); ++i)
               {
                  oss1 << "- " << sRefFiles[i].c_str() << " ";
               }
               LOG_DEBUG(m_dLogger, oss1.str().c_str());
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

            mp_arg_reader->testAndSet("c", bDoConf);
            mp_arg_reader->testAndSet("detail", iDetail);
            mp_arg_reader->testAndSet("smooth", iSmooth);
            mp_arg_reader->testAndSet("y", maxNgrams);
            mp_arg_reader->testAndSet("u", maxNgramsScore);
            // Make sure we are not trying to compute the BLEU::score on some
            // higher ngrams than what was calculated.
            if (maxNgramsScore == 0) maxNgramsScore = maxNgrams;
            if (maxNgramsScore > maxNgrams) maxNgramsScore = maxNgrams;
            if (!(maxNgrams > 0) || !(maxNgramsScore))
               error(ETFatal, "You must specify value for y and u greater then 0!");

            mp_arg_reader->testAndSet(0, "testfile", sTestFile);
            mp_arg_reader->getVars(1, sRefFiles);

            if (iSmooth<0 || iSmooth>2)
            {
               cerr << "Invalid smoothing type: " << iSmooth << endl;
               exit(1);
            }
         }

      }; // ends class ARG
   } // ends namespace bleumain
} // Portage

#endif // __BLEUMAIN_H__
