/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_basic_data.cc Basic structure shared by more than one strategy
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include "hmm_jump_basic_data.h"
#include <numeric> // for accumulate

using namespace Portage;

// Not inlined to avoid propagating dependency on <numeric>
double HMMJumpSingleCountOrProb::sum() const {
   return std::accumulate(jump.begin(), jump.end(), final);
}

double HMMJumpSingleCountOrProb::get_jump_p(
   const BiVector<double>& v,
   Uint i_prime, Uint i, Uint max_normal_I, Uint max_jump
) {
   // We remove the assertions here because this code is called too often, so
   // they're quite expensive!
   //assert(i <= max_normal_I);
   //assert(i_prime <= max_normal_I);
   const int delta(int(i)-int(i_prime));
   if ( max_jump && abs(delta) >= int(max_jump) ) {
      if ( delta < 0 ) {
         //assert((-delta) >= int(max_jump));
         //assert(i_prime > max_jump);
         return v[-max_jump] / (int(i_prime)-int(max_jump));
      } else {
         //assert(delta >= int(max_jump));
         //assert(max_normal_I + 1 > i_prime + max_jump);
         return v[max_jump] / (max_normal_I + 1 - i_prime - max_jump);
      }
   } else {
      return v[delta];
   }
}

double& HMMJumpSingleCountOrProb::get_count(
   BiVector<double>& v,
   Uint i_prime, Uint i, Uint max_normal_I, Uint max_jump
) {
   // We remove the assertions here because this code is called too often, so
   // they're quite expensive!
   //assert(i <= max_normal_I);
   //assert(i_prime <= max_normal_I);
   const int delta(int(i)-int(i_prime));
   if ( max_jump && abs(delta) >= int(max_jump) ) {
      if ( delta < 0 ) {
         //assert((-delta) >= int(max_jump));
         //assert(i_prime > max_jump);
         return v.setAt(-max_jump);
      } else {
         //assert(delta >= int(max_jump));
         //assert(max_normal_I + 1 > i_prime + max_jump);
         return v.setAt(max_jump);
      }
   } else {
      return v.setAt(delta);
   }
}


