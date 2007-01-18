/**
 * @author Aaron Tikuisis / George Foster
 * @file bleumain.cc  Program that calculates BLEU for a given set of source and nbest.
 *
 * $Id$
 *
 * Evaluation Module
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
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


   LOG_VERBOSE2(verboseLogger, "Opening test file");
   IMagicStream tst(arg.sTestFile);


   LOG_VERBOSE2(verboseLogger, "Creating references Reader");
   referencesReader  rReader(arg.sRefFiles);
   const Uint numRefs(arg.sRefFiles.size());

   BLEUstats total(arg.iSmooth);
   vector<BLEUstats> indiv;

   Uint compteur(0);
   while (!tst.eof()) {
      LOG_VERBOSE3(verboseLogger, "Reading Sentence(%d)", compteur);
      Sentence tstSentence;
      getline(tst, tstSentence);

      LOG_VERBOSE3(verboseLogger, "Reading reference(%d)", compteur);
      References refSentences(numRefs);
      rReader.poll(refSentences);

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
   }

   LOG_VERBOSE3(verboseLogger, "Checking integrity");
   rReader.integrityCheck(true);

   total.output();
   cout << "BLEU score: " << exp(total.score());

    if (arg.bDoConf) {
       BLEUcomputer bc;
       cout << " +/- " << bootstrapConfInterval(indiv.begin(), indiv.end(), bc, 0.95, 1000);
    }
    cout << endl;
} END_MAIN
