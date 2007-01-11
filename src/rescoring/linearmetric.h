/**
 * @author Aaron Tikuisis
 * @file linearmetric.h  Definition a linear metric for line max.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l.information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
*/

#ifndef LINEARMETRIC_H
#define LINEARMETRIC_H

namespace Portage
{
    /**
     * A class which is used to represent a score which is additive
     */
    class LinearMetric
    {
	public:
	    double val;  ///< value
            /// Constructor from value.
            /// @param v  value
	    LinearMetric(double v): val(v) {}
            /// Empty constructor, sets value to 0.
	    LinearMetric(): val(0) {}
            /// Get the value.
            /// @return Returns the value.
	    double score() { return val; }
            /// Outputs value to debugger.
	    void debugoutput() { RSC_DEBUG("Value = " << val); }
            /// Outputs the value to a stream.
            /// @param out  output stream.
	    void output(ostream &out = cout) { out << "Value = " << val; }
    };
    
    /**
     * Substraction operator for LinearMetric.
     * @relates LinearMetric
     * @param a  left-hand side operand
     * @param b  right-hand side operand
     * @return Returns the difference between a and b
     */
    LinearMetric operator-(const LinearMetric &a, const LinearMetric &b)
    {
	LinearMetric c(a.val - b.val);
	return c;
    }
    
    /**
     * Addition operator for LinearMetric.
     * @relates LinearMetric
     * @param a  left-hand side operand
     * @param b  right-hand side operand
     * @return Returns the sum of a and b
     */
    LinearMetric operator+(const LinearMetric &a, const LinearMetric &b)
    {
	LinearMetric c(a.val + b.val);
	return c;
    }
}


#endif // LINEAR_METRIC_H
