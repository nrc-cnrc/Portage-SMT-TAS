/**
 * @author George Foster
 * @file portage_defs.h  Basic definitions used in Portage.
 * 
 * 
 * COMMENTS: 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef PORTAGE_DEFS_H
#define PORTAGE_DEFS_H

#include <assert.h>

#ifdef INFINITY
#undef INFINITY
#endif

namespace Portage {

using namespace std;

/// @name Portage basic types.
//@{
/// Portage redefinition of basic types.
typedef unsigned char Uchar;
typedef unsigned short Ushort;
typedef unsigned int Uint;
typedef unsigned long Ulong;
//@}

#if __WORDSIZE == 64
typedef unsigned long       Uint64;
typedef long                Int64;
#else
typedef unsigned long long  Uint64;
typedef long long           Int64;
#endif

/// constant that represents infinity for Portage.
extern const double INFINITY;  

/// Non-flushing endl.  Should be used instead of "\n", so that it can be easy
/// to change, in one place only, if we do a port to a different platform.
static const char* const nf_endl = "\n";

#ifndef ARRAY_SIZE
/// Return the number of elements in array a.
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))
#endif

/**
 * Returns the maximum between a and b.
 *
 * Assuming that a and b are comparable (using <), returns the maximum of the two.  This
 * is safer than the macro version in case a or b are obtained via using function calls.
 *
 * @param a	The first item to compare.
 * @param b	The second item to compare.
 * @return	The maximum of a or b.
 */
template <class T, class S>
inline T max(T a, S b) { return (a > T(b)) ? a : T(b); }

/**
 * Returns the minimum between a and b.
 *
 * Assuming that a and b are comparable (using <), returns the minimum of the two.  This
 * is safer than the macro version in case a or b are obtained via using function calls.
 *
 * @param a	The first item to compare.
 * @param b	The second item to compare.
 * @return	The minimum of a or b.
 */
template <class T, class S>
inline T min(T a, S b) { return (a < T(b)) ? a : T(b); }

}
#endif
