/**
 * @author Eric Joanis
 * @file trie_node-cc.h  Implementation of template methods for trie_node.h.
 * $Id$
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Conseil national de recherches Canada / Copyright 2006, National Research Council Canada
 */

#ifndef __TRIE_NODE_CC_H__
#define __TRIE_NODE_CC_H__


template<class LeafDataT, class InternalDataT, bool NeedDtor>
const TrieKeyT TrieNode<LeafDataT, InternalDataT, NeedDtor>::NoKey =
   ~TrieKeyT(0) >> 2;

template<class LeafDataT, class InternalDataT, bool NeedDtor>
const float TrieNode<LeafDataT, InternalDataT, NeedDtor>::GrowthFactor = 1.05;

template<class LeafDataT, class InternalDataT, bool NeedDtor>
Uint TrieNode<LeafDataT, InternalDataT, NeedDtor>::find_position(
   TrieKeyT key_elem
) const {
   //cerr << dump_info();
   Uint start = 0;
   Uint end = list_alloc;
   // binary search through a sorted array
   while ( end > start ) {
      Uint mid = (start + end) >> 1; // same as / 2 but a bit faster.
      //cerr << "start=" << start << " mid=" << mid << " end=" << end << endl;
      TRIEDEBUG(assert (mid < end));
      if ( list[mid].key_elem < key_elem )
         start = mid + 1;
      // the "else if" clause is not necessarily, but this function is slightly
      // faster with it.
      else if ( list[mid].key_elem == key_elem )
         return mid;
      else
         end = mid;
   }
   TRIEDEBUG(assert (start == end));
   return start;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
TrieDatum<LeafDataT, InternalDataT, NeedDtor>*
   TrieNode<LeafDataT, InternalDataT, NeedDtor>::insert(
      TrieKeyT key_elem, Uint& position, TrieNodePool& node_pool,
      DatumArrayPool& da_pool, NodePtrArrayPool& npa_pool
) {
   //cerr << "i" << key_elem;
   if ( key_elem >= NoKey ) {
      error(ETFatal, "Overflow error in Trie; need larger key data type!");
   }

   //Uint bucket = hash(key_elem);
   //Uint position = find_position(bucket, key_elem);
   //vector<TrieDatum<LeafDataT, InternalDataT, NeedDtor> > &list = lists[bucket];
   position = find_position(key_elem);

   if ( position == list_alloc || list[position].key_elem != key_elem ) {
      // key_elem not already in, add it

      if ( position == list_alloc || list[list_alloc-1].key_elem != NoKey) {
         Uint old_size = list_alloc;
         list_alloc = Uint((list_alloc + 1) * GrowthFactor);
         TrieDatum<LeafDataT, InternalDataT, NeedDtor> *new_list =
            da_pool.alloc_array_no_ctor(list_alloc, list_alloc);
         // This for loop won't work for LeafDataT with mandatory ctor/dtor - 
         // we need to memcpy the elements we keep and call the ctor on the new
         // ones.
         //for ( Uint i = 0; i < position; ++i ) new_list[i] = list[i];
         memcpy(new_list, list, position * sizeof(*new_list));

         // Need to directly call the contructor, in case mandatory
         //new_list[position] = TrieDatum<LeafDataT, InternalDataT, NeedDtor>(key_elem);
         new(new_list+position) TrieDatum<LeafDataT, InternalDataT, NeedDtor>(key_elem);

         TRIEDEBUG(assert(!new_list[position].has_children));

         // And again use memcpy for the tail-end of the list.
         //for ( Uint i = position; i < old_size; ++i ) new_list[i+1] = list[i];
         memcpy(new_list+position+1, list+position,
                (old_size-position) * sizeof(*new_list));

         // Newly allocated by still unused element must be set to NoKey to
         // mark them as unused.
         for ( Uint i = old_size+1; i < list_alloc; ++i )
            //new_list[i] = TrieDatum<LeafDataT, InternalDataT, NeedDtor>(NoKey);
            new(new_list+i) TrieDatum<LeafDataT, InternalDataT, NeedDtor>(NoKey);

         if ( list ) da_pool.free_array_no_dtor(list, old_size);
         list = new_list;
      } else {
         TRIEDEBUG(assert(list[list_alloc-1].key_elem == NoKey));
         for ( Uint i = list_alloc - 1; i > position; --i )
            list[i] = list[i-1];
         list[position] = TrieDatum<LeafDataT, InternalDataT, NeedDtor>(key_elem);
         TRIEDEBUG(assert(!list[position].has_children));
      }

      if ( position < children_size() ) {
         // We also need to shift the children pointers since position is
         // before or at the last used one.
         TRIEDEBUG(assert(children_size() > 0));
         if ( children[children_size()-1] != node_pool.NoIndex ) {
            TRIEDEBUG(assert(list_alloc > children_size()));
            TRIEDEBUG(assert(list[children_size()].has_children));
            // children list size growth needed
            Uint prev_size = children_size();
            children_size(Uint((children_size()+1) * GrowthFactor));
            Uint actual_new_children_size;
            Uint* new_children = npa_pool.alloc_array_no_ctor(children_size(),
               actual_new_children_size);
            children_size(actual_new_children_size);
            for ( Uint i = 0; i < position; ++i )
               new_children[i] = children[i];
            new_children[position] = node_pool.NoIndex;
            for ( Uint i = position; i < prev_size; ++i )
               new_children[i+1] = children[i];
            for ( Uint i = prev_size+1; i < children_size(); ++i )
               new_children[i] = node_pool.NoIndex;

            TRIEDEBUG(assert(children));
            npa_pool.free_array_no_dtor(children, prev_size);
            children = new_children;
         } else {
            TRIEDEBUG(assert(list_alloc <= children_size() ||
                  ! list[children_size()].has_children));
            for ( Uint i = children_size() - 1; i > position; --i )
               children[i] = children[i-1];
            children[position] = node_pool.NoIndex;
         }
      }

      TRIEDEBUG(assert(!list[position].has_children));

      /*
      for ( Uint i = 0; i < list_alloc; ++i ) {
         if ( i < children_size() )
            assert ( list[i].has_children == (children[i] != node_pool.NoIndex) );
         else
            assert ( !list[i].has_children );
      }
      */
   }

   return &(list[position]);
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
bool TrieNode<LeafDataT, InternalDataT, NeedDtor>::find(
   TrieKeyT key_elem,
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *& datum,
   Uint& position
) const {
   //cerr << "f" << key_elem << "=";
   //Uint bucket = hash(key_elem);
   //Uint position = find_position(bucket, key_elem);
   //vector<TrieDatum<LeafDataT, InternalDataT, NeedDtor> > &list = lists[bucket];
   position = find_position(key_elem);
   if ( position < list_alloc && list[position].key_elem == key_elem ) {
      datum = &(list[position]);
      //cerr << "1";
      return true;
   } else {
      //cerr << "0";
      return false;
   }
}

/* too slow for something called this often - need to inline
template<class LeafDataT, class InternalDataT, bool NeedDtor>
TrieNode<LeafDataT, InternalDataT, NeedDtor> *
   TrieNode<LeafDataT, InternalDataT, NeedDtor>::get_children(Uint position
) const {
   TRIEDEBUG(assert(list_alloc > position));
   TRIEDEBUG(assert(children_size() > position));
   TRIEDEBUG(assert(list[position].has_children));
   return children[position];
}
*/

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::create_children(
      Uint position,
      Uint pre_allocated_node_index, TrieNodePool& node_pool,
      DatumArrayPool& da_pool, NodePtrArrayPool& npa_pool
) {
   TRIEDEBUG(assert(list_alloc > position));
   TRIEDEBUG(assert(list[position].key_elem != NoKey));
   TRIEDEBUG(assert(pre_allocated_node_index != node_pool.NoIndex));
   TRIEDEBUG(assert(! list[position].has_children));

   if ( children_size() <= position ) {
      Uint prev_size = children_size();
      children_size(Uint((position+1) * GrowthFactor));
      Uint actual_new_children_size;
      Uint* new_children = npa_pool.alloc_array_no_ctor(children_size(),
         actual_new_children_size);
      children_size(actual_new_children_size);

      for ( Uint i = 0; i < prev_size; ++i ) {
         TRIEDEBUG(
         if ( i < list_alloc )
            assert(list[i].has_children == (children[i] != node_pool.NoIndex))
         );
         new_children[i] = children[i];
      }

      // The unused children are initialized to NoIndex, so we know they are
      // not used.
      for ( Uint i = prev_size; i < children_size(); ++i ) {
         TRIEDEBUG(if ( i < list_alloc ) assert(!list[i].has_children));
         new_children[i] = node_pool.NoIndex;
      }

      if ( children ) npa_pool.free_array_no_dtor(children, prev_size);
      children = new_children;
   }

   TRIEDEBUG(assert(children_size() > position));
   list[position].has_children = true;
   children[position] = pre_allocated_node_index;
   TRIEDEBUG(assert(node_pool.get_ptr(children[position])->list == NULL));
   TRIEDEBUG(assert(node_pool.get_ptr(children[position])->children == NULL));

   /*
   for ( Uint i = 0; i < list_alloc; ++i ) {
      if ( i < children_size() )
         assert ( list[i].has_children == (children[i] != node_pool.NoIndex) );
      else
         assert ( !list[i].has_children );
   }
   */
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::set_leaf_value(
   Uint position, const LeafDataT& val
) {
   TRIEDEBUG(assert(position < list_alloc));
   TRIEDEBUG(assert(list[position].key_elem != NoKey));
   list[position].is_leaf = true;
   list[position].value = val;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
string TrieNode<LeafDataT, InternalDataT, NeedDtor>::dump_info() const {
   ostringstream ss;
   ss << "TrieNode*=" << this << " list alloc: " << list_alloc
      << " children size: " << children_size() << endl;
   return ss.str();
}

/*
template<class LeafDataT, class InternalDataT, bool NeedDtor>
string TrieNode<LeafDataT, InternalDataT, NeedDtor>::dump_info() const {
   ostringstream ss;
   ss << "TrieNode*=" << this << " hash_bits=" << hash_bits;
   ss << " bucket sizes: ";
   for (v_v_iter it = lists.begin(); it != lists.end(); ++it)
      ss << it->size();
   ss << endl;
   return ss.str();
}
*/

template<class LeafDataT, class InternalDataT, bool NeedDtor>
TrieNode<LeafDataT, InternalDataT, NeedDtor>::TrieNode()
   : list(NULL), children(NULL)
   //, list_size(0)
   , list_alloc(0)
   //, children_size(0)
   , intl_data_children_size(InternalDataT(), 0)
{}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::clear(
   TrieNodePool& node_pool, DatumArrayPool& da_pool, NodePtrArrayPool& npa_pool
) {
   for (Uint i = 0; i < list_alloc; ++i)
      if ( list[i].has_children ) {
         node_pool.get_ptr(children[i])->clear(node_pool, da_pool, npa_pool);
         if ( NeedDtor ) node_pool.get_ptr(children[i])->~TrieNode();
      }
   if ( list )
      da_pool.free_array_no_dtor(list, list_alloc);
   if ( children )
      npa_pool.free_array_no_dtor(children, children_size());
   non_recursive_clear(da_pool, npa_pool);
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::non_recursive_clear(
   DatumArrayPool& da_pool, NodePtrArrayPool& npa_pool
) {
   list = NULL;
   children = NULL;
   //list_size = 0;
   list_alloc = 0;
   children_size(0);
   internal_data(InternalDataT());
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::addStats(
   Uint &intl_nodes, Uint &trieDataUsed,
   Uint &trieDataAlloc, Uint &childrenAlloc,
   const TrieNodePool& node_pool
) const {
   ++intl_nodes;
   trieDataUsed += list_alloc;
   for ( Uint i = list_alloc; i > 0; --i ) {
      if ( list[i-1].key_elem != NoKey )
         break;
      else
         trieDataUsed--;
   }
   trieDataAlloc += list_alloc;
   childrenAlloc += children_size();
   for (Uint i = 0; i < list_alloc; ++i) {
      if ( list[i].has_children ) {
         assert(i < children_size());
         node_pool.get_ptr(children[i])->addStats(intl_nodes, trieDataUsed,
                                     trieDataAlloc, childrenAlloc, node_pool);
      }
   }
}

template<class LeafDataT, class InternalDataT, bool NeedDtor> template<class Visitor>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::traverse(
   Visitor& visitor, vector<Uint> &key_stack, const TrieNodePool& node_pool
) const {
   for ( Uint i = 0; i < list_alloc; ++i ) {
      if (list[i].getKey() == NoKey) break;
      key_stack.push_back(list[i].getKey());
      if ( list[i].isLeaf() )
         visitor(key_stack, list[i].getValue());
      if ( list[i].hasChildren() )
         node_pool.get_ptr(children[i])->traverse(visitor, key_stack, node_pool);
      key_stack.pop_back();
   }
}




#endif // __TRIE_NODE_CC_H__
