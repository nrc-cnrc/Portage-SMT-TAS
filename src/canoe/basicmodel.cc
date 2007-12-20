/**
 * @author Aaron Tikuisis
 * @file basicmodel.cc  This file contains the implementation of the BasicModel
 * class, which provides a log-linear model comprised of translation models,
 * language models, and built-in distortion and length penalties.
 *
 * $Id$ *
 * Canoe Decoder
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "basicmodel.h"
#include "backwardsmodel.h"
#include "phrasetable.h"
#include "lm.h"
#include <math.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <ext/hash_map>
#include <errors.h>

using namespace Portage;
using namespace std;

bool MarkedTranslation::operator==(const MarkedTranslation &b) const
{
   return src_words == b.src_words && markString == b.markString &&
          log_prob == b.log_prob;
} // operator==

// NB: The order in which these parameters are added to featureWeightsV should
// not be changed - the conversion from/to rescoring models depends on this. If
// you change it, you will break existing models stored on disk.
//
// Implementation note: it would be cleaner to initialize decoder_features and
// featureWeightsV from declarative information in CanoeConfig. A mechanism
// exists to do this, but currently only for feature weights (because it is
// needed for cow and elsewhere). Until a similar mechanism is developed for
// models, it seems better to continue to do everything "manually" here, even
// though this makes no use whatsoever of DecoderFeature's virtual
// constructor...

void BasicModelGenerator::InitDecoderFeatures(const CanoeConfig& c)
{
   if ( c.distortionModel.empty() )
      cerr << "Not using any distortion model" << endl;
   for ( Uint i(0); i < c.distortionModel.size(); ++i ) {
      if (c.distortionModel[i] != "none") {
         decoder_features.push_back(DecoderFeature::create(this,
            "DistortionModel", c.distortionModel[i], ""));
         featureWeightsV.push_back(c.distWeight[i]);
         if (c.randomWeights)
            random_feature_weight.push_back(c.rnd_distWeight.get(i));
      }
   }


   decoder_features.push_back(DecoderFeature::create(this,
      "LengthFeature", "", ""));
   featureWeightsV.push_back(c.lengthWeight);
   if (c.randomWeights)
      random_feature_weight.push_back(c.rnd_lengthWeight.get(0));


   if ( c.segmentationModel != "none" ) {
      decoder_features.push_back(DecoderFeature::create(this,
         "SegmentationModel", c.segmentationModel, ""));
      featureWeightsV.push_back(c.segWeight[0]);
      if (c.randomWeights)
         random_feature_weight.push_back(c.rnd_segWeight.get(0));
   } else {
      // This is the default - no need to warn!
      //cerr << "Not using any segmentation model" << endl;
   }


   for (Uint i = 0; i < c.ibm1FwdWeights.size(); ++i) {
      decoder_features.push_back(DecoderFeature::create(this,
         "IBM1FwdFeature", "", c.ibm1FwdFiles[i]));
      featureWeightsV.push_back(c.ibm1FwdWeights[i]);
      if (c.randomWeights)
         random_feature_weight.push_back(c.rnd_ibm1FwdWeights.get(i));
   }

}


BasicModelGenerator* BasicModelGenerator::create(
      const CanoeConfig& c,
      const vector<vector<string> > *sents,
      const vector<vector<MarkedTranslation> > *marks,
      PhraseTable* phrasetable)
{
   BasicModelGenerator *result;

   c.check_all_files();
   assert (c.loadFirst || sents && marks);

   if (c.backwards) {
      if (sents && marks)
         result = new BackwardsModelGenerator(c, *sents, *marks);
      else
         result = new BackwardsModelGenerator(c);
   } else {
      if (sents && marks)
         result = new BasicModelGenerator(c, *sents, *marks, phrasetable);
      else
         result = new BasicModelGenerator(c);
   }

   for (Uint i = 0; i < c.backPhraseFiles.size(); i++) {
      if (!c.forPhraseFiles.empty()) {
         if (!c.forwardWeights.empty()) {
            result->addTranslationModel(c.backPhraseFiles[i].c_str(),
                  c.forPhraseFiles[i].c_str(),
                  c.transWeights[i], c.forwardWeights[i]);
            if (c.randomWeights) {
               result->random_forward_weight.push_back(c.rnd_forwardWeights.get(i));
               result->random_trans_weight.push_back(c.rnd_transWeights.get(i));
            }
         } else {
            result->addTranslationModel(c.backPhraseFiles[i].c_str(),
                  c.forPhraseFiles[i].c_str(),
                  c.transWeights[i]);
            if (c.randomWeights) {
               result->random_trans_weight.push_back(c.rnd_transWeights.get(i));
            }
         }
      } else {
         result->addTranslationModel(c.backPhraseFiles[i].c_str(),
               c.transWeights[i]);
         if (c.randomWeights) {
            result->random_trans_weight.push_back(c.rnd_transWeights.get(i));
         }
      }
   }

   Uint weights_already_used = c.backPhraseFiles.size();
   for (Uint i = 0; i < c.multiProbTMFiles.size(); i++) {
      const Uint model_count =
         PhraseTable::countProbColumns(c.multiProbTMFiles[i].c_str()) / 2;
      vector<double> backward_weights, forward_weights;
      assert(c.transWeights.size() >= weights_already_used + model_count);
      backward_weights.assign(c.transWeights.begin() + weights_already_used,
            c.transWeights.begin() + weights_already_used + model_count);
      if ( c.forwardWeights.size() > 0 ) {
         assert(c.forwardWeights.size() >= weights_already_used+model_count);
         forward_weights.assign(
               c.forwardWeights.begin() + weights_already_used,
               c.forwardWeights.begin() + weights_already_used + model_count);
         for (Uint i(0); c.randomWeights && i<model_count; ++i) {
            const Uint index = weights_already_used + i;
            result->random_forward_weight.push_back(c.rnd_forwardWeights.get(index));
         }
      }
      result->addMultiProbTransModel(c.multiProbTMFiles[i].c_str(),
            backward_weights, forward_weights);
      for (Uint i(0); c.randomWeights && i<model_count; ++i) {
         const Uint index = weights_already_used + i;
         result->random_trans_weight.push_back(c.rnd_transWeights.get(index));
      }
      weights_already_used += model_count;
   }

   assert ( weights_already_used == c.transWeights.size() );
   // load language models
   for (Uint i = 0; i < c.lmFiles.size(); i++) {
      result->addLanguageModel(c.lmFiles[i].c_str(), c.lmWeights[i], c.lmOrder);

      if (c.randomWeights)
         result->random_lm_weight.push_back(c.rnd_lmWeights.get(i));

   }

   // HMM no need for all that filtering data, might as well get rid of it.
   result->tgt_vocab.freePerSentenceData();

   return result;
} // BasicModelGenerator::create()

BasicModelGenerator::BasicModelGenerator(const CanoeConfig& c,
                                         PhraseTable *phraseTable) :
   c(&c),
   verbosity(c.verbosity),
   tgt_vocab(0),
   phraseTable(phraseTable),
   limitPhrases(false),
   numTextTransWeights(0),
   lm_numwords(1),
   futureScoreLMHeuristic(lm_heuristic_type_from_string(c.futLMHeuristic)),
   addWeightMarked(log(c.weightMarked))
{
   if ( this->phraseTable == NULL ) {
      this->phraseTable = new PhraseTable(tgt_vocab, c.phraseTablePruneType.c_str());
   }
   InitDecoderFeatures(c);
}

BasicModelGenerator::BasicModelGenerator(
   const CanoeConfig &c,
   const vector<vector<string> > &src_sents,
   const vector<vector<MarkedTranslation> > &marks,
   PhraseTable *_phraseTable
) :
   c(&c),
   verbosity(c.verbosity),
   tgt_vocab(src_sents.size()),
   phraseTable(_phraseTable),
   limitPhrases(!c.loadFirst),
   numTextTransWeights(0),
   lm_numwords(1),
   futureScoreLMHeuristic(lm_heuristic_type_from_string(c.futLMHeuristic)),
   addWeightMarked(log(c.weightMarked))
{
   if ( this->phraseTable == NULL ) {
      this->phraseTable = new PhraseTable(tgt_vocab, c.phraseTablePruneType.c_str());
   }
   if (limitPhrases)
   {
      // Enter all source phrases into the phrase table, and into the vocab
      // in case they are OOVs we need to copy through to the output.
      phraseTable->addSourceSentences(src_sents);
      if (!src_sents.empty()) {
         assert(tgt_vocab.size() > 0);
         assert(tgt_vocab.getNumSourceSents() == src_sents.size() ||
                tgt_vocab.getNumSourceSents() == VocabFilter::maxSourceSentence4filtering);
      }

      // Add target words from marks to the vocab
      for ( vector< vector<MarkedTranslation> >::const_iterator it = marks.begin();
            it != marks.end(); ++it)
      {
         const Uint sent_no = (it - marks.begin());
         for ( vector<MarkedTranslation>::const_iterator jt = it->begin();
               jt != it->end(); ++jt)
         {
            for ( vector<string>::const_iterator kt = jt->markString.begin();
                  kt != jt->markString.end(); ++kt)
            {
               tgt_vocab.addWord(*kt, sent_no);
            }
         }
      }
   }

   InitDecoderFeatures(c);
} // BasicModelGenerator::BasicModelGenerator()

BasicModelGenerator::~BasicModelGenerator()
{
   for ( vector<DecoderFeature *>::iterator it = decoder_features.begin();
         it != decoder_features.end(); ++it )
      delete *it;

   if (phraseTable) {
      delete phraseTable;
      phraseTable = NULL;
   }

   for (vector<PLM *>::iterator it = lms.begin(); it != lms.end(); ++it)
      delete *it;

} // ~BasicModelGenerator

void BasicModelGenerator::addTranslationModel(const char *src_to_tgt_file,
        const char *tgt_to_src_file, double weight)
{
   phraseTable->read(src_to_tgt_file, tgt_to_src_file, limitPhrases);
   assert(transWeightsV.size() >= numTextTransWeights);
   transWeightsV.insert(transWeightsV.begin() + numTextTransWeights, weight);
   numTextTransWeights++;
} // addTranslationModel

void BasicModelGenerator::addTranslationModel(const char *src_to_tgt_file,
        const char *tgt_to_src_file, double weight, double forward_weight)
{
   addTranslationModel(src_to_tgt_file, tgt_to_src_file, weight);
   forwardWeightsV.insert(forwardWeightsV.begin() + numTextTransWeights, forward_weight);
} // addTranslationModel

void BasicModelGenerator::addTranslationModel(const char *src_to_tgt_file, double weight)
{
   addTranslationModel(src_to_tgt_file, NULL, weight);
} // addTranslationModel

void BasicModelGenerator::addMultiProbTransModel(
   const char *multi_prob_tm_file, vector<double> backward_weights,
   vector<double> forward_weights)
{
   phraseTable->readMultiProb(multi_prob_tm_file, limitPhrases);

   const Uint multi_prob_col_count =
      phraseTable->countProbColumns(multi_prob_tm_file);
   assert(multi_prob_tm_file != NULL);
   if ( backward_weights.size() * 2 != multi_prob_col_count )
      error(ETFatal, "wrong number of backward weights (%d) for %s",
            backward_weights.size(), multi_prob_tm_file);
   if ( forward_weights.size() != 0 &&
        forward_weights.size() != backward_weights.size() )
      error(ETFatal, "wrong number of forward weights (%d) for %s",
            forward_weights.size(), multi_prob_tm_file);

   assert(transWeightsV.size() >= numTextTransWeights);
   transWeightsV.insert(transWeightsV.begin() + numTextTransWeights,
                        backward_weights.begin(), backward_weights.end());
   if ( ! forward_weights.empty() ) {
      assert(forwardWeightsV.size() >= numTextTransWeights);
      forwardWeightsV.insert(forwardWeightsV.begin() + numTextTransWeights,
                             forward_weights.begin(), forward_weights.end());
   }

   numTextTransWeights += backward_weights.size();
}

void BasicModelGenerator::addLanguageModel(const char *lmFile, double weight,
   Uint limit_order, ostream *const os_filtered)
{
   if (!PLM::check_file_exists(lmFile))
      error(ETFatal, "Cannot read from language model file %s", lmFile);
   cerr << "loading language model from " << lmFile << endl;
   //time_t start_time = time(NULL);
   PLM *lm = PLM::Create(lmFile, &tgt_vocab, PLM::SimpleAutoVoc, LOG_ALMOST_0,
                         limitPhrases, limit_order, os_filtered);
   //cerr << " ... done in " << (time(NULL) - start_time) << "s" << endl;
   lms.push_back(lm);
   lmWeightsV.push_back(weight);

   // We trust the language model to tell us its order, not the other way around.
   if ( lm->getOrder() > lm_numwords ) lm_numwords = lm->getOrder();
} // addLanguageModel

string BasicModelGenerator::describeModel() const
{
   // The order of the features here must be the same as in
   // BasicModel::getFeatureFunctionVals()
   ostringstream description;
   Uint featureIndex(0);  // indicates column for the feature
   for ( vector<DecoderFeature*>::const_iterator it = decoder_features.begin();
         it != decoder_features.end(); ++it )
      description << ++featureIndex << "\t" << (*it)->describeFeature() << endl;
   for ( vector<PLM*>::const_iterator it = lms.begin(); it != lms.end(); ++it )
      description << ++featureIndex << "\t" << (*it)->describeFeature() << endl;
   description << ++featureIndex << "\t" << phraseTable->describePhraseTables(!forwardWeightsV.empty());

   return description.str();
} // describeModel

BasicModel *BasicModelGenerator::createModel(
   const vector<string> &src_sent,
   const vector<MarkedTranslation> &marks,
   bool alwaysTryDefault,
   vector<bool>* oovs)
{
   vector<PhraseInfo *> **phrases = createAllPhraseInfos(src_sent, marks,
      alwaysTryDefault, oovs);
   for ( vector<DecoderFeature *>::iterator it = decoder_features.begin();
         it != decoder_features.end(); ++it)
      (*it)->newSrcSent(src_sent, phrases);

   for (Uint i=0;i<lms.size();++i)
      lms[i]->clearCache();

   double **futureScores = precomputeFutureScores(phrases, src_sent.size());

   return new BasicModel(src_sent, phrases, futureScores, *this);
} // createModel

vector<PhraseInfo *> **BasicModelGenerator::createAllPhraseInfos(
   const vector<string> &src_sent,
   const vector<MarkedTranslation> &marks,
   bool alwaysTryDefault,
   vector<bool>* oovs)
{
   assert(transWeightsV.size() > 0);
   // Initialize the triangular array
   vector<PhraseInfo *> **result =
      CreateTriangularArray<vector<PhraseInfo *> >()(src_sent.size());

   if (oovs) oovs->assign(src_sent.size(), false);

   // Keep track of which ranges to skip (because they are marked)
   vector<Range> rangesToSkip;
   addMarkedPhraseInfos(marks, result, rangesToSkip, src_sent.size());
   sort(rangesToSkip.begin(), rangesToSkip.end());

   phraseTable->getPhraseInfos(result, src_sent, transWeightsV,
      c->phraseTableSizeLimit, log(c->phraseTableThreshold), rangesToSkip,
      verbosity, (forwardWeightsV.empty() ? NULL : &forwardWeightsV));

   // Use an iterator to go through the ranges to skip, so that we avoid these
   // when creating default translations
   vector<Range>::const_iterator rangeIt = rangesToSkip.begin();

   for (Uint i = 0; i < src_sent.size(); i++)
   {
      while (rangeIt != rangesToSkip.end() && rangeIt->end <= i)
         rangeIt++;

      if ( ( result[i][0].empty() &&
             (rangeIt == rangesToSkip.end() || i < rangeIt->start) )
           || alwaysTryDefault )
      {
         // The word has no translation and is not in a range to skip, so
         // create a default translation (use the source word as itself).
         Range curRange(i, i + 1);
         result[i][0].push_back(makeNoTransPhraseInfo(curRange,
            src_sent[i].c_str()));
         if (oovs) (*oovs)[i] = true;
      }
   }
   return result;
} // createAllPhraseInfos

void BasicModelGenerator::addMarkedPhraseInfos(
   const vector<MarkedTranslation> &marks,
   vector<PhraseInfo *> **result,
   vector<Range> &rangesToSkip,
   Uint sentLength)
{
   // Compute the total weight on phrase translation models, in order to
   // denormalize the probabilities
   double totalWeight = 0;
   for ( vector<double>::const_iterator it = transWeightsV.begin();
         it != transWeightsV.end(); it++ )
      totalWeight += *it;

   for ( vector<MarkedTranslation>::const_iterator it = marks.begin();
         it != marks.end(); it++)
   {
      // Create a MultiTransPhraseInfo for each marked phrase
      MultiTransPhraseInfo *newPI;
      if ( ! forwardWeightsV.empty() ) {
         ForwardBackwardPhraseInfo* newFBPI = new ForwardBackwardPhraseInfo;
         newFBPI->forward_trans_prob =
            (it->log_prob + addWeightMarked) * totalWeight;
         newFBPI->forward_trans_probs.insert(newFBPI->forward_trans_probs.end(),
            forwardWeightsV.size(), it->log_prob + addWeightMarked);
         newPI = newFBPI;
      } else {
         newPI = new MultiTransPhraseInfo;
      }
      newPI->src_words = it->src_words;
      newPI->phrase_trans_prob = (it->log_prob + addWeightMarked) * totalWeight;
      newPI->phrase_trans_probs.insert(newPI->phrase_trans_probs.end(),
         transWeightsV.size(), it->log_prob + addWeightMarked);
      for ( vector<string>::const_iterator jt = it->markString.begin();
            jt != it->markString.end(); jt++) {
         newPI->phrase.push_back(tgt_vocab.add(jt->c_str()));
      }

      // Store it in the appropriate location in the triangular array
      result[newPI->src_words.start]
         [newPI->src_words.end - newPI->src_words.start - 1].push_back(newPI);

      // If bypassMarked option isn't being used, remember to skip this phrase
      // when looking for normal phrase options
      if (!c->bypassMarked)
         rangesToSkip.push_back(it->src_words);
   } // for
} // addMarkedPhraseInfos

// Note that we use LOG_ALMOST_0 here and not PhraseTable::log_almost_0, as the
// latter is intended to be the value assigned by phrasetables to entries that
// occur exlusively in other phrasetables. Here we are dealing with source
// phrases that have no translations in any tables.

PhraseInfo *BasicModelGenerator::makeNoTransPhraseInfo(
   const Range &range, const char *word)
{
   MultiTransPhraseInfo *newPI;
   if ( ! forwardWeightsV.empty() ) {
      ForwardBackwardPhraseInfo* newFBPI = new ForwardBackwardPhraseInfo;
      newFBPI->forward_trans_probs.insert(newFBPI->forward_trans_probs.end(),
         forwardWeightsV.size(), LOG_ALMOST_0);
      newFBPI->forward_trans_prob = dotProduct(forwardWeightsV,
         newFBPI->forward_trans_probs, forwardWeightsV.size());
      newPI = newFBPI;
   } else {
      newPI = new MultiTransPhraseInfo;
   }
   newPI->src_words = range;
   newPI->phrase.push_back(tgt_vocab.add(word));
   newPI->phrase_trans_probs.insert(newPI->phrase_trans_probs.end(),
      transWeightsV.size(), LOG_ALMOST_0);
   newPI->phrase_trans_prob = dotProduct(newPI->phrase_trans_probs,
      transWeightsV, transWeightsV.size());
   return newPI;
} // makeNoTransPhraseInfo

double **BasicModelGenerator::precomputeFutureScores(
   vector<PhraseInfo *> **phrases, Uint sentLength)
{
   double **result = CreateTriangularArray<double>()(sentLength);
   for (Uint i = 0; i < sentLength; i++)
   {
      for (int j = i; j >= 0; j--)
      {
         if (verbosity >= 4)
            cerr << "Precomputing future score for [" << j << "," << (i+1) << ")" << endl;

         // Compute future score for range [j, i + 1), by maximizing the
         // translation probability over all possible translations of that
         // range.  ie.  future score for [j, i + 1) = max(best score for a
         // direct translation, max_{j < k < i + 1} (future score for [j, k) +
         // future score for [k, i + 1)) )

         // The order of iteration of ranges ensures that all future scores we
         // already need have already been computed.
         double &curScore = result[j][i - j];
         curScore = -INFINITY;
         for ( vector<PhraseInfo*>::const_iterator it = phrases[j][i-j].begin();
               it != phrases[j][i - j].end(); ++it )
         {
            // Translation score
            double newScore = dotProduct(
               ((MultiTransPhraseInfo *) *it)->phrase_trans_probs,
               transWeightsV, transWeightsV.size());

            // Add heuristic LM score
            if ( futureScoreLMHeuristic == LMH_UNIGRAM ) {
               // Our old way: use the unigram LM score
               for ( Phrase::const_iterator jt = (*it)->phrase.begin();
                     jt != (*it)->phrase.end(); ++jt ) {
                  for (Uint k = 0; k < lmWeightsV.size(); k++) {
                     newScore += lms[k]->wordProb(*jt, NULL, 0) * lmWeightsV[k];
                  }
               }
            } else if ( futureScoreLMHeuristic == LMH_INCREMENTAL ) {
               // The "incremental" heuristic does unigram prob for the first
               // word, bigram prob for the second, etc, and LM-order-gram prob
               // until the end.
               Uint phrase_size((*it)->phrase.size());
               Uint reversed_phrase[phrase_size];
               for ( Uint i(0); i < phrase_size; ++i )
                  reversed_phrase[i] = (*it)->phrase[phrase_size - i - 1];
               for ( Uint j(0); j < lms.size(); ++j )
                  for ( Uint i(0); i < phrase_size; ++i )
                     newScore += lms[j]->wordProb(reversed_phrase[i],
                                 &(reversed_phrase[i+1]), phrase_size-i-1)
                               * lmWeightsV[j];
            } else if ( futureScoreLMHeuristic == LMH_SIMPLE ) {
               // The "simple" heuristic is admissible by ignoring words with
               // insufficient context
               Uint phrase_size((*it)->phrase.size());
               Uint reversed_phrase[phrase_size];
               for ( Uint i(0); i < phrase_size; ++i )
                  reversed_phrase[i] = (*it)->phrase[phrase_size - i - 1];
               for ( Uint j(0); j < lms.size(); ++j )
                  for ( Uint i(0); i < phrase_size - (lms[j]->getOrder()-1); ++i )
                     newScore += lms[j]->wordProb(reversed_phrase[i],
                                 &(reversed_phrase[i+1]), phrase_size-i-1)
                               * lmWeightsV[j];
            } // else LMH==NONE: nothing to do!

            // Other feature scores
            for (Uint k = 0; k < decoder_features.size(); k++) {
               newScore += decoder_features[k]->precomputeFutureScore(**it)
                           * featureWeightsV[k];
               if ( verbosity >= 5 ) {
                  cerr << "k=" << k << ": "
                       << decoder_features[k]->precomputeFutureScore(**it)
                       << " * " << featureWeightsV[k] << endl;
               }
            }

            curScore = max(curScore, newScore);

            if (verbosity >= 4) {
               cerr << "    src_words " << (*it)->src_words.toString();
               string stringPhrase;
               getStringPhrase(stringPhrase, (*it)->phrase);
               cerr << " phrase " << stringPhrase << " score " << newScore
                    << endl;
            }
         }

         //if (i == j) assert(curScore > -INFINITY);

         // See if we can do any better by combining range [j, k)
         // with [k, i + 1), for j < k < i + 1.
         for (Uint k = j + 1; k < i + 1; k++)
            curScore = max(curScore, result[j][k - j - 1] + result[k][i - k]);

         if (verbosity >= 4)
            cerr << "Future score for [" << j << "," << (i+1) << ") is "
                 << curScore << endl;
      }
   }
   assert(sentLength == 0 || result[0][sentLength - 1] > -INFINITY);

   if (verbosity >= 3)
   {
      for (Uint i = 0; i < sentLength; i++)
      {
         for (Uint j = 0; j < sentLength - i; j++)
         {
            cerr << "future score from " << i << " to " << (i + j) << " is "
                 << result[i][j] << endl;
         }
      }
   }
   return result;
} // precomputeFutureScores

BasicModelGenerator::LMHeuristicType BasicModelGenerator::lm_heuristic_type_from_string(const string& s)
{
   if      ( s == "none" )          return LMH_NONE;
   else if ( s == "unigram" )       return LMH_UNIGRAM;
   else if ( s == "incremental" )   return LMH_INCREMENTAL;
   else if ( s == "simple" )        return LMH_SIMPLE;
   else { assert(false); return LMH_NONE; }
}

void BasicModelGenerator::getRawLM(
   vector<double> &lmVals,
   const PartialTranslation &trans)
{
   // this method is called millions of times, so keep this endPhrase buffer
   // static, to avoid reallocating it constantly for nothing.
   static Phrase endPhrase;
   endPhrase.clear();

   const Uint last_phrase_size = trans.lastPhrase->phrase.size();
   const Uint num_words(last_phrase_size + lm_numwords - 1);
   // Get the missing context words in the previous phrase(s);
   trans.getLastWords(endPhrase, num_words, true);

   if (endPhrase.size() < num_words)
   {
      // Start of sentence
      endPhrase.push_back(tgt_vocab.index(PLM::SentStart));
   }
   assert(endPhrase.size() > last_phrase_size);
   assert(endPhrase.size() <= num_words);

   const Uint context_len = endPhrase.size()-1;

   // Make room for results in vector
   lmVals.insert(lmVals.end(), lmWeightsV.size(), 0);
   // Create an iterator at the start of the results for convenience
   vector<double>::iterator results = lmVals.end() - lmWeightsV.size();

   for (int i = last_phrase_size - 1; i >= 0; i--)
   {
      for (Uint j = 0; j < lmWeightsV.size(); j++)
      {
         // Compute score for j-th language model for word reverseArray[i]
         results[j] += lms[j]->wordProb(endPhrase[i], &(endPhrase[i + 1]),
                                        context_len-i);
      }
   }

   if (trans.sourceWordsNotCovered.empty())
   {
      for (Uint j = 0; j < lmWeightsV.size(); j++)
      {
         // Compute score for j-th language model for end-of-sentence
         results[j] += lms[j]->wordProb(tgt_vocab.index(PLM::SentEnd),
            &(endPhrase[0]), context_len+1);
      }
   }
} // getRawLM

double BasicModelGenerator::dotProductLM(
   const vector<double> &weights,
   const PartialTranslation &trans)
{
   // This function is too complicated to replicate, so we'll just, rather
   // inefficiently, call getRawLM.
   vector<double> lmVals;
   getRawLM(lmVals, trans);
   return dotProduct(lmVals, weights, weights.size());
}

void BasicModelGenerator::getRawTrans(vector<double> &transVals,
        const PartialTranslation &trans)
{
   // This comes directly from the MultiTransPhraseInfo object.
   MultiTransPhraseInfo *phrase = dynamic_cast<MultiTransPhraseInfo *>(trans.lastPhrase);
   assert(phrase != NULL);
   transVals.insert(transVals.end(), phrase->phrase_trans_probs.begin(),
         phrase->phrase_trans_probs.end());
} // getRawTrans

double BasicModelGenerator::dotProductTrans(const vector<double> &weights,
        const PartialTranslation &trans)
{
   // This value was already calculated by PhraseTable::getPhrases!!!
   return trans.lastPhrase->phrase_trans_prob;
}

void BasicModelGenerator::getRawForwardTrans(vector<double> &forwardVals,
        const PartialTranslation &trans)
{
   if (forwardWeightsV.empty()) return;

   // This comes from the ForwardBackwardPhraseInfo object, assuming
   // we have forward weights, as asserted above.
   ForwardBackwardPhraseInfo *phrase =
      dynamic_cast<ForwardBackwardPhraseInfo *>(trans.lastPhrase);
   assert(phrase != NULL);
   forwardVals.insert(forwardVals.end(), phrase->forward_trans_probs.begin(),
         phrase->forward_trans_probs.end());
}

double BasicModelGenerator::dotProductForwardTrans(const vector<double> &weights,
        const PartialTranslation &trans)
{
   if (weights.empty()) return 0.0;

   // This comes from the ForwardBackwardPhraseInfo object, assuming
   // we have forward weights, as checked above.
   ForwardBackwardPhraseInfo *phrase =
      dynamic_cast<ForwardBackwardPhraseInfo *>(trans.lastPhrase);
   assert(phrase != NULL);
   return phrase->forward_trans_prob;
}

void BasicModelGenerator::getRawFeatures(vector<double> &ffVals,
        const PartialTranslation &trans)
{
   // The other features' weights come from their individual objects
   ffVals.reserve(ffVals.size() + decoder_features.size());
   for (Uint k = 0; k < decoder_features.size(); ++k) {
      ffVals.push_back(decoder_features[k]->score(trans));
   }
}

double BasicModelGenerator::dotProductFeatures(const vector<double> &weights,
        const PartialTranslation &trans)
{
   double result = 0.0;
   for (Uint k = 0; k < decoder_features.size(); ++k) {
      result += decoder_features[k]->score(trans) * weights[k];
   }
   return result;
}

void BasicModelGenerator::getStringPhrase(string &s, const Phrase &uPhrase)
{
   s.clear();
   if (uPhrase.empty()) return;
   bool first(true);
   for ( Phrase::const_iterator word(uPhrase.begin());
         word != uPhrase.end(); ++word ) {
      assert(*word < tgt_vocab.size());
      if ( first )
         first = false;
      else
         s.append(" ");
      s.append(tgt_vocab.word(*word));
   }
} // getStringPhrase

Uint BasicModelGenerator::getUintWord(const string &word)
{
   Uint index = tgt_vocab.index(word.c_str());
   if ( index == tgt_vocab.size() ) return PhraseDecoderModel::OutOfVocab;
   else return index;
} // getUintWord


/**
 * Help seed a vector of random distribution while changing the seed between
 * each seeding.
 * @param v  vector of random distribution to seed
 * @param seed  seed to use and increment
 */
