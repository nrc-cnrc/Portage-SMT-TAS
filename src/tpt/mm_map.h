// This file is derivative work from Ulrich Germann's Tightly Packed Tries
// package (TPTs and related software).
//
// Original Copyright:
// Copyright 2005-2009 Ulrich Germann; all rights reserved.
// Under licence to NRC.
//
// Copyright for modifications:
// Technologies langagieres interactives / Interactive Language Technologies
// Inst. de technologie de l'information / Institute for Information Technology
// Conseil national de recherches Canada / National Research Council Canada
// Copyright 2008-2010, Sa Majeste la Reine du Chef du Canada /
// Copyright 2008-2010, Her Majesty in Right of Canada



// (c) 2007,2008 Ulrich Germann
#ifndef __ugTokenIndex_map_hh
#define __ugTokenIndex_map_hh
#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include "tpt_typedefs.h"
#include "tpt_constants.h"
#include <vector>
#include <tr1/unordered_map>

using namespace std;
namespace bio=boost::iostreams;

namespace ugdiss
{
  class MMMap
  {
  public:
    /// First version's token.
    static const char* const version1;

    typedef const char* Key;   //< Key type.
    typedef const char* Value;  //< Value type.

    /** Key->Value lookup works via binary search in a vector of Entry instances */
    struct Entry
    {
      uint32_t key_offset;
      uint32_t value_offset;
    };

    /**
     * Memory Mapped Map Iterator.
     * Note that iterating over the memory mapped map can only be done in
     * read-only mode.
     */
    class const_iterator {
      private:
        const Entry* entry;  //< What key/value is this iterator pointing at.
        const MMMap& map;  //< A reference to the map to be able to retrieve the key/value.
      public:
        /**
         * Default constructor.
         *
         * @param e  the entry that the iterator is pointing at.
         * @param map the map that the e points into.
         */
        const_iterator(const Entry* e, const MMMap& map)
        : entry(e)
        , map(map)
        {
          assert(entry != NULL);
        }

        /**
         * Get the entry's key value.
         * @return the key's value.
         */
        Key getKey() const {
          return map.getKey(*entry);
        }
        /**
         * Get the entry's value.
         * @return the entry's value.
         */
        Value getValue() const {
          return map.getValue(*entry);
        }
        /**
         * The prefix increment operator.
         */
        const_iterator& operator++() {       // Prefix increment operator.
          ++entry;
          return *this;
        }
        /**
         * The postfix increment operator.
         */
        const_iterator operator++(int) {     // Postfix increment operator.
          return const_iterator(++entry, map);
        }

        /**
         * The equality operator.
         * @param other the other iterator to compare to.
         * @return true if the iterators point to the same entry.
         */
        bool operator==(const const_iterator& other) const {
          return entry == other.entry and &map == &(other.map);
        }
        /**
         * The inequality operator.
         * @param other the other iterator to compare to.
         * @return true if the iterators do not point to the same entry.
         */
        bool operator!=(const const_iterator& other) const {
          return !(*this).operator==(other);
        }
    };

  private:
    /** Comparison function object used for Entry instances */
    class CompFunc
    {
    public:
      Key base;  //< base offset to the begin of the keys.
      /**
       * Default constructor.
       * @param base offset in memory where to find the beginning of the keys.
       */
      CompFunc(Key base);
      bool operator()(Entry const& A, char const* w);
    };

    uint32_t numTokens;  //< the number of entries in the map aka map size.

    bio::mapped_file_source file;  //< the memory mapped map file.

    Entry const* startIdx;  //< the start address for the indices.
    Entry const* endIdx;  //< the end address for the indices.

    Key keyStart;  //< the start address of the keys.
    Value valueStart;  //< the start address of the values.

  public:
    /// Default constructor.
    MMMap();
    /**
     * Constructor.
     * @param fname filename of the memory mapped map.
     */
    MMMap(const string& fname);

    /**
     * Open the memory mapped map.
     * @param fname filename of the memory mapped map.
     */
    void open(const string& fname);

    /**
     * Given a key, return its value or NULL if not found.
     * @param k key
     * @return the key's value if present or else NULL.
     */
    Value operator[](Key k) const {
       const_iterator it = find(k);
       return (it != end() ? it.getValue() : NULL);
    }
    /**
     * Given a key, return its value or NULL if not found.
     * @param k key
     * @return the key's value if present or else NULL.
     */
    Value operator[](string const& k) const {
       const_iterator it = find(k);
       return (it != end() ? it.getValue() : NULL);
    }

    /**
     * @return the number of elements in the map.
     */
    uint32_t size() const {
       return numTokens;
    }

    /**
     * @return true if there is no element in the map.
     */
    bool empty() const {
       return size() == 0;
    }

    /**
     * @return constant iterator at the beginning of the map.
     */
    const_iterator begin() const {
       return const_iterator(startIdx, *this);
    }
    /**
     * @return constant iterator at the end of the map.
     */
    const_iterator end() const {
       return const_iterator(endIdx, *this);
    }

    /**
     * Finds a key in the map.
     * @param w key to lookup.
     * @return an iterator for the key or end() if not found.
     */
    const_iterator find(const char* w) const;
    /**
     * Finds a key in the map.
     * @param w key to lookup.
     * @return an iterator for the key or end() if not found.
     */
    const_iterator find(const string& w) const;

  protected:
    /**
     * Given a valide index, return its entry(keyOffset/valueOffset).
     * @param index the index of the element you are interested in.
     * @return entry(keyOffset/valueOffset) for the index.
     */
    Entry getOffsets(const uint32_t index) const {
       assert(index < size());
       return startIdx[index];
    }
    /**
     * @param offset is the key's value offset.
     * @return key's value.
     */
    Key getKey(const uint32_t offset) const {
       return keyStart + offset;
    }
    /**
     * @param offset is the value's value offset.
     * @return value's value.
     */
    Value getValue(const uint32_t offset) const {
       return valueStart + offset;
    }
    /**
     * @param e is an entry in the indice's list.
     * @return the key's value.
     */
    Key getKey(const Entry& e) const {
       return keyStart + e.key_offset;
    }
    /**
     * @param e is an entry in the indice's list.
     * @return the value's value.
     */
    Value getValue(const Entry& e) const {
       return valueStart + e.value_offset;
    }
  };

  /**
   * Converts a key\tvalue map to its memory mapped representation.
   * @param is the inpupt stream containing key\tvalue pairs.
   * @param os the memory map representation stream of is.
   */
  void mkMemoryMappedMap(istream& is, ostream& os);

}

#endif  // ends __ugTokenIndex_map_hh
