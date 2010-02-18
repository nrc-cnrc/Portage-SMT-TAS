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



// Basic type definitions for code related to tightly packed tries
// (c) 2006,2007,2008 Ulrich Germann

// This file defines basic types shared by various modules and classes
 
#ifndef   	TPT_TYPEDEFS_H
#define   	TPT_TYPEDEFS_H

#include "tpt_error.h"

#include <stdint.h>
// #define IS_32_BIT (sizeof(unsigned long)==4)
// #define IS_64_BIT (sizeof(unsigned long)==8)

// ID_TYPE      is the integer type used for token ids
// COUNT_TYPE   is the integer type used for token counts
// FILEPOS_TYPE is the integer type used for file offsets
// default is 32 bits for all three
#ifndef ID_TYPE
// #define ID_TYPE unsigned int
#define ID_TYPE uint32_t 
#endif

#ifndef COUNT_TYPE
// #define COUNT_TYPE unsigned int
#define COUNT_TYPE uint32_t
#endif

#ifndef FILEPOS_TYPE
// #define FILEPOS_TYPE unsigned long long int
#define FILEPOS_TYPE uint64_t
// we need 64 bit to support files > 2GB
#endif

namespace ugdiss
{
  typedef ID_TYPE      id_type;
  typedef COUNT_TYPE   count_type;
  typedef FILEPOS_TYPE filepos_type;
  typedef unsigned char uchar;
}
// #define bufsize 1000000

#endif	    // !TPT_TYPEDEFS_H
