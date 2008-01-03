/**
 * @author Samuel Larkin
 * @file rescore_train.h  Program rescore_train command line arguments
 * processing.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
*/
            
#ifndef __RESCORE_TRAIN_H__
#define __RESCORE_TRAIN_H__


#include <argProcessor.h>
#include <errors.h>
#include <file_utils.h>
#include "rescore_io.h"
#include "featurefunction.h"
#include <boost/random.hpp>
#include <boost/random/variate_generator.hpp>


namespace Portage { 
/// Program rescore_train's namespace.
/// Prevents pollution in global namespace.
namespace rescore_train {

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS USED IN TRAINING/TESTING
static const unsigned int NUM_INIT_RUNS(7);
static const double FTOL(0.01);
static const double BLEUTOL(0.0001);


////////////////////////////////////////////////////////////////////////////////
// HELP MESSAGE
/// Program rescore_train's usage.
static char help_message[] = "\n\
rescore_train [-vn][-sm smoothing][-a F][-f floor][-p ff-pref][-dyn]\n\
              [-r n][-e][-win nl][-wi powell-wt-file][-wo powell-wt-file][-s seed]\n\
              model_in model_out src nbest ref1 .. refN\n\
\n\
Train a rescoring model on given src, nbest, and ref1 .. refN texts. All text\n\
files must contain one space-tokenized segment per line. For each source\n\
segment in src, there must be a block of candidate translations in nbest\n\
and one reference translation per ref[1..R]. Blocks must be contiguous, and,\n\
unless -dyn is given, the same size for all source segments (pad with blank\n\
lines if necessary).\n\
\n\
The model_in file specifies a set of features and optional initial weight\n\
distributions. Each line is in the format:\n\
\n\
"

#ifndef NO_COMPUTED_FF

"   feature_name[:arg] [U(min,max)|N(mean,sigma)]\n\
\n\
where <arg> is an optional argument to the feature, and U() or N() means to\n\
choose initial weights for Powell's algorithm from uniform or normal distns\n\
with the given parameters [N(0,1)]. The model_out file is in the same format\n\
as model_in, but with the distribution replaced by an optimal weight. Comments\n\
may be included by placing # at the beginning of a line.\n\
\n\
Use -H for more info on features.\n\
"

#else

"   FileFF:file[,column] [U(min,max)|N(mean,sigma)]\n\
\n\
where <file> is a file of pre-computed feature values, and <column> is the\n\
desired column in this file. Each feature file should have the same number of\n\
lines as the nbest file.  U() or N() means to choose initial weights for\n\
Powell's algorithm from uniform or normal distns with the given parameters\n\
[N(0,1)]. The model_out file is in the same format as model_in, but with the\n\
distribution replaced by an optimal weight.\n\
"

#endif

"\n\
Options:\n\
\n\
-v    Write progress reports to cerr (repeatable) [don't].\n\
-n    Normalize output weights so maximum is 1 (recommended).\n\
-sm   bleu smoothing number 1 2 3 [1]\n\
-a    Also read in phrase alignment file F.\n\
-f    Floor output weights at 0, beginning with zero-based index i. [don't]\n\
-p    Prepend ff-pref to file names for FileFF features\n\
-dyn  Indicates that the nbest list and FileFF files are in variable-size\n\
      format, with lines prefixed by: \"<source#>\\t\" (starting at 0)\n\
-r    Use <n> runs of Powell's alg [0 = determine number of runs automatically]\n\
-e    Use approx expectation to determine stopping pt, with max given by -r [don't]\n\
-win  Use only the first <nl> lines from feature file read by -wi (0 for all) [3]\n\
-wi   Read initial feature wt vectors for Powell from file F (one vect per line)\n\
      NB: the number of vectors actually used depends on the -r and -e settings.\n\
-wo   Append final feature wts from Powell runs to file F, ordered by decr BLEU\n\
-s    Use <seed> as initial random seed [0 = use fixed seed for repeatability]\n\
-y    maximum NGRAMS for calculating BLEUstats matches [4]\n\
-u    maximum NGRAMS for calculating BLEUstats score [y]\n\
      where 1 <= y, 1 <= u <= y\n\
";

////////////////////////////////////////////////////////////////////////////////
// ARGUMENTS PROCESSING CLASS
/// Program rescore_train's allowed command line arguments.
const char* const switches[] = {"n", "f:", "dyn", "p:", "a:", "v", "K:", "r:", "e", "win:", "wi:", "wo:", "s:", "y:", "u:", "sm:"};

/// Specific argument processing class for rescore_train program
class ARG : public argProcessor
{
private:
   Logging::logger  m_vLogger;
   Logging::logger  m_dLogger;

public:
   bool     bVerbose;           ///< is verbose set
   bool     bNormalize;         ///< should the returned weights vector be normalized
   bool     bIsDynamic;         ///< are we running in dynamic mode => dynamic nbest list size => dynamic file reading
   Uint     flor;               ///< index of the first floored weight
   Uint     num_powell_runs;    ///< number of runs of Powell's alg
   bool     approx_expect;      ///< use approx. expectation alg to determine stopping pt
   Uint     weight_infile_nl;   ///< num lines read from beginning of weight_infile
   string   weight_infile;      ///< input weights file
   string   weight_outfile;     ///< output weights file
   Uint     seed;               ///< random seed
   string   model_in;           ///< input config file
   string   model_out;          ///< output config file
   string   src_file;           ///< source sentences file
   string   nbest_file;         ///< hypotheses sentences file
   vector<string>   refs_file;  ///< reference translations file
   string   alignment_file;     ///< alignment file
   Uint     K;                  ///< fix mode => number of hypotheses per source
   Uint     S;                  ///< number of sources
   string   ff_pref;            ///< feature function prefix (for rat.sh)
   Uint     maxNgrams;          ///< holds the max ngrams size for the BLEUstats
   Uint     maxNgramsScore;     ///< holds the max ngrams size when BLEUstats::score
   Uint     sm;                 ///< bleu smoothing

public:
   /**
    * Default constructor.
    * @param argc  same as the main argc
    * @param argv  same as the main argv
    */
   ARG(const int argc, const char* const argv[])
      : argProcessor(ARRAY_SIZE(switches), switches, 5, -1, help_message, "-h", true,
                     FeatureFunctionSet::help().c_str(), "-H")
      , m_vLogger(Logging::getLogger("verbose.main.arg"))
      , m_dLogger(Logging::getLogger("debug.main.arg"))
      , bVerbose(false)
      , bNormalize(false)
      , bIsDynamic(false)
      , flor(10000000)
      , num_powell_runs(0)
      , approx_expect(false)
      , weight_infile_nl(3)
      , weight_infile("")
      , weight_outfile("")
      , seed(0)
      , K(0)
      , S(0)
      , ff_pref("")
      , maxNgrams(4)
      , maxNgramsScore(0)
      , sm(1)
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
            LOG_DEBUG(m_dLogger, "Normalize: %s", (bNormalize ? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "K: %d", K);
            LOG_DEBUG(m_dLogger, "S: %d", S);
            LOG_DEBUG(m_dLogger, "flor: %d", flor);
            LOG_DEBUG(m_dLogger, "num powell runs: %d", num_powell_runs);
            LOG_DEBUG(m_dLogger, "approx expect: %s", (approx_expect ? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "num feature weights to read in: %d", weight_infile_nl);
            LOG_DEBUG(m_dLogger, "feature weights read from file: %s", weight_infile.c_str());
            LOG_DEBUG(m_dLogger, "feature weights written to file: %s", weight_outfile.c_str());
            LOG_DEBUG(m_dLogger, "random seed: %d", seed);
            LOG_DEBUG(m_dLogger, "ff_pref: %s", ff_pref.c_str());
            LOG_DEBUG(m_dLogger, "alignment file name: %s", alignment_file.c_str());
            LOG_DEBUG(m_dLogger, "model in file name: %s", model_in.c_str());
            LOG_DEBUG(m_dLogger, "model out file name: %s", model_out.c_str());
            LOG_DEBUG(m_dLogger, "source file name: %s", src_file.c_str());
            LOG_DEBUG(m_dLogger, "nbest file name: %s", nbest_file.c_str());
            LOG_DEBUG(m_dLogger, "Number of reference files: %d", refs_file.size());
            LOG_DEBUG(m_dLogger, "Maximum Ngrams size: %d", maxNgrams);
            LOG_DEBUG(m_dLogger, "Maximum Ngrams size for scoring BLEU: %d", maxNgramsScore);
            LOG_DEBUG(m_dLogger, "Bleu smoothing", sm);
            std::stringstream oss1;
            for (Uint i(0); i<refs_file.size(); ++i) {
               oss1 << "- " << refs_file[i].c_str() << " ";
            }
            LOG_DEBUG(m_dLogger, oss1.str().c_str());
         }
   }

