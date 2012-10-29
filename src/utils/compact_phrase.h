/**
 * @author Eric Joanis
 * @file compact_phrase.h Object that encapsulates a phrase compacted using
 *                        what is partly a misunderstanding of Uli Germann's
 *                        idea, a simplified Huffman-like encoding, but has
 *                        some uses.
 *
 * COMMENTS:
 *
 * A compact phrase is a Uint vector encoded into a character string using as
 * few bytes as possible.  A given Uint is represent by 1 to 5 non-null bytes,
 * least significant first, with all but the last having their first bit set,
 * and the seven remaining bits of each byte containing the Uint itself,
 * incremented by 1.
 *
 * In this representation, 0x00 is never used: only the most significant byte
 * has its first bit reset, and it cannot be 0 since we encode (x+1).
 * Because we do +1, MAX_UINT is handled seperately, by encoding 2^32 using 33
 * bits, which fits in 5 bytes at 7 content bits per byte.
 *
 * Although this representation could store arbitrarily large integral values,
 * the decoding algorithm silently shifts the bits in a Uint, so any encoding
 * representing a value larger than MAX_UINT will overflow and yield its
 * remainder in modulo 2^32.
 *
 * This structure is really a very poor cousin of Uli Germann's idea, which
 * can be found properly implemented in his code in the tpt/ subdirectory.
 * We keep this class here nonetheless, because it can be used to optimize
 * some things in particular cases, e.g., PhraseTable::findInAllTables.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */


#ifndef _COMPACT_PHRASE_H_
#define _COMPACT_PHRASE_H_

#ifdef COMPACT_PHRASE_DEBUG
   #define DBG(x) cerr << x
#else
   #define DBG(X)
#endif

#include "array_mem_pool.h"
#include "str_utils.h"
#include "voc.h"
#include <boost/operators.hpp>

namespace Portage {

// Note: VectorPhrase is defined at the end of this file.

/// Phrase represented as a sequence of Uints stored in a compact way.
class CompactPhrase {
   /// Data for this compact phrase.  The first byte is a reference counter,
   /// to avoid unecessary duplication, while the rest are the actual compact
   /// phrase.
   char* data;

   /// How big an array we want to be in the pool rather than outside
   static const Uint PoolMaxArraySize = 64;

   /// Memory pool for efficiently allocating small CompactPhrases.
   static ArrayMemPool<char, PoolMaxArraySize, 4096> mempool;

   /// Allocate the data buffer, using the mempool
   void alloc_data(Uint size);

   /// Free the data buffer, taking into consideration the mempool
   void free_data();

 public:
   /// Construct an empty compact phrase
   CompactPhrase() : data(NULL) { DBG("c0"); }
   /// Copy constructor
   CompactPhrase(const CompactPhrase& other) : data(NULL) {
      DBG("c1"); operator=(other);
   }
   /// Construct a compact phrase from a regular phrase
   CompactPhrase(const vector<Uint>& phrase) : data(NULL) {
      DBG("c2"); fromPhrase(phrase);
   }
   /// Construct a compact phrase from a char*, e.g., a previously serialized
   /// or externally stored compact phrase.
   CompactPhrase(const char* packed);
   /// Desctructor
   ~CompactPhrase() { DBG("d0"); clear(); }

   /// copy a phrase into this compact phrase
   CompactPhrase& operator=(const vector<Uint>& phrase) {
      DBG("=0"); fromPhrase(phrase); return *this;
   }
   /// CompactPhrase copying
   CompactPhrase& operator=(const CompactPhrase& other);

   /// Compares for equality to Compact Phrases
   /// @param that right-hand side operand
   /// @return true if *this and that represent the same phrase
   bool operator==(const CompactPhrase& that) const {
      return strcmp(c_str(), that.c_str()) == 0;
   }
   /// Compares for equality to Compact Phrases
   /// @param that right-hand side operand
   /// @return true if *this and that represent different phrases
   bool operator!=(const CompactPhrase& that) const {
      return !operator==(that);
   }

   /// (Re)initialize to represent the given regular phrase 
   /// @param phrase Phrase to represent
   void fromPhrase(const vector<Uint>& phrase);

