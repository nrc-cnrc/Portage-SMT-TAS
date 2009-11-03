/**
 * @author Samuel Larkin
 * @file boostDef.h  Definition of matrix and vector used by Powell's algorithm.
 *
 *
 * COMMENTS:
 *
 * 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
*/
             
#ifndef __BOOST_DEF_H__
#define __BOOST_DEF_H__

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>

namespace Portage {

/// Definition of the precision of the vectors and matrices.
typedef float uPrecision;
/// Definition of vector
typedef boost::numeric::ublas::vector<uPrecision> uVector;
/// Definition of matrix
typedef boost::numeric::ublas::matrix<uPrecision> uMatrix;
/// Definition of the identity matrix
typedef boost::numeric::ublas::identity_matrix<uPrecision> uIdentityMatrix;
/// Definition of a column of a matrix for Powell's algorithm
typedef boost::numeric::ublas::matrix_column<uMatrix> uColumn;

}

#endif   // __BOOST_DEF_H__
