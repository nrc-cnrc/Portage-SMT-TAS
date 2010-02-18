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

#ifndef __ug_mm_ctrack
#define __ug_mm_ctrack

#include <string>
#include <boost/iostreams/device/mapped_file.hpp>

#include "tpt_typedefs.h"
#include "ug_ctrack_base.h"

namespace ugdiss
{
  using namespace std;
  namespace bio=boost::iostreams;

  class mmCtrack : public Ctrack
  {
  private:
    bio::mapped_file_source file;
    id_type const* data;  // pointer to first word of first sentence
    id_type const* index; /* pointer to index (change data type for corpora 
			   * of more than four billion words)
			   */
  public:
    mmCtrack(string fname);
    mmCtrack();
    id_type const* sntStart(size_t sid) const; // return pointer to beginning of sentence
    id_type const* sntEnd(size_t sid) const;   // return pointer to end of sentence
    size_t size() const; // return size of corpus (in number of sentences)
    size_t numTokens() const;
    void open(string fname);
    // string str(id_type sid, TokenIndex const& T) const;
  };
}
#endif
