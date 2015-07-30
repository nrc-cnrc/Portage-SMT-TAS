/**
 * @author Samuel Larkin
 * @file wordClass.h
 * @brief An adapter class for word classes.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */


#ifndef  __WORD_CLASS_H__
#define __WORD_CLASS_H__

#include "binio.h"
#include "binio_maps.h"
#include "str_utils.h"
#include "file_utils.h"
#include "mm_map.h"
#include <tr1/unordered_map>

namespace Portage {

class IWordClass {
public:
   typedef pair<bool, const char*> findType;

   virtual void open(const string& fname) = 0;

   virtual bool empty() const = 0;
   virtual findType find(const char* w) const = 0;
   virtual findType find(const string& w) const = 0;
};


using namespace ugdiss;
class WordClassTightlyPacked : public IWordClass {
public:
   typedef ugdiss::MMMap Tags;

private:
   Tags tags;

public:
   WordClassTightlyPacked()
   {}
   WordClassTightlyPacked(const string& fname) {
      open(fname);
   }

   virtual void open(const string& fname) {
      //cerr << "Loading tightly packed classes " << fname << endl;
      tags.open(fname);
   }

   virtual bool empty() const {
      return tags.empty();
   }

   virtual findType find(const char* w) const {
      Tags::const_iterator it = tags.find(w);
      if (it == tags.end()) return findType(false, NULL);
      return findType(true, it.getValue());
   }
   virtual findType find(const string& w) const {
      return find(w.c_str());
   }
};



class WordClass : public IWordClass {
private:
   typedef std::tr1::unordered_map<string, string> Tags;
   Tags tags;

public:
   WordClass()
   {}
   WordClass(const string& fname) {
      open(fname);
   }

   virtual void open(const string& fname) {
      if (isSuffix(".bin", fname)) {
         //cerr << "Loading binary classes " << fname << endl;
         iSafeMagicStream is(fname);
         Portage::BinIO::readbin(is, tags);
      }
      else {
         cerr << "Loading text classes " << fname << endl;
         iSafeMagicStream is(fname);
         string line;
         const char* sep = " \t";
         while (getline(is, line)) {
            const Uint len = strlen(line.c_str());
            char work[len+1];
            strcpy(work, line.c_str());
            assert(work[len] == '\0');

            char* strtok_state;
            const char* word = strtok_r(work, sep, &strtok_state);
            if (word == NULL)
               error(ETFatal, "expected 'word\ttag' entries in <%s>", fname.c_str());
            const char* tag = strtok_r(NULL, sep, &strtok_state);
            if (tag == NULL)
               error(ETFatal, "expected 'word\ttag' entries in <%s>", fname.c_str());

            tags[word] = tag;
         }
      }
   }

   virtual bool empty() const {
      return tags.empty();
   }

   virtual findType find(const char* w) const {
      Tags::const_iterator it = tags.find(w);
      if (it == tags.end()) return findType(false, NULL);
      return findType(true, it->second.c_str());
   }
   virtual findType find(const string& w) const {
      return find(w.c_str());
   }

};

IWordClass* loadClasses(const string& fname);

}

#endif  // __WORD_CLASS_H__