   /// See argProcessor::processArgs()
   virtual void processArgs()
   {
      LOG_INFO(m_vLogger, "Processing arguments");

      mp_arg_reader->testAndSet("v", bVerbose);
      if ( getVerboseLevel() > 0 ) bVerbose = true;
      if ( bVerbose && getVerboseLevel() < 1 ) setVerboseLevel(1);
      mp_arg_reader->testAndSet("n", bNormalize);
      mp_arg_reader->testAndSet("f", flor);
      mp_arg_reader->testAndSet("r", num_powell_runs);
      mp_arg_reader->testAndSet("e", approx_expect);
      mp_arg_reader->testAndSet("win", weight_infile_nl);
      mp_arg_reader->testAndSet("wi", weight_infile);
      mp_arg_reader->testAndSet("wo", weight_outfile);
      mp_arg_reader->testAndSet("s", seed);
      mp_arg_reader->testAndSet("p", ff_pref);
      mp_arg_reader->testAndSet("a", alignment_file);
      mp_arg_reader->testAndSet("sm", sm);

      mp_arg_reader->testAndSet("y", maxNgrams);
      mp_arg_reader->testAndSet("u", maxNgramsScore);
      // Make sure we are not trying to compute the BLEU::score on some
      // higher ngrams than what was calculated.
      if (maxNgramsScore == 0) maxNgramsScore = maxNgrams;
      if (maxNgramsScore > maxNgrams) maxNgramsScore = maxNgrams;
      if (!(maxNgrams > 0) || !(maxNgramsScore))
         error(ETFatal, "You must specify value for y and u greater then 0!");
               
      mp_arg_reader->testAndSet(0, "model_in", model_in);
      mp_arg_reader->testAndSet(1, "model_out", model_out);
      mp_arg_reader->testAndSet(2, "src", src_file);
      mp_arg_reader->testAndSet(3, "nbest", nbest_file);
      mp_arg_reader->getVars(4, refs_file);

      mp_arg_reader->testAndSet("dyn", bIsDynamic);
      if (!bIsDynamic) {
         const Uint SK = countFileLines(nbest_file);
         S = countFileLines(src_file);
         K = SK / S;
         if (K == 0 || SK != S*K)
            error(ETFatal, "Inconsistency between nbest and src number of lines\n\tnbest: %d, src: %d => K: %d", SK, S, K);
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

      if (approx_expect && num_powell_runs <= NUM_INIT_RUNS+2)
         error(ETWarn, "Using -e with -r set to <= %d forces exactly %d iters of Powell's alg!",
               NUM_INIT_RUNS+2, NUM_INIT_RUNS+2);

   }
};
} // ends namespace rescore_train
} // ends namespace Portage

#endif   // __RESCORE_TRAIN_H__
