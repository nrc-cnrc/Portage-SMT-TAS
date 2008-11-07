/**
 * @author Eric Joanis
 * @file trie_node-cc.h  Implementation of template methods for trie_node.h.
 * $Id$
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
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
         // No more space, need to grow
         Uint old_size = list_alloc;
         list_alloc = Uint((list_alloc + 1) * GrowthFactor);
         TrieDatum<LeafDataT, InternalDataT, NeedDtor> *new_list =
            da_pool.alloc_array_no_ctor(list_alloc, &list_alloc);
         // This for loop won't work for LeafDataT with mandatory ctor/dtor - 
         // we need to memcpy the elements we keep and call the ctor on the new
         // ones.
         //for ( Uint i = 0; i < position; ++i ) new_list[i] = list[i];
         memcpy(new_list, list, position * sizeof(*new_list));

         // Need to directly call the contructor, in case it's mandatory
         //new_list[position] = TrieDatum<LeafDataT, InternalDataT, NeedDtor>(key_elem);
         new(new_list+position) TrieDatum<LeafDataT, InternalDataT, NeedDtor>(key_elem);

         TRIEDEBUG(assert(!new_list[position].has_children));

         // And again use memcpy for the tail-end of the list.
         //for ( Uint i = position; i < old_size; ++i ) new_list[i+1] = list[i];
         memcpy(new_list+position+1, list+position,
                (old_size-position) * sizeof(*new_list));

         // Newly allocated but still unused element must be set to NoKey to
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
               &actual_new_children_size);
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
         &actual_new_children_size);
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
   for (Uint i = 0; i < list_alloc; ++i) {
      if ( list[i].has_children ) {
         node_pool.get_ptr(children[i])->clear(node_pool, da_pool, npa_pool);
         if ( NeedDtor ) node_pool.get_ptr(children[i])->~TrieNode();
      }
   }
   if ( list ) {
      if ( NeedDtor )
         da_pool.free_array(list, list_alloc);
      else
         da_pool.free_array_no_dtor(list, list_alloc);
   }
   if ( children )
      npa_pool.free_array_no_dtor(children, children_size());
   non_recursive_clear();
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::non_recursive_clear() {
   list = NULL;
   children = NULL;
   //list_size = 0;
   list_alloc = 0;
   children_size(0);
   internal_data(InternalDataT());
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::addStats(
   Uint &intl_nodes, SimpleHistogram<Uint>& trieDataUsed,
   SimpleHistogram<Uint>& trieDataAlloc, SimpleHistogram<Uint>& childrenAlloc,
   const TrieNodePool& node_pool
) const {
   ++intl_nodes;
   Uint cptTrieDataUsed = list_alloc;
   for ( Uint i = list_alloc; i > 0; --i ) {
      if ( list[i-1].key_elem != NoKey )
         break;
      else
         cptTrieDataUsed--;
   }
   trieDataUsed.add(cptTrieDataUsed);
   trieDataAlloc.add(list_alloc);
   childrenAlloc.add(children_size());
   for (Uint i = 0; i < list_alloc; ++i) {
      if ( list[i].has_children ) {
         assert(i < children_size());
         node_pool.get_ptr(children[i])->addStats(intl_nodes, trieDataUsed,
                                     trieDataAlloc, childrenAlloc, node_pool);
      }
   }
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
   template<class Visitor>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::traverse(
   Visitor& visitor, vector<Uint> &key_stack, const TrieNodePool& node_pool
) const {
   for ( Uint i = 0; i < list_alloc; ++i ) {
      if (list[i].key_elem == NoKey) break;
      key_stack.push_back(list[i].key_elem);
      if ( list[i].is_leaf )
         visitor(key_stack, *list[i].getModifiableValue());
      if ( list[i].has_children )
         node_pool.get_ptr(children[i])->traverse(visitor, key_stack, node_pool);
      key_stack.pop_back();
   }
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
Uint TrieNode<LeafDataT, InternalDataT, NeedDtor>::write_binary(
   ofstream& ofs, const TrieNodePool& node_pool
) const {
   // Put a dummy skip offset in place
   iostream::pos_type start_pos(ofs.tellp());
   Int64 skip_offset(0);
   ofs.write((char*)&skip_offset, sizeof(skip_offset));

   // Leaf data: to write it out as compactly as possible, find the last used
   // leaf index and only write that many.
   Uint true_list_size(list_alloc);
   while ( true_list_size > 0 &&
           list[true_list_size-1].key_elem == NoKey )
      --true_list_size;
   ofs.write((char*)&true_list_size, sizeof(true_list_size));
   ofs.write((char*)list, sizeof(list[0]) * true_list_size);

   // Internal data and size of children array: again, make a tight estimate of
   // true length of children pointer array.
   Uint true_children_size(min(children_size(), true_list_size));
   while ( true_children_size > 0 &&
           ! list[true_children_size-1].has_children )
      --true_children_size;
   OptionalClassWrapper<InternalDataT, Uint> intl_data_true_children_size(
      intl_data_children_size);
   intl_data_true_children_size.second = true_children_size;
   ofs.write((char*)&intl_data_true_children_size,
             sizeof(intl_data_true_children_size));

   // Recursively write the children out
   Uint nodes_written(1);
   Uint actual_children_count(0);
   for ( Uint i(0); i < true_children_size; ++i ) {
      if ( list[i].has_children ) {
         //cerr << "Child " << i << " key " << list[i].key_elem;
         //cerr << " " << dump_info();
         ofs.write((char*)&i, sizeof(i));
         nodes_written +=
            node_pool.get_ptr(children[i])->write_binary(ofs, node_pool);
         ++actual_children_count;
      }
   }
   // Sanity check: count the number of children actually written out
   ofs.write((char*)&actual_children_count, sizeof(actual_children_count));

   // Seek back and write the correct skip offset now that we know it
   iostream::pos_type end_pos(ofs.tellp());
   skip_offset = (end_pos - start_pos) -
                 static_cast<iostream::pos_type>(sizeof(skip_offset));
   ofs.seekp(start_pos);
   ofs.write((char*)&skip_offset, sizeof(skip_offset));
   ofs.seekp(end_pos);

   //cerr << "start_pos " << start_pos << " end_pos " << end_pos;
   //cerr << " " << dump_info();

   return nodes_written;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
   template<typename Filter, typename Mapper>
Uint TrieNode<LeafDataT, InternalDataT, NeedDtor>::read_binary(
   istream& is, Filter& filter, Mapper& mapper, vector<Uint>& key_stack,
   TrieNodePool& node_pool, DatumArrayPool& da_pool, NodePtrArrayPool& npa_pool
) {
   assert(!NeedDtor);
   assert(list_alloc == 0);
   assert(children_size() == 0);
   is.ignore(sizeof(Int64)); // Ignore skip offset - this nodes was kept!

   // Read the leaf data
   Uint true_list_size;
   is.read((char*)&true_list_size, sizeof(true_list_size));
   if ( true_list_size > 0 ) {
      list = da_pool.alloc_array_no_ctor(true_list_size, &list_alloc);
      is.read((char*)list, sizeof(list[0]) * true_list_size);
      for ( Uint i(true_list_size); i < list_alloc; ++i )
         list[i].reset(NoKey);
   }

   // Read the internaldata and the size of the children array.
   OptionalClassWrapper<InternalDataT, Uint> intl_data_true_children_size(
      intl_data_children_size);
   is.read((char*)&intl_data_true_children_size,
           sizeof(intl_data_true_children_size));
   internal_data(intl_data_true_children_size.first());
   Uint true_children_size = intl_data_true_children_size.second;
   Uint allocated_children_size;
   if ( true_children_size > 0 ) {
      children = npa_pool.alloc_array_no_ctor(true_children_size,
                                              &allocated_children_size);
      children_size(allocated_children_size); // set new actual children size
   } else {
      children = NULL;
      allocated_children_size = 0;
   }

   vector<Uint> keys_kept;

   // Apply the filter to each datum and recursively read the children
   Uint number_of_children_found(0);
   Uint nodes_kept(1);
   Uint required_list_size(0);
   Uint required_children_size(0);
   assert(true_children_size <= true_list_size);
   for ( Uint i(0); i < true_list_size; ++i ) {
      if ( list[i].has_children ) {
         ++number_of_children_found;
         Uint j;
         is.read((char*)&j, sizeof(j));
         if ( i != j ) error(ETFatal,
            "Corrupt BinLM stream.  Expected child %d, got %d", i, j);
      } else if ( i < allocated_children_size ) {
         children[i] = ~(Uint(0));
      }
      key_stack.push_back(list[i].key_elem);
      //cerr << "Key " << join<Uint>(key_stack);
      if ( filter(key_stack) ) { // Keep this key
         //cerr << " keeping it" << endl;
         keys_kept.push_back(key_stack.back());
         ++required_list_size;
         if ( list[i].has_children ) {
            TrieNode<LeafDataT, InternalDataT, NeedDtor>* pre_alloc_node =
               node_pool.alloc(children[i]);
            Uint rec_nodes_kept =
               pre_alloc_node->read_binary(is, filter, mapper, key_stack,
                                           node_pool, da_pool, npa_pool);
            if ( rec_nodes_kept == 0 ) {
               // All information in that child node got pruned, discard it
               // (but keep the key, since that didn't get pruned by filter()).
               list[i].has_children = false;
               node_pool.release(children[i]);
               children[i] = ~(Uint(0));
            } else {
               required_children_size = required_list_size;
               nodes_kept += rec_nodes_kept;
            }
         }
      } else { // Throw away this key and its sub-trie
         //cerr << " discarding it" << endl;
         if ( list[i].has_children ) {
            Int64 skip_offset;
            is.read((char*)&skip_offset, sizeof(skip_offset));
            is.ignore(skip_offset);
         }
         // Reset this datum, thus marking it for later removal.
         list[i].reset(NoKey);
      }
      key_stack.pop_back();
   }

   // Sanity check: verify we got the number of children we expected.
   Uint expected_children_count;
   is.read((char*)&expected_children_count, sizeof(expected_children_count));
   if ( expected_children_count != number_of_children_found )
      error(ETFatal, "Corrupt BinLM stream.  Expected %d children, got %d",
            expected_children_count, number_of_children_found);

   /*
   cerr << dump_info();
   cerr << "Before removing discarded entries";
   for ( Uint i(0); i < list_alloc; ++i )
      cerr << " " << list[i].key_elem;
   cerr << endl;
   */

   if ( required_list_size != true_list_size ) {
      // The above condition is true iff at least one key was filtered out:
      if ( required_list_size == 0 ) {
         // No children remain
         assert(list_alloc > 0);
         da_pool.free_array_no_dtor(list, list_alloc);
         list = NULL;
         list_alloc = 0;
         if ( children_size() > 0 ) {
            npa_pool.free_array_no_dtor(children, children_size());
            children = NULL;
            children_size(0);
         }
         if ( internal_data() == InternalDataT() ) {
            //cerr << "Useless node" << endl;
            nodes_kept = 0;
         }
      } else {
         // Compress list and children by individually removing elements marked
         // for deletion.

         Uint new_list_alloc;
         TrieDatum<LeafDataT, InternalDataT, NeedDtor> *new_list;
         if ( (required_list_size+1) * GrowthFactor < true_list_size ) {
            new_list =
               da_pool.alloc_array_no_ctor(required_list_size, &new_list_alloc);
         } else {
            new_list = list;
            new_list_alloc = list_alloc;
         }

         Uint new_children_alloc;
         Uint* new_children;
         if ( required_children_size == 0 ) {
            new_children = NULL;
            new_children_alloc = 0;
         } else if ( (required_children_size+1) * GrowthFactor < true_children_size ) {
            new_children = npa_pool.alloc_array_no_ctor(required_children_size,
                                                        &new_children_alloc);
         } else {
            new_children = children;
            new_children_alloc = allocated_children_size;
         }

         Uint new_pos(0);
         for ( Uint pos(0); pos < true_list_size; ++pos ) {
            if ( list[pos].key_elem != NoKey ) {
               assert(new_pos < required_list_size);
               memcpy(new_list+new_pos, list+pos, sizeof(list[pos]));
               //new_list[new_pos] = list[pos];
               if ( list[pos].has_children ) {
                  assert(new_pos < required_children_size);
                  new_children[new_pos] = children[pos];
               }
               ++new_pos;
            }
         }
         for ( ; new_pos < new_list_alloc; ++new_pos )
            new_list[new_pos].reset(NoKey);

         if ( new_list != list ) {
            da_pool.free_array_no_dtor(list, list_alloc);
            list = new_list;
            list_alloc = new_list_alloc;
         }

         if ( new_children != children ) {
            npa_pool.free_array_no_dtor(children, children_size());
            children = new_children;
            children_size(new_children_alloc);
         }

      }
      //cerr << dump_info();
   } else {
      // No keys were filtered out, but maybe some children or subtrees were -
      // compress them if we have many more allocated than we need.
      if ( required_children_size == 0 && children_size() > 0 ) {
         npa_pool.free_array_no_dtor(children, children_size());
         children = NULL;
         children_size(0);
      } else if ( (required_children_size+1) * GrowthFactor < true_children_size ) {
         Uint new_children_alloc;
         Uint* new_children = npa_pool.alloc_array_no_ctor(
               required_children_size, &new_children_alloc);
         assert(required_children_size <= new_children_alloc);
         memcpy(new_children, children,
                sizeof(children[0]) * required_children_size);
         npa_pool.free_array_no_dtor(children, children_size());
         children = new_children;
         children_size(new_children_alloc);
      }
   }

   if ( list_alloc == 0 && children_size() == 0 &&
        internal_data() == InternalDataT() ) {
      //cerr << "Unexpected useless node" << endl;
      nodes_kept = 0;
   }

   if ( required_list_size > 0 &&
        static_cast<void*>(&mapper) != &(PTrieNullMapper::m) )
      apply_mapper(mapper, required_list_size, node_pool, da_pool, npa_pool);


   /*
   // Check that the list has the same keys as the keys_kept vector.
   bool ok(true);
   for ( Uint i(0); i < list_alloc; ++i ) {
      if ( i < keys_kept.size() && list[i].key_elem != keys_kept[i] ||
           i >= keys_kept.size() && list[i].key_elem != NoKey )
         ok = false;
   }
   if ( !ok ) {
      cerr << "Corrupted list should be " << join<Uint>(keys_kept)
           << " but is";
      for ( Uint i(0); i < list_alloc; ++i )
         cerr << " " << list[i].key_elem;
      cerr << endl;
   }
   */

   return nodes_kept;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
   template<typename Mapper>
