/**
 * @author Aaron Tikuisis
 * @file forced_phrase_finder.cc  This file contains the implementation of class
 * ForcedTargetPhraseFinder, which specializes by taking into account the
 * target sentence to restrict what is read from the phrase table.
 *
 * $Id$
 *
 * Split out of phrase_tm_score* by Matt Arnold in March 2005.
 * Optimized by Eric Joanis in August 2005.
 *
 * Translation-Model Utilities
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "basicmodel.h"
#include "forced_phrase_finder.h"
#include <cstdlib>
#include <iostream>
#include <math.h>

static const bool debug_forced_target_ff = false;

ForcedTargetPhraseFinder::ForcedTargetPhraseFinder(BasicModel &model,
      const vector<string> &tgt_sent)
   : srcLength(model.getSourceLength())
{
   vector<PhraseInfo *> **phrases = model.getPhraseInfo();
   //phrase options in triangular array
   //assert(src_sent.size() == srcLength);

   VectorPhrase uint_tgt_sent(tgt_sent.size());
   for (Uint i = 0; i < tgt_sent.size(); i++) {
      uint_tgt_sent[i] = model.getUintWord(tgt_sent[i]);
   }

   for (Uint i = 0; i < tgt_sent.size(); i++)
   {
      vector<PhraseInfo *> **curPhrases =
         TriangArray::Create< vector<PhraseInfo *> >()(srcLength);
      phrasesByTargetWord.push_back(curPhrases);
      for (Uint j = 0; j < srcLength; j++)
      {
         for (Uint k = 0; k < srcLength - j; k++)
         {
            for ( vector<PhraseInfo *>::const_iterator it =
                  phrases[j][k].begin(); it < phrases[j][k].end(); it++)
            {
               bool match = (i + (*it)->phrase.size() <= tgt_sent.size());
               Uint l(0);
               for ( Phrase::const_iterator w_it((*it)->phrase.begin());
                     match && w_it != (*it)->phrase.end(); ++w_it, ++l )
               {
                  match = ( *w_it == uint_tgt_sent[i+l] );
               }
               if (match)
               {
                  curPhrases[j][k].push_back(*it);

                  if ( debug_forced_target_ff ) {
                     cerr << (*it)->src_words
                          << " " << model.getStringPhrase((*it)->phrase)
                          << " " << exp((*it)->phrase_trans_prob) << endl;
                  }
               }
            }
         }
      }
      finderByTargetWord.push_back(RangePhraseFinder(curPhrases, srcLength,
               model.c->distLimit, model.c->itgLimit, model.c->distLimitSimple,
               model.c->distLimitExt, model.c->distPhraseSwap,model.c->distLimitITG));
   }
} // ForcedTargetPhraseFinder

ForcedTargetPhraseFinder::~ForcedTargetPhraseFinder()
{
   for (Uint i = 0; i < phrasesByTargetWord.size(); i++)
   {
      for (Uint j = 0; j < srcLength; j++)
      {
         delete [] phrasesByTargetWord[i][j];
      } // for
      delete [] phrasesByTargetWord[i];
   } // for
} // ~ForcedTargetPhraseFinder

void ForcedTargetPhraseFinder::findPhrases(vector<PhraseInfo *> &phrases,
      PartialTranslation &t)
{
   // WARNING: here we "incorrectly" rely on t.getLength() being well defined.
   // In general, the existence of recombined states means t can point to
   // multiple alternatives, not necessarily all having the same length.
   // However, in forced decoding, we set gen.lm_numwords = tgt_sent.size()
   // just before creating the BasicModel (see canoe.cc), so that recombination
   // is only possible when the complete history is shared, and therefore all
   // paths from t have not only the same length, but the same words, too.

   Uint length = t.getLength();
   assert(length <= finderByTargetWord.size());
   if (length < finderByTargetWord.size())
   {
      finderByTargetWord[length].findPhrases(phrases, t);
   } // if
   for ( vector<PhraseInfo *>::iterator it(phrases.begin());
         it != phrases.end(); ) {
      const bool lastSourcePhrase = (t.sourceWordsNotCovered == (*it)->src_words);
      const bool lastTargetPhrase = (length + (*it)->phrase.size() == finderByTargetWord.size());
      if ( lastSourcePhrase != lastTargetPhrase )
         phrases.erase(it);
      else
         ++it;
   }
} // findPhrases()

