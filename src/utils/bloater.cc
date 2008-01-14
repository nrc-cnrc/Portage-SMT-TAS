/**
 * @author George Foster
 * @file bloater.cc Program template.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "arg_reader.h"
#include "exception_dump.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
bloater [-maxiter <MAXITER>] blocksize\n\
\n\
Creates memory blocks of size blocksize every second.\n\
\n\
Options:\n\
\n\
-v       Write progress reports to cerr.\n\
-maxiter Will perform <MAXITER> iteration thus create <MAXITER> block in memory\n\
         [Uint::max].\n\
";

// globals

static bool verbose = false;
static Uint blocksize = 0;
static Uint maxIter = numeric_limits<Uint>::max();
static void getArgs(int argc, const char* const argv[]);

// main

int MAIN(argc, argv)
{
   getArgs(argc, argv);

   Uint  tot_size   = 0;
   char* bloat_vect = NULL;
   Uint  num_blocks = 0;

   Uint round(0);
   while (round++ < maxIter) {

      bloat_vect = new char[blocksize];
      bloat_vect[0] = 1;
      for (size_t i = 0; i * 256 < blocksize; ++i)
         bloat_vect[i*256] = i;

      tot_size += blocksize;
      if (verbose)
         cerr << ++num_blocks << " total size = " << tot_size << endl;
      sleep(1);
   }

   cout << bloat_vect[0] << endl;
} END_MAIN

// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v", "maxiter:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("maxiter", maxIter);
  
   arg_reader.testAndSet(0, "blocksize", blocksize);
}   
