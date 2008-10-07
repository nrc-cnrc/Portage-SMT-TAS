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
   system(cmd);
   system("ps fuxww 1>&2");

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
