/**
 * @author Samuel Larkin
 * @file rescore_test.h  Program rescore_test command line arguments
 * processing
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
            
#ifndef __RESCORE_TEST_H__
#define __RESCORE_TEST_H__

#include <argProcessor.h>
#include <errors.h>
#include <rescore_io.h>
#include <file_utils.h>

namespace Portage {
/// Program rescore_test's namespace
/// Prevents pollution of global namespace
namespace RescoreTest {

   /// Program rescore_test usage
   static char help_message[] = "\n\
rescore_test [-v][-p ff-pref][-dyn] model src nbest ref1 ... refN\n\
\n\
Test a rescoring model on given src, nbest and ref1 ... refN texts. See\n\
rescore_train -h for information on features and file formats.\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr.\n\
-p    Prepend ff-pref to file names for FileFF features\n\
-dyn  Indicates that the nbest list is in variable-size format, with\n\
      lines of the form: <source#>\\t<CandidateTranslation>\n\
";

   ////////////////////////////////////////////////////////////////////////////////
   // ARGUMENTS PROCESSING CLASS
   /// Program rescore_test allowed command line arguments
   const char* const switches[] = {"dyn", "p:", "v", "K:"};

   /// Specific argument processing class for rescore_test program
   class ARG : public argProcessor
   {
      private:
         Logging::logger  m_vLogger;
         Logging::logger  m_dLogger;

      public:
         bool     bVerbose;           ///< Should we display progress
         bool     bIsDynamic;         ///< are nbest lists in dynamic file format
         Uint     K;                  ///< number of hypotheses per source
         Uint     S;                  ///< number of source sentences
         string   ff_pref;            ///< feature function prefix
         string   model;              ///< config file
         string   src_file;           ///< file containing source sentences
         string   nbest_file;         ///< file containing nbest lists
         vector<string>   refs_file;  ///< List of reference files

      public:
      /**
       * Default constructor.
       * @param argc  same as the main argc
       * @param argv  same as the main argv
       */
      ARG(const int argc, const char* const argv[])
         : argProcessor(ARRAY_SIZE(switches), switches, 4, -1, help_message, "-h", true)
         , m_vLogger(Logging::getLogger("verbose.main.arg"))
         , m_dLogger(Logging::getLogger("debug.main.arg"))
         , bVerbose(false)
         , bIsDynamic(false)
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
            LOG_DEBUG(m_dLogger, "K: %d", K);
            LOG_DEBUG(m_dLogger, "S: %d", S);
            LOG_DEBUG(m_dLogger, "ff_pref: %s", ff_pref.c_str());
            LOG_DEBUG(m_dLogger, "model file name: %s", model.c_str());
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
         mp_arg_reader->testAndSet("p", ff_pref);

         mp_arg_reader->testAndSet(0, "model_in", model);
         mp_arg_reader->testAndSet(1, "src", src_file);
         mp_arg_reader->testAndSet(2, "nbest", nbest_file);
         mp_arg_reader->getVars(3, refs_file);
         
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
} // ends namespace RescoreTest
} // ends namespace Portage

#endif   // __RESCORE_TEST_H__
