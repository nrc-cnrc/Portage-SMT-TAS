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
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

//#include "wer.h"
#include "wermain.h"
#include "bootstrap.h"
#include "referencesReader.h"
#include "exception_dump.h"
#include "str_utils.h"
#include "file_utils.h"
#include "errors.h"
#include "portage_defs.h"
#include "printCopyright.h"
#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace Portage;
using namespace Portage::wermain;


template<class ScoreStats>
void score(const ARG& arg);

int MAIN(argc, argv)
{
   printCopyright(2004, "wermain");
   ARG arg(argc, argv);

   if (arg.bDoPer) {
      score<PERstats>(arg);
   }
   else {
      score<WERstats>(arg);
   }
} END_MAIN

template<class ScoreStats>
void score(const ARG& arg)
{
   LOG_VERBOSE2(verboseLogger, "Opening test file");
   iSafeMagicStream tst(arg.sTestFile);

   LOG_VERBOSE2(verboseLogger, "Creating references Reader");
   referencesReader  rReader(arg.sRefFiles);
   const Uint numRefs(arg.sRefFiles.size());

   vector<ScoreStats> indiv;    // distance and reference length per sentence
   ScoreStats total;
   int n(0);
   while (!tst.eof())
   {
      string tstSentence;
      getline(tst, tstSentence);
      vector<string> tstWords;
      split(tstSentence, tstWords);

      References refSentences(numRefs);
      rReader.poll(refSentences);

      ScoreStats stats(tstWords, refSentences);
      ++n;
      if (arg.bDetail && stats._reflen>0)
         cout << "Sentence " << n << " m" << ScoreStats::name() << " score: "
            << 100.0*stats.ratio() << " % (total dist: " << stats._changes
            << ", hyp. length: " << tstWords.size() << ", avg. ref. length: "
            << stats._reflen << ")" << endl;
      if (arg.bDoConf && stats._reflen>0.0f)
         indiv.push_back(stats);

      total += stats;
   } // while

   cout << "m" << ScoreStats::name() << " score: " << total._changes << " (" << 100.0*total.ratio() << " %)";
   if (arg.bDoConf) {
      typename ScoreStats::CIcomputer wc;
      cout << " +/- " << bootstrapConfInterval(indiv.begin(), indiv.end(), wc, 0.95, 1000);
   }
   cout << endl;
}
