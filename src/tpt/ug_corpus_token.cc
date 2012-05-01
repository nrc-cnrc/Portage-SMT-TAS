#include "ug_corpus_token.h"
// Simple wrapper around integer IDs for use with the Ctrack and TSA template classes.
// (c) 2007-2009 Ulrich Germann

namespace ugdiss
{
  /* inlined in the declaration
  id_type
  SimpleWordId::
  id() const 
  { 
    return theID; 
  }
  */

  int
  SimpleWordId::
  cmp(SimpleWordId other) const
  {
    return (theID < other.theID    ? -1
            : theID == other.theID ?  0
            :                         1);
  }

  /* inlined in the declaration
  SimpleWordId::
  SimpleWordId(id_type id)
  {
    theID = id;
  }
  */

}
