/**
 * @author Eric Joanis
 * @file trie.h Compact and fast implementation of a trie of Uints.
 *
 * $Id$
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __TRIE_H__
#define __TRIE_H__

#include "portage_defs.h"
#include "errors.h"
#include "str_utils.h"
#include "array_mem_pool.h"
#include "index_block_mem_pool.h"
#include "simple_histogram.h"
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

//#define DEBUG_PORTAGE_TRIE
#ifdef DEBUG_PORTAGE_TRIE
   #define TRIEDEBUG(expr) expr  ///< Trie's debugging macro
#else
   #define TRIEDEBUG(expr)       ///< Trie's debugging macro
#endif

namespace Portage {

/**
 * Trie key type definition.  TrieKeyT must be an unsigned integral type.  Its
 * two most significant bits will be used by the Trie data structure, so you
 * really have two fewer bits than your chosen key type has.  It's not a
 * template parameter, as it should really be, because I don't know who to get
 * the TrieDatum structure's ": 30" is a parameterized way.
 */
typedef Uint TrieKeyT;

/**
 * Dummy MapperFunctor for read_binary when mapping is not required
 */
class PTrieNullMapper {
 public:
   /// no-op: just copy back the input.
   Uint operator()(Uint key) { return key; }
   /// The unique instance of this class, anywhere, so that it can be
   /// recognized by mere pointer comparison
   static PTrieNullMapper m;
 private:
   // disable all possible ways to create new instances
   PTrieNullMapper();
   PTrieNullMapper(const PTrieNullMapper&);
   PTrieNullMapper& operator=(const PTrieNullMapper&);
};

/**
 * Dummy FilterFunctor for read_binary when filtering is not required
 */
inline bool PTrieKeepAll(const vector<Uint>& key_stack) { return true; }

};  //ends Portage namespace

#include "trie_datum.h"
#include "trie_node.h"

namespace Portage {

/// Empty class, for when InternalDataT is not required on a trie.
class Empty {};
/// Returns a string representing the name of class Empty.
template <> inline string typeName<Empty>() { return "Empty"; }

/// Template wrapper class for primitive types.
template <class PrimitiveT>
struct Wrap {
   PrimitiveT   value;   ///< the wrapped value

   /**
    * Constructor.  Default initialization to 0 (whatever that means for
    * PrimitiveT), and initialization from a primitive value.
    * @param init initial value
    */
   Wrap(PrimitiveT init = 0) : value(init) {}

   /**
    * Copy constructor.
    * @param x instance from which to instanciate
    */
   Wrap(const Wrap<PrimitiveT>& x) : value(x.value) {}

   /**
    * Assignment within the Wrap type.
    *
    * @param x instance to copy
    */
   Wrap& operator=(const Wrap<PrimitiveT>& x) {
      value = x.value; return *this;
   }

   /**
    * Assignment from the primitive type.
    * @param val value to copy
    */
   Wrap& operator=(PrimitiveT val) { value = val; return *this; }

   /**
    * Cast back to the primitive type.
    * @return The wrapped value
    */
   operator PrimitiveT() const { return value; }
};

/// typename for Wrap<float>.
template <> inline string typeName<Wrap<float> >() { return "Wrap<float>"; }

/**
 * "make_pair"-like Wrap creator.
 * @relates Wrap\<PrimitiveT\> 
 * @param value value to wrap
 */
template <class PrimitiveT>
Wrap<PrimitiveT> wrap(PrimitiveT value) {
   return Wrap<PrimitiveT>(value);
}

/**
 * Addition of Wraps.
 * @relates Wrap\<PrimitiveT\> 
 * @param x operand 1
 * @param y operand 2
 * @return x + y using PrimitiveT's + operator.
 */
template <class PrimitiveT>
Wrap<PrimitiveT> operator+(
   const Wrap<PrimitiveT>& x, const Wrap<PrimitiveT>& y
) {
   return Wrap<PrimitiveT>(x.value + y.value);
}

/**
 * Trie data structure mapping sequences of Uints to any type.
 *
 * Trie data structure for compact storage of a map from Uint[] to any value
 * type.  The key is actually stored in 30 bits, and one value is reserved, so
 * the valid key range is 0 .. 2^30 - 2.
 *
 * This class is temporarily called PTrie for "Portage Trie".
 *
 * InternalDataT is intended for storing information which occurs mostly (but
 * not necessarily exclusively) on internal nodes of the trie.  It must be a
 * class, and its default constructor should initialize it to the equivalent of
 * NULL/0/unassigned.  The utility template Wrap is provided for encapsulating
 * primitive types, e.g., you can set InternalDataT to Wrap\<float\>.  The
 * utility class Empty is provided in case you don't need InternalDataT.
 *
 * LeafDataT can be anything, but the structure is optimized for a small
 * leaf data type, of size 4 or 8.
 *
 * NeedDtor can be set to false if LeafDataT and InternalDataT can be safely
 * freed or moved without calling their destructors or constructors, such as
 * primitive types or simple classes without auxiliary structures.
 * Caveat: when NeedDtor is true, LeafDataT should still not use internal
 * pointers, because LeafDataT objects will be moved around using memcpy for
 * efficiency reasons.
 * TODO: test whether this optimisation is effective, and remove it if not.
 */
template <class LeafDataT, class InternalDataT = Empty, bool NeedDtor = true>
class PTrie {
   // We use hybrid hashing for root node, hashing on the root_hash_bits lowest
   // order bits of the first key element
   /// The root node of the trie
   vector<TrieNode<LeafDataT, InternalDataT, NeedDtor> > roots;
   typedef
      typename vector<TrieNode<LeafDataT, InternalDataT, NeedDtor> >::iterator
      root_iter;    ///< root node iterator
   typedef
      typename vector<TrieNode<LeafDataT, InternalDataT, NeedDtor> >::const_iterator
      root_c_iter;  ///< constant root node iterator

