// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.
#ifndef __MemTreeNode_fixed_depth_iterator
#define __MemTreeNode_fixed_depth_iterator
#include "ugMemTreeNode_tree_iterator.h"

template<typename val_t, typename key_t>
class
MemTreeNode<val_t,key_t>::
fixed_depth_iterator : public MemTreeNode<val_t,key_t>::tree_iterator
{
public:
  fixed_depth_iterator(MemTreeNode<val_t,key_t>* const _root=NULL, size_t depth=0); 
  fixed_depth_iterator(tree_iterator const& other) : tree_iterator(other) {};
  fixed_depth_iterator& operator++();
  fixed_depth_iterator operator++(int);
  // fixed_depth_iterator& operator=(tree_iterator const& other);
};

template<typename val_t, typename key_t>
MemTreeNode<val_t,key_t>::
fixed_depth_iterator::
fixed_depth_iterator(MemTreeNode<val_t,key_t>* const _root, size_t _depth)
  : tree_iterator(_root)
{
  if (_depth==0) return;
  typename MemTreeNode<val_t,key_t>::preorder_iterator p    = this->mRoot->preorder_begin();
  // cout << p << " z" << endl;
  typename MemTreeNode<val_t,key_t>::tree_iterator stop = this->mRoot->end();
  while (p != stop && p.depth() < _depth) 
    {
      // cout << p << " a" << endl;
      p++;
    }
  // cout << p << " y" << endl;
  *this = p;
  // cout << *this << " x" << endl;
}

template<typename val_t, typename key_t>
typename MemTreeNode<val_t,key_t>::fixed_depth_iterator&
MemTreeNode<val_t,key_t>::
fixed_depth_iterator::
operator++()
{
  // cout << *this << endl;
  assert(this->mPath.size() > 0);
  assert(this->mPath.back() != this->mRoot->mDtrs.end());
  if (!(*this)[-1].isLastChild())
    this->mPath.back()++;
  else
    {
      size_t d = this->depth();
      typename MemTreeNode<val_t,key_t>::tree_iterator stop = this->mRoot->end();
      while (this->depth() > 1 && (*this)[-1].isLastChild())
        {
          this->mPath.pop_back();
          // cout << *this << " az" << endl;
        }
      if (!(*this)[-1].isLastChild() 
          || &(this->mRoot->mDtrs.rbegin()->second) == &(this->mPath.back()->second))//  || this->depth() == 1)
        this->mPath.back()++;
      // cout << this->depth() << " x " << endl;
      while (this->depth() < d && *this != stop)
        {
          // cout << *this << " z" << endl;
          if (!(*this)[-1].isTerminal())
            {
              this->mPath.push_back((*this)[-1].mDtrs.begin());
              // cout << *this << " zx" << endl;
            }
          else
            {
              while (this->depth() > 1 && (*this)[-1].isLastChild())
                {
                  this->mPath.pop_back();
                  // cout << *this << " x" << endl;
                }
              // cout << *this << " xx" << endl;
              this->mPath.back()++;
            }
        }
    }
  // cout << *this << endl;
  return *this;
}

// OPERATOR ++ (postfix)
template<typename val_t, typename key_t>
typename MemTreeNode<val_t,key_t>::fixed_depth_iterator
MemTreeNode<val_t,key_t>::fixed_depth_iterator::
operator++(int)
{
  typename MemTreeNode<val_t,key_t>::fixed_depth_iterator ret = *this;
  ++(*this);
  return ret;
}
#endif
