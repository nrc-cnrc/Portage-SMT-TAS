// $Id$
/**
 * @author Samuel Larkin
 * @file process_bind.cc
 * @brief Bind your program to pgm1 and stop your program if pgm1 disappear.
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include "process_bind.h"
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>   // And sleep
#include <stdio.h>  // stderr, fprintf, printf & perror
#include <cstdlib> // for exit
#include <errno.h>  // errno
//#include <string.h>  // strerror used for debugging.

using namespace Portage;

namespace Portage {

static const unsigned int wait_time = 10;

// enforce C-style linking (pthread is a C library, not a C++ library)
extern "C" void* a_thread(void* ptr); 

/**
 * Thread to execute that will monitor a certain pid.
 * @param  ptr  a pointer on the pid of pid_t type.
 * @return Always NULL
 */
static void* process_bind_thread(void* ptr) {
   const pid_t pid = *((pid_t*)ptr);

   printf("Waiting for %d\n", pid);
   while(kill(pid, 0) == 0) sleep(wait_time);

   //fprintf(stderr, "errno: %d => %s\n", errno, strerror(errno));
   if (errno == EINVAL) {
      fprintf(stderr, "Invalid signal sent to %d\n", pid);
   }
   else if (errno == EPERM) {
      fprintf(stderr, "You do not have enough permission to send a signal to pid: %d.\n", pid);
   }
   else if (errno == ESRCH) {
   fprintf(stderr, "Process %d is no longer running.\n", pid);
   }
   else {
      fprintf(stderr, "process_bind unknown error: %d\n", errno);
   }

   exit(45);

   return NULL;
}

void process_bind(pid_t& pid) {
   using namespace std;

   pthread_t thread;
   printf("process_bind: %d\n", pid);
   if (pthread_create(&thread, NULL, process_bind_thread, (void*)&(pid)) != 0) {
      fprintf(stderr, "Unable to create watch me thread");
      exit(45);
   }
}

} // ends namespace Portage
