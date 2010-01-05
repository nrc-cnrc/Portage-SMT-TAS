/**
 * @author Samuel Larkin
 * @file scoremain.h  Program that calculates the score of a given source and nbest set.
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

#ifndef __SCOREMAIN_H__
#define __SCOREMAIN_H__

#include "portage_defs.h"
#include "basic_data_structure.h"
#include "file_utils.h"
#include "fileReader.h"
#include "referencesReader.h"
#include "bootstrap.h"
#include "logging.h"


namespace Portage
{
   /// Program bleumain namespace.
   /// Prevents global namespace pollution in doxygen.
   namespace scoremain
   {
      /// Specific argument processing class for bleumain program.
      struct ARG
      {
         public:
            bool           bVerbose;
            string         sTestFile;
            vector<string> sRefFiles;
            bool           bDoConf;
            int            iDetail;

         public:
         /**
          * Default constructor.
          */
         ARG()
         : bVerbose(false)
         , bDoConf(false)
         , iDetail(0)
         { }
      };

      template<class ScoreStats>
      void score(const ARG& arg)
      {
         LOG_VERBOSE2(verboseLogger, "Creating references Reader");
         referencesReader  rReader(arg.sRefFiles);
         const Uint numRefs(arg.sRefFiles.size());

         ScoreStats total;
         vector<ScoreStats> indiv;

         FileReader::FixReader<Sentence> inputReader(arg.sTestFile, 1);
         Uint compteur(0);
         while (inputReader.pollable()) {
            LOG_VERBOSE3(verboseLogger, "Reading Sentence(%d)", compteur);
            Sentence tstSentence;
            inputReader.poll(tstSentence);

            LOG_VERBOSE3(verboseLogger, "Reading reference(%d)", compteur);
            References refSentences(numRefs);
            rReader.poll(refSentences);

            // check if at least one of the references is non-empty    
            bool empty_refs = true;
            for (References::const_iterator itr=refSentences.begin(); itr!=refSentences.end(); ++itr)
               if (!(itr->empty())) {
                  empty_refs = false;
                  break;
               }

            if (empty_refs) {
               ++compteur;
               error(ETWarn, "All references are empty! Ignoring sentence %i", compteur);
            }
            else {
               LOG_VERBOSE3(verboseLogger, "Calculating ScoreStats(%d)", compteur);
               ScoreStats cur(tstSentence, refSentences);
               if (arg.bDoConf)
                  indiv.push_back(cur);

               total += cur;
               ++compteur;

               if (arg.iDetail>0 && !tstSentence.empty()) {        
                  if (arg.iDetail > 1)
                     cur.output();
                  printf("Sentence %d %s score: %g\n", compteur, ScoreStats::name(), ScoreStats::convertToDisplay(cur.score()));
               }
            }
         }

         LOG_VERBOSE3(verboseLogger, "Checking integrity");
         rReader.integrityCheck();

         total.output();

         double conf_interval(0);
         if (arg.bDoConf) {
            typename ScoreStats::CIcomputer wc;
            conf_interval = bootstrapConfInterval(indiv.begin(), indiv.end(), wc, 0.95, 1000);
         }

         printf("%s score: %f", ScoreStats::name(), ScoreStats::convertToDisplay(total.score()));
         if (arg.bDoConf) cout << " +/- " << conf_interval;
         cout << endl;
         printf("Human readable value: %.2f", 100*ScoreStats::convertToDisplay(total.score()));
         if (arg.bDoConf) printf(" +/- %.2f", 100*conf_interval);
         cout << endl;
      }
   } // ends namespace scoremain
} // Portage

#endif // __SCOREMAIN_H__
