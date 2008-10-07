/**
 * @author Samuel Larkin based on George Foster, based on "rescore" by Aaron Tikuisis
 * @file rescore_translate.h  Program rescore_translate command line arguments
 * processing.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef __RESCORE_TRANSLATE_H__
#define __RESCORE_TRANSLATE_H__

#include "argProcessor.h"
#include "file_utils.h"

namespace Portage {
/// Program rescore_translate's namespace
/// Prevents pollution of the global namespace.
namespace rescore_translate {

   /// Program rescore_translate usage.
   static char help_message[] = "\n\
rescore_translate [-vnsc][-p ff-pref][-a F][-dyn][-kout k] model src nbest\n\
\n\
Translate a src text using a rescoring model to choose the best candidate\n\
translation from the given nbest list.  Output is written to stdout, one\n\
line for each line in src. See rescore_train -h for more information on\n\
features and file formats.\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr (repeatable) [don't].\n\
-n    Print rank of the hypothesis within the N-best list too (index starting\n\
      at 1).\n\
-s    Print the logprob of each hypothesis prior to the hypothesis.\n\
-c    With -s, print out logprobs normalized over each nbest list.\n\
-p    Prepend ff-pref to file names for FileFF features\n\
-a    Also read in phrase alignment file F.\n\
-dyn  Indicates that the nbest list is in variable-size format, with\n\
      lines of the form: <source#>\\t<CandidateTranslation>\n\
-kout Print <k> best hypotheses per source sent, or all if k is 0. Unless -dyn\n\
      is specified, this will pad with blank lines if necessary to write\n\
      exactly <k> lines per source sentence. [1]\n\
";

   ////////////////////////////////////////////////////////////////////////////////
   // ARGUMENTS PROCESSING CLASS
   /// Program rescore_translate allowed command line switches.
   const char* const switches[] = {
      "dyn", "max:", "p:", "a:", "v", "K:", "n", "s", "c", "kout:"
   };
   /// Specific argument processing class for rescore_translate program
   class ARG : public argProcessor
   {
      private:
         Logging::logger  m_vLogger;
         Logging::logger  m_dLogger;

      public:
         bool     bVerbose;         ///< Should we display progress
         bool     bIsDynamic;       ///< Are we in dynamic nbest list size
         bool     bPrintRank;       ///< Should we print the rank of the best sentence
         bool     print_scores;     ///< Output hyp score(s)
         bool     conf_scores;      ///< Normalize hyp score(s) before printing
         Uint     kout;             ///< Number of output hyps per source
         Uint     K;                ///< Number of hypotheses per source
         Uint     S;                ///< Number of sources
         string   ff_pref;          ///< Feature function prefix
         string   model;            ///< config file containing feature functions and their weights
         string   src_file;         ///< file containing source sentences
         string   nbest_file;       ///< file containing nbest lists
         string   alignment_file;   ///< file containing alignments

      public:
      /**
       * Default constructor.
       * @param argc  same as the main argc
       * @param argv  same as the main argv
       */
      ARG(const int argc, const char* const argv[])
         : argProcessor(ARRAY_SIZE(switches), switches, 3, 3, help_message, "-h", true)
         , m_vLogger(Logging::getLogger("verbose.main.arg"))
         , m_dLogger(Logging::getLogger("debug.main.arg"))
         , bVerbose(false)
         , bIsDynamic(false)
         , bPrintRank(false)
         , print_scores(false)
         , conf_scores(false)
         , kout(1)
         , K(0)
         , S(0)
         , ff_pref("")
      {
         argProcessor::processArgs(argc, argv);
      }

      /// See argProcessor::printSwitchesValue()
      virtual void printSwitchesValue()
      {
         if (m_dLogger->isDebugEnabled())
         {
            LOG_DEBUG(m_dLogger, "Verbose: %s", (bVerbose ? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "Dynamic: %s", (bIsDynamic ? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "PrintRank: %s", (bPrintRank ? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "K: %d", K);
            LOG_DEBUG(m_dLogger, "S: %d", S);
            LOG_DEBUG(m_dLogger, "ff_pref: %s", ff_pref.c_str());
            LOG_DEBUG(m_dLogger, "model file name: %s", model.c_str());
            LOG_DEBUG(m_dLogger, "source file name: %s", src_file.c_str());
            LOG_DEBUG(m_dLogger, "nbest file name: %s", nbest_file.c_str());
            LOG_DEBUG(m_dLogger, "alignment file name: %s", alignment_file.c_str());
         }
      }

      /// See argProcessor::processArgs()
      virtual void processArgs()
      {
         LOG_INFO(m_vLogger, "Processing arguments");

         bVerbose = checkVerbose("v");
         mp_arg_reader->testAndSet("a", alignment_file);
         mp_arg_reader->testAndSet("p", ff_pref);
         mp_arg_reader->testAndSet("n", bPrintRank);
         mp_arg_reader->testAndSet("s", print_scores);
         mp_arg_reader->testAndSet("c", conf_scores);
         mp_arg_reader->testAndSet("kout", kout);

         mp_arg_reader->testAndSet(0, "model", model);
         mp_arg_reader->testAndSet(1, "src", src_file);
         mp_arg_reader->testAndSet(2, "nbest", nbest_file);

         if (conf_scores && !print_scores)
            error(ETWarn, "Ignoring -c, because -s has not been specified");

         mp_arg_reader->testAndSet("dyn", bIsDynamic);
         if (!bIsDynamic) {
            const Uint SK = countFileLines(nbest_file);
            S = countFileLines(src_file);
            K = SK / S;
            if (K == 0 || SK != S*K)
               error(ETFatal, "Inconsistency between nbest and src number of lines\n\tnbest: %d, src: %d => K: %d", SK, S, K);
            if (kout > K) {
               error(ETWarn, "-kout %d requests more hypotheses than are available: changing to -kout %d",
                     kout, K);
               kout = K;
            }
         }

         // K switch is depricated but if user pass a K option
         // we check for misusage and validity of K value
         if (mp_arg_reader->getSwitch("K:")) {
            error(ETWarn, "K argument will be ignored, obsolete argument");
            Uint falseK(0);
            mp_arg_reader->testAndSet("K:", falseK);
            if (K != falseK)
               error(ETFatal, "different values of k found, potential error");
         }
      }
   };
} // ends namespace rescore_translate
} // ends namespace Portage

#endif   // __RESCORE_TRANSLATE_H__
