/**
 * @author Aaron Tikuisis
 * @file linearmetric.h  Definition a linear metric for line max.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
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
         /**
          * Substraction operator for LinearMetric.
          * @relates LinearMetric
          * @param other  right-hand side operand
          * @return Returns the difference between a and b
          */
         LinearMetric& operator-=(const LinearMetric &other)
         {
            val -= other.val;
            return *this;
         }

         /**
          * Addition operator for LinearMetric.
          * @relates LinearMetric
          * @param other  right-hand side operand
          * @return Returns the sum of a and b
          */
         LinearMetric& operator+=(const LinearMetric &other)
         {
            val += other.val;
            return *this;
         }
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
      return LinearMetric(a.val - b.val);
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
      return LinearMetric(a.val + b.val);
   }
}


#endif // LINEAR_METRIC_H
