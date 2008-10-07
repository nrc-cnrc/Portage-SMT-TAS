/**
 * @author Eric Joanis
 * @file trie_node.h  Node structure for trie.h.
 * $Id$
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __TRIE_NODE_H__
#define __TRIE_NODE_H__

#include "simple_histogram.h"

namespace Portage {

template <class LeafDataT, class InternalDataT, bool NeedDtor> struct PTrie;

/// Zero-overhead optional class wrapper.  In struct { First; Second }, First
/// will always / occupy at least 1 byte even if it has no members.  If you use
/// struct { OptionalClassWrapper<First, Second> }, First will not occupy any
/// memory unless it actually contains elements.
/// OptionalClass (i.e., First) must support operator=() and copy construction.
/// MandatoryClass (i.e., Second) must support copy construction.
template <class OptionalClass, class MandatoryClass>
struct OptionalClassWrapper : OptionalClass {
   /// Member element of type MandatoryClass.  Can be assigned or read
   /// directly.
   MandatoryClass second;

   /// Constructor.
   /// @param first initial value for "member" of type OptionalClass
   /// @param second initial value for member of type MandatoryClass
   OptionalClassWrapper(const OptionalClass& first,
                        const MandatoryClass& second)
     : OptionalClass(first), second(second) {}

   /// Change the value of First (the "member" of type OptionalClass)
   /// @param first new value
   void first(const OptionalClass& first) { OptionalClass::operator=(first); }
   /// Get the value of First (the "member" of type OptionalClass)
   OptionalClass first() const { return OptionalClass(*this); };
};

/// Node for the trie.
template <class LeafDataT, class InternalDataT, bool NeedDtor>
class TrieNode {
   /**
    * Growing factor for the internal list.
    * List size growth: * 2 (std::vector's default) grows too fast and leaves a
    * lot of unused space in the lists, around 25%.  * 1.4 gave about 15%
    * unused, * 1.2 about 8%, * 1.1 about 4-5%, at only a small cost in time.
    * Thus, we use 1.05.
    */
   static const float GrowthFactor;

   /// List of key_elem/value pairs, kept sorted on key_elem.
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *list;

   /**
    * List of children.
    * For space efficiency, since we have far more leaves than internal node,
    * the children pointers are stored in a separate list here.
    * For further space efficiency on 64-bit machines, we use 32-bit indices
    * into the node mem pool rather than 64-bit pointers.  This makes no
    * difference in space on 32-bit machines, and benchmarking has shown that
    * it comes at a trivial cost in speed, so this design choice is acceptable
    * for both word sizes.
    */
   Uint* children;

   // Elements used in list.
   // Optimization: we don't store list_size anymore - this will save lots of
   // memory on 32 bit machines.  Instead, we initialized unused TrieDatum's
   // with the highest possible key and both flags false.  The binary search
   // will only in a small proportion of times need an extra iteration, so
   // inferring list_size is cheap.
   //Uint list_size;

   /// Number of elements allocated in list.
   /// We don't keep a "list size" with the number of elements used - this will
   /// save lots of memory on 32 bit machines.  Instead, we initialized unused
   /// TrieDatum's with key_elem = NoKey and both flags false.  The binary
   /// search will need an extra iteration in only a small proportion of times,
   /// so inferring the used list size is cheap, and this trade-off is
   /// beneficial.
   Uint list_alloc;

   /**
    * Combined variable: interal data and children size.
    * Having InternalDataT intl_data followed by Uint children_size would
    * cost sizeof(Uint) bytes whenever InternalDataT is EmptyClass.  To avoid
    * this space overhead in a structure that we are trying to make as tight
    * as possible, we use OptionalClassWrapper<>, but provide inline get and
    * set methods with clearer names.
    */
   OptionalClassWrapper<InternalDataT, Uint> intl_data_children_size;

   /// Get the number of children allocated.
   /// @return the size of the children array
   Uint children_size() const { return intl_data_children_size.second; }
   /// Set the number of children allocated.
   /// @param size the new size of the children array
   void children_size(Uint size) { intl_data_children_size.second = size; }

   /**
    * Returns the position where key_elem either is or should be inserted to
    * if it isn't there.
    * @param key_elem key element to look up
    * @return the position where every element at a smaller position has key
    *         less than key_elem, and where every element at that or greater
    *         position has key greater or equal to key_elem.
    */
   Uint find_position(TrieKeyT key_elem) const;

