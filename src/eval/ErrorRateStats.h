/**
 * @author Samuel Larkin
 * @file ErrorRateStats.h 
 * @brief Base class for PER/WER stats.
 * 
 * $Id$
 * 
 * COMMENTS: Base class for wer/per stats.  Equivalent to BLEUstats
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#ifndef __ERROR_RATE_STATS_H__
#define __ERROR_RATE_STATS_H__

#include "basic_data_structure.h"
#include <cmath>

namespace Portage {

/// Base class for error rate stats.
struct ErrorRateStats {
   Uint _changes;        ///< number of changes required to change the hypothese into the reference.
   double _reflen;       ///< cumulative references' length.
   Uint _source_length;  ///< source sentence lenght.

   /// Default constructor.
   ErrorRateStats()
   : _changes(0)
   , _reflen(0.0f)
   , _source_length(0)
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
    * @return Returns a score.
    */
   double score() const {
      return -ratio(); 
   }

   void output(ostream& out = cout) const {
      out << "(total dist: " << _changes
         << ", hyp. length: " << _source_length
         << ", avg. ref. length: " << _reflen
         << ")" << endl;
      out << "Score: " << score() << endl;
   }

   /**
    * Basic display.
    * @param out  Where to output the stats.
    */
   void write(ostream& out) const {
      out << _changes << '\t' << _reflen << endl;
   }

   /**
    * Convert "internal" value to display format.
    * @param value internal value (eg, from score()) 
    * @return display value.
    */
   static double convertToDisplay(double value) {
      return -value;
   }
   /**
    * Convert display format to internal format.
    * @param value display value
    * @return internal value.
    */
   static double convertFromDisplay(double value) {
      return -value;
   }

   /**
    * Convert "internal" score value to pnorm format: in [0,1],
    * higher scores are better. WER and PER, and in the range [0, +infinity), 
    * with they score being in the range (-infinity, 0], so the best choice is
    * a sigmoid inspired function that maps WER=0 to 1, WER=1 to a value "near
    * 0" (.238), and values of WER > 1 to values approaching zero
    * asymptotically: 2/(1+exp(2*WER)).  This function is inspired from
    * sigmoids used in neural networks: 1/(1+exp(x)), but it is adjusted to
    * make WER=0 yield PNORM=1, and to have PNORM close to 1-WER when WER is in
    * [0,0.5].  Is is closer to max(0, 1-WER) than exp(-WER) would be, which is
    * desirable.
    * @param value internal value (eg, from score()) 
    * @return pnorm value
    */
   static double convertToPnorm(double value) {
      return double(2.0) / (1.0 + exp(- 2 * value));
   }
   /**
    * Convert "internal" score from pnorm format: in [0,1],
    * Reverses the operation described in convertToPnorm().
    * @param value  pnorm value (eg, from convertToPnorm()) 
    * @return internal value.
    */
   static double convertFromPnorm(double value) {
      return -0.5 * log(2.0 / value - 1.0);
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
    * @param refs         its references.
    */
   void init(const Translation& translation, const References& refs) {
      init(translation.getTokens(), refs);
   }

   /**
    * Calculates the distance between a translation and its references.
    * @param translation  words of the translation.
    * @param refs         its references.
    */
   void init(const Tokens& translation, const References& refs) {
      const Uint numRefs(refs.size());
      Uint least(0);

      _source_length = translation.size();

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
      _changes -= other._changes;
      _reflen  -= other._reflen;
      _source_length -= _source_length;
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
      _source_length += _source_length;
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
}; // class ErrorRateStats

} // namespace Portage

#endif // __ERROR_RATE_STATS_H__
