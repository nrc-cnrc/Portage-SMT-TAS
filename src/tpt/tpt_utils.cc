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


/**
 * @author Darlene Stewart
 * @file tpt_utils.h Utility functions for the tpt module.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */
#include <sys/stat.h>
#include <cstring>
#include <cerrno>
#include "tpt_utils.h"
#include "tpt_typedefs.h"

#if IN_PORTAGE
   #include "file_utils.h"
#endif

namespace ugdiss {

#if defined(Darwin) || defined(CYGWIN)
#define stat64    stat
#endif

uint64_t
getFileSize(const std::string& fname)
{
  struct stat64 buf;
  if (stat64(fname.c_str(),&buf) < 0)
    return -1;
  return buf.st_size;
}

void
open_mapped_file_source(bio::mapped_file_source& mfs, const string& fname)
{
#if IN_PORTAGE
  error_unless_exists(fname, false, "memory mapped");
#endif
  try {
    mfs.open(fname);
    if (!mfs.is_open()) throw std::exception();
  } catch(std::exception& e) {
    cerr << efatal << "Unable to open memory mapped file '" << fname << "' for reading."
         << endl << e.what()
         << endl << "errno=" << errno << ": " << strerror(errno)
         << exit_1;
  }
}

} // namespace ugdiss
