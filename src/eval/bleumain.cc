/**
 * @author Aaron Tikuisis / George Foster
 * @file bleumain.cc 
 * @brief Program that calculates BLEU for a given set of source and nbest.
 *
 * Evaluation Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 *
 * A program for computing BLEU score, using my C++ implementation (as opposed to the
 * mteval perl script).  Naturally, this can be done using bestbleu with 1-best lists, but
 * this interface allows the test sentences to be in a single file.
 */

#include "bleumain.h"
#include "exception_dump.h"
#include "printCopyright.h"
#include "perSentenceStats.h"


using namespace std;
using namespace Portage;


int MAIN(argc, argv)
{
   printCopyright(2004, "bleumain");
   bleumain::ARG arg(argc, argv);


   BLEUstats::setMaxNgrams(arg.maxNgrams);
   BLEUstats::setMaxNgramsScore(arg.maxNgramsScore);
 
   FileReader::FixReader<Translation> inputReader(arg.sTestFile, 1);

   //LOG_VERBOSE2(verboseLogger, "Creating references Reader");
   referencesReader  rReader(arg.sRefFiles);

   using namespace scoremain;
   if (arg.perSentenceBLEU) {
      BLEUstats::setDefaultSmoothing(1);
      score<perSentenceStats<BLEUstats> >(arg, inputReader, rReader);
   }
   else {
      BLEUstats::setDefaultSmoothing(arg.iSmooth);
      score<BLEUstats>(arg, inputReader, rReader, arg.wts_file);
   }
} END_MAIN

