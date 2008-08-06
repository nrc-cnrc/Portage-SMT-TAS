/**
 * @author Samuel Larkin
 * @file ErrorRateStats.h
 * 
 * 
 * COMMENTS: Base class for wer/per stats.  Equivalent to BLEUstats
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#ifndef __ERROR_RATE_STATS_H__
#define __ERROR_RATE_STATS_H__

#include "basic_data_structure.h"
#include <math.h>

namespace Portage {

/// Base class for error rate stats.
struct ErrorRateStats {
   Uint _changes;   ///< number of changes required to change the hypothese into the reference.
   double _reflen;  ///< cumulative references' length.

   /// Default constructor.
   ErrorRateStats()
   : _changes(0)
   , _reflen(0.0f)
   { }

   /**
    * Returns the least amount of change over the cumulative references' lenght.
    * @return Returns the least amount of change over the cumulative references' lenght.
    */
   double ratio() const {
      assert(_reflen > 0.0f);
      return double(_changes) / _reflen; 
   }

   /**
    * Calculates a score for powell, i.e., with larger being better.
    * @return Returns a score (-log(ratio)).
    */
   double score() const {
      return -ratio(); 
      //return -log(ratio()); 
   }

   /**
    * Basic display.
    * @param out  Where to output the stats.
    */
   void write(ostream &out) const {
      out << "Changes: " << _changes << " RefLen: " << _reflen << endl;
   }

   static double convertToDisplay(double value) {
      return -value;
   }

   protected:
      /**
       * Calculates the distance between a translation and its reference.
       * @param translation  words of the translation.
       * @param ref          words of the reference.
       * @return Returns some distance between the translation and its reference.
       */
      virtual Uint calculateDistance(const Tokens& translation, const Tokens& ref) const = 0;

      /**
       * Calculates the distance between a translation and its references.
       * @param translation  a translation.
       * @param ref          its references.
       * @return Returns some distance between the translation and its references.
       */
      void init(const Translation& translation, const References& refs) {
         init(translation.getTokens(), refs);
      }

      /**
       * Calculates the distance between a translation and its references.
       * @param translation  words of the translation.
       * @param ref          its references.
       * @return Returns some distance between the translation and its references.
       */
      void init(const Tokens& translation, const References& refs) {
         const Uint numRefs(refs.size());
         Uint least(0);

         for (Uint i(0); i<numRefs; ++i) {
            const Tokens refWords = refs[i].getTokens();
            _reflen += refWords.size();

            const Uint cur = calculateDistance(translation, refWords);

            if (i == 0 || cur < least)
               least = cur;
         } // for
         _changes = least;
         _reflen /= numRefs;
      }

   protected:
   /**
    * Finds the difference in statistics between two ErrorRateStats objects.
    * @relates ErrorRateStats
    * @param other  right-hand side operand
    * @return Returns a ErrorRateStats containing this - other
    */
   ErrorRateStats& operator-=(const ErrorRateStats& other) {
      _changes += other._changes;
      _reflen  += other._reflen;
      return *this;
   }

   /**
    * Adds together the statistics of two ErrorRateStats objects, returning
    * the result.
    * @relates ErrorRateStats
    * @param other  right-hand side operand
    * @return Returns a ErrorRateStats containing this + other
    */
   ErrorRateStats& operator+=(const ErrorRateStats& other) {
      _changes += other._changes;
      _reflen  += other._reflen;
      return *this;
   }

   public:
   /// Callable entity for booststrap confidence interval
   template<class ScoreStats>
   struct CIcomputer
   {
      /// Define what is an iterator for a CIcomputer.
      typedef typename vector< ScoreStats >::const_iterator iterator;

      /**
       * Returns total least modifications / total reference length.
       * @param begin  start iterator.
       * @param end    end iterator.
       * @return Returns total least modifications / total reference length.
       */
      double operator()(iterator begin, iterator end) {
         ScoreStats total;
         return std::accumulate(begin, end, total).ratio();
      }
   };
}; // ends class ErrorRateStats
} // ends namespace Portage

#endif // __ERROR_RATE_STATS_H__
