/**
 * @author George Foster
 * @file portage_defs.h  Basic definitions used in Portage.
 * 
 * 
 * COMMENTS: 
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
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

/// constant that represents infinity for Portage.
extern const double INFINITY;  


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