void TrieNode<LeafDataT, InternalDataT, NeedDtor>::apply_mapper(
   Mapper& mapper, Uint required_list_size,
   TrieNodePool& node_pool, DatumArrayPool& da_pool, NodePtrArrayPool& npa_pool
) {
   // The user specified a real mapper, not the dummy one, so we need to
   // change all the key values using this mapper, and resort the elements
   // in this node.

   // Step 1: remap all the keys - as a result, list may be out of order.
   for ( Uint i(0); i < required_list_size; ++i )
      list[i].key_elem = mapper(list[i].key_elem);

   // Step 2: sort an array of list indices according to the new keys
   Uint positions[required_list_size];
   for ( Uint i(0); i < required_list_size; ++i )
      positions[i] = i;
   sort(&(positions[0]), &(positions[0])+required_list_size, KeyLessThan(this));

   // Step 3: Reorder list according to this new order.
   Uint new_list_alloc;
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *new_list
      = da_pool.alloc_array_no_ctor(required_list_size, &new_list_alloc);
   for ( Uint i(0); i < required_list_size; ++i ) {
      memcpy(new_list+i, list+positions[i], sizeof(list[0]));
      //new_list[i] = list[positions[i]];
   }
   for ( Uint i(required_list_size); i < new_list_alloc; ++i )
      new_list[i].reset(NoKey);
   da_pool.free_array_no_dtor(list, list_alloc);
   list = new_list;
   list_alloc = new_list_alloc;

   // Step 4: Reorder children according to this new order.
   if ( children_size() > 0 ) {
      int last_child_pos(-1);
      for ( int i(required_list_size-1); i >= 0; --i ) {
         if ( new_list[i].has_children ) {
            last_child_pos = i;
            break;
         }
      }
      if ( last_child_pos >= 0 ) {
         Uint new_children_alloc;
         Uint* new_children = npa_pool.alloc_array_no_ctor(
               last_child_pos + 1, &new_children_alloc);
         for ( Uint i(0); i <= Uint(last_child_pos); ++i )
            if ( new_list[i].has_children )
               new_children[i] = children[positions[i]];

         npa_pool.free_array_no_dtor(children, children_size());
         children = new_children;
         children_size(new_children_alloc);
      } else {
         /*
         error(ETWarn, "children_size() was %d, but last_child_pos is %d",
               children_size(), last_child_pos);
         npa_pool.free_array_no_dtor(children, children_size());
         children = NULL;
         children_size(0);
         */
      }
   }
}

#endif // __TRIE_NODE_CC_H__
