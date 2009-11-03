/**
 * @author George Foster
 * @file gfstats.cc Simple stats algorithms.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "gfstats.h"

using namespace Portage;

// Code snitched from somewhere on the web. Seems to work ok: I checked it
// against a 5 sig fig (on the output) table at 10 randomly-chosen points, and
// all matched.

/* Citation for this code later found on the web:
One-Dimensional Normal Law.
Cumulative distribution function.
Abramowitz, Milton et Stegun,
Handbook of MathematicalFunctions, 1968,
Dover Publication, New York, page 932 (26.2.18).
Precision 10^-7
Maximum absolute error: 7.5e^-8
*/
double Portage::stdNormalCDF(double x)
{
  const double b1 =  0.319381530;
  const double b2 = -0.356563782;
  const double b3 =  1.781477937;
  const double b4 = -1.821255978;
  const double b5 =  1.330274429;
  const double p  =  0.2316419;
  const double c  =  0.39894228;

  if(x >= 0.0) {
     double t = 1.0 / ( 1.0 + p * x );
     return (1.0 - c * exp( -x * x / 2.0 ) * t *
             ( t *( t * ( t * ( t * b5 + b4 ) + b3 ) + b2 ) + b1 ));
  } else {
     double t = 1.0 / ( 1.0 - p * x );
     return ( c * exp( -x * x / 2.0 ) * t *
              ( t *( t * ( t * ( t * b5 + b4 ) + b3 ) + b2 ) + b1 ));
  }
}
