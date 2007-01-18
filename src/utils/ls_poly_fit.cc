/**
 * @author George Foster
 * @file ls_poly_fit.cc  Least squares polynomial curve fitting.
 * 
 * 
 * COMMENTS:
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <iostream>
#include <vector>
#include "ls_poly_fit.h"
#include "matrix_solver.h"

using namespace Portage;

bool Portage::LSPolyFit(Uint m, double x[], double y[], Uint n, double c[], double *w)
{
   if (n >= m)
      return false;
   
   Uint ne = 2 * n + 1;		/* number of exponents in x */
   Uint nc = n + 1;		/* number of coefficients in polynomial */

   vector<double> a(nc*nc, 0.0);
   vector<double> b(nc, 0.0);
   vector<double> xsums(ne, 0.0);

   double *r, *p;
   double xx, xy;
   Uint i, j;
   bool status;

   /* fill in xsums and b */

   for (i = 0; i < m; i++) {	/* for each data point */
      for (xx = (w ? w[i] : 1), j = 0; j < ne; j++, xx *= x[i]) /* for each power j */
	 xsums[j] += xx;
      for (xy = (w ? w[i]*y[i] : y[i]), j = 0; j < nc; j++, xy *= x[i]) /* for each row j */
	 b[j] += xy;
   }
   /* load main matrix <a> */

   for (i = 0; i < nc; i++) {	/* for each row */
      r = &a[0] + nc * i;		/* location in a */
      p = &xsums[0] + i;		/* location in xsums */
      for (j = 0; j < nc; j++)	/* for each column */
	 *r++ = *p++;
   }

   /* solve for the coefficients */
   status = MatrixSolver(nc, &a[0], &b[0], c);

   return status;
}

double Portage::calcPoly(double x, Uint n, const double c[])
{
   double sum = 0;
   for (Uint i = 0; i <= n; ++i)
      sum = c[n-i] + x * sum;
   return sum;
}
