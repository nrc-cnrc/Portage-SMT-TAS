/**
 * @author Samuel Larkin based on George Foster, based on "rescore" by Aaron Tikuisis
 * @file rescore_translate.h  Program rescore_translate command line arguments
 * processing.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
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
rescore_translate [-vnsc][-p ff-pref][-a F][-dyn][-kout k][-co f][-fv f][-sc f]\n\
                  model src nbest\n\
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
-co   Read entries from file <f> that is line-aligned with nbest, and write \n\
      corresponding rescored entries to <f>.resc. This doesn't work with -mbr,\n\
      nor with variable-sized nbest lists.\n\
-fv   Write feature value vectors, line-aligned with output, to file <f>. No\n\
      sentence-index prefix. Same restrictions as for -co.\n\
-sc   Write hypothesis scores, line-aligned with output, to file <f>. This is\n\
      the same value written by -s. SAme restrictions as for -co.\n\
-mbr  Minimum Bayes risk: Determine hypothesis with min risk rather than max\n\
      score.  Note that this gives you 1-best output only! [don't]\n\
-gf   Scale all sentence probabilities by global factor <f> in MBR calculation [1]\n\
-bs   Use BLEU smoothing method <b> in MBR (see bleumain -h) [0 = no smoothing]\n\
-l    Use only top <l> hypotheses for MBR, sort by rescored sentence scores [all]\n\
-dump-for-mira PREFIX  Instead of rescoring, dump the files needed for mira.\n\
";


   ////////////////////////////////////////////////////////////////////////////////
   // ARGUMENTS PROCESSING CLASS
   /// Program rescore_translate allowed command line switches.
   const char* const switches[] = {
      "dyn", "max:", "p:", "a:", "v", "K:", "n", "s", "c", "kout:", "co:", "fv:", "sc:",
      "mbr", "gf:", "bs:", "l:", "dump-for-mira:"
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
         bool     bMbr;             ///< print Minimum Bayes risk hyp. rather than max. prob. one
         Uint     kout;             ///< Number of output hyps per source
         string   cofile;           ///< File that is line-aligned with nbest
         string   fvfile;           ///< Name of output feature-value file
         string   scfile;           ///< Name of output score file
         Uint     kmbr;             ///< Number of hyps per source used in MBR
         Uint     K;                ///< Number of hypotheses per source
         Uint     S;                ///< Number of sources
         Uint     smooth_bleu;      ///< Smoothing method for sentence-level BLEU
         float    glob_scale;       ///< Global scaling factor for sentence probabilities
         string   ff_pref;          ///< Feature function prefix
         string   model;            ///< config file containing feature functions and their weights
         string   src_file;         ///< file containing source sentences
         string   nbest_file;       ///< file containing nbest lists
         string   alignment_file;   ///< file containing alignments
         string   dump_for_mira;    ///< file prefix for dumping data files for mira

      public:
      /**
       * Default constructor.
       * @param argc  same as the main argc
       * @param argv  same as the main argv
       */
      ARG(int argc, const char* const argv[])
         : argProcessor(ARRAY_SIZE(switches), switches, 3, 3, help_message, "-h", true)
         , m_vLogger(Logging::getLogger("verbose.main.arg"))
         , m_dLogger(Logging::getLogger("debug.main.arg"))
         , bVerbose(false)
         , bIsDynamic(false)
         , bPrintRank(false)
         , print_scores(false)
         , conf_scores(false)
         , bMbr(false)
         , kout(1)
         , cofile("")
         , fvfile("")
         , scfile("")
         , kmbr(0)
         , K(0)
         , S(0)
         , smooth_bleu(0)
         , glob_scale(1.0)
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
            LOG_DEBUG(m_dLogger, "Minimum Bayes Risk: %s", (bMbr ? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "Global scaling factor for MBR: %f", glob_scale);
            LOG_DEBUG(m_dLogger, "BLEU smoothing method for MBR: %d", smooth_bleu);
            LOG_DEBUG(m_dLogger, "K: %d", K);
            LOG_DEBUG(m_dLogger, "K for MBR: %d", kmbr);
            LOG_DEBUG(m_dLogger, "S: %d", S);
            LOG_DEBUG(m_dLogger, "ff_pref: %s", ff_pref.c_str());
            LOG_DEBUG(m_dLogger, "model file name: %s", model.c_str());
            LOG_DEBUG(m_dLogger, "source file name: %s", src_file.c_str());
            LOG_DEBUG(m_dLogger, "nbest file name: %s", nbest_file.c_str());
            LOG_DEBUG(m_dLogger, "alignment file name: %s", alignment_file.c_str());
            LOG_DEBUG(m_dLogger, "dump-for-mira prefix: %s", dump_for_mira.c_str());
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
         mp_arg_reader->testAndSet("co", cofile);
         mp_arg_reader->testAndSet("fv", fvfile);
         mp_arg_reader->testAndSet("sc", scfile);
         mp_arg_reader->testAndSet("l", kmbr);
         mp_arg_reader->testAndSet("mbr", bMbr);
         mp_arg_reader->testAndSet("gf", glob_scale);
         mp_arg_reader->testAndSet("bs", smooth_bleu);
         mp_arg_reader->testAndSet("dump-for-mira", dump_for_mira);

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

         // Some options are only valid for MBR rescoring, check them
         if (!bMbr) {
           if (smooth_bleu!=0)
             error(ETWarn, "BLEU smoothing argument %d will be ignored, only needed for MBR rescoring", smooth_bleu);
           if (glob_scale!=1.0)
             error(ETWarn, "Global scaling factor %f will be ignored, only needed for MBR rescoring", glob_scale);
           if (kmbr!=0)
             error(ETWarn, "N-best length argument %d will be ignored, only needed for MBR rescoring", kmbr);
         }
         else {
           if (kout!=1)
             error(ETWarn, "N-best output length argument %d will be ignored, MBR rescoring generates 1-best only", kmbr);
         }
      }
   };
} // ends namespace rescore_translate
} // ends namespace Portage

#endif   // __RESCORE_TRANSLATE_H__
