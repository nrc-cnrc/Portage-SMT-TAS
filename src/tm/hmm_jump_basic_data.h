/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_basic_data.h Basic structure shared by more than one strategy.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef HMM_JUMP_BASIC_DATA_H
#define HMM_JUMP_BASIC_DATA_H

#include "portage_defs.h"
#include "bivector.h"

namespace Portage {

/// Structure containing all the probability or counts for a given case
/// for HMM jump strategies that condition jump probabilities on various
/// conditions other than jump distance and sentence length.
struct HMMJumpSingleCountOrProb {
   /// Regular jumps - stay, forward and backward
   BiVector<double> jump;
   /// jumps to the end anchor
   double final;
   /// Constructor initializes everything to zero.
   HMMJumpSingleCountOrProb() : final(0.0) {}
   /// Reset all values to 0
   void clear() { jump.clear(); final = 0.0; }
   /// Return the sum of all values stored, i.e. final + sum_i{jump[i]}
   double sum() const;
   /// Test for deep equality
   bool operator==(const HMMJumpSingleCountOrProb& that) const {
      return final == that.final && jump == that.jump;
   }
   /// Test for deep non-equality
   bool operator!=(const HMMJumpSingleCountOrProb& that) const {
      return ! operator==(that);
   }
   /// Add that to self
   HMMJumpSingleCountOrProb& operator+=(const HMMJumpSingleCountOrProb& that) {
      final += that.final;
      jump += that.jump;
      return *this;
   }
   /// Write self in binary format
   void writeBin(ostream& os) const {
      using namespace BinIOStream;
      writebin(os, final);
      os << jump;
   }
   /// Read self from binary format. Replaces any preexisting values.
   void readBin(istream& is) {
      using namespace BinIOStream;
      readbin(is, final);
      is >> jump;
   }
   /// Write self in human-readable format
   void write(ostream& os) const {
      os << final << " ";
      jump.write(os);
   }
   /// Read self in format written by write()
   /// @return true if read successfully, false otherwise
   bool read(istream& is) {
      is >> final;
      return jump.read(is);
   }

   /// Swap the contents of two HMMJumpSingleCountOrProb objects
   void swap(HMMJumpSingleCountOrProb& that) {
      jump.swap(that.jump);
      std::swap(final, that.final);
   }

   /// Handle the complexities of max_jump for probs (divide them)
   static double get_jump_p(const BiVector<double>& v,
         Uint i_prime, Uint i, Uint max_normal_I, Uint max_jump);
   /// Handle the complexities of max_jump for counts (conflate them)
   static double& get_count(BiVector<double>& v,
         Uint i_prime, Uint i, Uint max_normal_I, Uint max_jump);

}; // HMMJumpSingleCountOrProb

} // namespace Portage

#endif // HMM_JUMP_BASIC_DATA_H

