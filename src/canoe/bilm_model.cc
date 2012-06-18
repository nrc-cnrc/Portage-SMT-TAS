/**
 * @author Eric Joanis
 * @file bilm_model.cc  BiLM decoder feature implementation
 *
 * $Id$
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include "bilm_model.h"
#include "lm.h"
#include "vocab_filter.h"
#include "phrasetable.h"
#include "basicmodel.h"

using namespace Portage;

void BiLMModel::getLastBiWordsBackwards(VectorPhrase &biWords, Uint num, const PartialTranslation& trans)
{
   biWords.clear();
   const PartialTranslation* pt = &trans;
   // End condition is pt->back != NULL, which flags the PartialTranslation
   // associated with the initial decoder state, because it has a plain
   // PhraseInfo object with an empty phrase, and thus no bi_phrase.
   while (pt->back != NULL && num > 0) {
      const ForwardBackwardPhraseInfo* pi = dynamic_cast<const ForwardBackwardPhraseInfo *>(pt->lastPhrase);
      assert(pi);

      Phrase::const_reverse_iterator w_it(pi->bi_phrase.rbegin());
      Phrase::const_reverse_iterator w_end(pi->bi_phrase.rend());
      while (num > 0 && w_it != w_end) {
         biWords.push_back(*w_it);
         ++w_it;
         --num;
      }
      pt = pt->back;
   }
   if (num > 0)
      biWords.push_back(sentStartID);
}

BiLMModel::BiLMModel(BasicModelGenerator* bmg, const string& filename)
: bmg(bmg)
, filename(filename)
, bilm(NULL)
, order(0)
, voc(bmg->getBiPhraseVoc())
, sentStartID(Uint(-1)) // "uninit"
{
}

void BiLMModel::finalizeInitialization()
{
   assert(!bilm);
   assert(!bmg->limitPhrases || (voc.getNumSourceSents() > 0 && voc.size() > 0));
   cerr << "Loading BiLM model " << filename << endl;
   bilm = PLM::Create(filename, &voc, PLM::SimpleAutoVoc, LOG_ALMOST_0,
      bmg->limitPhrases, 0, NULL, false);
   assert(bilm);
   order = bilm->getOrder();
   sentStartID = voc.index(PLM::SentStart);
   assert(sentStartID != voc.size());
}

BiLMModel::~BiLMModel()
{
   /*
   cerr << "BiLMModel hits: ";
   bilm->getHits().display(cerr);
   bilm->clearHits();
   */
}

void BiLMModel::newSrcSent(const newSrcSentInfo& info)
{
   /*
   cerr << "BiLMModel hits: ";
   bilm->getHits().display(cerr);
   bilm->clearHits();
   */
}

double BiLMModel::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   assert(bilm);
   const ForwardBackwardPhraseInfo* pi = dynamic_cast<const ForwardBackwardPhraseInfo *>(&phrase_info);
   // We give a detailed diagnostic message here - this is the first method
   // called with any given phrase_info; other methods can just assume or
   // assert correct input
   if (!pi || pi->bi_phrase.empty()) {
      error(ETWarn, "Phrase pair without a bi-phrase.  Something is wrong in the code, but here is the phrase pair to give us a chance to figure out where the problem comes from:");
      phrase_info.display();
      error(ETFatal, "Stopping because of missing bi-phrase.");
   }

   // Adapted from BasicModelGenerator::precomputeFutureScores() with the LMH_INCREMENTAL heuristic
   Uint bi_phrase_size(pi->bi_phrase.size());
   Uint reversed_bi_phrase[bi_phrase_size];
   for (Uint i = 0; i < bi_phrase_size; ++i)
      reversed_bi_phrase[i] = pi->bi_phrase[bi_phrase_size - i - 1];
   double futureScore = 0;
   for (Uint i = 0; i < bi_phrase_size; ++i)
      futureScore += bilm->cachedWordProb(reversed_bi_phrase[i], &(reversed_bi_phrase[i+1]), bi_phrase_size-i-1);
   return futureScore;
}

double BiLMModel::futureScore(const PartialTranslation &trans)
{
   return 0; // The whole BiLM future score is done in precomputeFutureScores()
}

double BiLMModel::score(const PartialTranslation& pt)
{
   assert(bilm);
   const ForwardBackwardPhraseInfo* pi = dynamic_cast<const ForwardBackwardPhraseInfo *>(pt.lastPhrase);
   assert(pi);
   // non-reentrant!
   static VectorPhrase endBiPhrase;
   getLastBiWordsBackwards(endBiPhrase, order-1 + pi->bi_phrase.size(), pt);
   Uint context_length = endBiPhrase.size()-1;
   double result = 0;
   for (int i = pi->bi_phrase.size() - 1; i >= 0; --i)
      result += bilm->cachedWordProb(endBiPhrase[i], &(endBiPhrase[i+1]), context_length-i);
   return result;
}

double BiLMModel::partialScore(const PartialTranslation &trans)
{
   return 0; // The whole partial score is done in precomputeFutureScores()
}

Uint BiLMModel::computeRecombHash(const PartialTranslation &pt)
{
   // We need to go over the order-1 last bi_phrase elements
   // non-reentrant!
   static VectorPhrase endBiPhrase;
   getLastBiWordsBackwards(endBiPhrase, order-1, pt);
   Uint result = 0;
   for (Uint i = 0; i < endBiPhrase.size(); ++i) {
      result += endBiPhrase[i];
      result = result * voc.size();
   }
   return result;
}

bool BiLMModel::isRecombinable(const PartialTranslation &pt1,
                               const PartialTranslation &pt2)
{
   // We need to compare the order-1 last bi_phrase elements
   // non-reentrant!
   static VectorPhrase endBiPhrase1, endBiPhrase2;
   getLastBiWordsBackwards(endBiPhrase1, order-1, pt1);
   getLastBiWordsBackwards(endBiPhrase2, order-1, pt2);
   return endBiPhrase1 == endBiPhrase2;
}
