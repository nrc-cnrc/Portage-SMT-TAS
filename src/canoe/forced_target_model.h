/**
 * @author Eric Joanis
 * @file forced_target_model.h  Translation model generator (based on
 * BasicModelGenerator) which takes into consideration the reference sentences
 * in the target language.
 *
 * $Id$
 * vim:sw=3:
 *
 * THIS FILE IS NOT USED ANYWHERE YET - PREPARED FOR A CHANGE TO
 * PHRASE_TM_ALIGN THAT WAS DEFERRED INDEFINITELY.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef FORCED_TARGET_MODEL_H
#define FORCED_TARGET_MODEL_H

#include "canoe_general.h"
#include "basicmodel.h"
#include "phrasetable.h"
#include <string>
#include <vector>
#include <trie.h>

using namespace std;
using namespace __gnu_cxx;

namespace Portage
{
   /// Needs a description
   class ForcedTargetModelGenerator : public BasicModelGenerator
   {
   private:
      /**
       * Phrase table in the target language, used to prune translations
       * from the source language phrase table.
       * Trie is used as a set here, not as a map, so the value is always "true".
       */
      PTrie<bool,Empty,false> targetPhraseTable;

   public:
      /**
       * Creates a new ForcedTargetModelGenerator.
       * Note that all the parameters here, except verbosity, may be
       * changed before creating a model (since they are stored as public
       * instance variables).
       * @param c               Global canoe configuration object.
       *
       * Optional parameters:
       *
       * @param src_sents       The source sentences, used to determine the
       *                        phrases to limit to in the source language
       *                        (if limitPhrases is true).
       * @param tgt_sents       The target sentences, used to determine the
       *                        phrases to limit to in the target language
       *                        (if limitPhrases is true).
       * @param marks           All the marked translations.
       *
       * c.bypassMarked         Whether to use translations from the phrase
       *                        table in addition to marked translations,
       *                        when marked translations are provided.
       * c.addWeightMarked      The value to add to each marked.<BR>
       *                        translation's log probability.  This has a
       *                        real effect only if bypassMarked is true.<BR>
       * c.distWeight           The weight on the distortion probability.<BR>
       * c.lengthWeight         The weight on the length probability.<BR>
       * c.phraseTableSizeLimit The maximum number of translations per
       *                        source phrase, or NO_SIZE_LIMIT for no
       *                        limit.  If available, the forward
       *                        probability is used to determine which are
       *                        best.  It is recommended that no limit
       *                        should be used if forward probabilities are
       *                        not available, since quality may suffer
       *                        substantially.<BR>
       * c.phraseTableThreshold The probability threshold for phrases
       *                        in the phrase table.  This should be a
       *                        probability (not a log probability).<BR>
       * c.distLimit            The maximum distortion length allowed
       *                        between two phrases.  This is used to detect
       *                        whether partial translations can be
       *                        completed without violating this limit.<BR>
       * limitPhrases           Whether to limit the phrases loaded to ones
       *                        prespecified using src_sents.  If so, then
       *                        the language model is also limited to words
       *                        found in the translation options for these
       *                        sentences; thus, if this option is used, it
       *                        is imperative that the translation model be
       *                        loaded BEFORE the language model. [false].<BR>
       * c.verbosity            The verbosity level [1].<BR>
       */
      ForcedTargetModelGenerator(const CanoeConfig &c,
         const vector< vector<string> > &src_sents = vector<vector<string> >(),
         const vector< vector<string> > &tgt_sents = vector<vector<string> >(),
         const vector< vector<MarkedTranslation> > &marks =
            vector<vector<MarkedTranslation> >());

   }; // ForcedTargetModelGenerator
} // Portage

#endif // FORCED_TARGET_MODEL_H
