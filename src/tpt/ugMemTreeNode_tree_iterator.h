/* Base class for various types of tree iterators.
 * (c) 2007 Ulrich Germann; all rights reserved.
 * Licensed to NRC-CNRC under special agreement.
 */

#ifndef __MemTreeNode_tree_iterator
#define __MemTreeNode_tree_iterator
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include "tpt_typedefs.h"
namespace ugdiss
{
  using namespace std;

  // CLASS DEFINITION
  template<typename val_t, typename key_t>
  class
  MemTreeNode<val_t,key_t>::
  tree_iterator
  {
  protected:
    MemTreeNode* mRoot;
    vector<typename MemTreeNode<val_t,key_t>::map_t::iterator> mPath;
  public:
    friend class MemTreeNode<val_t,key_t>;

    tree_iterator(MemTreeNode<val_t,key_t>* const _root=NULL);
    tree_iterator(tree_iterator const& other);

    // OPERATORS
    bool           operator==(tree_iterator const& other);
    bool           operator!=(tree_iterator const& other);
    MemTreeNode&   operator[](int _idx) const;
    MemTreeNode*   operator->() const { return &(mPath.back()->second); }
    tree_iterator& operator=(tree_iterator const& other);

    size_t depth() const { return mPath.size(); }
    size_t size() const { return mPath.size(); }
    MemTreeNode<val_t,key_t>* root() { return mRoot; }

    bool up();   // returns true if successful; drops last item in the path
    bool down(); // returns true if successful; adds first child
    bool next(); // returns true if successful; switches to sibling
    bool extend(id_type _id, bool create=false);

    // string ids2string() const;
    // string ids2string_r() const; // same as ids2string but in reverse order

    vector<key_t> idSeq() const;
  };

  // CONSTRUCTORS ----------------------------------------------------------------
  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  tree_iterator(MemTreeNode<val_t,key_t>* const _root)
    : mRoot(_root)
  {};

  // COPY CONSTRUCTOR ------------------------------------------------------------
  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  tree_iterator(tree_iterator const& other)
    : mRoot(other.mRoot)
  {
    this->mPath = other.mPath;
  };

  // operator[] -----------------------------------------------------------------
  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>&
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  operator[](int _idx) const
  {
    assert(size_t(abs(_idx)) < mPath.size() || (_idx < 0 && size_t(-_idx) <= mPath.size()));
    size_t i = _idx >= 0 ? _idx : mPath.size()+_idx;
    return mPath[i]->second;
  }

  // operator== ------------------------------------------------------------------
  template<typename val_t, typename key_t>
  bool
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  operator==(tree_iterator const& other)
  {
    if (this->mRoot!=other.mRoot) return false;
    if (this->mPath.size()==0 && other.mPath.size()==0) return true;
    return this->mPath.back() == other.mPath.back();
  }

  // operator!= ------------------------------------------------------------------
  template<typename val_t, typename key_t>
  bool
  MemTreeNode<val_t, key_t>::
  tree_iterator::
  operator!=(tree_iterator const& other)
  {
    return !(*this==other);
  }

  // operator= -------------------------------------------------------------------
  template<typename val_t, typename key_t>
  typename MemTreeNode<val_t,key_t>::tree_iterator&
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  operator=(tree_iterator const& other)
  {
    this->mRoot = other.mRoot;
    this->mPath = other.mPath;
    return *this;
  }

  template<typename val_t, typename key_t>
  bool
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  up()
  {
    bool retval = (this->mPath.size());
    if (retval)
      this->mPath.pop_back();
    return retval;
  }

  template<typename val_t, typename key_t>
  bool
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  down()
  {
    MemTreeNode<val_t,key_t>& n = (this->mPath.size()
                                   ? (*this)[-1]
                                   : *mRoot);
    typename MemTreeNode<val_t,key_t>::map_t::iterator m = n.mDtrs.begin();
    if (m == n.mDtrs.end())
      return false;
    this->mPath.push_back(m);
    return true;
  }

  template<typename val_t, typename key_t>
  bool
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  next()
  {
    if (this->mPath.size() == 0)
      return false;
    MemTreeNode<val_t,key_t>& r = (this->mPath.size() > 1
                                   ? (*this)[-2]
                                   : *mRoot);
    typename MemTreeNode<val_t,key_t>::map_t::iterator m = this->mPath.back();
    // cerr << "[z] " << *this << " | " << m->first << endl;
    m++;
    if (m == r.mDtrs.end())
      return false;
    this->mPath.back() = m;
    return true;
  }

  template<typename val_t, typename key_t>
  bool
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  extend(MemTreeNode<val_t,key_t>::id_type _id, bool create)
  {
    bool retval;
    typedef typename MemTreeNode<val_t,key_t>::map_t::iterator   myIter;
    typedef typename MemTreeNode<val_t,key_t>::map_t::value_type entry_t;

    MemTreeNode<val_t,key_t>* n = mPath.size() ? &(*this)[-1] : mRoot;
    if (create)
      {
        entry_t e(_id,MemTreeNode(_id,0));
        pair<myIter,bool> foo = n->mDtrs.insert(e);
        this->mPath.push_back(foo.first);
        retval = foo.second;
      }
    else
      {
        myIter m = n->mDtrs.find(_id);
        retval = (m != n->mDtrs.end());
        if (retval)
          this->mPath.push_back(m);
      }
    return retval;
  }

  template<typename val_t, typename key_t>
  std::vector<key_t>
  MemTreeNode<val_t,key_t>::
  tree_iterator::
  idSeq()  const
  {
    vector<key_t> ret(this->size());
    for (size_t i = 0; i < this->size(); i++)
      ret[i] = (*this)[i].id;
    return ret;
  };


  // STREAMING OPERATOR (mostly for debugging) -----------------------------------
  template<typename MTNODE>
  ostream&
  operator<<(ostream& out, typename MTNODE::tree_iterator& p)
  {
    if (p.depth()>0)
      {
        for (size_t i = 0; i < p.depth(); i++)
          out << p[i].id << " ";
        out << "-> " << p[-1].val;
      }
    return out;
  }

} // end of namespace ugdiss
#endif
