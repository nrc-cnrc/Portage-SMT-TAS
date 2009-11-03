/**
 * @author Samuel Larkin
 * @file WERstats.h  WER stats object for powell's training.
 * 
 * $Id$
 * 
 * COMMENTS: wer stats for powell.  Equivalent to BLEUstats
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#ifndef __WER_STATS_H__
#define __WER_STATS_H__

#include "ErrorRateStats.h"
#include "wer.h"

namespace Portage {

/// Wrapper to handle WER inside powell.
struct WERstats : public ErrorRateStats {

   ///
   typedef ErrorRateStats Parent;

   /// Callable entity for booststrap confidence interval
   typedef Parent::CIcomputer<WERstats> CIcomputer;

   /// Default constructor.
   WERstats() 
   : ErrorRateStats()
   {}

   WERstats(const Translation& translation, const References& refs) {
      init(translation, refs);
   }
   WERstats(const Tokens& translation, const References& refs) {
      init(translation, refs);
   }

   virtual Uint calculateDistance(const Tokens& translation, const Tokens& ref) const {
      return find_mWER(translation, ref);
   }

   static const char* const name() {
      return "WER";
   }

   /**
    * Adds together the statistics of two WERstats objects, returning
    * the result.
    * @relates WERstats
    * @param other  right-hand side operand
    * @return Returns a WERstats containing this + other
    */
   WERstats& operator+=(const WERstats& other) {
      this->Parent::operator+=(other);
      return *this;
   }

   /**
    * Finds the difference in statistics between two WERstats objects.
    * @relates WERstats
    * @param other  right-hand side operand
    * @return Returns a WERstats containing this - other
    */
   WERstats& operator-=(const WERstats& other) {
      this->Parent::operator-=(other);
      return *this;
   }


   /**
    * Finds the difference in statistics between two WERstats objects.
    * @relates WERstats
    * @param other  right-hand side operand
    * @return Returns a WERstats containing s1 - s2
    */
   WERstats operator+(const WERstats& other) const {
      WERstats tmp(*this);
      tmp += other;
      return tmp;
   }

   /**
    * Finds the difference in statistics between two WERstats objects.
    * @relates WERstats
    * @param other  right-hand side operand
    * @return Returns a WERstats containing s1 - s2
    */
   WERstats operator-(const WERstats& other) const {
      WERstats tmp(*this);
      tmp -= other;
      return tmp;
   }
};

}

#endif // __WER_STATS_H__
