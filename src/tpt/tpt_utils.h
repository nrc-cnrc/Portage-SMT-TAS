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

#ifndef   	TPT_UTILS_H
#define   	TPT_UTILS_H

#include <boost/iostreams/device/mapped_file.hpp>
#include <string>
#include "tpt_typedefs.h"

namespace bio = boost::iostreams;

namespace ugdiss {
/// @return the size of file fname.
uint64_t getFileSize(const std::string& fname);

/**
 * Open a memory mapped file for reading, outputting an error if not successful.
 * @param mfs   memory mapped file source
 * @param fname name of the file to open
 */
void open_mapped_file_source(bio::mapped_file_source& mfs, const string& fname);

} // namespace ugdiss

#endif	    // !TPT_UTILS_H
