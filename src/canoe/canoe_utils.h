#ifndef __CANOE_UTILS____H
#define __CANOE_UTILS____H

#include "file_utils.h"
#include <string>

using namespace std;
using namespace Portage;

template <class Tags>
void loadClasses(Tags& tags, const string& infile) {
   cerr << "Loading classes " << infile << endl;
   iSafeMagicStream is(infile);
   string line;
   const char* sep = "\t";
   while (getline(is, line)) {
      const Uint len = strlen(line.c_str());
      char work[len+1];
      strcpy(work, line.c_str());
      assert(work[len] == '\0');

      char* strtok_state;
      const char* word = strtok_r(work, sep, &strtok_state);
      if (word == NULL)
         error(ETFatal, "expected 'word\ttag' entries in <%s>", infile.c_str());
      const char* tag = strtok_r(NULL, sep, &strtok_state);
      if (tag == NULL)
         error(ETFatal, "expected 'word\ttag' entries in <%s>", infile.c_str());

      tags[word] = tag;
   }
}

#endif // __CANOE_UTILS____H
