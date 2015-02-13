/**
 * @author Samuel Larkin
 * @file scoremain.h  Program that calculates the score of a given source and nbest set.
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
#include "translationReader.h"
#include "referencesReader.h"
#include "bootstrap.h"


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

      template<class ScoreMetric>
      void score(const ARG& arg, FileReader::FileReaderBase<Translation>& inputReader, referencesReader& rReader, istream* wts_file = NULL)
      {
         const Uint numRefs(arg.sRefFiles.size());

         ScoreMetric total;
         vector<ScoreMetric> indiv;

         string wts_line;

         Uint compteur(0);
         while (inputReader.pollable()) {
            LOG_VERBOSE3(verboseLogger, "Reading Sentence(%d)", compteur);
            Translation tstSentence;
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
               LOG_VERBOSE3(verboseLogger, "Calculating ScoreMetric(%d)", compteur);
               ScoreMetric cur(tstSentence, refSentences);
               if (wts_file != NULL) {
                  if (!getline(*wts_file, wts_line)) {
                     error(ETFatal, "Missing some weights");
                  }

                  double wts = conv<double>(wts_line);
                  cur = cur * wts;
               }
               if (arg.bDoConf)
                  indiv.push_back(cur);

               total += cur;
               ++compteur;

               if (arg.iDetail>0 && !tstSentence.empty()) {        
                  if (arg.iDetail > 1)
                     cur.output();
                  printf("Sentence %d %s score: %g\n", compteur, ScoreMetric::name(), ScoreMetric::convertToDisplay(cur.score()));
               }
            }
         }

         LOG_VERBOSE3(verboseLogger, "Checking integrity");
         rReader.integrityCheck();

         if (wts_file != NULL) {
            if (getline(*wts_file, wts_line)) {
               error(ETFatal, "Weight file too long.");
            }
         }

         total.output();

         double conf_interval(0);
         if (arg.bDoConf) {
            typename ScoreMetric::CIcomputer wc;
            conf_interval = bootstrapConfInterval(indiv.begin(), indiv.end(), wc, 0.95, 1000);
         }

         printf("%s score: %f", ScoreMetric::name(), ScoreMetric::convertToDisplay(total.score()));
         if (arg.bDoConf) cout << " +/- " << conf_interval;
         cout << endl;
         printf("Human readable value: %.2f", 100*ScoreMetric::convertToDisplay(total.score()));
         if (arg.bDoConf) printf(" +/- %.2f", 100*conf_interval);
         cout << endl;
      }
   } // ends namespace scoremain
} // Portage

#endif // __SCOREMAIN_H__
