/**
 * @author Aaron Tikuisis
 * @file forced_phrase_finder.h  This file contains the implementation of class
 * ForcedTargetPhraseFinder, which specializes by taking into account the
 * target sentence to restrict what is read from the phrase table.
 *
 * $Id$
 *
 * Translation-Model Utilities
 *
 * Split out of phrase_tm_score* by Matt Arnold in March 2005.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include <string>
#include <vector>
#include <canoe_general.h>
#include <phrasefinder.h>

namespace Portage {

class BasicModel;

/// Concrete implementation of PhraseFinder for use with known target sentence.
class ForcedTargetPhraseFinder: public PhraseFinder
{
   private:
      /**
       * Applicable phrases to each target word (stored here in order to be
       * deleted during destruction).
       */
      vector<vector<PhraseInfo *> **> phrasesByTargetWord;

      /**
       * A different RangePhraseFinder for each target word.
       */
      vector<RangePhraseFinder> finderByTargetWord;

      /**
       * Number of words in the source sentence.
       */
      Uint srcLength;
   public:
      /**
       * Creates a new ForcedTargetPhraseFinder.
       * @param model     The PhraseDecoderModel, used to get all phrases and to
       *                  determine the distortion limit and options.
       * @param tgt_sent  The target sentence.
       */
      ForcedTargetPhraseFinder(BasicModel &model, const vector<string> &tgt_sent);

      /**
       * Destructor.
       */
      virtual ~ForcedTargetPhraseFinder();

      virtual void findPhrases(vector<PhraseInfo *> &p, PartialTranslation &t);
}; // ForcedTargetPhraseFinder

}
