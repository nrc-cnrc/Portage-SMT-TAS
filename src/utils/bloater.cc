/**
 * @author George Foster
 * @file bloater.cc 
 * @brief Bloat up memory usage until death ensues.
 *
 *
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "arg_reader.h"
#include "exception_dump.h"
#include "show_mem_usage.h"
#include "process_bind.h"
#include <cstdlib>
#include <limits>
#include <boost/optional/optional.hpp>

using namespace Portage;
using namespace std;
using boost::optional;

static char help_message[] = "\n\
bloater [-v] [-maxiter <MAXITER>] blocksize\n\
\n\
Creates a memory block of blocksize bytes every second.\n\
\n\
Options:\n\
\n\
-v       Write progress reports to cerr.\n\
-maxiter Will perform <MAXITER> iterations thus creating <MAXITER> blocks in\n\
         memory [Uint::max].\n\
-bind PID  Binds your program to the presence of the running PID;\n\
";

// globals

static optional<pid_t> pid;
static bool verbose = false;
static Uint blocksize = 0;
static Uint maxIter = numeric_limits<Uint>::max();
static void getArgs(int argc, const char* const argv[]);

// main

int MAIN(argc, argv)
{
   getArgs(argc, argv);

   if (pid) process_bind(*pid);

   Uint  tot_size   = 0;
   char* bloat_vect = NULL;
   Uint  num_blocks = 0;

   Uint round(0);
   do {
      bloat_vect = new char[blocksize];
      //if ( round % 10 == 0 )
      for (size_t i = 0; i * 256 < blocksize; ++i)
         bloat_vect[i*256] = i;
      bloat_vect[0] = 32;

      tot_size += blocksize;
      if (verbose) {
         cerr << ++num_blocks << " total size = " << tot_size << endl;
         showMemoryUsage();
      }
      sleep(1);
   } while (++round < maxIter);

   cout << bloat_vect[0] << endl;
} END_MAIN

// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v", "maxiter:", "bind:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("maxiter", maxIter);
   arg_reader.testAndSet("bind:", pid);
  
   arg_reader.testAndSet(0, "blocksize", blocksize);
}   
