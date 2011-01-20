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



/* Tree structures to store data such as n-gram counts with
 * variable n-gram length. #included via ugTreeNode.h
 *
 * (c) 2006,2007 Ulrich Germann; all rights reserved
 */
#ifndef __ugMemTreeNode
#define __ugMemTreeNode
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include "tpt_typedefs.h"
#include "ugReadWriteValues.h"
#include "tpt_tightindex.h"
#include "tpt_pickler.h"

using namespace std;
using namespace ugdiss;

namespace ugdiss
{

  template<typename val_t=count_type, typename key_t=id_type>
  class MemTreeNode 
  {
  public:

    val_t  val;
    key_t  id;
  
    typedef key_t id_type;
    typedef val_t value_t;
    typedef map<key_t,MemTreeNode> map_t;
  
    class tree_iterator;
    class fixed_depth_iterator;
    class preorder_iterator;
    class valConvert;
    enum { NOCREATE=0, CREATE=1 };
  
  protected:
  
    friend class tree_iterator;
    friend class fixed_depth_iterator;
    friend class preorder_iterator;
    map_t mDtrs;
    valConvert val2fpos;

  public:

    MemTreeNode(key_t _id=0, val_t const& _val=val_t(0));
  
    virtual 
    ~MemTreeNode();
  

    // FUNCTIONS TO FIND NODES IN THE TREE

    /** returns reference to the child node indexed by idx, inserts a new node if necessary
     */
    virtual 
    MemTreeNode& 
    operator[](key_t idx);
 
    /** returns a pointer to the child node indexed by idx, NULL if no such node exits
     */
    virtual 
    MemTreeNode const* 
    get(key_t idx) const;

    /** returns a pointer to the child node indexed by idx.
     *  _createIfNecessary determines whether the function should create a new node if the 
     * requested node does not exits, or return NULL.
     */
    virtual 
    MemTreeNode* 
    get(key_t idx,bool _createIfNecessary=false);

    /** return a pointer to the node at the end of the path given
     *  /start/ and /stop/ allow the caller to specify a sub-sequence of /path/
     */
    MemTreeNode*
    find(vector<id_type> const& _path, 
         size_t _start=0, size_t _stop=0,
         bool _createIfNecessary = true);
  
    MemTreeNode*
    find(tree_iterator const& _tIter, size_t _start=0,
         bool _createIfNecessary = true);

    /** same as find(), but returns a tree iterator instead of a pointer
     */
    tree_iterator
    find2(vector<id_type> const& _path, size_t _start=0, size_t _stop=0, 
          bool _createIfNecessary = true);
  
    // WRITING THE TREE TO DISK

    /** writes the trie originating at this node as a tightly packed trie
     */
    template<typename valueWriter>
    pair<filepos_type,uchar>
    pickle3x(ostream& out, valueWriter& bw, const val_t* deflt,
             const val_t* th);

    /** wrapper function for pickle3x, to accommodate old legacy code
     */
    pair<filepos_type,uchar>
    pickle3(ostream& out, const val_t* deflt=NULL, const val_t* th=NULL);
 
  
    // OTHER FUNCTIONS
  
    /** true if the node is the last among its siblings
     */
    bool 
    isLastChild() const;

    /** return pointer to the first child
     */
    virtual 
    MemTreeNode* 
    firstChild();

    /** true if the node has no children
     */
    bool 
    isTerminal()  const { return mDtrs.empty(); }

    preorder_iterator 
    preorder_begin();
  
    fixed_depth_iterator 
    fixed_depth_begin(size_t _depth);
  
    tree_iterator 
    end();

    size_t 
    numChildren() const;
  
    /** This is a function for recursively merging tries with raw counts
     *  it's very slow.
     */
    MemTreeNode& 
    operator+=(MemTreeNode const& other);

    friend
    pair<filepos_type,uchar>
    toRepos<MemTreeNode<val_t,key_t>,val_t>(ostream& idx, ostream& dat,
                                            MemTreeNode<val_t,key_t>& node,
                                            filepos_type parent, vector<val_t>& remap,
                                            filepos_type &curIdxPos, filepos_type &curDatPos);
  };

