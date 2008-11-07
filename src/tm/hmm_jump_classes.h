/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_classes.h  Jump strategy conditioning jump probabilities on
 *                           word classes
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef HMM_JUMP_CLASSES_H
#define HMM_JUMP_CLASSES_H

#include "hmm_jump_strategy.h"
#include "hmm_jump_basic_data.h"
#include "word_classes.h"

namespace Portage {

class HMMJumpClasses : public HMMJumpStrategy {

   /// word --> class mapping
   WordClasses wc;

   /// whether to consider count the jump to end anchor as a special one
   bool end_dist;

   /// Structure containing all the probability or counts for a given case
   typedef HMMJumpSingleCountOrProb S;

   /// Probability parameters summed over all classes (used for OOVs, at least)
   S global_prob;
   /// Probability parameters for each word class (class i is in class_prob[i])
   vector<S> class_prob;
   /// Initial jump parameters
   BiVector<double> init_prob;

   /// Jump occurrence counts summed over all classes
   S global_count;
   /// Jump occurrence counts for each word class (class i in in class_count[i])
   vector<S> class_count;
   /// Initial jump counts
   BiVector<double> init_count;

   friend class HMMJumpStrategy;
   /**
    * constructor for use by base class only
    * @param word_class_file name of file to read word classes from - should
    *                        only be provided when creating a new
    *                        HMMJumpClasses from scratch, not when read() is
    *                        about to be called, since read() will reset the
    *                        classes.
    * @param end_dist        whether to use a distinct distribution for the
    *                        jump to the final anchor.
    */
   HMMJumpClasses(const char* word_class_file = NULL, bool end_dist = true);

   virtual void read(istream& in, const char* stream_name);
   virtual void writeBinCountsImpl(ostream& os) const;
   virtual void readAddBinCountsImpl(istream& is, const char* stream_name);

 public:
   virtual HMMJumpClasses* Clone() const;
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

   HMMJUMP_MAGIC_STRING("Portage HMM Alignment data file with classes: p0 uniform_po alpha lambda anchor end_dist max_jump class_count \\n global jump parameters \\n init jump parameters \\n loop{class jump parameters} \\n loop{word classes}");
   HMMJUMP_COUNT_STRING("NRC PORTAGE HMMAligner binary counts with classes");

}; // HMMJumpClasses

} // Portage

#endif // HMM_JUMP_CLASSES_H
