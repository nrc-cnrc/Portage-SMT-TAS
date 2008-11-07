/**
 * @author Eric Joanis
 * @file hmm_jump_simple.h  The simplest jump strategy.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef HMM_JUMP_SIMPLE_H
#define HMM_JUMP_SIMPLE_H

#include "portage_defs.h"
#include "hmm_jump_strategy.h"

namespace Portage {

/// Simple HMM jump strategy with parameters for forward and backward jump from
/// any position without any conditioning.
class HMMJumpSimple : public HMMJumpStrategy {

   /// Hook for unit testing of private members.
   friend class TestHMMJumpSimple;

protected:

   /**
    * d(i-a_{j-1}) parameter in George's text, c(i-i') in Och+Ney sct 2.2.1
    * Initially left empty, which should be interpreted as uniform probability.
    * When a required parameter is not initialized yet, it should be
    * interpreted as 0.0, leaving the alpha smoothing assign a small
    * probability.
    * forward_jump_p[0] is d(0), i.e., stay.  i is d(i), i.e., jump forward i.
    * backward_jump_p[i] is d(-(i+1)), i.e., jump back i+1 positions.
    */
   vector<double> forward_jump_p;
   vector<double> backward_jump_p;

   /**
    * Return the d(i-a_{j-1}) = c(i-i') parameter for i-i' = delta.
    * The value returned is affected by:
    *  - max_jump: if max_jump > 0 and abs(i-i') >= max_jump, the max_jump bin
    *    is uniformly divided among possible i values for such jumps.
    * @param i_prime start position of jump
    * @param i       end position of jump
    * @param I       source sentence length
    */
   double jump_p(Uint i_prime, Uint i, Uint I) const;
    
   /// Forward counts for the regular jump parameters.
   /// Indexed like forward_jump_p.
   vector<double> forward_jump_counts;
   /// Backward counts for the regular jump parameters.
   /// Indexed like backward_jump_p.
   vector<double> backward_jump_counts;

   /**
    * Return the d(i-a_{j-1}) = c(i-i') count for i-i' = delta.
    * The value returned is affected by:
    *  - max_jump: if max_jump > 0 and abs(i-i') >= max_jump, the max_jump bin
    *    tallies together counts from possible i values for such jumps.
    * @param i_prime start position of jump
    * @param i       end position of jump
    * @param I       source sentence length
    */
   virtual double& jump_count(Uint i_prime, Uint i, Uint I);

   virtual void read(istream& in, const char* stream_name);
   virtual void writeBinCountsImpl(ostream& os) const;
   virtual void readAddBinCountsImpl(istream& is, const char* stream_name);

public:

   virtual HMMJumpSimple* Clone() const;
   virtual void write(ostream& out, bool bin = false) const;
   virtual bool hasSameCounts(const HMMJumpStrategy* s);
   virtual void initCounts(const TTable& tt);
   virtual void countJumps(const dMatrix& A_counts,
         const vector<string>& src_toks,
         const vector<string>& tgt_toks,
         Uint I);
   virtual void estimate();
   virtual void fillHMMJumpProbs(HMM* hmm, 
         const vector<string>& src_toks,
         const vector<string>& tgt_toks,
         Uint I) const;

   HMMJUMP_MAGIC_STRING("Portage HMM Alignment data file: back_count for_count p0 alpha lambda \\n backward jump parameters \\n forward jump parameters");
   HMMJUMP_COUNT_STRING("NRC PORTAGE HMMAligner binary counts v1.0");

}; // HMMJumpSimple


} // Portage

#endif // HMM_JUMP_SIMPLE_H
