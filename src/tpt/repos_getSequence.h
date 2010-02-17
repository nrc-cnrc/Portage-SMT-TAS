// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.

#ifndef __repos_helpers_hh
#define __repos_helpers_hh
// auxiliary functions for handling repositories
// (c) Ulrich Germann

#include <iostream>
#include <vector>
#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"

namespace ugdiss
{
  using namespace std;

  vector<id_type> 
  getSequence(char const* p);

  vector<string> 
  getSequence(char const* p,TokenIndex& tdx);

  string
  getSequenceAsString(char const* p,TokenIndex& tdx);

  vector<id_type> 
  getSequence(istream& in, filepos_type pos);

  // Optimized versions of all the above, minimizing allocation of transient
  // memory for the vectors returned
  void
  getSequence(vector<id_type>& seq, char const* p);

  void
  getSequence(vector<string>& seq, char const* p,TokenIndex& tdx);

  void
  getSequenceAsString(string& seq, char const* p,TokenIndex& tdx);

  void
  getSequence(vector<id_type>& seq, istream& in, filepos_type pos);

}  
#endif