static void seed_helper(vector<rnd_distribution*>& v, unsigned int& seed)
{
   typedef vector<rnd_distribution*>::iterator IT;
   for (IT it(v.begin()); it!=v.end(); ++it) {
      assert(*it);
      (*it)->seed(seed);
      seed += 5879;
   }
   //for_each(v.begin(), v.end(),
   //   bind2nd(mem_fun(&rnd_distribution::seed), seed));
}

void BasicModelGenerator::setRandomWeights(unsigned int seed) {
   seed_helper(random_feature_weight, seed);
   seed_helper(random_trans_weight, seed);
   seed_helper(random_forward_weight, seed);
   seed_helper(random_lm_weight, seed);

   setRandomWeights();
}


void BasicModelGenerator::setRandomWeights() {
   // For each feature weights in the vector, take the corresponding random
   // weight distribution and ask it for a random weight by using the class's
   // funcion "operator()"
   assert(random_feature_weight.size() == featureWeightsV.size());
   transform(random_feature_weight.begin(), random_feature_weight.end(),
         featureWeightsV.begin(), mem_fun(&rnd_distribution::operator()));

   assert(random_trans_weight.size() == transWeightsV.size());
   transform(random_trans_weight.begin(), random_trans_weight.end(),
         transWeightsV.begin(), mem_fun(&rnd_distribution::operator()));

   assert(random_forward_weight.size() == forwardWeightsV.size());
   transform(random_forward_weight.begin(), random_forward_weight.end(),
         forwardWeightsV.begin(), mem_fun(&rnd_distribution::operator()));

   assert(random_lm_weight.size() == lmWeightsV.size());
   transform(random_lm_weight.begin(), random_lm_weight.end(),
         lmWeightsV.begin(), mem_fun(&rnd_distribution::operator()));
}


