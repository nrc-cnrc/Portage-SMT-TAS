/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_end_dist.h  Jump strategy with distinct end distributions.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef HMM_JUMP_END_DIST_H
#define HMM_JUMP_END_DIST_H

#include "portage_defs.h"
#include "hmm_jump_simple.h"

namespace Portage {

/// HMM jump strategy with distinct distributions for jumping from <s> or to
/// </s>, but not other conditioning.  This strategy inherits from
/// HMMJumpSimple because it is uses the parameters of that strategy and adds a
/// few more to it.
class HMMJumpEndDist : public HMMJumpSimple {

   /// Jump parameters from the initial position, if end_dist is true.
   /// i is jump forward i+1 positions from start.
   vector<double> init_jump_p;
   /// Jump parameters to the final anchor, if end_dist and anchor are true.
   /// i is jump forward i+1 positions to end positions.
   vector<double> final_jump_p;
   /// Counts for init_jump_p, if end_dist is true.
   /// Indexed like init_jump_p.
   vector<double> init_jump_counts;
   /// Counts for final_jump_p, if end_dist and anchor are true.
   /// Indexed like final_jump_p.
   vector<double> final_jump_counts;

   /**
    * Return the d(i-a_{j-1}) = c(i-i') parameter for i-i' = delta.
    * So that we can optionally follow Liang et al, 2007, the value returned is
    * affected by:
    *  - end_dist: if true, and i_prime == 0 or i == I && anchor, uses
    *    init/final_jump_p instead of forward/backward_jump_p.
    *  - anchor: only has an effect of end_dist is true - see above
    *  - max_jump: if max_jump > 0 and abs(i-i') >= max_jump, the max_jump bin
    *    is uniformly divided among possible i values for such jumps,
    *    distinguishing only whether it's a jump back or a jump forward.
    * @param i_prime start position of jump
    * @param i       end position of jump
    * @param I       source sentence length
    */
   double jump_p(Uint i_prime, Uint i, Uint I) const;

   /**
    * Return the d(i-a_{j-1}) = c(i-i') count for i-i' = delta.
    * So that we can follow Liang et al, 2006, the value returned is affected
    * by:
    *  - if i_prime == 0 or i == I && anchor, uses init/final_jump_count
    *    instead of forward/backward_jump_count.
    *  - max_jump: if max_jump > 0 and abs(i-i') >= max_jump, the max_jump bin
    *    tallies together counts from possible i values for such jumps,
    *    distinguishing only whether it's a jump back or a jump forward.
    * @param i_prime start position of jump
    * @param i       end position of jump
    * @param I       source sentence length
    */
   virtual double& jump_count(Uint i_prime, Uint i, Uint I);

   virtual void read(istream& in, const char* stream_name);
   virtual void writeBinCountsImpl(ostream& os) const;
   virtual void readAddBinCountsImpl(istream& is, const char* stream_name);

public:
   virtual HMMJumpEndDist* Clone() const;
   virtual void write(ostream& out, bool bin = false) const;
   virtual bool hasSameCounts(const HMMJumpStrategy* s);
   virtual void initCounts(const TTable& tt);
   /* countJumps() works correctly as inherited from HMMJumpSimple:
   virtual void countJumps(const dMatrix& A_counts,
         const vector<string>& src_toks,
         const vector<string>& tgt_toks,
         Uint I);
   */
   virtual void estimate();
   virtual void fillHMMJumpProbs(HMM* hmm,
         const vector<string>& src_toks,
         const vector<string>& tgt_toks,
         Uint I) const;

   HMMJUMP_MAGIC_STRING("Portage HMM Alignment data file v1.1: back_count for_count init_count final_count p0 up0 alpha lambda anchor end_dist max_jump \\n backward jump parameters \\n forward jump parameters \\n initial jump parameters \\n final jump parameters");
   HMMJUMP_COUNT_STRING("NRC PORTAGE HMMAligner binary counts v1.1");

}; // HMMJumpEndDist

} // Portage

#endif // HMM_JUMP_END_DIST_H
