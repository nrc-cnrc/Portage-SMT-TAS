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

#include "wermain.h"
#include "exception_dump.h"
#include "printCopyright.h"

using namespace std;
using namespace Portage;


int MAIN(argc, argv)
{
   printCopyright(2004, "wermain");
   wermain::ARG arg(argc, argv);

   using namespace scoremain;
   if (arg.bDoPer) {
      score<PERstats>(arg);
   }
   else {
      score<WERstats>(arg);
   }
} END_MAIN

