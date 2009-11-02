/**
 * @author Nicola Ueffing
 * @file worderrors.cc
 * @brief Tagging each word as correct/incorrect based on WER or PER.
 *
 * $Id$
 *
 * COMMENTS: Tag each word in a translation as correct/incorrect based on WER or PER
 * If several references are given, the one with minimal distance is used.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "levenshtein.h"
#include "worderrors.h"
#include "str_utils.h"
#include "file_utils.h"
#include <referencesReader.h>
#include <exception_dump.h>
#include <str_utils.h>
#include <file_utils.h>
#include <errors.h>
#include <portage_defs.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace Portage;
using namespace Portage::worderror;

int main(int argc, const char* const argv[]) {

   ARG arg(argc, argv);

   LOG_VERBOSE2(verboseLogger, "Opening test file");
   iSafeMagicStream tst(arg.sTestFile);

   LOG_VERBOSE2(verboseLogger, "Creating references Reader");
   referencesReader  rReader(arg.sRefFiles);
   const Uint numRefs(arg.sRefFiles.size());

   if (arg.bDoPer) {
     cerr << "PER-based tagging is not implemented yet!!!" << endl << endl;
     exit(1);
   }

   Levenshtein<string> lev;
   Uint   n      = 0;
   string tstSentence;

   while (getline(tst, tstSentence)) {

     vector<string> hypWords;
     split(tstSentence, hypWords);
     
     References refSentences(numRefs);
     rReader.poll(refSentences);
     
     Uint           minDist = 0;
     vector<int>    bestAlig;
     vector<string> bestRef;

     for (Uint i = 0; i < numRefs; i++) {
       Tokens refWords = refSentences[i].getTokens();
       
       vector<int> alig = lev.LevenAlig(hypWords, refWords);
       
       if (arg.bVerbose) {
         cerr << "Distance: " << lev.getLevenDist() << endl;
         lev.dumpAlign(hypWords, refWords, alig, cerr);
       }
       /**/
       
       Uint cur = lev.getLevenDist();
       if (i == 0 || cur < minDist) {
         minDist = cur;
         bestAlig = alig;
         bestRef = refWords;
       }
     } // for i
     n++;     

     for (Uint i=0; i<hypWords.size(); i++)
       if (bestAlig[i] == -1)
         cout << "0 ";
       else if (hypWords[i] == bestRef[bestAlig[i]])
         cout << "1 ";
       else
         cout << "0 ";
     cout << endl;     
   } // while
   
}

