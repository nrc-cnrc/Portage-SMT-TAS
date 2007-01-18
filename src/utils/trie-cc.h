/**
 * @author Eric Joanis
 * @file trie-cc.h  Implementation of template methods for trie.h.
 * $Id$
 *
 * COMMENTS:
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __TRIE_CC_H__
#define __TRIE_CC_H__


template<class LeafDataT, class InternalDataT, bool NeedDtor>
Uint PTrie<LeafDataT, InternalDataT, NeedDtor>::hash(TrieKeyT key_elem) const {
   if ( root_hash_bits > 0 ) {
      TrieKeyT mask = ~TrieKeyT(0) >> (8*sizeof(TrieKeyT)-root_hash_bits);
      //cerr << "mask=" << mask << endl;
      return key_elem & mask;
   } else {
      return 0;
   }
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
const Uint PTrie<LeafDataT, InternalDataT, NeedDtor>::MaxKey = 
   TrieNode<LeafDataT, InternalDataT, NeedDtor>::NoKey - 1;

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void PTrie<LeafDataT, InternalDataT, NeedDtor>::insert(
   const TrieKeyT key[], Uint key_size, const LeafDataT& val, LeafDataT** p_val
) {
   TRIEDEBUG(cerr << "insert ");
   assert(key_size > 0);
   TrieNode<LeafDataT, InternalDataT, NeedDtor> *node;
   Uint start_i;
   if ( key_size > 1 && insert_cache.size() > 0 && insert_cache[0].first == key[0] ) {
      node = insert_cache[0].second;
      start_i = 1;
      TRIEDEBUG(cerr << "Got root node from cache" << endl);
   } else {
      insert_cache.resize(0);
      Uint bucket = hash(key[0]);
      TRIEDEBUG(cerr << "Root bucket " << bucket << endl);
      node = &(roots[bucket]);
      start_i = 0;
   }
   // This could be done recursively, but a loop is faster
   for (Uint i = start_i; i < key_size - 1; ++i) {
      TRIEDEBUG(cerr << node->dump_info());
      if ( insert_cache.size() > i ) {
         if ( insert_cache[i].first == key[i] ) {
            node = insert_cache[i].second;
            TRIEDEBUG(cerr << "Got node from cache" << endl);
            continue;
         } else {
            insert_cache.resize(i);
         }
      }

      Uint new_pos;
      TrieDatum<LeafDataT, InternalDataT, NeedDtor> *datum =
         node->insert(key[i], new_pos, nodePool, datumArrayPool, nodePtrArrayPool);
      if ( ! datum->hasChildren() ) {
         Uint pre_alloc_node_index;
         TrieNode<LeafDataT, InternalDataT, NeedDtor>* pre_alloc_node =
            nodePool.alloc(pre_alloc_node_index);
         TRIEDEBUG(assert(nodePool.get_ptr(pre_alloc_node_index) == pre_alloc_node));
         node->create_children(new_pos, pre_alloc_node_index,
            nodePool, datumArrayPool, nodePtrArrayPool);
         TRIEDEBUG(cerr << node->dump_info());
         node = pre_alloc_node;
         TRIEDEBUG(assert(nodePool.get_ptr(pre_alloc_node_index) == node));
         TRIEDEBUG(cerr << "new child index=" << pre_alloc_node_index
                    << " ptr=" << node << endl);
      } else {
         TRIEDEBUG(cerr << "existing child index=" << node->get_children(new_pos));
         node = nodePool.get_ptr(node->get_children(new_pos));
         TRIEDEBUG(cerr << " ptr=" << node << endl);
      }
      TRIEDEBUG(assert(insert_cache.size() == i));
      insert_cache.push_back(
         make_pair<Uint, TrieNode<LeafDataT, InternalDataT, NeedDtor> *>(key[i], node));
   }
   TRIEDEBUG(cerr << node->dump_info());
   Uint leaf_pos;
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *leaf_datum =
      node->insert(key[key_size-1], leaf_pos, nodePool, datumArrayPool, nodePtrArrayPool);
   //TRIEDEBUG(cerr << leaf_datum->dump());
   node->set_leaf_value(leaf_pos, val);
   TRIEDEBUG(cerr << node->dump_info());
   if ( p_val ) *p_val = leaf_datum->getModifiableValue();
   //TRIEDEBUG(cerr << leaf_datum->dump());
   TRIEDEBUG(cerr << "done insert" << endl);
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
bool PTrie<LeafDataT, InternalDataT, NeedDtor>::find_or_insert(
   const TrieKeyT key[], Uint key_size, LeafDataT*& p_val
) {
   assert(key_size > 0);
   TrieNode<LeafDataT, InternalDataT, NeedDtor> *node = &(roots[hash(key[0])]);
   // This could be done recursively, but a loop is faster
   for (Uint i = 0; i < key_size - 1; ++i) {
      Uint new_pos;
      TrieDatum<LeafDataT, InternalDataT, NeedDtor> *datum =
         node->insert(key[i], new_pos, nodePool, datumArrayPool,
            nodePtrArrayPool);
      if ( ! datum->hasChildren() ) {
         Uint pre_alloc_node_index;
         TrieNode<LeafDataT, InternalDataT, NeedDtor>* pre_alloc_node =
            nodePool.alloc(pre_alloc_node_index);
         node->create_children(new_pos, pre_alloc_node_index,
            nodePool, datumArrayPool, nodePtrArrayPool);
         node = pre_alloc_node;
      } else {
         node = nodePool.get_ptr(node->get_children(new_pos));
      }
   }

   Uint leaf_pos;
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *leaf_datum =
      node->insert(key[key_size-1], leaf_pos, nodePool, datumArrayPool,
         nodePtrArrayPool);
   bool found;
   if ( leaf_datum->isLeaf() ) {
      found = true;
   } else {
      found = false;
      node->set_leaf_value(leaf_pos, LeafDataT());
   }
   p_val = leaf_datum->getModifiableValue();
   return found;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
bool PTrie<LeafDataT, InternalDataT, NeedDtor>::find(
   const TrieKeyT key[], Uint key_size, LeafDataT& val, Uint* depth
) const {
   LeafDataT* p_val;
   bool rc = const_cast<PTrie*>(this)->find(key, key_size, p_val, depth);
   if ( p_val ) val = *p_val;
   return rc;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
bool PTrie<LeafDataT, InternalDataT, NeedDtor>::find(
   const TrieKeyT key[], Uint key_size, LeafDataT*& p_val, Uint* depth
) {
   //cerr << "find ";
   p_val = NULL;
   if (key_size == 0) return false;
   Uint bucket = hash(key[0]);
   const TrieNode<LeafDataT, InternalDataT, NeedDtor> *node = &(roots[bucket]);
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *datum;
   if ( depth ) *depth = 0;
   for (Uint i = 0; i < key_size - 1; ++i) {
      Uint find_pos;
      if ( node->find(key[i], datum, find_pos) ) {
         if ( depth && datum->isLeaf() ) {
            p_val = datum->getModifiableValue();
            *depth = i+1;
         }
         if ( datum->hasChildren() ) {
            node = nodePool.get_ptr(node->get_children(find_pos));
         } else {
            //cerr << " not found A" << endl;
            return false;
         }
      } else {
         //cerr << " not found B" << endl;
         return false;
      }
   }
   Uint leaf_pos;
   if ( node->find(key[key_size-1], datum, leaf_pos) && datum->isLeaf() ) {
      p_val = datum->getModifiableValue();
      if (depth) *depth = key_size;
      //cerr << " found" << endl;
      return true;
   } else {
      //cerr << " not found C" << endl;
      return false;
   }
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
bool PTrie<LeafDataT, InternalDataT, NeedDtor>::sum(
   const TrieKeyT key[], Uint min_depth, Uint max_depth, LeafDataT& val
) const {
   if (max_depth == 0) return false;
   Uint bucket = hash(key[0]);
   const TrieNode<LeafDataT, InternalDataT, NeedDtor> *node = &(roots[bucket]);
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *datum;
   bool found = false;
   for (Uint depth = 1; depth <= max_depth; ++depth) {
      Uint find_pos;
      if ( node->find(key[depth-1], datum, find_pos) ) {
         if ( depth >= min_depth && datum->isLeaf() ) {
            val = val + datum->getValue();
            found = true;
         }
         if ( datum->hasChildren() ) {
            node = nodePool.get_ptr(node->get_children(find_pos));
         } else {
            return found;
         }
      } else {
         return found;
      }
   }
   return found;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void PTrie<LeafDataT, InternalDataT, NeedDtor>::set_internal_node_value(
   const TrieKeyT key[], Uint key_size, const InternalDataT& intl_val
) {
   assert(key_size > 0);
   TrieNode<LeafDataT, InternalDataT, NeedDtor> *node;
   Uint start_i;
   if ( insert_cache.size() > 0 && insert_cache[0].first == key[0] ) {
      node = insert_cache[0].second;
      start_i = 1;
   } else {
      insert_cache.resize(0);
      Uint bucket = hash(key[0]);
      node = &(roots[bucket]);
      start_i = 0;
   }

   for (Uint i = start_i; i < key_size; ++i) {
      if ( insert_cache.size() > i ) {
         if ( insert_cache[i].first == key[i] ) {
            node = insert_cache[i].second;
            continue;
         } else {
            insert_cache.resize(i);
         }
      }

      Uint new_pos;
      TrieDatum<LeafDataT, InternalDataT, NeedDtor> *datum =
         node->insert(key[i], new_pos, nodePool, datumArrayPool, nodePtrArrayPool);
      if ( ! datum->hasChildren() ) {
         Uint pre_alloc_node_index;
         TrieNode<LeafDataT, InternalDataT, NeedDtor>* pre_alloc_node =
            nodePool.alloc(pre_alloc_node_index);
         node->create_children(new_pos, pre_alloc_node_index,
            nodePool, datumArrayPool, nodePtrArrayPool);
         node = pre_alloc_node;
         TRIEDEBUG(assert(nodePool.get_ptr(pre_alloc_node_index) == node));
      } else {
         node = nodePool.get_ptr(node->get_children(new_pos));
      }
      assert(insert_cache.size() == i);
      insert_cache.push_back(
         make_pair<Uint, TrieNode<LeafDataT, InternalDataT, NeedDtor> *>(key[i], node));
   }

   node->internal_data(intl_val);
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
InternalDataT PTrie<LeafDataT, InternalDataT, NeedDtor>::get_internal_node_value(
   const TrieKeyT key[], Uint key_size
) const {
   if (key_size == 0) return InternalDataT();
   Uint bucket = hash(key[0]);
   const TrieNode<LeafDataT, InternalDataT, NeedDtor> *node = &(roots[bucket]);
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *datum;
   for (Uint i = 0; i < key_size; ++i) {
      Uint find_pos;
      if ( node->find(key[i], datum, find_pos) ) {
         if ( datum->hasChildren() ) {
            node = nodePool.get_ptr(node->get_children(find_pos));
         } else {
            return InternalDataT();
         }
      } else {
         return InternalDataT();
      }
   }

   return node->internal_data();
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
InternalDataT PTrie<LeafDataT, InternalDataT, NeedDtor> ::sum_internal_node_values(
      const TrieKeyT key[], Uint min_len, Uint max_len
) const {
   if (max_len == 0) return InternalDataT();
   Uint bucket = hash(key[0]);
   const TrieNode<LeafDataT, InternalDataT, NeedDtor> *node = &(roots[bucket]);
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *datum;
   InternalDataT result;
   for (Uint depth = 1; depth <= max_len; ++depth) {
      Uint find_pos;
      if ( node->find(key[depth-1], datum, find_pos) ) {
         if ( datum->hasChildren() ) {
            node = nodePool.get_ptr(node->get_children(find_pos));
         } else {
            return result;
         }
         if ( depth >= min_len ) result = result + node->internal_data();
      } else {
         return result;
      }
   }

   return result;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
Uint PTrie<LeafDataT, InternalDataT, NeedDtor>::find_path(
      const TrieKeyT key[], Uint key_size, LeafDataT* values[]
) {
   //cerr << "find ";
   if (key_size == 0) return 0;
   Uint bucket = hash(key[0]);
   const TrieNode<LeafDataT, InternalDataT, NeedDtor> *node = &(roots[bucket]);
   TrieDatum<LeafDataT, InternalDataT, NeedDtor> *datum;
   Uint depth = 0;
   for (Uint i = 0; i < key_size; ++i) {
      values[i] = NULL;
      Uint find_pos;
      if ( node->find(key[i], datum, find_pos) ) {
         depth = i+1;
         if ( datum->isLeaf() ) {
            values[i] = datum->getModifiableValue();
         }
         if ( datum->hasChildren() && i < key_size - 1 ) {
            node = nodePool.get_ptr(node->get_children(find_pos));
         } else {
            for ( ++i; i < key_size; ++i ) values[i] = NULL;
            break;
         }
      } else {
         for ( ++i; i < key_size; ++i ) values[i] = NULL;
         break;
      }
   }
   return depth;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
PTrie<LeafDataT, InternalDataT, NeedDtor>::PTrie(Uint root_hash_bits)
   : roots(1<<root_hash_bits)
   , root_hash_bits(root_hash_bits)
   , end_iter(*this, NULL, false, true, 0, 0)
{ }

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void PTrie<LeafDataT, InternalDataT, NeedDtor>::clear() {
   for ( root_iter it = roots.begin(); it != roots.end(); ++it )
      if ( NeedDtor ) {
         it->clear(nodePool, datumArrayPool, nodePtrArrayPool);
      } else {
         it->non_recursive_clear(datumArrayPool, nodePtrArrayPool);
      }
   datumArrayPool.clear();
   nodePtrArrayPool.clear();
   nodePool.clear();
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
void PTrie<LeafDataT, InternalDataT, NeedDtor>::getStats(
   Uint &intl_nodes, Uint &trieDataUsed, Uint &trieDataAlloc,
   Uint &childrenAlloc, Uint &memoryUsed, Uint &memoryAlloc
) const {
   intl_nodes = trieDataUsed = trieDataAlloc = childrenAlloc = 0;
   for ( root_c_iter it = roots.begin(); it != roots.end(); ++it )
      it->addStats(intl_nodes, trieDataUsed, trieDataAlloc, childrenAlloc,
                   nodePool);

   Uint64 mem_used =
      static_cast<Uint64>(
         sizeof(PTrie<LeafDataT, InternalDataT, NeedDtor>))
      + intl_nodes * sizeof(TrieNode<LeafDataT, InternalDataT, NeedDtor>)
      + (intl_nodes - roots.size()) * sizeof(Uint)
      + trieDataUsed * sizeof(TrieDatum<LeafDataT, InternalDataT, NeedDtor>);
   memoryUsed = Uint(mem_used / 1024 / 1024);

   Uint64 mem_alloc =
      static_cast<Uint64>(
         sizeof(PTrie<LeafDataT, InternalDataT, NeedDtor>))
      + intl_nodes * sizeof(TrieNode<LeafDataT, InternalDataT, NeedDtor>)
      + childrenAlloc * sizeof(Uint)
      + trieDataAlloc * sizeof(TrieDatum<LeafDataT, InternalDataT, NeedDtor>);
   memoryAlloc = Uint(mem_alloc / 1024 / 1024);
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
string PTrie<LeafDataT, InternalDataT, NeedDtor>::getStats() const {
   Uint intl_nodes = 0;
   Uint trieDataUsed = 0;
   Uint trieDataAlloc = 0;
   Uint childrenAlloc = 0;
   Uint memoryUsed = 0;
   Uint memoryAlloc = 0;
   getStats(intl_nodes, trieDataUsed, trieDataAlloc, childrenAlloc,
            memoryUsed, memoryAlloc);
   ostringstream ss;
   ss.precision(4);
   ss << "Nodes: " << intl_nodes 
      // << " Vectors: " << vectors
      // << "(" << (1.0 * vectors / intl_nodes) << ":1)"
      << " Data used: " << trieDataUsed
      << " / " << trieDataAlloc
      << " (" << (100.0 * trieDataUsed / trieDataAlloc) << "%)"
      << " Children used: " << (intl_nodes - roots.size())
      << " / " << childrenAlloc
      << " (" << (100.0 * (intl_nodes - roots.size()) / childrenAlloc) << "%)"
      << " Memory: " << memoryUsed << "MB / " << memoryAlloc << "MB"
      << " (" << (100.0 * memoryUsed / memoryAlloc) << "%)"
      ;
   return ss.str();
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
string PTrie<LeafDataT, InternalDataT, NeedDtor>::getSizeOfs() {
   ostringstream ss;
   string dataT = typeName<InternalDataT>() + "," + typeName<LeafDataT>();
   ss << "sizeof(PTrie<" << dataT << " >) = "
         << sizeof(PTrie<LeafDataT, InternalDataT, NeedDtor>) << endl
      << "sizeof(TrieNode<" << dataT << " >) = "
         << sizeof(TrieNode<LeafDataT, InternalDataT, NeedDtor>) << endl
      << "sizeof(TrieNode<" << dataT << " >[10]) = "
         << sizeof(TrieNode<LeafDataT, InternalDataT, NeedDtor>[10]) << endl
      << "sizeof(TrieDatum<" << dataT << " >) = "
         << sizeof(TrieDatum<LeafDataT, InternalDataT, NeedDtor>) << endl
      << "sizeof(TrieDatum<" << dataT << " >[10]) = "
         << sizeof(TrieDatum<LeafDataT, InternalDataT, NeedDtor>[10]) << endl
      << "sizeof(Wrap<float>) = " << sizeof(Wrap<float>) << endl
      << "sizeof(OptionalClassWrapper<Empty,float>) = "
         << sizeof(OptionalClassWrapper<Empty,float>) << endl
      << "sizeof(OptionalClassWrapper<Wrap<float>,float>) = "
         << sizeof(OptionalClassWrapper<Wrap<float>,float>) << endl
      ;
   return ss.str();
} // PTrie::getSizeOfs

template<class LeafDataT, class InternalDataT, bool NeedDtor>
Uint PTrie<LeafDataT, InternalDataT, NeedDtor>::write_binary(
   ofstream& ofs
) const {
   assert(!NeedDtor);
   assert(sizeof(Uint64) == 8);
   ofs.write((char*)&root_hash_bits, sizeof(root_hash_bits));
   Uint nodes_written(0);
   for ( Uint bucket = 0; bucket < roots.size(); ++bucket )
      nodes_written += roots[bucket].write_binary(ofs, nodePool);
   return nodes_written;
} // PTrie::write_binary

template<class LeafDataT, class InternalDataT, bool NeedDtor>
   template<class Filter>
Uint PTrie<LeafDataT, InternalDataT, NeedDtor>
   ::read_binary(istream& is, Filter& filter)
{
   clear();
   assert(!NeedDtor);
   assert(sizeof(Uint64) == 8);
   is.read((char*)&root_hash_bits, sizeof(root_hash_bits));
   roots.resize(1<<root_hash_bits);
   vector<Uint> key_stack;
   Uint nodes_kept(0);
   for ( Uint bucket = 0; bucket < roots.size(); ++bucket ) {
      nodes_kept +=
         roots[bucket].read_binary(is, filter, key_stack, nodePool,
                                   datumArrayPool, nodePtrArrayPool);
      if ( (bucket+1) % (roots.size()/8) == 0 ) cerr << ".";
   }
   return nodes_kept;
}

template<class LeafDataT, class InternalDataT, bool NeedDtor>
typename PTrie<LeafDataT, InternalDataT, NeedDtor>::iterator 
   PTrie<LeafDataT, InternalDataT, NeedDtor>::begin_children() const
{
   // Since we need the full logic of ++ to get to the first valid node, start
   // before the first possible node and let ++ do the rest of the work.
   iterator it(*this, &(roots[0]), true, false, 0, -1);
   ++it;
   return it;
} // PTrie::begin_children()

template<class LeafDataT, class InternalDataT, bool NeedDtor>
typename PTrie<LeafDataT, InternalDataT, NeedDtor>::iterator
   PTrie<LeafDataT, InternalDataT, NeedDtor>::iterator::begin_children() const
{
   if ( has_children() ) {
      iterator it(parent, 
         parent.nodePool.get_ptr(node->get_children(position)),
         false, false, 0, -1);
      ++it;
      return it;
   } else {
      return parent.end_iter;
   }
} // PTrie::iterator::begin_children()
   
template<class LeafDataT, class InternalDataT, bool NeedDtor>
bool PTrie<LeafDataT, InternalDataT, NeedDtor>::iterator::operator== (
   const iterator& other
) const {
   if ( is_end || other.is_end ) return (is_end == other.is_end);
   return is_root == other.is_root 
       && root_bucket == other.root_bucket
       && position == other.position
       && node == other.node
       && &parent == &(other.parent);
} // PTrie::iterator::operator==

template<class LeafDataT, class InternalDataT, bool NeedDtor>
typename PTrie<LeafDataT, InternalDataT, NeedDtor>::iterator &
   PTrie<LeafDataT, InternalDataT, NeedDtor>::iterator::operator++()
{
   if ( is_end ) return *this;
   if ( ! is_root ) {
      ++position;
      if ( position >= node->size() || get_key() == node->NoKey )
         is_end = true;
   } else {
      ++position;
      while ( root_bucket < parent.roots.size() ) {
         if ( position >= node->size() || get_key() == node->NoKey ) {
            ++root_bucket;
            node = &(parent.roots[root_bucket]);
            position = 0;
         } else {
            break;
         }
      }
      if ( root_bucket == parent.roots.size() ) is_end = true;
   }
   return *this;
} // PTrie::iterator::operator++



template<class LeafDataT, class InternalDataT, bool NeedDtor>
   template<class Visitor>
void PTrie<LeafDataT, InternalDataT, NeedDtor>
   ::traverse(Visitor& visitor) const
{
   vector<Uint> key_stack;
   vector<TrieNode<LeafDataT, InternalDataT, NeedDtor> *> node_stack;
   for ( Uint bucket = 0; bucket < roots.size(); ++bucket )
      roots[bucket].traverse(visitor, key_stack, nodePool);
}


#endif // __TRIE_CC_H__


