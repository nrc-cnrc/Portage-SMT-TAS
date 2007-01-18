/**
 * @author Aaron Tikuisis
 * @file fmax.h  fmax.
 *
 * $Id$
 * 
 * K-best Rescoring Module
 * 
 * Declaration of max_func() and max_1to1_func().  Each takes an array representing a
 * function \f$v:S x T \rightarrow R\f$, and maximizes the sum \f$\sum{s \in S} v(s, f(s))\f$ over a set of
 * functions \f$f:S \rightarrow T\f$.  max_func maximizes over the set of all functions \f$ f:S \rightarrow T\f$ while
 * max_1to1_func requires \f$v\f$ to be non-negative and maximizes over the set of all \f$(1-1)\f$
 * functions, with special circumstances for when \f$|T| < |S|\f$.
 * 
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada 
 */


#include <portage_defs.h>

namespace Portage
{
    /**
     * Given a set \f$S\f$ and a set \f$T\f$ and a function \f$v:S x T \bigcup {nil} \rightarrow [0, \inf)\f$ with
     * \f$v(s, nil) = 0\f$ for all \f$s \in S\f$, finds a function \f$f:S \rightarrow T \bigcup {nil}\f$ such that \f$f\f$ is
     * \f$(1-1)\f$ on \f$T\f$ (ie. if \f$f(s) = f(s') \in T\f$ then \f$s = s'\f$) and maximizes the value of
     * \f$\sum_{s \in S} v(s, f(s))\f$.  This uses heuristics so it may not be perfect.
     * @param fvals	A 2D array representing the function \f$v\f$; that is, \f$v(s, t) =
     * 			fvals[s][t]\f$.
     * @param numS	The size of the set \f$S\f$.
     * @param numT	The size of the set \f$T\f$.
     * @param func	The values of the optimizing function are stored in this array;
     * 			func[s] is -1 where \f$f(s) = nil\f$, and func[s] = f(s) otherwise.
     * @return	The maximal value found.
     */
    double max_1to1_func(double **fvals, Uint numS, Uint numT, int *func);
    
    /**
     * Given a set \f$S\f$ and a set \f$T\f$ and a function \f$v:S x T \bigcup {nil} \rightarrow [0, \inf)\f$ with
     * \f$v(s, nil) = 0\f$ for all \f$s \in S\f$, finds a function \f$f:S \rightarrow T \bigcup {nil}\f$ such that \f$f\f$ is
     * \f$(1-1)\f$ on \f$T\f$ (ie. if \f$f(s) = f(s') \in T\f$ then \f$s = s'\f$) and maximizes the value of
     * \f$\sum_{s \in S} v(s, f(s))\f$.  This uses heuristics so it may not be perfect.
     * @param fvals	A 2D array representing the function \f$v\f$; that is, \f$v(s, t) =
     * 			fvals[s][t]\f$.
     * @param numS	The size of the set \f$S\f$.
     * @param numT	The size of the set \f$T\f$.
     * @return	The maximal value found.
     */
    inline double max_1to1_func(double **fvals, Uint numS, Uint numT)
    {
	int func[numS];
	return max_1to1_func(fvals, numS, numT, func);
    } // max_1to1_func()
    
    /**
     * Given a set \f$S\f$ and a set \f$T\f$ and a function \f$v:S x T \rightarrow R\f$, finds a function \f$f:S->T\f$ that
     * maximizes the value of \f$\sum_{s \in S} v(s, f(s))\f$.
     * If \f$|T|\f$ == 0 then return 0 and puts 0 into each element of func.
     * @param fvals	A 2D array representing the function \f$v\f$; that is, \f$v(s, t) =
     * 			fvals[s][t]\f$.
     * @param numS	The size of the set \f$S\f$.
     * @param numT	The size of the set \f$T\f$.
     * @param func	The values of the optimizing function are stored in this array.
     * @return	The maximal value found.
     */
    double max_func(double **fvals, Uint numS, Uint numT, Uint *func);
    
    /**
     * Given a set \f$S\f$ and a set \f$T\f$ and a function \f$v:S x T \rightarrow R\f$, finds a function \f$f:S->T\f$ that
     * maximizes the value of \f$\sum_{s \in S} v(s, f(s))\f$.
     * If \f$|T|\f$ == 0 then return 0 and puts 0 into each element of func.
     * @param fvals	A 2D array representing the function \f$v\f$; that is, \f$v(s, t) =
     * 			fvals[s][t]\f$.
     * @param numS	The size of the set \f$S\f$.
     * @param numT	The size of the set \f$T\f$.
     * @return	The maximal value found.
     */
    inline double max_func(double **fvals, Uint numS, Uint numT)
    {
	Uint func[numS];
	return max_func(fvals, numS, numT, func);
    } // max_func()
}