   /// Pool for more efficient allocation of TrieNode's
   typename TrieNode<LeafDataT, InternalDataT, NeedDtor>::TrieNodePool
      nodePool;

   /// Pool for more efficient allocation of TrieDatum arrays
   typename TrieNode<LeafDataT, InternalDataT, NeedDtor>::DatumArrayPool
      datumArrayPool;

   /// Pool for more efficient allocation of TrieNode pointer arrays.
   typename TrieNode<LeafDataT, InternalDataT, NeedDtor>::NodePtrArrayPool
      nodePtrArrayPool;

   /// The number of lowest order bits of key_elem's to use for root hashing.
   Uint root_hash_bits;

   /**
    * Find the root bucket for key_elem, based on its root_hash_bits least
    * significant bits.
    * @param key_elem key to find
    * @return root bucket index
    */
   Uint hash(TrieKeyT key_elem) const;

   /**
    * Cache to speed up sequences of many inserts.  This vector is used to
    * cache the last results of an insert into the trie, so that if another
    * element is inserted nearby (which is often the case), it won't have to
    * do all the binary searches, only the ones for parts of the search that
    * have changed.  The cache is immediately reset if it can't be used, since
    * it's only valid as long as no intervening insert happens.
    */
   vector<pair<Uint, TrieNode<LeafDataT, InternalDataT, NeedDtor> *> >
      insert_cache;

   /**
    * Helper for read_binary(): if a mapper is used, the direct children of the
    * root are no longer in the right buckets.  This method moves them all to
    * the right buckets, assuming they are sorted in the buckets where they are
    * found, but just not in the right bucket.
    */
   void fix_root_buckets();

 public:
   class iterator;
 private:
   const iterator end_iter;

 public:
   /// Largest allowable value for a TrieKeyT parameter in any public method.
   static const TrieKeyT MaxKey;

   /**
    * Insert <key, val> into the trie.
    * @param key      Key to be inserted
    * @param key_size Key size
    * @param val      Value to be inserted
    * @param p_val    If not NULL, will be set to the address of the inserted
    *                 value for further manipulation, which is only guaranteed
    *                 to be valid until the next non-const operation on the
    *                 trie.
    */
   void insert(const TrieKeyT key[], Uint key_size, const LeafDataT& val,
               LeafDataT** p_val = NULL);

   /**
    * Lookup key in the trie and create it if not found.  Lookup key in the
    * trie, inserting it with a default-initialized value if not found.
    * @param key      Key to be looked up and possibly inserted
    * @param key_size Key size
    * @param p_val    Will be set to the address of value inserted or found,
    *                 usable for further manipulation.  Only guaranteed to be
    *                 valid until the next non-const operation on the trie.
    * @return true if the key already existed, false if it had to be inserted
    */
   bool find_or_insert(const TrieKeyT key[], Uint key_size, LeafDataT*& p_val);

