// Memory-mapped corpus track

// (c) Ulrich Germann. All rights reserved
// Licensed to NRC under special agreement.

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
