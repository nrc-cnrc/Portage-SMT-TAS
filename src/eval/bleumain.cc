/**
 * @author Aaron Tikuisis / George Foster
 * @file bleumain.cc  Program that calculates BLEU for a given set of source and nbest.
 *
 * $Id$
 *
 * Evaluation Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 *
 * A program for computing BLEU score, using my C++ implementation (as opposed to the
 * mteval perl script).  Naturally, this can be done using bestbleu with 1-best lists, but
 * this interface allows the test sentences to be in a single file.
 */

#include <bleumain.h>
#include <exception_dump.h>
#include <file_utils.h>
#include <fileReader.h>
#include <referencesReader.h>
#include <bootstrap.h>
#include <printCopyright.h>

using namespace std;
using namespace Portage;


int MAIN(argc, argv)
{
   printCopyright(2004, "bleumain");
   using namespace bleumain;
   ARG arg(argc, argv);


   BLEUstats::setMaxNgrams(arg.maxNgrams);
   BLEUstats::setMaxNgramsScore(arg.maxNgramsScore);
   

   LOG_VERBOSE2(verboseLogger, "Creating references Reader");
   referencesReader  rReader(arg.sRefFiles);
   const Uint numRefs(arg.sRefFiles.size());

   BLEUstats total(arg.iSmooth);
   vector<BLEUstats> indiv;

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
        if (itr->size()>0) {
          empty_refs = false;
          break;
        }
      if (empty_refs) {
        ++compteur;
        error(ETWarn, "All references are empty! Ignoring sentence %i", compteur);
      }
      else {
        LOG_VERBOSE3(verboseLogger, "Calculating BLEUstats(%d)", compteur);
        BLEUstats cur(tstSentence, refSentences, arg.iSmooth);
        if (arg.bDoConf)
          indiv.push_back(cur);
        
        total = total + cur;
        ++compteur;
        
        if (arg.iDetail>0 && tstSentence.size()>0) {        
          if (arg.iDetail>1)
            cur.output();
          cout << "Sentence " << compteur << " BLEU score: " << exp(cur.score()) << endl;
        }
      } // else
   }

   LOG_VERBOSE3(verboseLogger, "Checking integrity");
   rReader.integrityCheck();

   total.output();
   cout << "BLEU score: " << exp(total.score());

    if (arg.bDoConf) {
       BLEUcomputer bc;
       cout << " +/- " << bootstrapConfInterval(indiv.begin(), indiv.end(), bc, 0.95, 1000);
    }
    cout << endl;
} END_MAIN