   /**
    * Find key in the trie.  If key is found, returns true, copies the value
    * into val, and the depth where key was found into depth, if depth is not
    * NULL.  If key is not found, returns false, but if depth is not NULL,
    * copies into val and depth the value and depth of the longest prefix of
    * key which exists.  In this case, a depth of 0 means no prefix was found.
    * @param key      Key to be looked up
    * @param key_size Key size
    * @param val      Will be set to the value associated with key, if found,
    *                 or the value associated with the longest prefix of key
    *                 which was found, if depth is not NULL.
    * @param depth    If not NULL, will be set to the length of the longest
    *                 prefix of key found in the trie.
    * @return true if the key was found
    */
   bool find(const TrieKeyT key[], Uint key_size, LeafDataT& val,
      Uint* depth = NULL) const;

   /**
    * Find key in the trie, getting a modifiable value.  Same as const find,
    * but returns a modifiable pointer to the value in p_val.  p_val is only
    * guaranteed to be valid until the next non-const operation on the trie.
    * @param key      Key to be looked up
    * @param key_size Key size
    * @param p_val    Will be set to the address of the value associated with
    *                 key, if found, or the address of the value associated
    *                 with the longest prefix of key which was found, if depth
    *                 is not NULL.
    * @param depth    If not NULL, will be set to the length of the longest
    *                 prefix of key found in the trie.
    * @return true if the key was found
    */
   bool find(const TrieKeyT key[], Uint key_size, LeafDataT*& p_val,
      Uint* depth = NULL);

   /**
    * Sum leaf values over a sequence of prefixes of a key.  Add the values
    * found in the trie for prefixes of key of depth [min_depth .. max_depth].
    * key_size is assumed to be >= max_depth.  val should be initialized
    * before calling sum(), each value found will be added to it.  The +
    * operator must be defined for LeafDataT.  Returns true if at least one
    * prefix was found, false otherwise.
    *
    * This function is equivalent to a loop using find() above, but it is
    * more efficient since the trie structure is only traversed once here,
    * but (max_depth-min_depth+1) times in an equivalent loop using find().
    * @param key        Key to be looked up
    * @param min_depth  shortest prefix of key to include in sum
    * @param max_depth  longest prefix of key to include in sum
    * @param val        The values found will be added to this variable.
    * @return true if at least one prefix was found, false otherwise.
    */
   bool sum(const TrieKeyT key[], Uint min_depth, Uint max_depth,
      LeafDataT& val) const;

   /**
    * Set the internal node value of key.  Key should already be an interal
    * node (i.e., it is the prefix of a previously insert key) or be expected
    * to become an internal node, otherwise this operation can be very space
    * consuming!
    * @param key        Key for which to set the internal node value
    * @param key_size   size of key
    * @param intl_val   new internal value for key
    */
   void set_internal_node_value(const TrieKeyT key[], Uint key_size,
      const InternalDataT& intl_val);

   /**
    * Look up the internal node value of key.
    * @param key        Key to look-up
    * @param key_size   size of key
    * @return The internal node value of key if key is found and its internal
    *         node value is set, or InternalDataT() (i.e., with default
    *         initialization, typically "0", whatever that means for type
    *         InternalDataT) if key is not found or if it doesn't have internal
    *         data set.
    */
   InternalDataT get_internal_node_value(const TrieKeyT key[],
      Uint key_size) const;

   /**
    * Sum internal node values over a sequence of prefixes of a key.  Add the
    * internal node values for the prefixes of key of lengths [min_len ..
    * max_len].  The + operator must be defined on InternalDataT.  If no
    * values are found, InternalDataT() (i.e., "0") is returned.
    * @param key        Key to be looked up
    * @param min_len    shortest prefix of key to include in the sum
    * @param max_len    longest prefix of key to include in the sum
    * @return InternalDataT() (= "0") + the sum of the internal values found.
    */
   InternalDataT sum_internal_node_values(const TrieKeyT key[], Uint min_len,
      Uint max_len) const;

   /**
    * Look up a whole path in the trie.
    * Equivalent to calling find on every prefix of key, at only the cost of
    * looking up key itself once.
    * 
    * @param key        Key to be looked up
    * @param key_size   Length of key
    * @param values     Will be filled with a pointer to the leaf value for
    *                   each prefix of key found, with NULL for each prefix of
    *                   key not found.  These pointers are modifiable, but are
    *                   only guaranteed to be valid until the next non-const
    *                   operation on the trie.
    * @return           The lenght of the longest prefix of key that exists in 
    *                   the trie, disregarding whether it is a leaf or not;
    *                   0 if no prefix of key exists in the trie.
    * @pre value and key must be arrays of size key_size.
    */
   Uint find_path(const TrieKeyT key[], Uint key_size, LeafDataT* values[]);

