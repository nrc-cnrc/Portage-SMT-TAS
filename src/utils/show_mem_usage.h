// $Id$
/**
 * @author Samuel Larkin
 * @file show_mem_usage.h
 * @brief Prints the current memory usage.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef __SHOW_MEM_USAGE_H__
#define __SHOW_MEM_USAGE_H__

#include <iostream>
#include <fstream>

// Process id
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>


namespace Portage {

using namespace std;

inline void showMemoryUsage() {
   try {
      const pid_t pid = getpid();
      char file[32];

      cerr << "PID: " << pid << endl;
      snprintf(file, 31, "/proc/%d/status", pid);
      //cerr << file << endl;
      ifstream status(file);
      cerr << status.rdbuf() << endl;
      status.close();

      snprintf(file, 31, "/proc/%d/stat", pid);
      //cerr << file << endl;
      ifstream stat(file);
      cerr << stat.rdbuf() << endl;
      stat.close();

      
      char cmd[32];
      snprintf(cmd, 31, "top -bn 1 -p %d 1>&2", pid);
      cerr << cmd << endl;
      if ( system(cmd) )
         cerr << "System call failed - probably not enough memory to run it." << endl;
      cerr << "ps fuxww 1>&2" << endl;
      if ( system("ps fuxww 1>&2") )
         cerr << "System call failed - probably not enough memory to run it." << endl;
   } catch (std::bad_alloc& e) {
      cerr << "Caught std::bad_alloc (again!) - not enough memory to provide full troubleshooting information..." << endl;
   }

/*   struct rusage resources;
   //int who = RUSAGE_SELF;
   int who = RUSAGE_CHILDREN;
   if(getrusage(who, &resources) == 0) {
      cerr << "Maximum resident set size: " << resources.ru_maxrss << endl;
      cerr << "Resident set size: " << resources.ru_idrss << endl;
      cerr << resources.ru_inblock << endl;
      cerr << resources.ru_isrss << endl;
      cerr << resources.ru_ixrss << endl;
      cerr << resources.ru_utime.tv_sec << endl;
   }
   else
      perror("getrusage failed");
*/
}

}; // ends namespace Portage

#endif // __SHOW_MEM_USAGE_H__
