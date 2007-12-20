/**
 * @author Eric Joanis
 * @file forced_target_phrasetable.h  This file contains the definition of the
 * ForcedTargetPhraseTable subclass of the PhraseTable class, which uses the
 * target sentences to restrict what phrases get actually loading into memory
 * when reading phrase tables, and what phrases get returned when reading the
 * phrase table for translating a sentence.
 * 
 * $Id$
 * vim:sw=3:
 * 
 * THIS FILE IS NOT USED ANYWHERE YET - PREPARED FOR A CHANGE TO
 * PHRASE_TM_ALIGN THAT WAS DEFERRED INDEFINITELY.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef FORCED_TARGET_PHRASETABLE_H
#define FORCED_TARGET_PHRASETABLE_H

#include <canoe_general.h>
#include <phrasetable.h>
#include <vector>
#include <string>
#include <voc.h>

using namespace std;

namespace Portage
{
   /// Needs documentation
   class ForcedTargetPhraseTable : public PhraseTable
   {
   private:
      /**
       * The target sentence for the source sentence currently being
       * aligned or scored.
       */
      vector<Uint> tgt_sent;

   public:
      /**
       * Creates a new ForcedTargetPhraseTable using the given vocab.
       */
      ForcedTargetPhraseTable(VocabFilter &tgtVocab) : PhraseTable(tgtVocab) {};

      //@{
      /**
       * Set the target sentence.  This must be called before getPhraseInfos,
       * so that the correct target sentence is taken into account.
       * EJJ: Not the most elegant solution, but this way we don't need to
       * change the interface of the base class.
       * @param tgt_sent    The current target sentence as a sequence of words
       *                    in either string or Uint (using tgt_vocab) format.
       */
      void setTargetSentence ( const vector<string> &tgt_sent );
      void setTargetSentence ( const vector<Uint> &tgt_sent );
      //@}

   protected:
      /** 
       * Does the same as PhraseTable::getPhrases, with the same parameters,
       * except that only phrases in the target language which occur in
       * tgt_sent are considered.
       * @copydoc PhraseTable::getPhrases()
       */
      virtual void getPhrases(vector<pair<double, PhraseInfo *> > &phrases,
         TargetPhraseTable &tgtTable, const Range &src_words, Uint &numPruned,
         const vector<double> &weights, double logPruneThreshold, Uint verbosity,
         const vector<double> *forward_weights = NULL);

   };  // ends class ForcedTargetPhraseTable
} // ends namespace Portage

#endif // FORCED_TARGET_PHRASETABLE_H
