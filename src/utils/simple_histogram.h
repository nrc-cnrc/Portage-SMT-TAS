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
#include <vector>
#include <map>

namespace Portage {

using namespace std;

/// Cumulates stats to display histogram representation of data
template <class T = Uint>
struct SimpleHistogram {

   map<Uint, Uint>  bin; ///< all bins
   const Uint binSize;   ///< size of each bin
   T sum;                ///< Sum
   T sum2;               ///< Sum square
   Uint num;             ///< number of items
   
   SimpleHistogram(const Uint binSize = 10)
   : binSize(binSize)
   , sum(0)
   , sum2(0)
   , num(0)
   {
      assert(binSize>0);
   }

   /**
    * Adds data point.
    * @param v  new data point to add
    */
   void add(const T& v)
   {
      typedef map<Uint, Uint>::iterator  MIT;

      ++num;
      sum += v;
      sum2 += v * v;

      // Classify into bins
      const Uint index = Uint(v) / binSize;  // get the class 
      MIT  binit = bin.find(index);
      if (binit == bin.end())
         bin[index] = 1;
      else
         binit->second += 1;
   }
   
   /**
    * Displays the histogram.
    * @param out      where to output the histogram
    */
   void display(ostream& out, const char* identation = NULL) const
   {
      typedef map<Uint, Uint>::const_iterator  MIT;

      if (identation == NULL) identation = "";

      out << identation << "There are " << num << " entries" << endl;
      out << identation << "mean: " << _mean(sum, num) << endl;
      out << identation << "var: "  << _var(sum, sum2, num) << endl;
      out << identation << "sdev: " << _sdev(sum, sum2, num) << endl;
      out << identation << "bin's size: " << binSize << endl;

      // Prints all bin and their counts
      out << identation;
      for (MIT b(bin.begin()); b!=bin.end(); ++b) {
         const Uint start = b->first * binSize;
         out << start << "+: " << b->second << " ";
      }
      out << endl;
   }
}; // ends class SimpleHistogram
}; // ends namesapce Portage

#endif // __SIMPLE_HISTOGRAM_H__
