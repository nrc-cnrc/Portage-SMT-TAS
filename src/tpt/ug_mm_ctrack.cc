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

#include "ug_mm_ctrack.h"
#include "tpt_pickler.h"
#include "tpt_utils.h"

namespace ugdiss
{
  using namespace std;
  
  id_type const* 
  mmCtrack::
  sntStart(size_t sid) const // return pointer to beginning of sentence
  {
    check_sid(sid);
    return data+index[sid];
  }

  id_type const* 
  mmCtrack::
  sntEnd(size_t sid) const // return pointer to end of sentence
  {
    // Note: index has one extra entry for the end of the last sentence.
    check_sid(sid);
    return data+index[sid+1];
  }
  
  size_t 
  mmCtrack::
  size() const // return size of corpus (in number of sentences)
  {
    return numSent;
  }

  size_t 
  mmCtrack::
  numTokens() const // return size of corpus (in number of words)
  {
    return numWords;
  }

  mmCtrack::
  mmCtrack()
  {
    data = index = NULL;
    numSent = numWords = 0;
  }

  mmCtrack::
  mmCtrack(string fname)
  {
    open(fname);
  }

  void 
  mmCtrack::
  open(string fname)
  {
    open_mapped_file_source(file, fname);
    filepos_type idxOffset;
    char const* p = file.data();
    uint64_t fSize = getFileSize(fname);
    if (fSize < sizeof(filepos_type)+sizeof(id_type)+sizeof(id_type))
       cerr << efatal << "Bad memory mapped corpus track file '" << fname << "'."
            << exit_1;
    p = numread(p,idxOffset);
    p = numread(p,numSent);
    p = numread(p,numWords);
    data  = reinterpret_cast<id_type const*>(p);
    index = reinterpret_cast<id_type const*>(file.data()+idxOffset);
    // Include the end index for the last sentence when computing file size.
    if (fSize != idxOffset + numSent*sizeof(id_type) + sizeof(id_type))
       cerr << efatal << "Bad memory mapped corpus track file '" << fname << "'."
            << exit_1;
  }
}
