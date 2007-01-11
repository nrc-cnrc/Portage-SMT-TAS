/**
 * @author George Foster, Eric Joanis
 * @file gfmath.h Simple math stuff.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#ifndef GFMATH_H
#define GFMATH_H

#include <cmath>
#include "portage_defs.h"

namespace Portage {

extern double log2(double x);
extern double exp2(double x);

// Round up to the smallest power of 2 >= x
extern Uint next_power_of_2(Uint x);

}
#endif
