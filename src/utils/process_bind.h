// $Id$
/**
 * @author Samuel Larkin
 * @file process_bind.h
 * @brief Bind your program to pgm1 and stop your program if pgm1 disappear.
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef __PROCESS_BIND_H__
#define __PROCESS_BIND_H__

#include <sys/types.h>  // for pid_t

namespace Portage {

/**
 * Watches, in the background, for the presence of a pid, if not present, does a
 * exit(45).
 * NOTE: this function will start a thread and return immediately.
 * WARNING: the variable containing the pid must exist long enough for the
 * thread to start and read the value.  This is why the pid is passed as const
 * reference to this function.
 * @param pid  the pid to look for.
 */
void process_bind(pid_t& pid);

} // ends naspace Portage


#endif  // __PROCESS_BIND_H__
