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

#include "repos_getSequence.h"
#include "tpt_pickler.h"
namespace ugdiss
{
  vector<id_type>
  getSequence(char const* p)
  {
    id_type          id;
    filepos_type offset;
    vector<id_type> ret;
    for (binread(binread(p,id),offset); offset; binread(binread(p,id),offset))
      {
        ret.push_back(id);
        p -= offset;
      }
    return ret;
  }

  vector<string>
  getSequence(char const* p, TokenIndex& tdx)
  {
    id_type          id;
    filepos_type offset;
    vector<string> ret;
    for (binread(binread(p,id),offset); offset; binread(binread(p,id),offset))
      {
        ret.push_back(tdx[id]);
        p -= offset;
      }
    return ret;
  }

  string
  getSequenceAsString(char const* p, TokenIndex& tdx)
  {
    id_type          id;
    filepos_type offset;
    string ret;
    for (binread(binread(p,id),offset); offset; binread(binread(p,id),offset))
      {
        ret += ret.size() ? " " : "";
        ret += tdx[id];
        p -= offset;
      }
    return ret;
  }

  vector<id_type>
  getSequence(istream& in, filepos_type pos)
  {
    uint64_t offset;
    vector<id_type> ret;
    id_type id;
    while (pos != 0)
      {
	in.clear();
	in.seekg(pos);
	// cout << "pos=" << pos << "/" << in.tellg() << " ";
	binread(in,id);
	// cout << "id=" << id << " ";
	if (id==0) 
	  break;
	binread(in,offset);
	// cout << "offset=" << offset << " ";
	if (offset == 0)
	  break;
	pos -= offset;
	// cout << "pos=" << offset << endl;
	ret.push_back(id);
      }
    return ret;
  }


  // optimized versions

  void
  getSequence(vector<id_type>& ret, char const* p)
  {
    id_type          id;
    filepos_type offset;
    ret.clear();
    for (binread(binread(p,id),offset); offset; binread(binread(p,id),offset))
      {
        ret.push_back(id);
        p -= offset;
      }
  }

  void
  getSequence(vector<string>& ret, char const* p, TokenIndex& tdx)
  {
    id_type          id;
    filepos_type offset;
    ret.clear();
    for (binread(binread(p,id),offset); offset; binread(binread(p,id),offset))
      {
        ret.push_back(tdx[id]);
        p -= offset;
      }
  }

  void
  getSequenceAsString(string& ret, char const* p, TokenIndex& tdx)
  {
    id_type          id;
    filepos_type offset;
    ret.clear();
    for (binread(binread(p,id),offset); offset; binread(binread(p,id),offset))
      {
        ret += ret.size() ? " " : "";
        ret += tdx[id];
        p -= offset;
      }
  }

  void
  getSequence(vector<id_type>& ret, istream& in, filepos_type pos)
  {
    uint64_t offset;
    ret.clear();
    id_type id;
    while (pos != 0)
      {
	in.clear();
	in.seekg(pos);
	// cout << "pos=" << pos << "/" << in.tellg() << " ";
	binread(in,id);
	// cout << "id=" << id << " ";
	if (id==0) 
	  break;
	binread(in,offset);
	// cout << "offset=" << offset << " ";
	if (offset == 0)
	  break;
	pos -= offset;
	// cout << "pos=" << offset << endl;
	ret.push_back(id);
      }
  }


} // end of namespace ugdiss

