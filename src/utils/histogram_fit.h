/**
 * @author George Foster
 * @file histogram_fit.h  Histogram data fitting.
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

#ifndef HISTOGRAM_FIT_H
#include <portage_defs.h>

namespace Portage {

/**
 * Fit a set of data points using a histogram.
 * @param m number of points; must be > 1
 * @param x array[m] of x coordinates (modified in place)
 * @param y array[m] of y coordinates (modified in place)
 * @param n the number of histogram boundaries to use; must be < m
 * @param b output array[n] of segment boundaries
 * @param v output array[n+1] of segment y values
 */
extern void histoFit(Uint m, double x[], double y[], Uint n, double b[], double v[]);

/**
 * Calculate the value of a histogram at a given point
 * @param x point at which to calculate value
 * @param n number of histogram boundaries
 * @param b array[n] of segment boundaries
 * @param v array[n+1] of segment y values
 * return estimated y(x)
 */
extern double histoVal(double x, Uint n, const double b[], const double v[]);

}

#endif