BasicModel::BasicModel(const vector<string> &src_sent,
                       vector<PhraseInfo *> **phrases,
                       double **futureScore,
                       BasicModelGenerator &parent) :
   c(parent.c), src_sent(src_sent), lmWeights(parent.lmWeightsV),
   transWeights(parent.transWeightsV), forwardWeights(parent.forwardWeightsV),
   featureWeights(parent.featureWeightsV),
   phrases(phrases), futureScore(futureScore), parent(parent) {}

BasicModel::~BasicModel()
{
   for (Uint i = 0; i < src_sent.size(); i++) {
      for (Uint j = 0; j < src_sent.size() - i; j++) {
         for (vector<PhraseInfo *>::iterator it = phrases[i][j].begin(); it !=
               phrases[i][j].end(); it++) {
            delete *it;
         }
      }
      delete [] phrases[i];
      delete [] futureScore[i];
   }
   delete [] phrases;
   delete [] futureScore;
} // ~BasicModel


void BasicModel::setContext(const PartialTranslation& trans)
{
   for (vector<DecoderFeature *>::iterator it(parent.decoder_features.begin());
         it != parent.decoder_features.end(); ++it)
   {
      (*it)->setContext(trans);
   }
}

double BasicModel::scoreTranslation(const PartialTranslation &trans, Uint verbosity)
{
   assert(trans.lastPhrase != NULL);
   assert(trans.back != NULL);
   assert(trans.back->lastPhrase != NULL);

   // Translation score
   const double transScore = parent.dotProductTrans(transWeights, trans);
   if (verbosity >= 3) cerr << "\ttranslation score " << transScore << endl;

   // Forward translation model score
   const double forwardScore = parent.dotProductForwardTrans(forwardWeights, trans);
   if (verbosity >= 3) cerr << "\tforward trans score " << forwardScore << endl;

   // LM score
   static vector<double> lmVals;
   lmVals.clear();
   parent.getRawLM(lmVals, trans);
   const double lmScore = dotProduct(lmVals, lmWeights, lmWeights.size());
   if (verbosity >= 3) cerr << "\tlanguage model score " << lmScore << endl;

   // Other feature scores
   const double ffScore = parent.dotProductFeatures(featureWeights, trans);
   if (verbosity >= 3) cerr << "\tother features score " << ffScore << endl;

   return transScore + forwardScore + lmScore + ffScore;
} // scoreTranslation

