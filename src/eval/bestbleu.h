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

#include <bestscore.h>
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
bestbleu [-v][-r][-c][-x][-dyn][-n n][-y y][-u u][-s sm][-f f][-o opt]\n\
         nbest-file ref1 ... refR\n\
\n\
Output the best (oracle) BLEU score for an nbest list. The nbest file must\n\
contain concatenated sets of K-best translation candidates (in fixed or dynamic\n\
format), for successive source sentences. Each ref1..refR should contain a\n\
complete set of reference translations, line-aligned to the source text.\n\
Results are written to stdout, in the format specified by the -o option.\n\
\n\
NB: you must use -dyn if nbest-file is in dynamic format!\n\
\n\
Options:\n\
-v    Write progress reports to cerr (repeatable).\n\
-r    Reverse oracle: find worst translations instead of best.\n\
-c    Compute confidence intervals using bootstrap resampling (-o std only).\n\
-x    Scale sentence-level BLEU by normalized best-reference length, in order\n\
      to better approximate real BLEU. Only applies if -o nbest is selected.\n\
-dyn  Nbest-file is in dynamic format.\n\
-n    Use only the first n candidates in each nbest list. [0 = use all]\n\
-y    Max ngram len > 0 for calculating reference matches [4]\n\
-u    Max ngram len <= y for calculating BLEU score [y]\n\
-s    BLEU smoothing method: 1 = replace 0 ngram freqs with epsilon,\n\
      2 = add-1 smoothing (Lin and Och), 3 = mystery, 4 = mteval-v13a.pl\n\
      [0 = no smoothing]\n\
-f    When calculating oracle substitution BLEU, scale stats of current\n\
      hypothesis by n/f, where n is number of source sentences. Eg, 10 would\n\
      treat the current sentence as 1 out of 10 total sentences. Only applies\n\
      if -o nbest is selected. [0 => n]\n\
-o    Select output, one of:\n\
      - std = BLEU score of the oracle translation\n\
      - oracle = the full oracle translation, with nbest indexes before each hyp\n\
      - nbest = BLEU scores for hyps in <nbest-file> (line-aligned), in format:\n\
           src-index sent-level-bleu oracle-substitution-bleu\n\
";

/// Program bestbleu command line switches.
const char* const switches[] =
   {"v", "r", "c", "x", "s:", "f:", "o:",
    "dyn", "n:", "y:", "u:"};

/// Specific argument processing class for bestbleu program.
class ARG : public argProcessor, public bestscore::ARG
{
private:
   Logging::logger  m_vLogger;
   Logging::logger  m_dLogger;

public:
   Uint maxNgrams;       ///< max ngrams size for the BLEUstats
   Uint maxNgramsScore;  ///< max ngrams size when BLEUstats::score
   int iSmooth;          ///< smoothed-BLEU option

public:
   /**
    * Default constructor.
    * @param argc  same as the main argc
    * @param argv  same as the main argv
    */
   ARG(const int argc, const char* const argv[]) :
      argProcessor(ARRAY_SIZE(switches), switches, 2, -1, help_message, "-h", true),
      m_vLogger(Logging::getLogger("verbose.main.arg")),
      m_dLogger(Logging::getLogger("debug.main.arg")),
      maxNgrams(4),
      maxNgramsScore(0),
      iSmooth(DEFAULT_SMOOTHING_VALUE)
   {
      argProcessor::processArgs(argc, argv);
   }

   /// See argProcessor::printSwitchesValue()
   virtual void printSwitchesValue()
   {
      LOG_INFO(m_vLogger, "not printing arguments");
   }

   /// See argProcessor::processArgs()
   virtual void processArgs()
   {
      LOG_INFO(m_vLogger, "Processing arguments");

      // general flags

      bVerbose = checkVerbose("v");
      mp_arg_reader->testAndSet("r", rev);
      mp_arg_reader->testAndSet("c", conf);
      mp_arg_reader->testAndSet("x", sentscale);
      mp_arg_reader->testAndSet("dyn", bDyn);
      mp_arg_reader->testAndSet("n", maxn);
      mp_arg_reader->testAndSet("y", maxNgrams);
      mp_arg_reader->testAndSet("u", maxNgramsScore);
      mp_arg_reader->testAndSet("s", iSmooth);
      mp_arg_reader->testAndSet("f", f);
      mp_arg_reader->testAndSet("o", outopt);

      // checks

      if (maxNgramsScore == 0) maxNgramsScore = maxNgrams;
      if (maxNgramsScore > maxNgrams) maxNgramsScore = maxNgrams;
      if (!(maxNgrams > 0) || !(maxNgramsScore))
         error(ETFatal, "You must specify value for y and u greater then 0!");
      if (outopt != "std" && outopt != "oracle" && outopt != "nbest")
         error(ETFatal, "bad outopt value: %s", outopt.c_str());

      // mandatory arguments

      mp_arg_reader->testAndSet(0, "Nbest List File", sNBestFile);
      mp_arg_reader->getVars(1, sRefFiles);

      // Set values of computed "arguments": S, R, K.

      R = sRefFiles.size();
      S = countFileLines(sRefFiles.front());
      if (S == 0)
         error(ETFatal, "Empty reference file: %s", sRefFiles.front().c_str());
      if (!bDyn) {
         const Uint NB = countFileLines(sNBestFile);
         K = NB / S;
         if (NB < S || NB != S * K)
            error(ETFatal, "Inconsistency between nbest (%d) and ref (%d) linecounts", NB, S);
      }
   }
};
} // ends namespace bestbleu
} // ends namespace Portage

#endif // BESTBLEU_H
