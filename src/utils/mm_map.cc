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
const char* const Portage::MMMap::version2 = "Portage TPMap-2.0\tconst char* -> const char*";

namespace Portage
{
   MMMap::
   MMMap()
   : numberKeys(0)
   , numberValues(0)
   , startIdx(NULL)
   , endIdx(NULL)
   , startValues(NULL)
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
            error(ETFatal, "Bad token index '%s' (%s)", fname.c_str(), version.c_str());
         }
         else {
            if (version != version2) {
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

      // TODO: doesn't really validate anything.
      if (file.size() < 4 + 2*sizeof(uint32_t))
         error(ETFatal, "Bad token index (size) '%s'", fname.c_str());

      uint32_t offset = strlen(version2) + 1;

      numberKeys = *(reinterpret_cast<uint32_t const*>(file.data() + offset));
      offset += sizeof(uint32_t);

      numberValues = *(reinterpret_cast<const uint32_t*>(file.data() + offset));
      offset += sizeof(uint32_t);

      const uint32_t keySize = *(reinterpret_cast<const uint32_t*>(file.data() + offset));
      offset += sizeof(uint32_t);

      const uint32_t valueSize = *(reinterpret_cast<const uint32_t*>(file.data() + offset));
      offset += sizeof(uint32_t);

      startIdx  = reinterpret_cast<Entry const*>(file.data() + offset);
      endIdx    = startIdx + numberKeys;
      startValues = reinterpret_cast<const uint32_t*>(startIdx + numberKeys);
      keyStart    = reinterpret_cast<const char*>(startValues + numberValues);
      valueStart  = keyStart + keySize;

      // spot check the first entry.
      if (getKey(0) > file.data() + file.size())
         error(ETFatal, "Bad token index getKey(0) '%s'", fname.c_str());
      if (getValue(0) > file.data() + file.size())
         error(ETFatal, "Bad token index getValue(0) '%s'", fname.c_str());

      if (getKey(getOffsets(0)) > file.data() + file.size())
         error(ETFatal, "Bad token index '%s'", fname.c_str());
      if (getValue(getOffsets(0)) > file.data() + file.size())
         error(ETFatal, "Bad token index '%s'", fname.c_str());
      if (valueStart + valueSize != file.data() + file.size())
         error(ETFatal, "Bad token index valueStart + valueSize '%s'", fname.c_str());
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
    * | M the number of unique values in the map
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
    * +----------
    * | value1's offset
    * | value2's offset
    * | ...
    * | valueM's offset
    * +---------- keys' stream:
    * | key1\0
    * | key2\0
    * | key3\0
    * | ...
    * | keyN\0
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
      // Values maps a unique id to every values.
      typedef std::tr1::unordered_map<string, uint32_t> Values;

      vector<Entry> entries;
      Values values;
      string line;
      const char* sep = " \t";
      while (getline(is, line)) {
         const uint32_t len = strlen(line.c_str());
         char work[len+1];
         strcpy(work, line.c_str());
         assert(work[len] == '\0');

         char* strtok_state;
         const char* key = strtok_r(work, sep, &strtok_state);
         if (key == NULL)
            error(ETFatal, "expected 'key\ttag' entries");
         const char* value = strtok_r(NULL, sep, &strtok_state);
         if (value == NULL)
            error(ETFatal, "expected 'key\ttag' entries");

         entries.push_back(Entry(key, value));
         // Let's keep track of what classes we've seen so far.
         // Later on, we will use the value to assign a unique id to each value.
         values[value] = 0;
      }
      sort(entries.begin(), entries.end());

      // Write the values we've seen and calculate their offset in the stream.
      ostringstream valueStream;
      uint32_t valueOffset = 0;
      typedef Values::iterator IT;
      for (IT it(values.begin()); it!=values.end(); ++it) {
         valueStream << it->first << char(0);
         it->second = valueOffset;
         valueOffset += it->first.size() + 1;
      }

      ostringstream keys;
      ostringstream indices;
      uint32_t key_offset = 0;

      typedef vector<Entry>::const_iterator Iter;
      for (Iter it(entries.begin()); it!=entries.end(); ++it) {
         const string& key = it->first;
         const string& value  = it->second;
         const uint32_t value_offset = values[value];
         indices.write(reinterpret_cast<const char*>(&key_offset), sizeof(uint32_t));
         indices.write(reinterpret_cast<const char*>(&value_offset), sizeof(uint32_t));
         //cerr << '(' << key << ',' << value << ") => [" << key_offset << ',' << value_offset << ']' << endl;
         keys << key << char(0);
         key_offset += key.size() + 1;
      }

      // We need a version number.
      os << MMMap::version2 << '\n';;

      const uint32_t numberEntries = entries.size();
      os.write(reinterpret_cast<const char*>(&numberEntries), sizeof(uint32_t));

      const uint32_t numberValues = values.size();
      os.write(reinterpret_cast<const char*>(&numberValues), sizeof(uint32_t));

      const uint32_t keySize = keys.str().size();
      os.write(reinterpret_cast<const char*>(&keySize), sizeof(uint32_t));

      const uint32_t valueSize = valueStream.str().size();
      os.write(reinterpret_cast<const char*>(&valueSize), sizeof(uint32_t));

      os << indices.str();

      // Added the values in alphabetical order.
      typedef vector<pair<string, uint32_t> > ToSort;
      ToSort toSort(values.begin(), values.end());
      sort(toSort.begin(), toSort.end());
      for (ToSort::const_iterator it(toSort.begin()); it!=toSort.end(); ++it) {
         const uint32_t offset = it->second;
         os.write(reinterpret_cast<const char*>(&offset), sizeof(uint32_t));
      }

      os << keys.str();
      os << valueStream.str();

   }

}
