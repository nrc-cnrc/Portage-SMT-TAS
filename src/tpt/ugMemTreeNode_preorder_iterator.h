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
#ifndef __MemTreeNode_preorder_iterator
#define __MemTreeNode_preorder_iterator
#include "ugMemTreeNode_tree_iterator.h"
using namespace std;

template<typename val_t, typename key_t>
class
MemTreeNode<val_t,key_t>::
preorder_iterator : public MemTreeNode<val_t,key_t>::tree_iterator
{
public:
  typedef MemTreeNode<val_t,key_t> node_t;
  // typedef  MemTreeNode<val_t,key_t>::id_type id_type;
  // typedef map<id_type,node_t> map_t;
  typedef typename map_t::iterator myIterType;

  preorder_iterator(MemTreeNode<val_t,key_t>* _root,
                    myIterType & _iter); 
  preorder_iterator(tree_iterator const& other) : tree_iterator(other) {};
  preorder_iterator& operator++();
  preorder_iterator  operator++(int);

};

// CONSTRUCTOR
template<typename val_t, typename key_t>
MemTreeNode<val_t,key_t>::
preorder_iterator:: 
preorder_iterator(MemTreeNode<val_t,key_t>* _root,
                  myIterType& _iter) 
  : tree_iterator(_root)
{
  this->mPath.push_back(_iter);
};


// OPERATOR ++ (prefix)
template<typename val_t, typename key_t>
typename MemTreeNode<val_t,key_t>::preorder_iterator&
MemTreeNode<val_t,key_t>::preorder_iterator::
operator++()
{
  assert(this->mPath.size() > 0);
  assert(this->mPath.back() != this->mRoot->mDtrs.end());
  if (!this->down())
    {
      while (this->mPath.size() && !this->next())
        this->mPath.pop_back();
    }
  return *this;
}

// OPERATOR ++ (postfix)
template<typename val_t, typename key_t>
typename MemTreeNode<val_t,key_t>::preorder_iterator
MemTreeNode<val_t,key_t>::preorder_iterator::
operator++(int)
{
  typename MemTreeNode<val_t,key_t>::preorder_iterator ret = *this;
  ++(*this);
  return ret;
}

#endif