  template<typename val_t, typename key_t>
  class 
  MemTreeNode<val_t,key_t>:: 
  valConvert : public unary_function<filepos_type,val_t>
  {
  public:
    filepos_type operator()(val_t foo);
  };

  // if implicit conversion is not possible,
  // you have to define a custom function!
  template<typename val_t, typename key_t>
  filepos_type
  MemTreeNode<val_t,key_t>::
  valConvert::
  operator()(val_t foo)
  {
    return foo;
  }

  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>::
  MemTreeNode(key_t _id, val_t const& _val)
  {
    this->id   = _id;
    this->val = _val;
  };

  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>::
  ~MemTreeNode()
  {};

  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>*
  MemTreeNode<val_t,key_t>::
  find(vector<id_type> const& _path,
       size_t _start, size_t _stop,
       bool _createIfNecessary)
  {
    if (_stop==0)
      _stop = _path.size();
    else
      _stop = min(_stop,_path.size());
    if(_start >= _stop)
      return this;
    else
      {
        typename map_t::iterator m = this->mDtrs.find(_path[_start]);
        if (m != this->mDtrs.end())
          {
            if (_start+1 == _stop)
              return &(m->second);
            else
              return m->second.find(_path,_start+1,_stop,_createIfNecessary);
          }
        else if (_createIfNecessary)
          {
            if (_start+1 == _stop)
              return &((*this)[_path[_start]]);
            else
              return (*this)[_path[_start]].find(_path,_start+1,_stop,true);
          }
        else
          return NULL;
      }
  }

  template<typename val_t, typename key_t>
  typename MemTreeNode<val_t,key_t>::tree_iterator
  MemTreeNode<val_t,key_t>::
  find2(vector<id_type> const& v, size_t _start, size_t _stop, bool _create)
  {
    tree_iterator ret(this);
    if (_stop==0)
      _stop = v.size();
    else
      _stop = min(_stop,v.size());
    bool exist = true;
    while (_start < _stop && (exist||_create))
      exist = ret.extend(v[_start++]);
    return ret;
  }

  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>*
  MemTreeNode<val_t,key_t>::
  find(tree_iterator const& _tIter,
       size_t _start,
       bool _createIfNecessary)
  {
    if(_tIter.depth() == _start)
      return this;
    else
      {
        typename map_t::iterator m = this->mDtrs.find(_tIter[_start].Id);
        if (m != this->mDtrs.end())
          {
            if (_start+1 == _tIter.depth())
              return &(m->second);
            else
              return m->second.find(_tIter,_start+1,_createIfNecessary);
          }
        else if (_createIfNecessary)
          {
            if (_start+1 == _tIter.depth())
              return &((*this)[_tIter[_start].Id]);
            else
              return (*this)[_tIter[_start].Id].find(_tIter,_start+1,true);
          }
        else
          return NULL;
      }
  }

  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>*
  MemTreeNode<val_t,key_t>::firstChild()
  {
    typename map_t::iterator m = mDtrs.begin();
    return m == mDtrs.end() ? NULL : &(m->second);
  }


  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>&
  MemTreeNode<val_t,key_t>::
  operator[](key_t _idx)
  {
    typename map_t::iterator m = mDtrs.find(_idx);
    typename map_t::iterator z = mDtrs.end();

    if (m == mDtrs.end())
      {
        typename map_t::value_type foo(_idx,MemTreeNode<val_t,key_t>(_idx,val_t(0)));
        m = this->mDtrs.insert(foo).first;
      }
    return m->second;
  }



