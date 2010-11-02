/**
 * @author Eric Joanis, based on Uli Germann's pthread-example.cc and George
 *         Foster's bloater.cc.
 * @file bloater_pthread.cc 
 * @brief Bloat up memory usage in multiple threads until death ensues
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <pthread.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "arg_reader.h"
#include "exception_dump.h"
#include <limits>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
bloater_pthread [-v] [-n <NUMTHREADS>] [-maxiter <MAXITER>] blocksize\n\
\n\
Creates a memory block of blocksize bytes every second.\n\
\n\
Options:\n\
\n\
-v       Write progress reports to cerr.\n\
-n       Start <NUMTHREADS> bloating together [4].\n\
-maxiter Will perform <MAXITER> iterations in each thread, thus creating\n\
         <NUMTHREADS> * <MAXITER> blocks in memory [Uint::max].\n\
";

// globals

static bool verbose = false;
static size_t blocksize = 0;
static Uint numThreads = 4;
static Uint maxIter = numeric_limits<Uint>::max();
static void getArgs(int argc, const char* const argv[]);

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // semaphore

// enforce C-style linking (pthread is a C library, not a C++ library)
extern "C" void* a_thread(void* ptr); 


// thread main
// ptr points to a struct or built-in data type used for passing information
// between the parent thread and this one
void* a_thread(void* ptr) 
{
   int id  = *((int*)ptr);
   sleep(id);

   pthread_mutex_lock(&lock); // claim exclusive access to some resource
   cerr << "Hello from thread No. " << id << endl;
   pthread_mutex_unlock(&lock);

   size_t tot_size  = 0;
   char* bloat_vect = NULL;
   Uint  num_blocks = 0;

   Uint round(0);
   do {
      bloat_vect = new char[blocksize];
      for (size_t i = 0; i * 256 < blocksize; ++i)
         bloat_vect[i*256] = i;
      bloat_vect[0] = 32;

      tot_size += blocksize;
      if (verbose) {
         pthread_mutex_lock(&lock); // claim exclusive access to some resource
         cerr << "Thread " << id << " block " << ++num_blocks
              << " total size = " << tot_size << endl;
         showMemoryUsage();
         pthread_mutex_unlock(&lock);
      }
      sleep(numThreads);

   } while (++round < maxIter);

   if (bloat_vect != NULL) {
      cerr << "Thread " << id << " " << bloat_vect[0] << endl;
   }

   return NULL;
}

// main

int MAIN(argc, argv)
{
   getArgs(argc, argv);

   pthread_t threads[numThreads];
   int threadIds[numThreads];

   // Start up threads. They may start up right away or with a delay.
   // The following, for example, won't work as one might think:
   //
   // int id = 1;
   // pthread_create(&t1,NULL,a_thread,(void*)&id);
   // id++;
   // pthread_create(&t2,NULL,a_thread,(void*)&id);
   // id++;
   // ...
   //
   // because there's no guarantee that any of the threads will start running
   // before id is increased. So chances are that several or all will be
   // called with the same value for id.
   for ( Uint i = 0; i < numThreads; ++i ) {
      threadIds[i] = i+1;
      if (pthread_create(&(threads[i]), NULL, a_thread, (void*)&(threadIds[i])) != 0) {
         cerr << "Unable to create thread: " << threads[i] << endl;
      }
   }

   // any code inserted here would run in parallel with the other threads

   // wait for the threads to finish (otherwise, exiting will kill them,
   // finished or not);
   for ( Uint i = 0; i < numThreads; ++i )
      pthread_join(threads[i], NULL);

   return 0;
} END_MAIN

// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v", "n:", "maxiter:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("maxiter", maxIter);
   arg_reader.testAndSet("n", numThreads);
  
//   arg_reader.testAndSet(0, "blocksize", blocksize);
   if( sizeof(blocksize) <= sizeof(unsigned int)) {
      Uint bsize;
      arg_reader.testAndSet(0, "blocksize", bsize);
      blocksize = (size_t)bsize;
   } else {
      Uint64 bsize;
      arg_reader.testAndSet(0, "blocksize", bsize);
      blocksize = (size_t)bsize;
   }
}   


