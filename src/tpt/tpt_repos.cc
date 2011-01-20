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

//#define DEBUG_TPT

#include "tpt_repos.h"
#include "tpt_tightindex.h"
#include "tpt_pickler.h"

namespace ugdiss
{
  using namespace std;

#ifdef USE_PTRIE
  typedef tp_trie_t::iterator trie_node_t;
  typedef tp_trie_t::iterator trie_node_t_ref;
  typedef tp_trie_t::iterator trie_node_iterator_t;

  inline trie_node_t& get_node(trie_node_iterator_t &it) { return it; }
  inline trie_node_t_ref get_node(tp_trie_t &trie, vector<uint32_t>::const_iterator &it) {
     return trie.find(*it);
  }
  inline uint32_t get_node_id(trie_node_t const &node) { return node.get_key(); }
  inline uint32_t get_node_val(trie_node_t const &node) { return node.get_value(); }
  inline trie_node_iterator_t begin_children(trie_node_t &node) {
     return node.begin_children();
  }
  inline const trie_node_iterator_t& end_children(trie_node_t &node) {
     return node.end_children();
  }
#else
  typedef MemTreeNode<uint32_t,uint32_t> trie_node_t;
  typedef trie_node_t& trie_node_t_ref;
  typedef trie_node_t::map_t::iterator trie_node_iterator_t;

  inline trie_node_t& get_node(trie_node_iterator_t &it) { return it->second; }
  inline trie_node_t_ref get_node(tp_trie_t &trie, trie_node_iterator_t &it) {
     return it->second;
  }
  inline uint32_t get_node_id(trie_node_t &node) { return node.id; }
  inline uint32_t get_node_val(trie_node_t &node) { return node.val; }
  // can't inline begin_children and end_children because mDtrs is protected.
  #define begin_children(node)  (node.mDtrs.begin())
  #define end_children(node)    (node.mDtrs.end())
#endif

  template<>
  pair<filepos_type,uchar>
  toRepos<trie_node_t, uint32_t>(ostream& idx,
                                 ostream& dat,
                                 trie_node_t& node,
                                 filepos_type parent,
                                 vector<uint32_t>& remap,
                                 filepos_type &curIdxPos,
                                 filepos_type &curDatPos)
  {
    static uint32_t flagmask = FLAGMASK;
    pair<filepos_type,uchar> ret(0,0);

    TPT_DBG(assert(curDatPos == (filepos_type)dat.tellp()));
    filepos_type myDatPos = curDatPos;

    TPT_DBG(cerr << "  toRepos node: 0x" << hex << (uint64_t)&node << dec << endl);
    TPT_DBG(cerr << "    node.id=" << get_node_id(node) << " node.val=" << get_node_val(node)
                 << " parent=" << parent << endl);
    TPT_DBG(cerr << "    writing (dat): id = " << get_node_id(node)
                 << ", myDatPos-parent = " << myDatPos-parent
                 << " (" << myDatPos << "-" << parent << ")" << endl);
    // write node in the repository
    curDatPos += binwrite(dat, get_node_id(node));

    curDatPos += binwrite(dat, myDatPos-parent);
    if (get_node_val(node) >= remap.size()) {
      cerr << ewarn << "Encountered pid (" << get_node_val(node) << ") greater than "
           << "expected highest pid (" << remap.size()-1 << ")." << endl;
      remap.resize(get_node_val(node)+1,0);
    }
    remap[get_node_val(node)] = myDatPos;

    // process children and write index
    vector<pair<uint32_t,filepos_type> > tmpidx;
    trie_node_iterator_t it(begin_children(node));
    trie_node_iterator_t end_it(end_children(node));
    TPT_DBG(if (it != end_it) cerr << "  processing children" << endl);
    for (; it != end_it; ++it)
      {
        pair<filepos_type,uchar> jar;
        jar = toRepos(idx, dat, get_node(it), myDatPos, remap, curIdxPos, curDatPos);
        key_t key = (get_node_id(get_node(it))<<FLAGBITS)+jar.second;
        tmpidx.push_back(pair<uint32_t,filepos_type>(key,jar.first));
      }

    TPT_DBG(assert(curIdxPos == (filepos_type)idx.tellp()));
    filepos_type idxStart = curIdxPos;
    TPT_DBG(if (tmpidx.size() > 0)
              cerr << "  writing tmpidx for node: " << hex << (uint64_t)&node << dec << endl);
    for (size_t i = 0; i < tmpidx.size(); i++)
      {
        TPT_DBG(cerr << "    writing (idx): i=" << i << ": "
                     << (tmpidx[i].first>>FLAGBITS) << ","
                     << (tmpidx[i].first&FLAGMASK) << " " << tmpidx[i].second << endl);
        curIdxPos += tightwrite(idx, tmpidx[i].first, false);
        if (tmpidx[i].first&flagmask)
           curIdxPos += tightwrite(idx, idxStart-tmpidx[i].second, true);
        else
           curIdxPos += tightwrite(idx, tmpidx[i].second, true);
      }
    TPT_DBG(assert(curIdxPos == (filepos_type)idx.tellp()));
    filepos_type myPos = curIdxPos;
    if (tmpidx.size() > 0)
      {
        TPT_DBG(cerr << "  writing (idx): myPos-idxStart = " << myPos-idxStart << " ("
                     << myPos << "-" << idxStart << ") myDatPos = " << myDatPos << endl);
        curIdxPos += binwrite(idx, myPos-idxStart);
        curIdxPos += binwrite(idx, myDatPos);
        ret.second += HAS_CHILD_MASK;
        ret.second += HAS_VALUE_MASK;
        ret.first  = myPos;
      }
    else
      ret.first = myDatPos;
    TPT_DBG(cerr << "  toRepos: returning ret.first=" << ret.first
                 << " ret.second=" << (ret.second&0xff) << endl);
    return ret;
  }

