/**
 * @author Samuel Larkin
 * @file PERstats.h  PER stats object for powell's training.
 * 
 * $Id$
 * 
 * COMMENTS: per stats for powell.  Equivalent to BLEUstats
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#ifndef __PER_STATS_H__
#define __PER_STATS_H__

#include "ErrorRateStats.h"
#include "wer.h"

namespace Portage {

/// Wrapper to handle PER inside powell.
struct PERstats : public ErrorRateStats {

   ///
   typedef ErrorRateStats Parent;

   /// Callable entity for booststrap confidence interval
   typedef Parent::CIcomputer<PERstats> CIcomputer;

   /// Default constructor.
   PERstats()
   : ErrorRateStats()
   {}

   PERstats(const Translation& translation, const References& refs) {
      init(translation, refs);
   }

   PERstats(const Tokens& translation, const References& refs) {
      init(translation, refs);
   }

   virtual Uint calculateDistance(const Tokens& translation, const Tokens& ref) const {
      return find_mPER(translation, ref);
   }

   static const char* const name() {
      return "PER";
   }

   /**
    * Adds together the statistics of two PERstats objects, returning
    * the result.
    * @relates PERstats
    * @param other  right-hand side operand
    * @return Returns a PERstats containing this + other
    */
   PERstats& operator+=(const PERstats& other) {
      this->Parent::operator+=(other);
      return *this;
   }

   /**
    * Finds the difference in statistics between two PERstats objects.
    * @relates PERstats
    * @param other  right-hand side operand
    * @return Returns a PERstats containing this - other
    */
   PERstats& operator-=(const PERstats& other) {
      this->Parent::operator-=(other);
      return *this;
   }

   /**
    * Finds the difference in statistics between two PERstats objects.
    * @relates PERstats
    * @param other  right-hand side operand
    * @return Returns a PERstats containing s1 - s2
    */
   PERstats operator+(const PERstats& other) const {
      PERstats tmp(*this);
      tmp += other;
      return tmp;
   }

   /**
    * Finds the difference in statistics between two PERstats objects.
    * @relates PERstats
    * @param other  right-hand side operand
    * @return Returns a PERstats containing s1 - s2
    */
   PERstats operator-(const PERstats& other) const {
      PERstats tmp(*this);
      tmp -= other;
      return tmp;
   }
};  // ends struct PERstats

PERstats operator*(PERstats s, double c) {
   s._changes       *= c;
   s._reflen        *= c;
   s._source_length *= c;
   return s;
}

}   // ends namespace Portage

#endif // __PER_STATS_H__