   /**
    * Constructor.
    * @param root_hash_bits determines how many hashing bits the root of the
    * trie will use.  The default should be suitable for most applications.
    */
   PTrie(Uint root_hash_bits = 8);

   /**
    * Destructor.
    */
   ~PTrie() { clear(); }

   /**
    * Removes all entries.
    */
   void clear();

   /**
    * Get cumulative stats about all nodes in this trie.
    * This is quite slow!  Call only for debugging / optimizing purposes.
    * @param intl_nodes    number of internal nodes
    * @param trieDataUsed  number of TrieDatum objects used
    * @param trieDataAlloc number of allocated spaces in vectors for TrieDatum
    *                      objects
    * @param childrenAlloc number of TrieNode* allocated
    * @param memoryUsed    Total memory used in MB
    * @param memoryAlloc   Total memory allocated in MB
    */
   void getStats(
      Uint &intl_nodes,
      SimpleHistogram<Uint>& trieDataUsed,
      SimpleHistogram<Uint>& trieDataAlloc,
      SimpleHistogram<Uint>& childrenAlloc,
      Uint &memoryUsed,
      Uint &memoryAlloc
   ) const;

   /**
    * Get cumulative stats in a printable format.  This is quite slow!  Call
    * only for debugging / optimizing purposes.
    * @return the cumulative stats in a printable format
    */
   string getStats() const;

   /**
    * Get the size_of figures of all structures in the Trie.
    * Intended for debugging and optimizing the structure.
    * @return a string showing the output of size_of for all structures and
    * sub--structures of the trie.
    */
   static string getSizeOfs();

   /**
    * Write out the trie in binary format.
    * Can only be used if the leaf and internal node data types can be
    * correctly copied with a simple memcpy and require not constructor or
    * destructor.  Includes seek pointers so that sub-tries can be efficiently
    * skipped at reading time.
    * @param ofs output file stream to use - must be seekable
    * @return the number of internal nodes written
    */
   Uint write_binary(ofstream& ofs) const;

   /**
    * Load the trie from the binary format written by write_binary().
    * @param is  input stream located at the position where write_binary
    *            started writing.
    * @return the number of internal nodes read and loaded
    */
   Uint read_binary(istream& is) {
      return read_binary(is, PTrieKeepAll, PTrieNullMapper::m);
   }

   //@{
   /**
    * Load the trie from the binary format written by write_binary(), with on
    * the fly filtering of the contents.
    *
    * @param is  input stream located at the position where write_binary
    *            started writing.
    *
    * @param filter  This functor will be called for each key, as it is found
    * in the input stream, to determine if it should be filtered out during
    * reading.  
    * Filter should be a function or a function object implementing
    *    bool operator()(const vector<Uint>& key_stack)
    * which should return true if key_stack should be kept, false otherwise.
    * When false is returned, the leaf and sub-tree rooted at key_stack are
    * skipped.
    * Guarantees about key_stack, when calling filter():
    * 1) When key_stack.size() > 1, filter was already called and returned true
    * for each prefix of key_stack.
    * 2) key_stack.size() >= 1.
    *
    * @param mapper  (optional) This functor will be called for each key after
    * filter() has been applied, to remap the keys on disk to new keys.
    * Filter must be a function object (a regular function will not work)
    * implementing
    *    Uint operator()(Uint key)
    * which should map each possible 30 bit key to another, unique 30 bit key.
    * No mapping is done when mapper is left out.
    *
    * @return the number of internal nodes kept.
    */
   template <typename Filter, typename Mapper>
      Uint read_binary(istream& is, Filter filter, Mapper& mapper);

   template <typename Filter> Uint read_binary(istream& is, Filter& filter) {
      return read_binary(is, filter, PTrieNullMapper::m);
   }
   //@}

   /**
    * Traverse the whole trie, calling the Visitor functor for every leaf node.
    *
    * Class Visitor must implement this functor method:
    *
    *   void operator()(const vector<Uint> &key, InternalDataT value);
    *
    * A typical application would be dumping the trie to file.  See
    * test_trie.cc for an example.
    *
    * Although this method works well, it is easier and faster for most
    * applications to use the iterator class recursively with begin() and end()
    * instead.
    *
    * @param visitor visitor that will explore every nodes
    */
   template <class Visitor> void traverse(Visitor& visitor) const;


   /**
    * Get an iterator to the beginning of the root node's children.
    * See rec_dump_trie in test_trie.cc for a sample use.
    * @return an iterator which can be used to traverse the root in a
    * non-specified order, through which the whole trie can be traversed
    * recursively (using the iterator's begin_children() and end_children()
    * methods)
    */
   iterator begin_children();
   /**
    * Get an iterator to the end of the root node's children.
    * @return iterator on end position
    */
   const iterator& end_children() const { return end_iter; }

