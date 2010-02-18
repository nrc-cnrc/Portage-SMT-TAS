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



// Memory-mapped corpus track

// (c) Ulrich Germann. All rights reserved

#include <sstream>

#include "ug_ctrack_base.h"

namespace ugdiss
{
  using namespace std;
  
  /** @return string representation of sentence /sid/ */
  string
  Ctrack::
  str(id_type sid, TokenIndex const& T) const
  {
    assert(sid < numTokens());
    id_type const* stop = sntEnd(sid);
    id_type const* strt = sntStart(sid);
    ostringstream buf;
    if (strt < stop) buf << T[*strt];
    while (++strt < stop)
      buf << " " << T[*strt];
    return buf.str();
  }

  string
  Ctrack::
  str(id_type sid, Vocab const& V) const
  {
    assert(sid < numTokens());
    id_type const* stop = sntEnd(sid);
    id_type const* strt = sntStart(sid);
    ostringstream buf;
    if (strt < stop) buf << V[*strt].str;
    while (++strt < stop)
      buf << " " << V[*strt].str;
    return buf.str();
  }

  /** @return length of sentence /sid/ */
  size_t 
  Ctrack::
  sntLen(size_t sid) const
  {
    return sntEnd(sid) - sntStart(sid);
  }

}
