/**
 * @author Aaron Tikuisis / Samuel Larkin
 * @file wermain.cc  Program for computing mWER or mPER score.
 *
 * $Id$
 * 
 * Evaluation Module
 * 
 * A program for computing mWER or mPER score.  Naturally, this can be done using bestwer
 * with 1-best lists, but this interface allows the test sentences to be in a single file.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "wer.h"
#include "wermain.h"
#include "bootstrap.h"
#include <referencesReader.h>
#include <exception_dump.h>
#include <str_utils.h>
#include <file_utils.h>
#include <errors.h>
#include <portage_defs.h>
#include <printCopyright.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace Portage;
using namespace Portage::wermain;


int MAIN(argc, argv)
{
   printCopyright(2004, "wermain");
   ARG arg(argc, argv);

   LOG_VERBOSE2(verboseLogger, "Opening test file");
   IMagicStream tst(arg.sTestFile);

   LOG_VERBOSE2(verboseLogger, "Creating references Reader");
   referencesReader  rReader(arg.sRefFiles);
   const Uint numRefs(arg.sRefFiles.size());

   vector< pair<int,double> > indiv;    // distance and reference length per sentence
   Uint   total  = 0;
   Uint   n      = 0;
   double reflen = 0; // total avg. ref. length
   while (!tst.eof())
     {
       string tstSentence;
       getline(tst, tstSentence);
       vector<string> tstWords;
       split(tstSentence, insert_iterator<vector<string> >(tstWords, tstWords.begin()));
       
       References refSentences(numRefs);
       rReader.poll(refSentences);
       
       Uint   least = 0;
       double rflen = 0;  // avg. ref. length 
       for (uint i = 0; i < numRefs; i++)
         {
           Tokens refWords = refSentences[i].getTokens();
           rflen += refWords.size();
           
           Uint cur = (arg.bDoPer ? find_mPER(tstWords, refWords) : find_mWER(tstWords,refWords));
           
           if (i == 0 || cur < least)
             least = cur;
         } // for
       n++;
       rflen /= numRefs;
       if (arg.bDetail && rflen>0)
         cout << "Sentence " << n << (arg.bDoPer ? " mPER" : " mWER") << " score: " 
              << 100.0*least/rflen << " % (total dist: " << least 
              << ", hyp. length: " << tstWords.size() << ", avg. ref. length: " 
              << rflen << ")" << endl;
       if (arg.bDoConf) 
         indiv.push_back( pair<int,double>(least,rflen) );
       
       total   = total + least;
       reflen += rflen;
     } // while
   
   cout << (arg.bDoPer ? "mPER" : "mWER") << " score: " << total << " (" << 100.0*total/reflen << " %)";
   if (arg.bDoConf) {
     ERcomputer wc;
     cout << " +/- " << bootstrapConfInterval(indiv.begin(), indiv.end(), wc, 0.95, 1000);
   }
   cout << endl;
} END_MAIN