   /// Conversion from a packed to a regular phrase
   /// @param phrase The result will be stored in phrase
   /// @param packed compact phrase to unpack; unpack self if NULL
   void toPhrase(vector<Uint>& phrase, const char* packed = NULL) const;

   /// Encode just one number into a string.  this version does not do the +1,
   /// so a value of zero is actually encoded as a NULL byte.
   /// @param value  Number to pack
   /// @return string with the packed representation of value
   static string packNumber(Uint64 value);

   /// Decode just one number from a string
   /// @param pos  where to start reading the number; pos will point just past
   ///             the last byte read after unpacking one number.
   /// @param the unpacked value
   static Uint64 unpackNumber(const char*& pos);

   /// clears the data from this CompactPhrase
   void clear();

   /// Count the number of Uints in a compact phrase representation
   /// @param packed representation to count; count self if NULL
   Uint size(const char* packed = NULL) const;

   /// Return true iff this phrase is empty (i.e., if size() == 0)
   bool empty() const { return !data || *(data+1) == 0x00; }

   /// Return the value of the first element
   Uint front() const { assert(!empty()); return *(begin()); }

   /// Return the value of the last element
   Uint back() const { assert(!empty()); return *(rbegin()); }

   /// Get the char* itself, e.g., for serialization or extenal storage
   /// Warning: this char* may be deleted when self is deleted or modified
   const char* c_str() const;

   /// Get the ref count for the data part
   Uint ref_count() const;

   // forward declaration
   class const_iterator;
   
   /// Return a const_iterator to the beginning of the phrase
   const_iterator begin() const {
      if ( data ) return const_iterator(data+1);
      else return const_iterator(NULL);
   }

   /// Return a const_iterator to the beginning of the phrase
   const const_iterator& end() const {
      return end_iter;
   }
   
   // forward declaration
   class const_reverse_iterator;

   /// Return a const_reverse_iterator beginning at the end of the phrase
   const_reverse_iterator rbegin() const {
      if ( data ) return const_reverse_iterator(data+1);
      else return const_reverse_iterator(NULL);
   }

   /// Return a const_reverse_iterator at its end position
   const const_reverse_iterator& rend() const {
      return end_rev_iter;
   }


   /// const_iterator over a compact phrase
   class const_iterator 
      : public boost::input_iterator_helper<const_iterator, Uint>
   {
      friend class CompactPhrase;
      /// always points to the next position, not current, or NULL if at end.
      const char* pos;
      /// value at current position, if not end
      Uint current_val;
      /// Constructor for use by begin() and end() in owner class
      const_iterator(const char* data) : pos(data), current_val(0) {
         if ( data ) operator++();
      }

    public:
      /// Default constructor - initialize with = before using!
      const_iterator() : pos(NULL), current_val(0) {}

      /// Prefix ++: move to next element.
      const_iterator& operator++();

      /// Postfix ++: less efficient, avoid using unless really needed!
      const_iterator operator++(int);

      /// += is just a loop over ++, implemented for convenience, but not
      /// particularly efficient.
      const_iterator& operator+=(int n);

      /// Get the value at the current position
      /// @pre *this != end()
      Uint operator*() {
         assert (pos);
         return current_val;
      }

      /// Test for equality
      bool operator==(const const_iterator& other) {
         return pos == other.pos;
      }

      /// Test for inequality
      bool operator!=(const const_iterator& other) {
         return !operator==(other);
      }
   }; // class const_iterator


   /// const_iterator over a compact phrase
   class const_reverse_iterator 
      : public boost::input_iterator_helper<const_reverse_iterator, Uint>
   {
      friend class CompactPhrase;
      /// always points to the current position, or NULL if at end.
      const char* pos;
      /// pointer to the beginning of the CompactPhrase's data
      const char* start_pos;
      /// value at current position, if not end
      Uint current_val;
      /// Constructor for use by rbegin() and rend() in owner class
      const_reverse_iterator(const char* data)
         : pos(NULL), start_pos(data)
      {
         if ( data ) { pos = data + strlen(data); operator++(); }
      }

    public:
      /// Default constructor - initialize with = before using!
      const_reverse_iterator() : pos(NULL), start_pos(NULL), current_val(0) {}

      /// Prefix ++: move to next element.
      const_reverse_iterator& operator++();

      /// Postfix ++: less efficient, avoid using unless really needed!
      const_reverse_iterator operator++(int);

      /// += is just a loop over ++, implemented for convenience, but not
      /// particularly efficient.
      const_reverse_iterator& operator+=(int n);

      /// Get the value at the current position
      /// @pre *this != end()
      Uint operator*() {
         assert (pos);
         return current_val;
      }

      /// Test for equality
      bool operator==(const const_reverse_iterator& other) {
         return pos == other.pos && start_pos == other.start_pos;
      }

      /// Test for inequality
      bool operator!=(const const_reverse_iterator& other) {
         return !operator==(other);
      }
   }; // class const_iterator