/*
 * Estimate future score.  Returns the difference between the future score for
 * the given translation and the score for the translation.  Specifically, the
 * future score estimation takes is the maximum phrase translation score +
 * unigram language model score + distortion score for all possible
 * translations of the remaining source words.  Note: the computation here
 * makes use of the condition that the ranges in trans.sourceWordsNotCovered
 * are in ascending order.
 * @param trans   The translation to score
 * @return        The incremental log future-probability.
 */
double BasicModel::computeFutureScore(const PartialTranslation &trans)
{
   // EJJ, Nov 2007:
   // The distortion limit test is now done when considering candidate phrase,
   // so it is no longer necessary to do it again here.

   double precomputedScore = 0;
   Uint lastEnd = trans.lastPhrase->src_words.end;
   for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin(); it !=
         trans.sourceWordsNotCovered.end(); it++)
   {
      assert(it->start < it->end);
      assert(it->end <= src_sent.size());
      precomputedScore += futureScore[it->start][it->end - it->start - 1];
      lastEnd = it->end;
   } // for

   // Other features
   double ffScore = 0;
   for (Uint k = 0; k < parent.decoder_features.size(); ++k) {
      ffScore += parent.decoder_features[k]->futureScore(trans)
                 * featureWeights[k];
   }

   return precomputedScore + ffScore;
} // computeFutureScore

