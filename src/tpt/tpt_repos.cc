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

#include "tpt_repos.h"
#include "tpt_tightindex.h"
#include "tpt_pickler.h"
#include "ugMemTable.h"
#include "ugMemTreeNode.h"

namespace ugdiss
{
  using namespace std;
  template<>
  pair<filepos_type,uchar>
  toRepos<MemTreeNode<uint32_t,uint32_t>,uint32_t>(ostream& idx, 
                                                   ostream& dat, 
                                                   MemTreeNode<uint32_t,uint32_t>& node, 
                                                   uint32_t parent, 
                                                   vector<uint32_t>& remap)
  {
    static uint32_t flagmask = FLAGMASK; 
    pair<filepos_type,uchar> ret(0,0);

    filepos_type myDatPos = dat.tellp();
  
    // write node in the repository
    binwrite(dat,node.id);

    // cout << myDatPos << " parent:" << parent << endl;
    
    binwrite(dat,myDatPos-parent);
    if (node.val >= remap.size()) {
      cerr << ewarn << "Encountered pid (" << node.val << ") greater than "
           << "expected highest pid (" << remap.size()-1 << ")." << endl;
      remap.resize(node.val+1,0);
    }
    remap[node.val] = myDatPos;

    // process children and write index
    typedef MemTreeNode<uint32_t>::map_t::iterator myIter;
    vector<pair<uint32_t,uint32_t> > tmpidx;
    for (myIter m = node.mDtrs.begin(); m != node.mDtrs.end(); m++)
      {
	pair<filepos_type,uchar> jar;
	jar = toRepos(idx,dat,m->second,myDatPos,remap);
	key_t key = (m->first<<FLAGBITS)+jar.second;
        tmpidx.push_back(pair<uint32_t,uint32_t>(key,jar.first));
      }
    filepos_type idxStart = idx.tellp();
    for (size_t i = 0; i < tmpidx.size(); i++)
      {
	tightwrite(idx,tmpidx[i].first,false);
        if (tmpidx[i].first&flagmask)
          tightwrite(idx,idxStart-tmpidx[i].second,true);
        else
          tightwrite(idx,tmpidx[i].second,true);
      }
    filepos_type myPos = idx.tellp();
    if (node.mDtrs.size())
      {
	binwrite(idx,myPos-idxStart);
	binwrite(idx,myDatPos);
	ret.second += HAS_CHILD_MASK;
	ret.second += HAS_VALUE_MASK;
	ret.first  = myPos;
      }
    else
      ret.first = myDatPos;
    return ret;
  }

  template<>
  vector<uint32_t>
  toRepos<MemTable<uint32_t>,uint32_t>(string bname, 
                                       MemTable<uint32_t>& M, 
                                       id_type  numTokens, 
                                       uint32_t highestPid)
  {
    vector<uint32_t> remap(highestPid+1,0);
    ofstream dat((bname+".dat").c_str());
    if (dat.fail())
      cerr << efatal << "Unable to open file '" << (bname+".dat") << "' for writing."
           << exit_1;
    ofstream idx((bname+".idx").c_str());
    if (idx.fail())
      cerr << efatal << "Unable to open file '" << (bname+".idx") << "' for writing."
           << exit_1;
    
    numwrite(idx,filepos_type(0));  // reserve room for start idx pos.
    numwrite(idx,numTokens); // record idx size.
    binwrite(idx,0U); // root value, not needed for repositories
    binwrite(idx,0U); // default value, also not needed
    
    // root node of the repository:
    binwrite(dat,0U); 
    binwrite(dat,0U); 
    
    vector<pair<filepos_type,uchar> > 
      index(numTokens,pair<filepos_type,uchar>(0,0));
    
    // process children and write index
    typedef MemTreeNode<uint32_t>::map_t::iterator myIter;
    for (myIter m = M.mDtrs.begin(); m != M.mDtrs.end(); m++)
      {
        if (m->first >= index.size())
          cerr << efatal << "Encountered id (" << m->first << ") >= numTokens ("
               << numTokens << ") in in-memory trie of sequences."
               << exit_1;
        index[m->first] = toRepos(idx,dat,m->second,0,remap);
      }
    
    filepos_type sIdx = idx.tellp();
    // store start position of index
    idx.seekp(0);
    numwrite(idx,sIdx);
    idx.seekp(sIdx);
    // write index ...
    for (size_t i = 0; i < index.size(); i++)
      {
        numwrite(idx,index[i].first);
        idx.put(index[i].second);
      }
    return remap;
  }
} // end of namespace ugdiss