 private:  
   /// Static end() const_iterator since we ask for this very often!
   static const const_iterator end_iter;
   /// Static rend() const_iterator since we also ask for this very often!
   static const const_reverse_iterator end_rev_iter;

 public:
   /// Print ref count statistics, if compiled with -DCOMPACT_PHRASE_STATS,
   /// do nothing otherwise.
   static void print_ref_count_stats();

   /// Unit testing
   /// @return true if all tests are successful
   static bool test();


}; // CompactPhrase

/// Stable ordering for CompactPhrase - stable, but not "logical" - this is
/// just implemented so CompactPhrase can be the key type in a map.
/// @return true iff x is consered to come before y
inline bool operator<(const CompactPhrase& x, const CompactPhrase& y) {
   return strcmp(x.c_str(), y.c_str()) < 0;
}


/// Phrase stored in a regular, unpacked vector.
/// VectorPhrase and CompactPhrase are designed to be fully interchangeable
/// and support automatic conversions in both directions.
class VectorPhrase : public vector<Uint> {
 public:
   using vector<Uint>::operator=;
   VectorPhrase& operator=(const VectorPhrase& other)
      { vector<Uint>::operator=(other); return *this; }
   VectorPhrase& operator=(const CompactPhrase& cPhrase)
      { cPhrase.toPhrase(*this); return *this; }
   VectorPhrase(const CompactPhrase& cPhrase) { cPhrase.toPhrase(*this); }
   VectorPhrase(const VectorPhrase& other) { *this = other; }
   VectorPhrase(size_t size, Uint default_val = 0)
      : vector<Uint>(size,default_val) {}
   template <class InputIterator>
   VectorPhrase(InputIterator first, InputIterator last)
      : vector<Uint>(first, last) {}
   VectorPhrase() {}
}; // VectorPhrase

/**
 * Convert a phrase from a Uint sequence (VectorPhrase or CompactPhrase) to a
 * space separated string.
 * @param begin    beginning of Uint sequence
 * @param end      end of Uint sequence
 * @param voc      vocabulary to convert Uint to strings
 * @pre all elements of uPhrase must be < voc.size()
 * @return the phrase in a single string, with a space separating each token
 */
template <class PhraseIter>
string phrase2string(PhraseIter begin, PhraseIter end, const Voc& voc, const char* sep = " ") {
   string result;
   const Uint voc_size = voc.size();
   bool first(true);
   for ( PhraseIter it = begin; it != end; ++it ) {
      if ( first ) first = false; else result += sep;
      assert(*it < voc_size);
      result += voc.word(*it);
   }
   return result;
}

/**
 * Convert a phrase from a Uint sequence (VectorPhrase or CompactPhrase) to a
 * space separated string.
 * @param uPhrase  Uint sequence phrase
 * @param voc      vocabulary to convert Uint to strings
 * @pre all elements of uPhrase must be < voc.size()
 * @return the phrase in a single string, with a space separating each token
 */
template <class PhraseT>
string phrase2string(const PhraseT& uPhrase, const Voc& voc, const char* sep = " ") {
   return phrase2string(uPhrase.begin(), uPhrase.end(), voc, sep);
}

} // Portage

// Make it easy to use a VectorPhrase in a hash table.
namespace std {
   namespace tr1 {
      template<> struct hash<Portage::VectorPhrase> {
         std::size_t operator()(const Portage::VectorPhrase& x) const {
            return boost::hash_range(x.begin(), x.end());
         }
      };
   }
}

#endif // _COMPACT_PHRASE_H_

