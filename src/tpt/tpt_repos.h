// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.
#ifndef __ugMemTreeNode_toRepos_hh
#define __ugMemTreeNode_toRepos_hh

#include <iostream>
#include <vector>
#include "tpt_typedefs.h"

namespace ugdiss
{
  using namespace std;

  /** Writes the branch rooted at /node/ into a sequence repository
   *  The values originally stored at the nodes serve as keys to a
   *  mapping from old values (e.g., preliminary sequence IDs) 
   *  to actual sequence Ids.
   *  @param idx the index data structure for the repository
   *  @param dat the actual repository 
   *  @param node the root of the branch to be stored
   *  @param parent position of parent node in the repository
   *  @param remap maps from old values to new sequence IDs
   *  @return file position in idx and rotrie flags for the node
   */
  template<typename MTNODE, typename val_t>
  pair<filepos_type,uchar>
  toRepos(ostream& idx, ostream& dat, MTNODE& node, 
	  uint32_t parent, vector<val_t>& remap);
 
  template<typename MEMTABLE,typename val_t>
  vector<val_t>
  toRepos(string bname, MEMTABLE& M, id_type numTokens, val_t highestPids);

}
#endif