  template<typename val_t, typename key_t>
  template<typename valueWriter>
  pair<filepos_type,uchar>
  MemTreeNode<val_t,key_t>::
  pickle3x(ostream& out, valueWriter& bw, const val_t* deflt, const val_t* th)
  {
    static key_t flagmask = FLAGMASK;
    pair<filepos_type,uchar> retval(0,0);
    if (th && this->val < *th) return retval;

    map<key_t,filepos_type> index;
    for (typename map_t::iterator m = mDtrs.begin(); m != mDtrs.end(); m++)
      {
        if (!(th && m->second.val < *th))
          {
            pair<filepos_type,uchar> jar
              = m->second.pickle3x(out,bw,deflt,th);
            key_t key=(m->first<<FLAGBITS)+jar.second;
            index[key] = jar.first;
          }
      }
    assert(out.tellp() > 0);
    filepos_type idxStart = out.tellp();
    typedef typename map<key_t,filepos_type>::iterator myIter;
    for (myIter i = index.begin(); i != index.end(); i++)
      {
        tightwrite(out, i->first, false); // indicator bit 0 for ID
        if (i->first&flagmask)
          tightwrite(out, idxStart - i->second, true); //  ind. bit  1 for offset
        else
          tightwrite(out, i->second, true);
      }
    assert(out.tellp() >= 0);
    filepos_type myPosition = out.tellp();

    // MOVED BELOW April 1, 2008 (UG)
    //    if (!deflt || (this->val != *deflt && index.size()))
    //       {
    //         // binwrite(out,this->val);
    //         bw(out,this->val);
    //         retval.second += HAS_VALUE_MASK;
    //       }

    if (index.size())
      {
        binwrite(out,myPosition - idxStart);
        retval.second += HAS_CHILD_MASK;
      }

    // MOVED HERE
    filepos_type indexEntry;
    if ((index.size() && (!deflt || this->val != *deflt))
        || !bw(indexEntry,this->val))
      {
        bw(out,this->val);
        retval.second += HAS_VALUE_MASK;
      }
    else
      myPosition = indexEntry;
    retval.first = myPosition;
    return retval;
  }

  template<typename val_t, typename key_t>
  pair<filepos_type,uchar>
  MemTreeNode<val_t,key_t>::
  pickle3(ostream& out, val_t const* deflt, val_t const* th)
  {
//    return pickle3x(out,GenericValueWriter<val_t>(),deflt,th);
    GenericValueWriter<val_t> vw;
    return pickle3x(out,vw,deflt,th);
  }

  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t> const*
  MemTreeNode<val_t,key_t>::
  get(key_t _idx) const
  {
    typename map_t::const_iterator f = this->mDtrs.find(_idx);
    if (f == this->mDtrs.end())
      return NULL;
    else
      return &(f->second);
  }

  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>*
  MemTreeNode<val_t,key_t>::
  get(key_t _idx, bool _createIfNecessary)
  {
    typename map_t::iterator m = mDtrs.find(_idx);
    if (m == mDtrs.end())
      {
        if (_createIfNecessary)
          {
            typename map_t::value_type foo(_idx,MemTreeNode<val_t,key_t>(_idx,val_t(0)));
            m = this->mDtrs.insert(foo).first;
          }
        else
          return NULL;
      }
    return &(m->second);
  }

  template<typename val_t, typename key_t>
  typename MemTreeNode<val_t,key_t>::fixed_depth_iterator
  MemTreeNode<val_t,key_t>::
  fixed_depth_begin(size_t _depth)
  {
    return fixed_depth_iterator(this,_depth);
  }

  template<typename val_t, typename key_t>
  typename MemTreeNode<val_t,key_t>::preorder_iterator
  MemTreeNode<val_t,key_t>::
  preorder_begin()
  {
    typename map_t::iterator m = this->mDtrs.begin();
    return preorder_iterator(this,m);
  }

  template<typename val_t, typename key_t>
  typename MemTreeNode<val_t,key_t>::tree_iterator
  MemTreeNode<val_t,key_t>::
  end()
  {
    tree_iterator ret(this);
    ret.mPath.push_back(this->mDtrs.end());
    return ret;
  }

  template<typename val_t, typename key_t>
  size_t
  MemTreeNode<val_t,key_t>::
  numChildren() const
  {
    return mDtrs.size();
  }

  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>&
  MemTreeNode<val_t,key_t>::
  operator+=(MemTreeNode<val_t,key_t> const& other)
  {
    this->val += other.val;
    typename map_t::const_iterator m;
    for (m = other.mDtrs.begin(); m != other.mDtrs.end(); m++)
      (*this)[m->first] += m->second;
    return *this;
  }

} // end of namespace
#include "ugMemTreeNode_preorder_iterator.h"
#include "ugMemTreeNode_fixed_depth_iterator.h"

#endif