public:
   /// Largest possible key_elem value, reserved to mean "not a real key".
   /// We declare it here instead of in TrieDatum because we only use it here.
   static const TrieKeyT NoKey;

   /// Pool for more efficient allocation of TrieDatum arrays.
   /// Typedef'd here, but has to be owned by the parent PTrie class and passed
   /// as parameter whenever needed.
   typedef ArrayMemPool<TrieDatum<LeafDataT, InternalDataT, NeedDtor>, 32>
      DatumArrayPool;

   /// Pool for more efficient allocation of TrieNode index (indirect pointers)
   /// arrays.
   /// Typedef'd here, but has to be owned by the parent PTrie class and passed
   /// as parameter whenever needed.
   typedef ArrayMemPool<Uint, 32> NodePtrArrayPool;

   /// Pool for efficient of allocation of TrieNodes
   /// Typedef'd here, but has to be owned by the parent PTrie class and passed
   /// as parameter whenever needed.
   typedef IndexBlockMemPool<TrieNode<LeafDataT, InternalDataT, NeedDtor> >
      TrieNodePool;

   /**
    * Find an element, and insert it if not found.
    * @param key_elem   key element to look up and/or insert
    * @param position   will be set to position of found or inserted element.
    *                   (This value can be passed to get_children and
    *                   create_children, but is only valid until the next
    *                   insert.)
    * @param node_pool  Parent PTrie's TrieNodePool
    * @param da_pool    Parent PTrie's DatumArrayPool
    * @param npa_pool   Parent PTrie's NodePtrArrayPool
    * @return The TrieDatum that was found and/or inserted.
    */
   TrieDatum<LeafDataT, InternalDataT, NeedDtor>* insert(
      TrieKeyT key_elem, Uint& position, TrieNodePool& node_pool,
      DatumArrayPool& da_pool, NodePtrArrayPool& npa_pool);

   /**
    * Find an element.
    * If found, returns true, sets datum to point to the TrieDatum, and sets
    * position to the position of the element (needed for get_children and
    * create_children).
    * Returns false otherwise.
    * @param key_elem   key element to look up.
    * @param datum      will be set to the TrieDatum associated with key_elem,
    *                   if found
    * @param position   will be set to the position of key_elem, if found.
    *                   (This value can be passed to get_children and
    *                   create_children, but is only valid until the next
    *                   insert.)
    * @return whether key_elem was found.
    */
   bool find(TrieKeyT key_elem,
      TrieDatum<LeafDataT, InternalDataT, NeedDtor> *& datum,
      Uint& position) const;

   /**
    * Get the child pointer for the key at position.
    * @pre list_alloc > position && list[position].has_children
    * @param position   position of child pointer to get, as returned by find()
    *                   or insert()
    * @return An indirect pointer to children at position.  This pointer can be
    *         converted into a real pointer by calling get_ptr() on the parent
    *         PTrie's TrieNodePool.
    */
   Uint get_children(Uint position) const {
      return children[position];
   }

   /**
    * Create a child pointer for the key at position.
    * @pre list_alloc > position && !list[position].key_elem == NoKey &&
    *      !list[position].has_children
    * To reduce memory allocation overhead, pass a fresh node in as
    * pre_allocated_node (mandatory).
    * @param position   Position where to insert children, as returned by
    *                   insert() or find().
    * @param pre_allocated_node_index Indirect pointer to the new node, which
    *                   must have been preallocated by calling alloc() on the
    *                   parent PTrie's TrieNodePool.
    * @param node_pool  Parent PTrie's TrieNodePool
    * @param da_pool    Parent PTrie's DatumArrayPool
    * @param npa_pool   Parent PTrie's NodePtrArrayPool
    */
   void create_children(Uint position,
      Uint pre_allocated_node_index, TrieNodePool& node_pool,
      DatumArrayPool& da_pool, NodePtrArrayPool& npa_pool);

   /// Get the current number of datums allocated.
   /// @return the number of datums allocated
   Uint size() const { return list_alloc; }

   /**
    * Get the TrieDatum at a given position.
    * @pre position < size()
    * @param position   position of datum to get, as returned by find()
    *                   or insert()
    * @return the TrieDatum at position
    */
   const TrieDatum<LeafDataT, InternalDataT, NeedDtor>*
      get_datum(Uint position) const
   {
      return &(list[position]);
   }

   /**
    * Associate val with the key_elem at position, making it a leaf.
    * @pre position < size() && list[position].key_elem != NoKey
    * @param position   position of the leaf value to set, as returned by
    *                   find() or insert()
    * @param val        new leaf value for the key at position
    */
   void set_leaf_value(Uint position, const LeafDataT& val);

   /// Get the internal node value for this node.
   /// @return the current internal node value
   InternalDataT internal_data() const {
      return InternalDataT(intl_data_children_size.first());
   }
   /// Set the internal node value for this node.
   /// @param value the new internal node value
   void internal_data(const InternalDataT& value) {
      intl_data_children_size.first(value);
   }

   /// Dump some useful info about this trie node.
   /// @return a printable string with debugging info about this node.
   string dump_info() const;

   /// Constructor.
   TrieNode();

   /// Destructor. Doesn't recursively clear the structure, PTrie has to do
   /// that by calling clear(), since the memory pools are needed for that
   /// operation.
   ~TrieNode() {}

   /**
    * Delete all children and empty self.
    * Recursive - runs in linear time in the total size of the tree structure.
    * Required if NeedDtor is true.  Use non_recursive_clear() instead if
    * NeedDtor is false.
    * @param node_pool  Parent PTrie's TrieNodePool
    * @param da_pool    Parent PTrie's DatumArrayPool
    * @param npa_pool   Parent PTrie's NodePtrArrayPool
    */
   void clear(TrieNodePool& node_pool, DatumArrayPool& da_pool,
      NodePtrArrayPool& npa_pool);

   /**
    * Equivalent to clear() if NeedDtor is false.
    * When using memory pools and a LeafDataT that doesn't have a mandatory
    * destructor, recursive clear() is wasteful, we can just clear the root
    * (non-recursively) and then clear the memory pools (the parent PTrie must
    * do that step).
    */
   void non_recursive_clear();

   /**
    * Add cumulative stats about this node and its children nodes.
    * @param intl_nodes     number of internal nodes
    * @param trieDataUsed   number of TrieDatum objects used
    * @param trieDataAlloc  number of allocated spaces in vectors for TrieDatum
    *                       objects
    * @param childrenAlloc  number of TrieNode* allocated
    * @param node_pool      Parent PTrie's TrieNodePool needed to get access to
    *                       real TrieNode pointers
    */
   void addStats(
      Uint &intl_nodes,
      SimpleHistogram<Uint>& trieDataUsed,
      SimpleHistogram<Uint>& trieDataAlloc,
      SimpleHistogram<Uint>& childrenAlloc,
      const TrieNodePool& node_pool
   ) const;

   /**
    * Traverse the sub-trie rooted at this node, calling the Visitor functor
    * for every leaf node.  Class Visitor must implement this functor method:
    *
    *   void operator()(const vector<Uint> &key, InternalDataT value);
    *
    * key_stack should contain the key elements of the parents of this node.
    * @param visitor    Visitor object functor
    * @param key_stack  key_elems of parent nodes
    * @param node_pool  Parent PTrie's TrieNodePool 
    */
   template <class Visitor>
   void traverse(Visitor& visitor, vector<Uint>& key_stack,
      const TrieNodePool& node_pool) const;

   /**
    * Write the sub-trie rooted at this node in binary format.
    * @param ofs        output file stream - must be seekable
    * @param node_pool  Parent PTrie's TrieNodePool
    * @return the number of internal nodes written
    */
   Uint write_binary(ofstream& ofs, const TrieNodePool& node_pool) const;

   /**
    * Recursively deserialize the sub-trie written out but write_binary,
    * starting at the current location in is.
    * @param is         stream to read from
    * @param filter     filter which determines subtrees to prune (see
    *                   PTrie::read_binary() for details).
    * @param mapper     mapper to remap keys while loading (see
    *                   PTrie::read_binary() for details).
    * @param key_stack  complete key to the node about to be read
    * @param node_pool  Parent PTrie's TrieNodePool
    * @param da_pool    Parent PTrie's DatumArrayPool
    * @param npa_pool   Parent PTrie's NodePtrArrayPool
    * @return           The number of nodes kept.  When the return value is
    *                   zero, even the node itself contains no information, and
    *                   can safely be discarded.
    */
   template <typename Filter, typename Mapper>
   Uint read_binary(istream& is, Filter& filter, Mapper& mapper,
                    vector<Uint>& key_stack,
                    TrieNodePool& node_pool, DatumArrayPool& da_pool,
                    NodePtrArrayPool& npa_pool);

 private:

   /// When read_binary is called with a non-trivial mapper, does the real work
   /// of mapping old keys to new keys are resorting.
   template <typename Mapper>
   void apply_mapper(Mapper& mapper, Uint required_list_size,
                     TrieNodePool& node_pool, DatumArrayPool& da_pool,
                     NodePtrArrayPool& npa_pool);

   /// For resorting the node keys in apply_mapper()
   class KeyLessThan {
      TrieNode<LeafDataT, InternalDataT, NeedDtor>* node;
    public:
      KeyLessThan(TrieNode<LeafDataT, InternalDataT, NeedDtor>* node)
         : node(node) {}
      /// Returns true iff the element at position x in node has a key which
      /// smaller than that at y
      bool operator()(Uint x, Uint y) {
         return node->list[x].getKey() < node->list[y].getKey();
      }
   };
   friend class KeyLessThan;

   // for PTrie::fix_root_buckets().
   friend class PTrie<LeafDataT, InternalDataT, NeedDtor>;

}; // TrieNode

#include "trie_node-cc.h"

} // Portage namespace


#endif //  __TRIE_NODE_H__

