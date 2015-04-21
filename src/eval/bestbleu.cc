/**
 * @author Aaron Tikuisis / Samuel Larkin / GF
 * @file bestbleu.cc
 * @brief Program that finds the highest achievable score with a given set of
 * source and nbest.
 *
 * Evaluation Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 *
 * This file contains a main program that computes (probably) the best BLEU
 * score obtainable by taking one target sentence from each of a set of N-best
 * lists.
 */

#include <bleu.h>
#include "fileReader.h"
#include <bestbleu.h>
#include <bestscore.h>
#include <exception_dump.h>
#include <printCopyright.h>

using namespace std;
using namespace Portage;
using namespace Portage::bestbleu;

////////////////////////////////////////////////////////////////////////////////////
// MAIN
//
int MAIN(argc, argv)
{
   printCopyright(2004, "bestbleu");
   ARG arg(argc, argv);

   BLEUstats::setMaxNgrams(arg.maxNgrams);
   BLEUstats::setMaxNgramsScore(arg.maxNgramsScore);
   BLEUstats::setDefaultSmoothing(arg.iSmooth);

   //LOG_VERBOSE2(verboseLogger, "Creating references Reader");
   referencesReader  rReader(arg.sRefFiles);

   //LOG_VERBOSE3(verboseLogger, "Creating file readers");
   NbestReader  nbestReader(FileReader::create<Translation>(arg.sNBestFile, arg.K));

   using namespace bestscore;
   bestScore<BLEUstats>(arg, rReader, nbestReader);
} END_MAIN
