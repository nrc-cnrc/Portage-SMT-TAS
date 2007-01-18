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
// DEFINITION OF RANDOM NUMBER GENERATOR
/// Definition of the random number generator.
typedef boost::taus88 RNG_gen;
//typedef boost::mt19937 RNG_gen;   // Alternate RNG
/// Definition of a distribution.
typedef boost::normal_distribution<double>  RNG_dist;
/// Definition of a random generator based on RNG and a distribution.
typedef boost::variate_generator<RNG_gen, RNG_dist> RNG_type;


////////////////////////////////////////////////////////////////////////////////
// CONSTANTS USED IN TRAINING/TESTING
static const unsigned int NUM_INIT_RUNS(7);
static const double FTOL(0.01);
static const double BLEUTOL(0.0001);


////////////////////////////////////////////////////////////////////////////////
// HELP MESSAGE
/// Program rescore_train's usage.
static char help_message[] = "\n\
rescore_train [-vn][-a F][-f floor][-p ff-pref][-dyn]\n\
              [-wi powell weight input file] [-wo powell weight output file]\n\
              model_in model_out src nbest ref1 .. refN\n\
\n\
Train a rescoring model on given src, nbest, and ref1 .. refN texts. All text\n\
files must contain one space-tokenized segment per line. For each source\n\
segment in src, there must be a block of candidate translations in nbest\n\
and one reference translation per ref[1..R]. Blocks must be contiguous, and,\n\
unless -dyn is given, the same size for all source segments (pad with blank\n\
lines if necessary).\n\
\n\
The model_in file specifies a set of features and optional initial weights;\n\
model_out is the output from this program, having weights set to optimum values.\n\
Each line in these files is in the format:\n\
\n\
   feature_name[:arg] [weight]\n\
\n\
where <arg> is an optional argument to the feature.\n\
\n\
Use -H for more info on features.\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr.\n\
-n    Normalize weights so maximum is 1 (recommended).\n\
-a    Also read in phrase alignment file F.\n\
-f    Floor weights at 0, beginning with zero-based index i. [don't]\n\
-wi   Read in feature weights for Powell from file F (e.g. from previous Powell run)\n\
-wo   Print feature weights for best Powell runs to file F\n\
-p    Prepend ff-pref to file names for FileFF features\n\
-dyn  Indicates that the nbest list and FileFF files are in variable-size\n\
      format, with lines prefixed by: \"<source#>\\t\" (starting at 0)\n\
";

////////////////////////////////////////////////////////////////////////////////
// ARGUMENTS PROCESSING CLASS
/// Program rescore_train's allowed command line arguments.
const char* const switches[] = {"n", "f:", "dyn", "p:", "a:", "v", "K:", "wi:", "wo:"};

/// Specific argument processing class for rescore_train program
class ARG : public argProcessor
{
private:
   Logging::logger  m_vLogger;
   Logging::logger  m_dLogger;

public:
   bool     bVerbose;           ///< is verbose set
   bool     bNormalize;         ///< should the returned weights vector be normalized
   bool     bReadAlignments;    ///< do we need to read the alignments
   bool     bIsDynamic;         ///< are we running in dynamic mode => dynamic nbest list size => dynamic file reading
   Uint     flor;               ///< index of the first floored weight
   string   weight_infile;      ///< input config file
   string   weight_outfile;     ///< output config file
   string   model_in;           ///< input config file
   string   model_out;          ///< output config file
   string   src_file;           ///< source sentences file
   string   nbest_file;         ///< hypotheses sentences file
   vector<string>   refs_file;  ///< reference translations file
   string   alignment_file;     ///< alignment file
   Uint     K;                  ///< fix mode => number of hypotheses per source
   Uint     S;                  ///< number of sources
   string   ff_pref;            ///< feature function prefix (for rat.sh)

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
      , bReadAlignments(false)
      , bIsDynamic(false)
      , flor(10000000)
      , weight_infile(""), weight_outfile("")
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
            LOG_DEBUG(m_dLogger, "Normalize: %s", (bNormalize ? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "ReadAlignment: %s", (bReadAlignments ? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "K: %d", K);
            LOG_DEBUG(m_dLogger, "S: %d", S);
            LOG_DEBUG(m_dLogger, "flor: %d", flor);
            LOG_DEBUG(m_dLogger, "feature weights read from file: %s", weight_infile.c_str());
            LOG_DEBUG(m_dLogger, "feature weights written to file: %s", weight_outfile.c_str());
            LOG_DEBUG(m_dLogger, "ff_pref: %s", ff_pref.c_str());
            LOG_DEBUG(m_dLogger, "alignment file name: %s", alignment_file.c_str());
            LOG_DEBUG(m_dLogger, "model in file name: %s", model_in.c_str());
            LOG_DEBUG(m_dLogger, "model out file name: %s", model_out.c_str());
            LOG_DEBUG(m_dLogger, "source file name: %s", src_file.c_str());
            LOG_DEBUG(m_dLogger, "nbest file name: %s", nbest_file.c_str());
            LOG_DEBUG(m_dLogger, "Number of reference files: %d", refs_file.size());
            std::stringstream oss1;
            for (Uint i(0); i<refs_file.size(); ++i)
               {
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
      mp_arg_reader->testAndSet("wi", weight_infile);
      mp_arg_reader->testAndSet("wo", weight_outfile);
      mp_arg_reader->testAndSet("p", ff_pref);

      mp_arg_reader->testAndSet(0, "model_in", model_in);
      mp_arg_reader->testAndSet(1, "model_out", model_out);
      mp_arg_reader->testAndSet(2, "src", src_file);
      mp_arg_reader->testAndSet(3, "nbest", nbest_file);
      mp_arg_reader->getVars(4, refs_file);

      bReadAlignments = !alignment_file.empty();
         
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
   }
};
} // ends namespace rescore_train
} // ends namespace Portage

#endif   // __RESCORE_TRAIN_H__
