/**
 * @author Samuel Larkin
 * @file mm_map.h
 * @brief A Memory mapped map.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */


#include "mm_map.h"
#include "file_utils.h"

#include <cstring>
#include <algorithm>
#include <iostream>
#include <tr1/unordered_map>

using namespace std;

const char* const Portage::MMMap::version1 = "Portage TPMap-1.0\tconst char* -> const char*";

namespace Portage
{
   MMMap::
   MMMap()
   : numTokens(0)
     , keyStart(NULL)
     , valueStart(NULL)
   {};

   MMMap::
   MMMap(const string& fname)
   {
      this->open(fname);
   };

   void
   MMMap::
   open(const string& fname)
   {
      {
         Portage::iSafeMagicStream is(fname);
         string version;
         if (!getline(is, version)) {
            error(ETFatal, "Bad token index '%s'", fname.c_str());
         }
         else {
            if (version != version1) {
               error(ETFatal, "Unsupported version '%s'", fname.c_str());
            }
         }
      }

      try {
          file.open(fname);
          if (!file.is_open())
             error(ETFatal, "Unable to open memory mapped file.");
      }
      catch(std::exception& e) {
         error(ETFatal, "Unable to open memory mapped file '%s'' for reading (errno=%d, %s).", fname.c_str(), errno, strerror(errno));
      }

      if (file.size() < 4 + 2*sizeof(uint32_t))
         error(ETFatal, "Bad token index '%s'", fname.c_str());

      uint32_t offset = strlen(version1) + 1;

      this->numTokens = *(reinterpret_cast<uint32_t const*>(file.data() + offset));
      offset += sizeof(uint32_t);

      const uint32_t keySize = *(reinterpret_cast<const uint32_t*>(file.data() + offset));
      offset += sizeof(uint32_t);

      const uint32_t valueSize = *(reinterpret_cast<const uint32_t*>(file.data() + offset));
      offset += sizeof(uint32_t);

      startIdx  = reinterpret_cast<Entry const*>(file.data() + offset);
      endIdx    = startIdx + numTokens;
      keyStart    = reinterpret_cast<const char*>(endIdx);
      valueStart  = keyStart + keySize;

      // spot check the first entry.
      if (getKey(0) > file.data() + file.size()
            || getValue(0) > file.data() + file.size()
            || getKey(getOffsets(0)) > file.data() + file.size()
            || getValue(getOffsets(0)) > file.data() + file.size()
            || valueStart + valueSize != file.data() + file.size()
         )
         error(ETFatal, "Bad token index '%s'", fname.c_str());
   }

   MMMap::
   CompFunc::
   CompFunc(Key base)
   : base(base)
   {};

   bool
   MMMap::
   CompFunc::
   operator()(Entry const& A, char const* w)
   {
      return strcmp(base + A.key_offset, w) < 0;
   };


   MMMap::const_iterator
   MMMap::
   find(const char* w) const
   {
      // NOTE: lower_bound doesn't return endIdx if w is not found thus we need to make sure that bla
      // points to w.
      Entry const * const bla = lower_bound(startIdx, endIdx, w, CompFunc(keyStart));
      if (bla == endIdx) return end();
      return strcmp(getKey(*bla), w) ? end() : const_iterator(bla, *this);
   }

   MMMap::const_iterator
   MMMap::
   find(const string& w) const
   {
      return find(w.c_str());
   }


   /*
    * +----------
    * | Version String
    * +----------
    * | N the number of keys in the map
    * +----------
    * | key's size in bytes
    * +----------
    * | value's size in bytes
    * +---------- indices' stream:
    * | key's Offset 1
    * | value's Offset 1
    * | key's Offset 2
    * | value's Offset 2
    * | key's Offset 3
    * | value's Offset 3
    * | ...
    * | key's Offset N
    * | value's Offset N
    * +---------- keys' stream:
    * | word1\0
    * | word2\0
    * | word3\0
    * | ...
    * | wordN\0
    * +---------- values' stream:
    * | value1\0
    * | value2\0
    * | value3\0
    * | ...
    * | valueM\0   <= note that the values are deduplicated thus M <= N.
    * +----------
    */

   void mkMemoryMappedMap(istream& is, ostream& os) {
      typedef pair<string, string> Entry;
      // Tags maps a unique id to every tags.
      typedef std::tr1::unordered_map<string, uint32_t> Tags;

      vector<Entry> token2tag;
      Tags tags;
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
            error(ETFatal, "expected 'word\ttag' entries");
         const char* tag = strtok_r(NULL, sep, &strtok_state);
         if (tag == NULL)
            error(ETFatal, "expected 'word\ttag' entries");

         token2tag.push_back(Entry(word, tag));
         // Let's keep track of what classes we've seen so far.
         // Later on, we will use the value to assign a unique id to each tag.
         tags[tag] = 0;
      }
      sort(token2tag.begin(), token2tag.end());

      // Write the tags we've seen and calculate their offset in the stream.
      ostringstream tagStream;
      uint32_t tagOffset = 0;
      typedef Tags::iterator IT;
      for (IT it(tags.begin()); it!=tags.end(); ++it) {
         tagStream << it->first << char(0);
         it->second = tagOffset;
         tagOffset += it->first.size() + 1;
      }

      ostringstream keys;
      ostringstream indices;
      uint32_t key_offset = 0;

      typedef vector<Entry>::const_iterator Iter;
      for (Iter it(token2tag.begin()); it!=token2tag.end(); ++it) {
         const string& word = it->first;
         const string& tag  = it->second;
         const uint32_t value_offset = tags[tag];
         indices.write(reinterpret_cast<const char*>(&key_offset), sizeof(uint32_t));
         indices.write(reinterpret_cast<const char*>(&value_offset), sizeof(uint32_t));
         keys << word << char(0);
         key_offset += word.size() + 1;
      }

      // We need a version number.
      os << MMMap::version1 << '\n';;

      const Uint numberEntries = token2tag.size();
      os.write(reinterpret_cast<const char*>(&numberEntries), sizeof(Uint));

      const uint32_t keySize = keys.str().size();
      os.write(reinterpret_cast<const char*>(&keySize), sizeof(uint32_t));

      const uint32_t valueSize = tagStream.str().size();
      os.write(reinterpret_cast<const char*>(&valueSize), sizeof(uint32_t));

      os << indices.str();
      os << keys.str();
      os << tagStream.str();
   }

}
