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
    : unkLabel(default_unk_token),numTokens(0)
  {};
  
  TokenIndex::
  TokenIndex(string fname, string unkToken) 
    : unkLabel(unkToken)
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
    const size_t numToks = endIdx-startIdx;

    // cout << "tokenindex has " << numToks << " tokens" << endl;

    vector<char const*> v(numToks,NULL); 
    // v.reserve(endIdx-startIdx);
    for (Entry const* x = startIdx; x != endIdx; x++)
      {
	if (x->id >= v.size()) 
	  v.resize(x->id+1, NULL);
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
    if (!ridx.size()) ridx = reverseIndex();
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
    if (!ridx.size()) ridx = reverseIndex();
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
    string buf;
    if (start < stop)
      buf = ridx[*start];
    while (++start < stop)
      (buf += " ") += ridx[*start];
    return buf;
  }

  string 
  TokenIndex::
  toString(id_type const* start, id_type const* const stop) const
  {
    if (!ridx.size()) ridx = reverseIndex();
    assert (ridx.size());
    string buf;
    if (start < stop)
      buf = ridx[*start];
    while (++start < stop)
      (buf += " ") += ridx[*start];
    return buf;
  }

  void
  TokenIndex::
  toIdSeq(vector<id_type>& idSeq, string const& line) const
  {
    idSeq.clear();
    size_t q(0), p(0);
    while (true)
      {
        p = line.find_first_not_of(" \t", q);
        if (p == string::npos) break;
        q = line.find_first_of(" \t", p);
        idSeq.push_back((*this)[line.substr(p,q-p)]);
        if (q == string::npos) break;
      }
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