Uint BasicModel::computeRecombHash(const PartialTranslation &trans)
{
   // This might overflow result, but who cares.
   Uint result = 0;
   // make this endPhrase static so we don't have to reallocate this vector
   // millions of times.  (this method gets called ridiculously often...)
   static Phrase endPhrase;
   endPhrase.clear();
   trans.getLastWords(endPhrase, parent.lm_numwords - 1);
   for (Phrase::const_iterator it = endPhrase.begin();
         it != endPhrase.end(); it++)
   {
      result += *it;
      result = result * parent.tgt_vocab.size();
   } // for
   for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin();
         it != trans.sourceWordsNotCovered.end(); it++)
   {
      result += it->start + it->end * src_sent.size();
   } // for
   for (vector<DecoderFeature *>::iterator it = parent.decoder_features.begin();
         it != parent.decoder_features.end(); ++it)
   {
      result += (*it)->computeRecombHash(trans);
   }
   return result;
} // computeRecombHash

/**
 * Note: the computation here makes use of the condition that the ranges in the
 * sourceWordsNotCovered vectors are in ascending order.
 */
bool BasicModel::isRecombinable(const PartialTranslation &trans1,
        const PartialTranslation &trans2)
{
   bool result =
      trans1.sameLastWords(trans2,parent.lm_numwords - 1) &&
      trans1.sourceWordsNotCovered == trans2.sourceWordsNotCovered;
   for (vector<DecoderFeature *>::iterator it = parent.decoder_features.begin();
         result && it != parent.decoder_features.end(); ++it)
   {
      result = result && (*it)->isRecombinable(trans1, trans2);
   }

   return result;
} // isRecombinable

void BasicModel::getFeatureFunctionVals(vector<double> &vals,
        const PartialTranslation &trans)
{
   // The order of the features here must be the same as in
   // BasicModelGenerator::describeModel().
   if (trans.back == NULL) {
      vals.insert(vals.end(),
            parent.getNumFFs() + parent.getNumLMs() + parent.getNumTMs() + parent.getNumFTMs(),
            0);
   } else {
      parent.getRawFeatures(vals, trans);
      parent.getRawLM(vals, trans);
      parent.getRawTrans(vals, trans);
      parent.getRawForwardTrans(vals, trans);
   }
} // getFeatureFunctionVals

void BasicModel::getTotalFeatureFunctionVals(vector<double> &vals,
        const PartialTranslation &trans)
{
   assert(vals.empty());
   getFeatureFunctionVals(vals, trans);
   if (trans.back != NULL) {
      vector<double> last;
      getTotalFeatureFunctionVals(last, *(trans.back));
      assert(vals.size() == last.size());
      for (Uint i = 0; i < last.size(); i++) {
         vals[i] += last[i];
      }
   }
} // getTotalFeatureFunctionVals