  template<>
  vector<uint32_t>
  toRepos<tp_trie_t, uint32_t>(string bname,
                               tp_trie_t& trie,
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

    TPT_DBG(cerr << "toRepos: bname=" << bname << " numTokens=" << numTokens
                 << " highestPid=" << highestPid << endl);

    numwrite(idx,filepos_type(0));  // reserve room for start idx pos.
    numwrite(idx,numTokens);        // record idx size.
    filepos_type curIdxPos = sizeof(filepos_type) + sizeof(id_type);
    curIdxPos += binwrite(idx, 0U); // root value, not needed for repositories
    curIdxPos += binwrite(idx, 0U); // default value, also not needed

    // root node of the repository:
    filepos_type curDatPos = 0;
    curDatPos += binwrite(dat, 0U);
    curDatPos += binwrite(dat, 0U);

    vector<pair<filepos_type,uchar> >
      index(numTokens,pair<filepos_type,uchar>(0,0));

    // process children and write index
#ifdef USE_PTRIE
    trie_node_iterator_t roots_it(trie.begin_children());
    vector<uint32_t>roots;
    for (; roots_it != trie.end_children(); ++roots_it)
       roots.push_back(roots_it.get_key());
    sort(roots.begin(), roots.end());
    vector<uint32_t>::const_iterator it(roots.begin());
    vector<uint32_t>::const_iterator end_it(roots.end());
#else
    trie_node_iterator_t it(begin_children(trie));
    trie_node_iterator_t end_it(end_children(trie));
#endif
    for (; it != end_it; ++it)
      {
        trie_node_t_ref node(get_node(trie, it));
        if (get_node_id(node) >= index.size())
           cerr << efatal << "Encountered id (" << get_node_id(node) << ") >= numTokens ("
                << numTokens << ") in in-memory trie of sequences."
                << exit_1;
        TPT_DBG(cerr << "\ntoRepos (top) it: " << get_node_id(node) << " => 0x"
                     << hex << (uint64_t)&node << dec << endl);
        index[get_node_id(node)] = toRepos(idx, dat, node, 0, remap,
                                           curIdxPos, curDatPos);
        TPT_DBG(cerr << "toRepos (top) index[" << get_node_id(node) << "] = <"
                     << index[get_node_id(node)].first << " "
                     << (int)index[get_node_id(node)].second  << ">" << endl);
      }

    TPT_DBG(assert(curIdxPos == (filepos_type)idx.tellp()));
    filepos_type sIdx = curIdxPos;
    // write index ...
    TPT_DBG(cerr << "\ntoRepos (top): writing index at start position: " << endl);
    for (size_t i = 0; i < index.size(); i++)
      {
        TPT_DBG(cerr << "    writing (idx): i=" << i << ": " << index[i].first
                     << " " << (int)(index[i].second&0xFF) << endl);
        numwrite(idx,index[i].first);
        idx.put(index[i].second);
      }
    // store start position of index
    idx.seekp(0);
    TPT_DBG(cerr << "\ntoRepos (top): writing (idx): start position: " << sIdx
                 << " at position 0." << endl);
    numwrite(idx,sIdx);
    return remap;
  }

} // end of namespace ugdiss
