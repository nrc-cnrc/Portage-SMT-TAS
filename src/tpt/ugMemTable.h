// (c) 2006,2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.
/* (c) 2007 Ulrich Germann. All rights reserved */
#ifndef __ugMemTable
#define __ugMemTable

#include <vector>
#include <fstream>

#include "ugMemTreeNode.h"
#include "tpt_pickler.h"
#include "tpt_typedefs.h"
#include "tpt_repos.h"

using namespace std;

namespace ugdiss
{

template<typename val_t=count_type, typename key_t=id_type>
  class 
  MemTable : public MemTreeNode<val_t,key_t>
  {

  public:
    typedef val_t value_t;
    typedef MemTreeNode<val_t,key_t> node_type;

    vector<MemTreeNode<val_t,key_t>*> mIndex;

  public:
    MemTable(size_t _siz=0); 
    size_t size() const { return this->mIndex.size(); }
    void pickle3(string fname, val_t deflt);

    filepos_type 
    pickle3(ostream& out, val_t deflt);
    
    template<typename valueWriter>
    filepos_type 
    pickle3y(ostream& out, valueWriter& bw, 
             const val_t* deflt, const val_t* th);
    
    MemTreeNode<val_t,key_t>& 
    operator[](id_type _idx);

    MemTreeNode<val_t,key_t> const* 
    get(id_type _idx) const;

    MemTreeNode<val_t,key_t>* 
    get(id_type _idx, bool _createIfNecessary=false);

    friend
    vector<val_t>
    toRepos<MemTable<val_t>,val_t>(string bname, MemTable<val_t>& M, 
                                   id_type numTokens, val_t highestPid);
  };
  
  template<typename val_t, typename key_t>
  MemTable<val_t,key_t>::
  MemTable(size_t _siz)
  {
    this->val = val_t(0);
    this->mIndex.resize(_siz,NULL);
  }
  
  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>&
  MemTable<val_t,key_t>::
  operator[](id_type _idx)
  {
    typedef MemTreeNode<val_t,key_t> node_type;
    if (_idx >= this->mIndex.size())
      {
        this->mIndex.resize(_idx+1,NULL);
        MemTreeNode<val_t> foo(_idx,/*this,*/val_t(0));
        typename map<key_t,node_type>::value_type bar(_idx,foo);
        this->mIndex[_idx] = &(this->mDtrs.insert(bar).first->second);
      }
    else if (this->mIndex[_idx] == NULL)
      {
        MemTreeNode<val_t,key_t> foo(_idx,/*this,*/val_t(0));
        typename map<id_type,node_type>::value_type bar(_idx,foo);
        this->mIndex[_idx] = &(this->mDtrs.insert(bar).first->second);
      }
    return *(this->mIndex[_idx]);
  }
  
  template<typename val_t, typename key_t>
  void
  MemTable<val_t,key_t>::
  pickle3(string fname, val_t deflt)
  {
    ofstream out(fname.c_str());
    if (out.fail())
      cerr << efatal << "Unable to open MemTable file '" << fname << "' for writing."
           << exit_1;
    pickle3(out,deflt);
    out.close();
  }
  
  template<typename val_t, typename key_t>
  filepos_type
  MemTable<val_t,key_t>::
  pickle3(ostream& out, val_t deflt)
  {
    filepos_type cPos = out.tellp();

    numwrite(out,filepos_type(0));        // reserve room for start index pos.
    numwrite(out,id_type(mIndex.size())); // reserve room for index size
    binwrite(out,this->val);              // root value     
    binwrite(out,deflt);                  // default value 

    vector<pair<filepos_type,uchar> > index(this->mIndex.size());
    for (size_t i = 0; i < this->mIndex.size(); i++)
      {
        if (this->mIndex[i] != NULL)
          index[i] = this->mIndex[i]->pickle3(out,&deflt,NULL);
        else
          index[i] = pair<filepos_type,uchar>(0,0);
      }
    filepos_type sIdx = out.tellp();
    // store start position of index
    out.seekp(cPos);
    numwrite(out,sIdx);
    out.seekp(sIdx);
    for (size_t i = 0; i < index.size(); i++)
      {
        numwrite(out,index[i].first);
        out.put(index[i].second);
      }
    return cPos;
  }

  
  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t> const*
  MemTable<val_t,key_t>::
  get(id_type _idx) const
  {
    return (_idx >= mIndex.size()) ? 0 : mIndex[_idx];
  }

  template<typename val_t, typename key_t>
  MemTreeNode<val_t,key_t>*
  MemTable<val_t,key_t>::
  get(id_type _idx, bool _createIfNecessary)
  {
    if (_idx >= mIndex.size() || mIndex[_idx] == NULL)
      return &((*this)[_idx]);
    else
      return mIndex[_idx];
  }

  template<typename val_t, typename key_t>
  template<typename valueWriter>
  filepos_type
  MemTable<val_t,key_t>::
  pickle3y(ostream& out, valueWriter& bw, 
           const val_t* deflt, const val_t* th)
  { 
    filepos_type cPos = out.tellp();
    numwrite(out,filepos_type(0)); // reserve room for start index pos.
    numwrite(out,id_type(mIndex.size()));      // reserve room for index size
    bw(out,this->val);       // root value     
    if (deflt) 
      bw(out,*deflt);           // default value 
    else 
      binwrite(out,0U);
    vector<pair<filepos_type,uchar> > index(this->mIndex.size());
    for (size_t i = 0; i < this->mIndex.size(); i++)
      {
        if (this->mIndex[i] != NULL)
          index[i] = this->mIndex[i]->pickle3x(out,bw,deflt,th);
        else
          index[i] = pair<filepos_type,uchar>(0,0);
      }
    filepos_type sIdx = out.tellp();
    // store start position of index
    out.seekp(cPos);
    numwrite(out,sIdx);
    out.seekp(sIdx);
    for (size_t i = 0; i < index.size(); i++)
      {
        numwrite(out,index[i].first);
        out.put(index[i].second);
      }
    return cPos;
  }
  
}
#endif
