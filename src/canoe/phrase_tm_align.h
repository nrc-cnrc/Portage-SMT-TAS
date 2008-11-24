/**
 * @author Aaron Tikuisis
 *   **Modified by Nicola Ueffing to use all decoder models
 * @file phrase_tm_align.h  This file contains the declaration of
 * computePhraseTM(), which computes the phrase-based translation probability
 * for a given sentence.  It also contains a function to constrain and load a
 * phrase table, and the class ForcePhrasePDM which is used by the
 * computePhraseTM() function.
 *
 * $Id$
 *
 * Translation-Model Utilities
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef PHRASE_TM_ALIGN_H
#define PHRASE_TM_ALIGN_H

#include "basicmodel.h"
#include <string>
#include <vector>
#include <ostream>


using namespace std;

namespace Portage
{
   /**
    * Class which computes the phrase-based translation probability for a
    * given sentence.  It also contains a function to constrain and load a
    * phrase table.
    */
   class PhraseTMAligner
   {
      private:
         /**
          * The generator used to produce models used in decoding.
          */
         BasicModelGenerator* gen;

         /**
          * Verbosity level
          */
         Uint verbosity;

      public:
         /**
          * Creates a new PhraseTMAligner.
          * @param c      canoe config
          * c.distWeight  The distortion weight, relative to a weight of 1.0
          *               for the translation model.
          * c.distLimit   The maximum distortion distance allowed between two
          *               words, or NO_MAX_DISTORTION for no limit.
          * c.verbosity   The verbosity level (1 to 4) [1 -- quiet]
          */
         PhraseTMAligner(const CanoeConfig& c);

         /**
          * Creates a new PhraseTMAligner, optionally limiting the phrases
          * loaded to ones in a given source sentence.
          * @param c  canoe config
          * c.distWeight     The distortion weight, relative to a weight of 1.0
          *                  for the translation model.
          * c.distLimit      The maximum distortion distance allowed between
          *                  two words, or NO_MAX_DISTORTION for no limit.
          * @param src_sents If limitPhrases is true, the phrases are limited
          *                  to those found in these sentences.
          * @param marked_src_sents Source sentences with mark-up.
          * c.limitPhrases   Whether to limit phrases to subphrases of
          *                  src_sents.
          * c.verbosity      The verbosity level (1 to 4) [1 -- quiet]
          */
         PhraseTMAligner(const CanoeConfig& c,
               const vector< vector<string> > &src_sents,
               const vector< vector<MarkedTranslation> > &marked_src_sents);

         /**
          * Maximizes the weighted distortion + segmentation + translation
          * model score over all alignments from src_sent to tgt_sent,
          * constrained by distLimit.
          * Only phrases matching tgt_sent will be considered.
          * @param new_src_sent_info The source sentence and related info
          * @param out        Outout stream
          * @param n          Number of forced alignments per sentence pair to be determined
          * @param noscore    Do not print score of the found alignment
          * @param onlyscore  Print only score of the found alignment
          * @param threshold  hypothesis stack relative threshold (ratio, not log!)
          * @param pruneSize  hypothesis stack size
          * @param covLimit   coverage pruning limit
          * @param covThreshold coverage pruning threshold (ratio, not log!)
          */
         void computePhraseTM(newSrcSentInfo& new_src_sent_info,
               ostream &out,
               Uint n,
               bool noscore,
               bool onlyscore,
               double threshold,
               Uint pruneSize,
               Uint covLimit,
               double covThreshold);
   }; // PhraseTMAligner
} // Portage

#endif // PHRASE_TM_ALIGN_H
