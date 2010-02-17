// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.
#include <sstream>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <boost/pool/pool_alloc.hpp>

#include "tpt_tokenindex.h"
#include "tpt_utils.h"

using namespace std;
namespace ugdiss
{

  TokenIndex::
  TokenIndex() 
    : ridx(NULL),unkLabel("UNK"),numTokens(0)
  {};
  
  TokenIndex::
  TokenIndex(string fname, string unkToken) 
    : ridx(NULL),unkLabel(unkToken)
  {
    this->open(fname,unkToken);
  };

  void
  TokenIndex::
  open(string fname, string unkToken)
  {
    open_mapped_file_source(file, fname);

    if (file.size() < 4+sizeof(id_type))
      cerr << efatal << "Bad token index '" << fname << "'." << exit_1;
    this->numTokens = *(reinterpret_cast<uint32_t const*>(file.data()));
    unkId = *(reinterpret_cast<id_type const*>(file.data()+4));

    startIdx = reinterpret_cast<Entry const*>(file.data()+4+sizeof(id_type));
    endIdx   = startIdx + numTokens;
    comp.base = reinterpret_cast<char const*>(endIdx);
    // spot check the first entry.
    if (comp.base + startIdx->offset > file.data() + file.size() ||
        (startIdx->id != unkId && startIdx->id > numTokens))
      cerr << efatal << "Bad token index '" << fname << "'." << exit_1;
    if (!unkToken.empty())
      {
	Entry const* bla = lower_bound(startIdx,endIdx,unkToken.c_str(),comp);
	unkId = ((bla < endIdx && unkToken == comp.base+bla->offset) 
                 ? bla->id 
                 : numTokens);
      }
  }
  
  TokenIndex::
  CompFunc::
  CompFunc() 
  {};
  
  bool
  TokenIndex::
  CompFunc::
  operator()(Entry const& A, char const* w)
  {
    return strcmp(base+A.offset,w) < 0;
  };

  id_type 
  TokenIndex::
  operator[](char const* p) const
  {
    Entry const* bla = lower_bound(startIdx,endIdx,p,comp);
    if (bla == endIdx) return unkId;
    return strcmp(comp.base+bla->offset,p) ? unkId : bla->id;
  }

  id_type 
  TokenIndex::
  operator[](string const& w) const
  {
    return (*this)[w.c_str()];
  }

  vector<char const*> 
  TokenIndex::
  reverseIndex() const
  {
    size_t numToks = endIdx-startIdx;

    // cout << "tokenindex has " << numToks << " tokens" << endl;

    vector<char const*> v(numToks,NULL); 
    // v.reserve(endIdx-startIdx);
    for (Entry const* x = startIdx; x != endIdx; x++)
      {
	if (x->id >= v.size()) 
	  v.resize(x->id+1);
	v[x->id] = comp.base+x->offset;
      }
    // cout << "done reversing index " << endl;
    return v;
  }

  char const* const
  TokenIndex::
  operator[](id_type id) const
  {
    // for use of the const version of operator[], you need to call iniReverseIndex() first
    if (!ridx.size())
        cerr << efatal << "You need to call iniReverseIndex() "
             << "on the TokenIndex class before using operatorp[](id_type id)."
             << exit_1;
    if (id >= ridx.size()) return unkLabel.c_str();
#if 0
    char const* foo = ridx[id];
    assert((*this)[foo] == id);
#endif
    return ridx[id];
  }

  void
  TokenIndex::
  iniReverseIndex() 
  {
    if (!ridx.size()) ridx = reverseIndex();
  }

  
  char const* const
  TokenIndex::
  operator[](id_type id) 
  {
    if (!ridx.size()) ridx = reverseIndex();
    if (id >= ridx.size()) return unkLabel.c_str();
#if 0
    char const* foo = ridx[id];
    assert((*this)[foo] == id);
#endif
    return ridx[id];
  }

  string 
  TokenIndex::
  toString(vector<id_type> const& v) 
  {
    if (!ridx.size()) ridx = reverseIndex();
    ostringstream buf;
    for (size_t i = 0; i < v.size(); i++)
      buf << (i ? " " : "") << ridx[v[i]];
    return buf.str();
  }

  string 
  TokenIndex::
  toString(vector<id_type> const& v) const
  {
    assert (ridx.size());
    ostringstream buf;
    for (size_t i = 0; i < v.size(); i++)
      buf << (i ? " " : "") << ridx[v[i]];
    return buf.str();
  }

  string 
  TokenIndex::
  toString(id_type const* start, id_type const* const stop) 
  {
    if (!ridx.size()) ridx = reverseIndex();
    ostringstream buf;
    if (start < stop)
      buf << ridx[*start];
    while (++start < stop)
      buf << " " << ridx[*start];
    return buf.str();
  }

  string 
  TokenIndex::
  toString(id_type const* start, id_type const* const stop) const
  {
    assert (ridx.size());
    ostringstream buf;
    if (start < stop)
      buf << ridx[*start];
    while (++start < stop)
      buf << " " << ridx[*start];
    return buf.str();
  }

  void
  TokenIndex::
  toIdSeq(vector<id_type>& idSeq, string const& line) const
  {
    idSeq.clear();
    istringstream buf(line);
    string w;
    while (buf>>w)
       idSeq.push_back((*this)[w]);
  }

  id_type
  TokenIndex::
  getNumTokens() const
  {
    return numTokens;
  }

  id_type
  TokenIndex::
  getUnkId() const
  {
    return unkId;
  }

  char const* const
  TokenIndex::
  getUnkToken() const
  {
    return (*this)[unkId];
  }
  
}