   friend class iterator;
   /**
    * Trie iterator.  Iterator to traverse a trie node, possibly the root,
    * possibly one of its children.
    */
   class iterator {
      PTrie& parent;           ///< Parent PTrie object
      TrieNode<LeafDataT, InternalDataT, NeedDtor>* node; ///< Current TrieNode
      bool is_root;            ///< Whether this is a root iterator
      bool is_end;             ///< Whether this iter is on the end position
      Uint root_bucket;        ///< For a root iterator, which bucket we're in
      Uint position;           ///< Current position in node.

      friend class PTrie;
      /**
       * Constructor.
       * @param parent       Parent PTrie object
       * @param node         Current node
       * @param is_root      Whether we're constructing a root iterator
       * @param is_end       Whether we're constreucting an iter in end pos.
       * @param root_bucket  For a root iterator, which bucket we start in.
       * @param position     Start position.  Use -1 and call ++ to correctly
       *                     get the beginning of a node.
       */
      iterator(PTrie& parent,
               TrieNode<LeafDataT, InternalDataT, NeedDtor>* node,
               bool is_root, bool is_end, Uint root_bucket, Uint position)
         : parent(parent), node(node), is_root(is_root), is_end(is_end)
         , root_bucket(root_bucket), position(position) {}

    public:
      /**
       * Equality.
       * @param other operand
       * @return true if both iterators are at the end position or if both
       *         iterators point to the same object, false otherwise
       */
      bool operator==(const iterator& other) const;

      /**
       * Inequality.
       * @param other operand
       * @return true iff operator== returns false.
       */
      bool operator!=(const iterator& other) const {
         return ! operator==(other);
      }

      /**
       * Prefix ++ operator.
       * Move to next record, return ref to self (post move)
       *
       * @pre self != end()
       * @return the incremented iterator.
       */
      iterator& operator++();

      /**
       * Postfix ++ operator.
       * Move to next record, return copy of self before move.
       * Expensive, use prefix ++ whenever possible!!!
       * @pre self != end()
       * @return a copy not incremented of the iterator.
       */
      iterator operator++(int) {
         iterator copy(*this);
         operator++();
         return copy;
      }

      /**
       * Get the key for this node.
       * @pre self != end()
       * @return the key for this node
       */
      TrieKeyT get_key() const {
         return node->get_datum(position)->getKey();
      }

      /**
       * Check if the current node is a leaf node.
       * @pre self != end()
       * @return true iff the current node is a leaf node
       */
      bool is_leaf() const {
         return node->get_datum(position)->isLeaf();
      }

      /**
       * Get the leaf value for the current node.  The reference returned is
       * only guaranteed to be valid until the next non-const operation on the
       * parent PTrie.
       * @pre self != end() and is_leaf()
       * @return the leaf value for the current node.
       */
      const LeafDataT& get_value() const {
         return node->get_datum(position)->getValue();
      }

      /**
       * Get the internal node value for the current node.
       * @return the internal node value if set, InternalDataT() otherwise.
       */
      InternalDataT get_internal_node_value() const;

      /**
       * Check if the current node has children.
       * @pre self != end()
       * @return true iff the current node has children
       */
      bool has_children() const {
         return node->get_datum(position)->hasChildren();
      }

      /**
       * Get an iterator to this node's children.  Returns an iterator to this
       * node's children, for recursing through the trie.
       * @pre self != end()
       * @return an iterator to this node's children.
       */
      iterator begin_children();

      /**
       * Get an end iterator for this node's children.
       * @pre self != end()
       * @return end iterator
       */
      const iterator& end_children() const { return parent.end_iter; }

      /**
       * Insert a child for this node.
       * If that child already exists, replaces its value.
       * Invalidates any iterator to this node's children.
       * @param leaf_key last element of the new child's key, added as a
       *                 suffix to this iterator's full key.
       * @param val      Value to be inserted
       */
      void insert(const TrieKeyT leaf_key, const LeafDataT& val);

      /**
       * Delete the leaf value for a child of this node, if it exists,
       * do nothing otherwise.
       * @param leaf_key last element of the child's key, added as a
       *                 suffix to this iterator's full key.
       */
      void delete_leaf(const TrieKeyT leaf_key);

   }; // class iterator

}; // PTrie

} // Portage namespace

#include "trie-cc.h"


#endif //  __TRIE_H__

