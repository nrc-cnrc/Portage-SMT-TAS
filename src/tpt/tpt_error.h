/**
 * @author Darlene Stewart
 * @file tpt_error.h Exception, error and debug handling in the tpt module.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef   	TPT_ERROR_H
#define   	TPT_ERROR_H

// IN_PORTAGE determines whether the tpt is part of Portage or stand-alone
#ifndef IN_PORTAGE
#define IN_PORTAGE 1
#endif

// Make the Portage exception dump macros to trap exceptions available.
#if IN_PORTAGE
#include "exception_dump.h"
using namespace Portage;
#else
#define MAIN(argc, argv) main(int argc, const char* const argv[])
#define END_MAIN
#endif  // IN_PORTAGE


//#define DEBUG_TPT
// Execute the expression (usually assert or output) if in debug mode.
#ifdef DEBUG_TPT
   #define TPT_DBG(expr) expr
#else
   #define TPT_DBG(expr)
#endif


namespace ugdiss {

extern const char* const efatal;
extern const char* const ewarn;

/**
 * Output stream I/O manipulator to add a newline and exit with error code 1.
 * @param os output stream
 * @returns output stream
 */
ostream& exit_1(ostream& os);

} // namespace ugdiss

#endif	    // !TPT_ERROR_H
