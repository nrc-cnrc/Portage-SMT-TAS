/**
 * @author George Foster
 * @file ls_poly_fit.h  Least squares polynomial curve fitting.
 * 
 * 
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / 
 * Copyright 2005, National Research Council of Canada
 */

#ifndef LS_POLY_FIT_H
#include <portage_defs.h>

namespace Portage {

/**
 * Fit a set of data points using a least squares polynomial.
 * @param m number of points
 * @param x array[m] of x coordinates
 * @param y array[m] of y coordinates
 * @param n the order of the polynomial to fit
 * @param c array[n+1] of results; the fitted polynomial is: 
 *  y = c[0] x^0 + c[1] x^1 + ... + c[n] x^n
 * @param w if not NULL, an array[m] of weights on the data points
 * @return Returns 
 */
extern bool LSPolyFit(Uint m, double x[], double y[], Uint n, double c[], double *w = NULL);

/**
 * Calculate the value of a polynomial at a given point x.
 * @param x 
 * @param n
 * @param c
 * @return Returns
 */
extern double calcPoly(double x, Uint n, const double c[]);

}

#endif
