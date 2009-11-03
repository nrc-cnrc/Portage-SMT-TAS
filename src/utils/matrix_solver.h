/**
 * @author George Foster
 * @file matrix_solver.h  Solve for x in the matrix equation ax = b, using LUP decomposition.
 * 
 * COMMENTS:
 *
 * Naively typed in straight from some text in my misbegotten youth. Probably
 * won't do the right thing in the tricky cases.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef MATRIX_SOLVER_H
#define MATRIX_SOLVER_H

#include <portage_defs.h>

namespace Portage {

/**
 * Solve for x in the matrix equation ax = b, using LUP decomposition.
 * Return false if no solution can be found.
 *
 * @param n  matrix size
 * @param a  an array[n][n] representing an n * n coefficient matrix. Note that
 *           the contents of a are changed by this function. 
 * @param b  an array[n] representing a constant vector
 * @param x  an array[n] into which the solution will be copied
 * @return true if successful
 */
 extern bool MatrixSolver(Uint n, double *a, double b[], double x[]);
 
}

#endif
