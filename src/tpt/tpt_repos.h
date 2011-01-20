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
#ifndef __ugMemTreeNode_toRepos_hh
#define __ugMemTreeNode_toRepos_hh

#include <iostream>
#include <vector>
#include "tpt_typedefs.h"

#if IN_PORTAGE
#define USE_PTRIE
#endif  // IN_PORTAGE

#ifdef USE_PTRIE
#include "trie.h"
#else
#include "ugMemTable.h"
#include "ugMemTreeNode.h"
#endif


namespace ugdiss
{
  using namespace std;

  // Trie type used to encode phrases.
#ifdef USE_PTRIE
  typedef PTrie<uint32_t, Wrap<uint32_t>, false> tp_trie_t;
#else
  typedef MemTable<uint32_t> tp_trie_t;
#endif

  /** Writes the branch rooted at /node/ into a sequence repository
   *  The values originally stored at the nodes serve as keys to a
   *  mapping from old values (e.g., preliminary sequence IDs) 
   *  to actual sequence Ids.
   *  @param idx the index data structure for the repository
   *  @param dat the actual repository 
   *  @param node the root of the branch to be stored
   *  @param parent position of parent node in the repository
   *  @param remap maps from old values to new sequence IDs
   *  @param curIdxPos current position in index file
   *  @param curDatPos current position in the repository (dat) file
   *  @return file position in idx and rotrie flags for the node
   */
  template<typename MTNODE, typename val_t>
  pair<filepos_type,uchar>
  toRepos(ostream& idx, ostream& dat, MTNODE& node,
	  filepos_type parent, vector<val_t>& remap,
	  filepos_type& curIdxPos, filepos_type& curDatPos);
 
  template<typename MEMTABLE,typename val_t>
  vector<val_t>
  toRepos(string bname, MEMTABLE& M, id_type numTokens, val_t highestPids);
}

#endif
