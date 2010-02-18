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
