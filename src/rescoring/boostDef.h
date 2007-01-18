/**
 * @author Samuel Larkin
 * @file boostDef.h  Definition of matrix and vector used by Powell's algorithm.
 *
 *
 * COMMENTS:
 *
 * 
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
*/
             
#ifndef __BOOST_DEF_H__
#define __BOOST_DEF_H__

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>

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

#endif   // __BOOST_DEF_H__
