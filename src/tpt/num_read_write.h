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


// (c) 2006,2007,2008 Ulrich Germann
#ifndef __num_read_write_hh
#define __num_read_write_hh
#include <stdint.h>
#include <iostream>
#ifdef Darwin
#include <machine/endian.h>
#else
#include <endian.h>
#endif
#include <byteswap.h>
#include "tpt_typedefs.h"

namespace ugdiss {

template<typename uintNumber>
void
numwrite(std::ostream& out, uintNumber const& x)
{
#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) || (defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN)
  uintNumber y;
  switch (sizeof(uintNumber))
    {
    case 2: y = bswap_16(x); break;
    case 4: y = bswap_32(x); break;
    case 8: y = bswap_64(x); break;
    default: y = x;
    }
  out.write(reinterpret_cast<char*>(&y),sizeof(y));
#else
  out.write(reinterpret_cast<char const*>(&x),sizeof(x));
#endif
}

template<typename uintNumber>
void
numread(std::istream& in, uintNumber& x)
{
  in.read(reinterpret_cast<char*>(&x),sizeof(uintNumber));
#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) || (defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN)
  switch (sizeof(uintNumber))
    {
    case 2: x = bswap_16(x); break;
    case 4: x = bswap_32(x); break;
    case 8: x = bswap_64(x); break;
    default: break;
    }
#endif  
}

template<typename uintNumber>
char const*
numread(char const* src, uintNumber& x)
{
  // ATTENTION: THIS NEEDS TO BE VERIFIED FOR BIG-ENDIAN MACHINES!!!
  x = *reinterpret_cast<uintNumber const*>(src);
#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) || (defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN)
  switch (sizeof(uintNumber))
    {
    case 2: x = bswap_16(x); break;
    case 4: x = bswap_32(x); break;
    case 8: x = bswap_64(x); break;
    default: break;
    }
#endif  
  return src+sizeof(uintNumber);
}
} // end of namespace ugdiss
#endif
