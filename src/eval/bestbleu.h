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
         [-K <N-Best List Size>]\n\
         [-n nbest [-n nbest [ .. ]]]\n\
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
-ci   Compute confidence intervals using bootstrap resampling (-o std only).\n\
-bi   Displays best translation index.\n\
-bs   Displays best translation sentence.\n\
-ws   Displays worst translation sentence.\n\
-dyn  Nbest-file is in dynamic format (requires -K or -n).\n\
-K    Sets the maximum N-Best List size in Dynamic mode\n\
-n    Use only the first n candidates in each nbest list.\n\
      -n may be repeated with different values. [use the whole list]\n\
-y    Max ngram len > 0 for calculating reference matches [4]\n\
-u    Max ngram len <= y for calculating BLEU score [y]\n\
-distrib For files containing distributions.\n\
-o    Select output, one of:\n\
      - std = BLEU score of the oracle translation\n\
      - nbest = BLEU scores for hyps in <nbest-file> (line-aligned), in format:\n\
           src-index sent-level-bleu oracle-substitution-bleu\n\
";

/// Program bestbleu command line switches.
const char* const switches[] =
   {"v", "r", "bs", "ws", "ci", "bi", "o:",
    "K:", "distrib",
    "dyn", "n:", "y:", "u:"};

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
   string outopt;

   string sNBestFile;  ///< file containing the nbest
   vector<string> sRefFiles;  ///< list of reference files.

   Uint maxNumBest;       ///< maximum of hypotheses per nbest to use
   vector<Uint> numBest;  ///< limiting nbest size to < nbest.size

   Uint S;  ///< number of sources
   Uint R;  ///< number of references
   Uint K;  ///< number of hypotheses per source

   bool bDyn;  ///< dynamic sized nbest list provided.

   Uint maxNgrams;       ///< max ngrams size for the BLEUstats
   Uint maxNgramsScore;  ///< max ngrams size when BLEUstats::score

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
      bVerbose(false),
      bBestIndex(false),
      bBestSentence(false),
      bWorseSentence(false),
      bWorseScore(false),
      bCi(false),
      bDistribution(false),
      outopt("std"),
      maxNumBest(0),
      S(0), R(0), K(0),
      bDyn(false),
      maxNgrams(4),
      maxNgramsScore(0)
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

      // general flags

      bVerbose = checkVerbose("v");
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

      mp_arg_reader->testAndSet("dyn", bDyn);
      mp_arg_reader->testAndSet("y", maxNgrams);
      mp_arg_reader->testAndSet("u", maxNgramsScore);
      mp_arg_reader->testAndSet("o", outopt);

      // checks

      if (maxNgramsScore == 0) maxNgramsScore = maxNgrams;
      if (maxNgramsScore > maxNgrams) maxNgramsScore = maxNgrams;
      if (!(maxNgrams > 0) || !(maxNgramsScore))
         error(ETFatal, "You must specify value for y and u greater then 0!");
      if (outopt != "std" && outopt != "nbest")
         error(ETFatal, "bad outopt value: %s", outopt.c_str());

      // mandatory arguments

      mp_arg_reader->testAndSet("K", K);
      mp_arg_reader->testAndSet(0, "Nbest List File", sNBestFile);
      mp_arg_reader->getVars(1, sRefFiles);

      // Set values of computed "arguments": S, R, K.

      R = sRefFiles.size();
      S = countFileLines(sRefFiles.front());
      if (S == 0)
         error(ETFatal, "Empty reference file: %s", sRefFiles.front().c_str());

      if (!bDyn) {
         const Uint NB = countFileLines(sNBestFile);
         const Uint K0 = NB / S;
         if (NB < S || NB != S * K0)
            error(ETFatal, "Inconsistency between nbest (%d) and ref (%d) linecounts", NB, S);
            error(ETFatal, "Inconsistency between nbest and ref number of lines\n\tnbest: %d, ref %s: %d => K: %d",
                           NB, S, sRefFiles.front().c_str(), K0);

         // If -K is provided, use it as sanity checking (we
         // keep it for backward compatibility anyway).
         if (K != 0 && K0 != K)
            error(ETFatal, "-K %d specified, but %d inferred from ref and nbest files",
                           K, K0);
         K = K0;
      }

      // multi arguments n switch

      if (mp_arg_reader->getSwitch("n")) {
         vector<string>  snBest;
         mp_arg_reader->testAndSet("n", snBest);
         int next(0);
         numBest.reserve(snBest.size());
         for (vector<string>::const_iterator it(snBest.begin()); it!=snBest.end(); ++it)
         {
            if (!conv(*it, next))
               error(ETFatal, "%s is not a number", it->c_str());
            if (next <= 0)
               error(ETFatal, "Invalid value for nbest");
            numBest.push_back((Uint)next);
            maxNumBest = max(maxNumBest, (Uint)next);
         }
      } else {
         if (K == 0) {
            error(ETWarn, "Using -dyn without -K or -n: assuming -K <maxint>");
            K = Uint(-1);
         }
         numBest.push_back(K);
         maxNumBest = K;
         LOG_WARN(m_vLogger, "Using single default value for n: K=%d", K);
      }

      // Checking integrity

      if (K != 0 && K < maxNumBest)
         error(ETFatal, "numBest is greater than the number of candidate translations");
   }
};
} // ends namespace bestbleu
} // ends namespace Portage

#endif // BESTBLEU_H
