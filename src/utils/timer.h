/**
 * @author Evan Stratford
 * @file timer.h  Simple timer class.
 * 
 * COMMENTS: 
 *
 * A Timer is a medium-resolution timer based off of clock() in <ctime>,
 * intended for measuring CPU (NOT real) time elapsed during program execution.
 * Do NOT use this to replace a proper profiler; its use should be limited to
 * statistics collection only.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef TIMER_H
#define TIMER_H

#include <ctime>
#include "portage_defs.h"

namespace Portage {

class Timer {
   clock_t start;
   // allow injection of other timing functions for testing
   virtual clock_t getTime() const { return clock(); }
   // disallow copying, which would be semantically weird for running timers
   Timer& operator=(const Timer& rhs);
   Timer(const Timer& rhs);
public:
   Timer() : start(0) {}
   /// start the timer.
   void reset() { start = getTime(); }
   /// Get the time so far, in ticks elapsed
   clock_t ticksElapsed() const { return getTime() - start; }
   /// Get the time so far, in seconds (truncated)
   clock_t secsElapsed() const { return ticksElapsed()/CLOCKS_PER_SEC; }
   /// Get the time so far, in fractions of seconds - on Linux, the resolution
   /// seems to be 0.01 seconds.
   double secsElapsed(bool) const { return double(ticksElapsed())/CLOCKS_PER_SEC; }
}; // class Timer

} // namespace Portage

#endif
