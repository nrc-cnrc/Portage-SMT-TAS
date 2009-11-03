/**
 * @author Nicola Ueffing
 * @file worderrors.h tagging each word as correct/incorrect based on WER or PER
 *
 * $Id$
 *
 * Evaluation Module
 *
 * This file contains some stuff needed for WER computation.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __WORDERROR_H__
#define __WORDERROR_H__

#include "portage_defs.h"
#include "argProcessor.h"
#include <vector>

namespace Portage
{
  /// Program worderror namespace.
  /// Prevents polluting the global namespace with some worderror specifics.
  namespace worderror
  {
    /// Program worderror usage.
    static char help_message[] = "\n\
worderrors [-v][-per][-n N] testfile ref1 ref2 ... refn\n \
\n\
Tags each word in a translation as correct or incorrect, based either on\n\
mWER or mPER, using the reference files ref1, ... , refn.\n\
Errors are represented by 0s and correct words by 1s.\n\
Each file should have one sentence per line.\n\
The testfile can contain N translations per reference, where N is constant.\n\
\n\
Options:\n\
\n\
-v         Write progress reports to cerr (repeatable) [don't].\n\
-per       Calculate mPER instead of mWER.\n\
-n N       Number of translation hypotheses per source sentence [1]\n\n\
";

    /// Program worderror command line switches.
      const char* const switches[] = {"v", "per", "n:"};
    /// Specific argument processing class for worderror program.
        class ARG : public argProcessor
      {
      private:
        Logging::logger  m_vLogger;
        Logging::logger  m_dLogger;
        
      public:
        string         sTestFile;  ///< source file.
        vector<string> sRefFiles;  ///< list of reference files.
        bool           bVerbose;   ///< verbose mode
        bool           bDoPer;     ///< true = use PER, false = use WER
        int            iNumHyps;   ///< number of translation hypotheses per source sentence

    public:
      ARG(const int argc, const char* const argv[])
        : argProcessor(ARRAY_SIZE(switches), switches, 2, -1, help_message, "-h", true)
        , m_vLogger(Logging::getLogger("verbose.main.arg"))
        , m_dLogger(Logging::getLogger("debug.main.arg"))
        , bVerbose(false)
        , bDoPer(false)
        , iNumHyps(1)
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
            LOG_DEBUG(m_dLogger, "Calculate %s", (bDoPer? "mPER" : "mWER"));
            LOG_DEBUG(m_dLogger, "Test file name: %s", sTestFile.c_str());
            
            LOG_DEBUG(m_dLogger, "Number of reference files: %d", sRefFiles.size());
            LOG_DEBUG(m_dLogger, "Number of translation hypotheses per source sentence: %i", iNumHyps);
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
         bVerbose = checkVerbose("v");
         mp_arg_reader->testAndSet("per", bDoPer);
         mp_arg_reader->testAndSet("n", iNumHyps);

         mp_arg_reader->testAndSet(0, "testfile", sTestFile);
         mp_arg_reader->getVars(1, sRefFiles);
      }
      };  // ends class ARG

  } // ends namespace worderror
} // Portage

#endif // __WORDERROR_H__
