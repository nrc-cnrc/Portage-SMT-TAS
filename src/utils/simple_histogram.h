/**
 * @author Samuel Larkin
 * @file simple_histogram.h
 *
 * $Id$
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __SIMPLE_HISTOGRAM_H_
#define __SIMPLE_HISTOGRAM_H_

#include "portage_defs.h"
#include "gfstats.h"
#include <map>

namespace Portage {

using namespace std;

/**
 * Two way functor for classifying histogram data point into fixed size bins.
 * Implements the strategy pattern.
 */
struct Binner {
   /// Destructor.
   virtual ~Binner() {}
   /**
     * Converts a data point into a bin's index.
     * @param v data point
     * @return Retunrs the bin's index for the data point
     */
   virtual Uint whichBin(Uint v) const = 0;
   /**
     * Takes a bin's index and converts it to the lower bound data point value.
     * @param v bin's index
     * @return Returns the lower bound value for the bin v
     */
   virtual Uint toIndex(Uint v) const = 0;
};

/**
 * Two way functor for classifying histogram data point into fixed size bins.
 */
struct fixBinner : public Binner {
   const Uint binSize;
   fixBinner(Uint binSize = 10)
   : binSize(binSize)
   {
      assert(binSize > 0);
   }
   Uint whichBin(Uint v) const {
      return Uint(v) / binSize;  // get the class
   }
   Uint toIndex(Uint v) const {
      return v * binSize;
   }
};


/**
 * Two way functor for classifying histogram data point into log bins.
 * Where:
 *  - BASE is the log base value [2]
 *  - B is an offset [1].  This says values below B get their separate bins.
 */
struct logBinner : public Binner {
   const double log_base;   ///< log value of the base for base conversion
   const Uint B; ///<

   /// Default constructor.
   logBinner(Uint BASE = 2, Uint B = 0)
   : log_base(log(BASE))
   , B(B)
   {
      assert(BASE > 0);
      assert(B >= 0);
   }
   virtual Uint whichBin(Uint v) const {
      if (v > B) {
         v -= (B-1);
         // IMPORTANT Uint cast or else weird rounding errors.
         v  = Uint(floor(log(double(v) + 0.5f) / log_base));
         v += B;
      }
      return v;
   }
   virtual Uint toIndex(Uint v) const {
      double start = v;
      if (start > B) {
         start -= B;
         start = exp(start * log_base);
         start += (B-1);  // we are 0-based index
      }
      return Uint(start + 0.5f);
   }
};


/// Cumulates stats to display histogram representation of data
template <class T = Uint>
class SimpleHistogram {
   private:
      map<Uint, Uint>  bin; ///< all bins
      T sum;                ///< Sum
      T sum2;               ///< Sum square
      Uint num;             ///< number of items
      const Binner* binner;       ///< Functor to get bin index of values.

   public:
      /// Default constructor.
      /// @param binner  a strategy on how to bin values. 
      ///                Will be freed by the histogram.
      SimpleHistogram(Binner* binner = new fixBinner)
         : sum(0)
         , sum2(0)
         , num(0)
         , binner(binner)
      {
         assert(binner != NULL);
      }

      /// Destructor.
      ~SimpleHistogram() {
         if (binner != NULL) {
            delete binner;
         }
      }

      /// Resets the histogram.
      void clear() {
         num  = T(0);
         sum  = T(0);
         sum2 = T(0);
         bin.clear();
      }

      /// Get the underlying Binner.
      /// @return the the Binner.
      const Binner* getBinner() const { return binner; }

      /// Get the number of data points.
      /// @return the number of data points.
      Uint getNum() const { return num; }

      /// Get the sum of all data points.
      /// @return the sum of all data points.
      Uint getSum() const { return sum; }

      /// Get the squared sum of all data points.
      /// @return the squared sum of all data points.
      Uint getSum2() const { return sum2; }

      /// Get the number of bins.
      /// @return the number of bins in the histogram.
      Uint getNumBin() const { return bin.size(); }

      /// Get the number of data points in a bin.
      /// Usuful for debugging values.
      /// @param bin_id  bin id.
      /// @return number of element in a bin.
      Uint getNumDataPointInBin(Uint bin_id) const {
         typedef map<Uint, Uint>::const_iterator  MIT;
         MIT  binit = bin.find(bin_id);
         if (binit == bin.end())
            return 0;
         else
            return binit->second;
      }

      /// Adds data point.
      /// @param v  new data point to add
      void add(const T& v)
      {
         typedef map<Uint, Uint>::iterator  MIT;

         ++num;
         sum  += v;
         sum2 += v * v;

         // Classify into bins
         const Uint index = binner->whichBin(v);  // get the class
         //cerr << v << " => " << index << endl;  // SAM DEBUG
         MIT  binit = bin.find(index);
         if (binit == bin.end())
            bin[index] = 1;
         else
            binit->second += 1;
      }

      /**
       * Displays the histogram.
       * @param out      where to output the histogram.
       * @param identation a string of spaces for pretty printing the results.
       */
      void display(ostream& out, const char* identation = NULL) const
      {
         typedef map<Uint, Uint>::const_iterator  MIT;

         if (identation == NULL) identation = "";

         out << identation << "Count:" << num
             << " mean:" << _mean(sum, num)
             << " sdev: " << _sdev(sum, sum2, num) << endl;

         // Prints all bin and their counts
         out << identation;
         for (MIT b(bin.begin()); b!=bin.end(); ++b) {
            //out << b->first << " => ";  // SAM DEBUG
            out << binner->toIndex(b->first) << "+: " << b->second << " ";
            //out << endl;  // SAM DEBUG
         }
         out << endl;
      }
}; // ends class SimpleHistogram
}; // ends namesapce Portage

#endif // __SIMPLE_HISTOGRAM_H__
