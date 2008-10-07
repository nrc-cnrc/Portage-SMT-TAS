/**
 * @author George Foster
 * @file matrix_solver.cc  Solve for x in the matrix equation ax = b, using LUP decomposition.
 * 
 * COMMENTS:
 *
 * Naively typed in straight from some text in my misbegotten youth. Probably
 * won't do the right thing in the tricky cases.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "matrix_solver.h"
#include "str_utils.h"
#include <cmath>

namespace Portage {

bool MatrixSolver(Uint n, double *a, double *b, double *x)
{
   Uint i, j, k, km = 0;
   double p, *r;
   Uint *pi = new Uint[n];
   double *y = new double[n];

   // Do LUP decomposition of <a> in place

   for (i = 0; i < n; i++)
      pi[i] = i;

   for (k = 0; k < n - 1; k++) {
      p = 0.0;
      for (i = k; i < n; i++)
	 if (abs(a[i*n + k]) > p) {
	    p = abs(a[i*n + k]);
	    km = i;
	 }
      if (p == 0.0) {
	 delete[] pi;
	 delete[] y;
	 return false;
      }
      swap(pi[k], pi[km]);
      for (i = 0; i < n; i++)
	 swap((a+n*k)[i], (a+n*km)[i]);
      for (i = k + 1; i < n; i++) {
	 (a+n*i)[k] /= (a+n*k)[k];
	 for (j = k + 1; j < n; j++)
	    (a+n*i)[j] -= (a+n*i)[k] * (a+n*k)[j];
      }
   }

   // solve for y, then x

   for (i = 0; i < n; i++) {
      y[i] = b[pi[i]];
      r = (a+n*i);
      for (j = 0; j < i; j++)
	 y[i] -= r[j] * y[j];
   }
   for (int i = n - 1; i >= 0; i--) {
      x[i] = y[i];
      r = (a+n*i);
      for (j = i + 1; j < n; j++)
	 x[i] -= r[j] * x[j];
      x[i] /= r[i];
   }

   delete[] pi;
   delete[] y;

   return true;
}
} // ends namespace Portage
