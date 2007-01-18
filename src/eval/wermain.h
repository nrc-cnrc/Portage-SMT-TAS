/**
 * @author Samuel Larkin
 * @file wermain.h  Program that calculates the wer score of a given source and nbest set.
 *
 * $Id$
 *
 * Evaluation Module
 *
 * This file contains some stuff needed for WER computation.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef __WERMAIN_H__
#define __WERMAIN_H__

#include <portage_defs.h>
#include <argProcessor.h>
#include <vector>

namespace Portage
{
  /// Program wermain namespace.
  /// Prevents polluting the global namespace with some wermain specifics.
  namespace wermain
  {
    /// Program wermain usage.
    static char help_message[] = "\n\
wermain [-c][-detail d] testfile ref1 ref2 ... refn\n \
\n\
Computes the mWER / mPER score for the set of translations in testfile, using the\n\
reference files ref1, ... , refn. Each file should have one sentence per line,\n\
and the sentences in testfile should match line for line with the sentences in\n\
each reference file.\n\
\n\
Options:\n\
\n\
-c         Compute a 95% confidence interval around the score using bootstrap\n\
           resampling.\n\
-per       Calculate mPER instead of mWER.\n\
-detail    Print the mWER/mPER for each single sentence.\n\n\
";

    /// Program wermain command line switches.
    const char* const switches[] = {"c", "detail", "per"};
    /// Specific argument processing class for wermain program.
    class ARG : public argProcessor
    {
    private:
      Logging::logger  m_vLogger;
      Logging::logger  m_dLogger;
      
    public:
      string         sTestFile;  ///< source file.
      vector<string> sRefFiles;  ///< list of reference files.
      bool           bDoPer;     ///< true = use PER, false = use WER
      bool           bDoConf;    ///< calculate bootstrap confidence interval
      bool           bDetail;    ///< display per source details.
      
    public:
      ARG(const int argc, const char* const argv[])
        : argProcessor(ARRAY_SIZE(switches), switches, 2, -1, help_message, "-h", true)
        , m_vLogger(Logging::getLogger("verbose.main.arg"))
        , m_dLogger(Logging::getLogger("debug.main.arg"))
        , bDoPer(false)
        , bDoConf(false)
        , bDetail(false)
      {
        argProcessor::processArgs(argc, argv);
      }
      
      /// See argProcessor::printSwitchesValue()
      virtual void printSwitchesValue()
      {
        LOG_INFO(m_vLogger, "Printing arguments");
        if (m_dLogger->isDebugEnabled())
          {
            LOG_DEBUG(m_dLogger, "Do conf: %s", (bDoConf? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "Calculate %s", (bDoPer? "mPER" : "mWER"));
            LOG_DEBUG(m_dLogger, "Detailed output: %s", (bDetail? "ON" : "OFF"));
            LOG_DEBUG(m_dLogger, "Test file name: %s", sTestFile.c_str());
            
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
        mp_arg_reader->testAndSet("c", bDoConf);
        mp_arg_reader->testAndSet("per", bDoPer);
        mp_arg_reader->testAndSet("detail", bDetail);
        
        mp_arg_reader->testAndSet(0, "testfile", sTestFile);
        mp_arg_reader->getVars(1, sRefFiles);
      }
    };  // ends class ARG
    
    /// Callable entity for booststrap confidence interval
    struct ERcomputer
    {
      /**
       *
       * @param begin
       * @param end
       * @return Returns total least modifications / total reference length.
       */
      double operator()(vector< pair<int,double> >::const_iterator begin,
                        vector< pair<int,double> >::const_iterator end)
      {
        double dist=0, reflen=0;
        for (vector< pair<int,double> >::const_iterator itr=begin; itr!=end; ++itr) 
        {
          dist   += itr->first;
          reflen += itr->second;
        }
        return dist/reflen;
      }
    }; // ends struct ERcomputer
  } // ends namespace wermain
} // Portage

#endif // __WERMAIN_H__
