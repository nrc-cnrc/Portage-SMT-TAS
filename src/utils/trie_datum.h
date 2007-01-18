/**
 * @author Eric Joanis
 * @file trie_datum.h  Datum structure for PTrie.
 * $Id$
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __TRIE_DATUM_H__
#define __TRIE_DATUM_H__

namespace Portage {

template <class LeafDataT, class InternalDataT, bool NeedDtor> class TrieNode;

/// Datum for the trie.
template <class LeafDataT, class InternalDataT, bool NeedDtor>
class TrieDatum {
   /// Leaf Data value.
   LeafDataT value;             // Might get moved out as optimization
   /// Trie Key.
   TrieKeyT key_elem    : 30;   // : 8*sizeof(KeyT) - 2;
   /// Is this a leaf.
   bool is_leaf         :  1;
   /// Is there any children to this node.
   bool has_children    :  1;
   friend class TrieNode<LeafDataT, InternalDataT, NeedDtor>;
 public:
   /// Constructor.
   TrieDatum(TrieKeyT key_elem = 0) : /*child(NULL),*/ key_elem(key_elem),
      is_leaf(false), has_children(false) {}
   /// Reset key_elem to given value, and is_lead and has_children to false.
   void reset(TrieKeyT key_elem = 0) {
      this->key_elem = key_elem; is_leaf = has_children = false;
   }

   /**
    * Dumps the datum to a human readable string.  << must be defined for
    * LeafDataT.
    * @return Return a human readable string.
    */
   string dump() const;
   /// Gets the value (not modifiable).
   /// @return Returns the value (not modifiable).
   const LeafDataT& getValue() const { return value; }
   /// Gets the modifiable value.
   /// @return Returns a pointer on the internal value.
   LeafDataT* getModifiableValue() { return &value; }
   /// Gets the key.
   /// @return Returns the key of this TrieDatum.
   TrieKeyT getKey() const { return key_elem; }
   /// Checks if it is a leaf.
   /// @return Returns true if this TrieDatum is a leaf.
   bool isLeaf() const { return is_leaf; }
   /// Checks if it has children.
   /// @return Returns true if this TrieDatum has children
   bool hasChildren() const { return has_children; }
}; // TrieDatum

template<class LeafDataT, class InternalDataT, bool NeedDtor>
string TrieDatum<LeafDataT, InternalDataT, NeedDtor>::dump() const {
   ostringstream ss;
   ss << "TrieDatum*=" << this << " K=" << key_elem << " V=" << value
      << " L=" << is_leaf << " HC=" << has_children // << " C*=" << child
      << endl;
   return ss.str();
}

} // namespace Portage

#endif // __TRIE_DATUM_H__
