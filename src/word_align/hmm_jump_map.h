/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_map.h  Jump strategy conditioning jump probabilities on
 *                       individual words, using Bayesian MAP training, with
 *                       non-lexicalized jump probs as a prior.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef HMM_JUMP_MAP_H
#define HMM_JUMP_MAP_H

#include "hmm_jump_strategy.h"
#include "hmm_jump_basic_data.h"
#include "voc.h"

namespace Portage {

/**
 * Jump strategy conditioning jump probabilities on individual words, using
 * Bayesian MAP training, with non-lexicalized jump probs as a prior.
 * This class implements the approach presented by Xiadong He at ACL-WMT07.
 */
class HMMJumpMAP : public HMMJumpStrategy, private NonCopyable {
   virtual void set_p_zero(double _p_zero) { prior->set_p_zero(_p_zero); p_zero = _p_zero;}
   virtual void set_uniform_p0(double _uniform_p0) { prior->set_uniform_p0(_uniform_p0); uniform_p0 = _uniform_p0; }
   virtual void set_alpha(double _alpha) { prior->set_alpha(_alpha); alpha = _alpha; }
   virtual void set_lambda(double _lambda) { prior->set_lambda(_lambda); lambda = _lambda; }
   virtual void set_anchor(bool _anchor) { prior->set_anchor(_anchor); anchor = _anchor; }
   virtual void set_max_jump(Uint _max_jump) { prior->set_max_jump(_max_jump); max_jump = _max_jump; }

   /// Structure containing all the probability or counts for a given case
   typedef HMMJumpSingleCountOrProb S;

   /// Model parameter: tau - controls balance between lexicalized evidence and
   /// non-lexical prior.  The higher tau is, the more weigth the prior has.
   /// See He's paper for details.
   double tau;

   /// Model parameter: min_count - controls pruning of the model.  If the
   /// total occurrence count for a word is below min_count, then that word is
   /// removed from the model and the prior alone is used.
   double min_count;

   /// whether to consider count the jump to end anchor as a special one
   bool end_dist;

   /// Mapping from words to the index used for word_prob and word_count.
   Voc voc;

   /// Baseline model and (probs and counts) used as a (non-lexicalized) prior
   HMMJumpStrategy* prior;

   /// Lexicalized jump parameters: word_prob[i] has the parameters for
   /// voc.word(i).  Invariant: word_prob.size() == voc.size().
   vector<S> word_prob;

   /// Lexicalized jump counts: word_count[i] has the counts for voc.word(i).
   vector<S> word_count;

   friend class HMMJumpStrategy;

   /**
    * constructor for use by base class only when creating a new model
    * @param tau         weight of the prior distribution
    * @param min_count   words with < min_count total counts are pruned
    * @param end_dist    whether to use a distinct distribution for the
    *                    jump to the final anchor.
    * @param prior       jump distribution to use as a prior
    */
   HMMJumpMAP(double tau, double min_count, bool end_dist,
              HMMJumpStrategy* prior);

   /**
    * constructor for use by base class only when about to read an existing
    * model.
    */
   HMMJumpMAP();

   /**
    * copy constructor as helper for Clone()
    */
   HMMJumpMAP(const HMMJumpMAP&);

   virtual void read(istream& in, const char* stream_name);
   virtual void writeBinCountsImpl(ostream& os) const;
   virtual void readAddBinCountsImpl(istream& is, const char* stream_name);

 public:
   virtual ~HMMJumpMAP();
   virtual HMMJumpMAP* Clone() const;
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

   HMMJUMP_MAGIC_STRING("Portage HMM Alignment data file using MAP: p0 uniform_po alpha lambda anchor end_dist max_jump tau min_count voc_size \\n loop{vocabulary} \\n loop{lexicalized parameters} \\n prior model");
   HMMJUMP_COUNT_STRING("NRC PORTAGE HMMAligner binary counts using MAP");

}; // class HMMJumpMAP

} // namespace Portage

#endif // HMM_JUMP_MAP_H
