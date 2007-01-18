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
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "phrasedecoder_model.h"
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
   if ( c.distModelName != "none" ) {
      decoder_features.push_back(DecoderFeature::create(this, "DistortionModel",
                                                        c.distModelName, c.distModelArg));
      featureWeightsV.push_back(c.distWeight[0]);
   } else {
      cerr << "Not using any distortion model" << endl;
   }
   
   decoder_features.push_back(DecoderFeature::create(this, "LengthFeature", "", ""));
   featureWeightsV.push_back(c.lengthWeight);

   if ( c.segModelName != "none" ) {
      decoder_features.push_back(DecoderFeature::create(this, "SegmentationModel",
         c.segModelName, c.segModelArgs));
      featureWeightsV.push_back(c.segWeight[0]);
   } else {
      cerr << "Not using any segmentation model" << endl;
   }

}


BasicModelGenerator* BasicModelGenerator::create(
        const CanoeConfig& c,
        const vector<vector<string> > *sents,
        const vector<vector<MarkedTranslation> > *marks)
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
         result = new BasicModelGenerator(c, *sents, *marks);
      else
         result = new BasicModelGenerator(c);
   }

   for (Uint i = 0; i < c.backPhraseFiles.size(); i++) {
      if (!c.forPhraseFiles.empty()) {
         if (!c.forwardWeights.empty()) {
            result->addTranslationModel(c.backPhraseFiles[i].c_str(),
                    c.forPhraseFiles[i].c_str(),
                    c.transWeights[i], c.forwardWeights[i]);
         } else {
            result->addTranslationModel(c.backPhraseFiles[i].c_str(),
                    c.forPhraseFiles[i].c_str(),
                    c.transWeights[i]);
         }
      } else {
         result->addTranslationModel(c.backPhraseFiles[i].c_str(),
                 c.transWeights[i]);
      }
   }

   Uint weights_already_used = c.backPhraseFiles.size();
   for (Uint i = 0; i < c.multiProbTMFiles.size(); i++) {
      Uint model_count =
         PhraseTable::countProbColumns(c.multiProbTMFiles[i].c_str()) / 2;
      vector<double> backward_weights, forward_weights;
      assert(c.transWeights.size() >= weights_already_used + model_count);
      backward_weights.assign(c.transWeights.begin() + weights_already_used,
         c.transWeights.begin() + weights_already_used + model_count);
      if ( c.forwardWeights.size() > 0 ) {
         assert(c.forwardWeights.size() >= weights_already_used + model_count);
         forward_weights.assign(c.forwardWeights.begin() + weights_already_used,
            c.forwardWeights.begin() + weights_already_used + model_count);
      }
      result->addMultiProbTransModel(c.multiProbTMFiles[i].c_str(),
         backward_weights, forward_weights);
      weights_already_used += model_count;
   }

   assert ( weights_already_used == c.transWeights.size() );
   for (Uint i = 0; i < c.lmFiles.size(); i++) {
      result->addLanguageModel(c.lmFiles[i].c_str(), c.lmWeights[i], c.lmOrder);
   }

   return result;
} // BasicModelGenerator::create()

BasicModelGenerator::BasicModelGenerator(const CanoeConfig& c,
                                         PhraseTable *phraseTable):
   verbosity(c.verbosity),
   phraseTable(phraseTable), limitPhrases(false),
   numTextTransWeights(0),
   lm_numwords(1),
   bypassMarked(c.bypassMarked), addWeightMarked(log(c.weightMarked)),
   phraseTableSizeLimit(c.phraseTableSizeLimit),
   phraseTableThreshold(c.phraseTableThreshold), distLimit(c.distLimit)
{
   if ( this->phraseTable == NULL ) {
      this->phraseTable = new PhraseTable(tgt_vocab, c.phraseTablePruneType.c_str());
   }
   InitDecoderFeatures(c);
}

BasicModelGenerator::BasicModelGenerator(const CanoeConfig &c,
                                         const vector<vector<string> > &src_sents,
                                         const vector<vector<MarkedTranslation> > &marks,
                                         PhraseTable *phraseTable):
   verbosity(c.verbosity),
   phraseTable(phraseTable), limitPhrases(!c.loadFirst), numTextTransWeights(0),
   lm_numwords(1),
   bypassMarked(c.bypassMarked),
   addWeightMarked(log(c.weightMarked)), phraseTableSizeLimit(c.phraseTableSizeLimit),
   phraseTableThreshold(c.phraseTableThreshold), distLimit(c.distLimit)
{
    if ( this->phraseTable == NULL ) {
        this->phraseTable = new PhraseTable(tgt_vocab, c.phraseTablePruneType.c_str());
    }
    InitDecoderFeatures(c);
    if (limitPhrases)
    {
        // Enter all source phrases into the phrase table, and into the vocab
        // in case they are OOVs we need to copy through to the output.
        for (vector<vector<string> >::const_iterator it = src_sents.begin(); it !=
                src_sents.end(); ++it)
        {
            const char *s[it->size()];
            for (Uint i = 0; i < it->size(); i++)
            {
                s[i] = (*it)[i].c_str();
                tgt_vocab.add(s[i]);
            } // for

            for (Uint i = 0; i < it->size(); i++)
            {
                this->phraseTable->addPhrase(s + i, it->size() - i);
            } // for
        } // for

        // Add target words from marks to the vocab
        for (vector< vector<MarkedTranslation> >::const_iterator it = marks.begin();
             it != marks.end(); ++it)
        {
            for (vector<MarkedTranslation>::const_iterator jt = it->begin();
                 jt != it->end(); ++jt)
            {
                for (vector<string>::const_iterator kt = jt->markString.begin();
                     kt != jt->markString.end(); ++kt)
                {
                    tgt_vocab.add(kt->c_str());
                } // for
            } // for
        } // for
    } // if
} // BasicModelGenerator

BasicModelGenerator::~BasicModelGenerator()
{
    // EJJ 11Jul2006 Optimization: don't delete the features, we're about
    // to exit, so we can let the OS recover the memory for us.
    return;

    for (vector<DecoderFeature *>::iterator it = decoder_features.begin();
         it != decoder_features.end(); ++it)
    {
        delete *it;
    }

    if (phraseTable)
    {
        delete phraseTable;
        phraseTable = NULL;
    }

    for (vector<PLM *>::iterator it = lms.begin(); it != lms.end(); ++it)
    {
        delete *it;
    } // for

} // ~BasicModelGenerator

void BasicModelGenerator::filterTranslationModel(const char *const src_to_tgt_file
   , const string& oSrc2tgt
   , const char *const tgt_to_src_file
   , const string& oTgt2src)
{
   OMagicStream src_filtered(oSrc2tgt);

   if (tgt_to_src_file != NULL)
   {
      OMagicStream tgt_filtered(oTgt2src);
      
      phraseTable->read(src_to_tgt_file, tgt_to_src_file, limitPhrases,
         &src_filtered, &tgt_filtered);
   }
   else
   {
      phraseTable->read(src_to_tgt_file, tgt_to_src_file, limitPhrases,
         &src_filtered, NULL);
   }
}

void BasicModelGenerator::filterMultiProbTransModel(
   const char * multi_prob_tm_file, const string& oFiltered)
{
   OMagicStream filtered(oFiltered);
   phraseTable->readMultiProb(multi_prob_tm_file, limitPhrases, &filtered);
}

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
   forwardWeightsV.insert(forwardWeightsV.begin() + numTextTransWeights,  forward_weight);
   addTranslationModel(src_to_tgt_file, tgt_to_src_file, weight);
} // addTranslationModel

void BasicModelGenerator::addTranslationModel(const char *src_to_tgt_file, double weight)
{
   addTranslationModel(src_to_tgt_file, NULL, weight);
} // addTranslationModel

void BasicModelGenerator::addMultiProbTransModel(
   const char *multi_prob_tm_file, vector<double> backward_weights,
   vector<double> forward_weights)
{

   Uint multi_prob_col_count = 
      phraseTable->countProbColumns(multi_prob_tm_file);
   if ( backward_weights.size() * 2 != multi_prob_col_count ) 
      error(ETFatal, "wrong number of backward weights (%d) for %s",
            backward_weights.size(), multi_prob_tm_file);
   if ( forward_weights.size() != 0 &&
        forward_weights.size() != backward_weights.size() )
      error(ETFatal, "wrong number of forward weights (%d) for %s",
            forward_weights.size(), multi_prob_tm_file);
   phraseTable->readMultiProb(multi_prob_tm_file, limitPhrases);

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
    if (!check_if_exists(lmFile))
        error(ETFatal, "Cannot read from language model file %s", lmFile);
    cerr << "loading language model from " << lmFile << endl;
    //time_t start_time = time(NULL);
    PLM *lm = PLM::Create(lmFile, &tgt_vocab, false, limitPhrases,
                          limit_order, LOG_ALMOST_0, os_filtered);
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
   for ( vector<DecoderFeature*>::const_iterator it = decoder_features.begin();
         it != decoder_features.end(); ++it )
      description << (*it)->describeFeature() << endl;
   for ( vector<PLM*>::const_iterator it = lms.begin(); it != lms.end(); ++it )
      description << (*it)->describeFeature() << endl;
   description << phraseTable->describePhraseTables(!forwardWeightsV.empty());

   return description.str();
} // describeModel

PhraseDecoderModel *BasicModelGenerator::createModel(const vector<string> &src_sent,
                                                     const vector<MarkedTranslation> &marks,
                                                     bool alwaysTryDefault,
                                                     vector<bool>* oovs)
{
   vector<PhraseInfo *> **phrases = createAllPhraseInfos(src_sent, marks,
                                                         alwaysTryDefault, oovs);
   for ( vector<DecoderFeature *>::iterator it = decoder_features.begin();
         it != decoder_features.end(); ++it)
   {
      (*it)->newSrcSent(src_sent, phrases);
   }

   for(Uint i=0;i<lms.size();++i)
      lms[i]->clearCache();

   double **futureScores = precomputeFutureScores(phrases, src_sent.size());

   return new BasicModel(src_sent, phrases, futureScores, *this);
} // createModel

vector<PhraseInfo *> **BasicModelGenerator::createAllPhraseInfos(const vector<string> &src_sent,
                                                                 const vector<MarkedTranslation> &marks,
                                                                 bool alwaysTryDefault,
                                                                 vector<bool>* oovs)
{
    assert(transWeightsV.size() > 0);
    // Initialize the triangular array
    vector<PhraseInfo *> **result = CreateTriangularArray<vector<PhraseInfo *> >()
        (src_sent.size());

    if (oovs) oovs->assign(src_sent.size(), false);

    // Keep track of which ranges to skip (because they are marked)
    vector<Range> rangesToSkip;
    addMarkedPhraseInfos(marks, result, rangesToSkip, src_sent.size());
    sort(rangesToSkip.begin(), rangesToSkip.end());

    phraseTable->getPhraseInfos(result, src_sent, transWeightsV, phraseTableSizeLimit,
            log(phraseTableThreshold), rangesToSkip, verbosity, 
            (forwardWeightsV.empty() ? NULL : &forwardWeightsV));

    // Use an iterator to go through the ranges to skip, so that we avoid these when
    // creating default translations
    vector<Range>::const_iterator rangeIt = rangesToSkip.begin();

    for (Uint i = 0; i < src_sent.size(); i++)
    {
        while (rangeIt != rangesToSkip.end() && rangeIt->end <= i)
        {
            rangeIt++;
        } // while
        if ((result[i][0].size() == 0 && (rangeIt == rangesToSkip.end() || i <
                    rangeIt->start)) || alwaysTryDefault)
        {
            // The word has no translation and is not in a range to skip, so create a
            // default translation (use the source word as itself).
            Range curRange(i, i + 1);
            result[i][0].push_back(makeNoTransPhraseInfo(curRange, src_sent[i].c_str()));
            if (oovs) (*oovs)[i] = true;
        } // if
    } // for
    return result;
} // createAllPhraseInfos

void BasicModelGenerator::addMarkedPhraseInfos(const vector<MarkedTranslation> &marks,
        vector<PhraseInfo *> **result, vector<Range> &rangesToSkip, Uint sentLength)
{
    // Compute the total weight on phrase translation models, in order to denormalize the
    // probabilities
    double totalWeight = 0;
    for (vector<double>::const_iterator it = transWeightsV.begin(); it !=
            transWeightsV.end(); it++)
    {
        totalWeight += *it;
    } // for
    for (vector<MarkedTranslation>::const_iterator it = marks.begin(); it !=
            marks.end(); it++)
    {
        // Create a MultiTransPhraseInfo for each marked phrase
        MultiTransPhraseInfo *newPI;
        if ( ! forwardWeightsV.empty() ) {
            ForwardBackwardPhraseInfo* newFBPI = new ForwardBackwardPhraseInfo;
            newFBPI->forward_trans_prob = (it->log_prob + addWeightMarked) * totalWeight;
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
        for (vector<string>::const_iterator jt = it->markString.begin(); jt !=
                it->markString.end(); jt++)
        {
            newPI->phrase.push_back(tgt_vocab.index(jt->c_str()));
        } // for

        // Store it in the appropriate location in the triangular array
        result[newPI->src_words.start][newPI->src_words.end - newPI->src_words.start - 1].push_back(newPI);

        // If bypassMarked option isn't being used, remember to skip this phrase when
        // looking for normal phrase options
        if (!bypassMarked)
        {
            rangesToSkip.push_back(it->src_words);
        } // if
    } // for
} // addMarkedPhraseInfos

PhraseInfo *BasicModelGenerator::makeNoTransPhraseInfo(const Range &range, const char *word)
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
    newPI->phrase_trans_probs.insert(newPI->phrase_trans_probs.end(), transWeightsV.size(),
            LOG_ALMOST_0);
    newPI->phrase_trans_prob = dotProduct(newPI->phrase_trans_probs, transWeightsV,
            transWeightsV.size());
    return newPI;
} // makeNoTransPhraseInfo

double **BasicModelGenerator::precomputeFutureScores(vector<PhraseInfo *> **phrases, Uint sentLength)
{
    double **result = CreateTriangularArray<double>()(sentLength);
    for (Uint i = 0; i < sentLength; i++)
    {
        for (int j = i; j >= 0; j--)
        {
            if (verbosity >= 4) {
                cerr << "Precomputing future score for [" << j << "," << (i+1) << ")" << endl;
            }
            // Compute future score for range [j, i + 1), by maximizing the translation
            // probability over all possible translations of that range.  ie.
            // future score for [j, i + 1) = max(best score for a direct translation,
            //  max_{j < k < i + 1} (future score for [j, k) + future score for [k, i + 1)) )

            // The order of iteration of ranges ensures that all future scores we already
            // need have already been computed.
            double &curScore = result[j][i - j];
            curScore = -INFINITY;
            for (vector<PhraseInfo *>::const_iterator it = phrases[j][i - j].begin(); it
                    != phrases[j][i - j].end(); it++)
            {
                // Translation score
                double newScore = dotProduct(((MultiTransPhraseInfo *)
                                  *it)->phrase_trans_probs, transWeightsV, transWeightsV.size());

                // Add unigram LM score
                for (Phrase::const_iterator jt = (*it)->phrase.begin(); jt <
                        (*it)->phrase.end(); jt++)
                {
                    for (Uint k = 0; k < lmWeightsV.size(); k++)
                    {
                        newScore += lms[k]->wordProb(*jt, NULL, 0) * lmWeightsV[k];
                    } // for
                } // for

                // Other feature scores
                for (Uint k = 0; k < decoder_features.size(); k++)
                {
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
                    cerr << " phrase " << stringPhrase << " score " << newScore << endl;
                }
            } // for

            //if (i == j) assert(curScore > -INFINITY);
            // See if we can do any better by combining range [j, k) with [k, i + 1), for
            // j < k < i + 1.
            for (Uint k = j + 1; k < i + 1; k++)
            {
                curScore = max(curScore, result[j][k - j - 1] + result[k]
                        [i - k]);
            } // for
            if (verbosity >= 4) {
                cerr << "Future score for [" << j << "," << (i+1) << ") is "
                     << curScore << endl;
            }
        } // for
    } // for
    assert(sentLength == 0 || result[0][sentLength - 1] > -INFINITY);

    if (verbosity >= 3)
    {
        for (Uint i = 0; i < sentLength; i++)
        {
            for (Uint j = 0; j < sentLength - i; j++)
            {
                cerr << "future score from " << i << " to " << (i + j) << " is " <<
                    result[i][j] << endl;
            } // for
        } // for
    } // if
    return result;
} // precomputeFutureScores

void BasicModelGenerator::getRawLM(vector<double> &lmVals,
        const PartialTranslation &trans)
{
    Phrase endPhrase;
    trans.getLastWords(endPhrase, trans.lastPhrase->phrase.size() + lm_numwords - 1);
    Uint context_len;

    // Put the phrase into an array backwards (because context works backwards)
    Uint reverseArray[endPhrase.size() + 1];
    for (Uint i = 0; i < endPhrase.size(); i++)
    {
        reverseArray[endPhrase.size() - i - 1] = endPhrase[i];
    } // for

    if (endPhrase.size() < trans.lastPhrase->phrase.size() + lm_numwords - 1)
    {
        // Start of sentence
        reverseArray[endPhrase.size()] = tgt_vocab.index(PLM::SentStart);
        context_len = endPhrase.size();
    } else
    {
        context_len = endPhrase.size()-1;
    } // if

    // Make room for results in vector
    lmVals.insert(lmVals.end(), lmWeightsV.size(), 0);
    // Create an iterator at the start of the results for convenience
    vector<double>::iterator results = lmVals.end() - lmWeightsV.size();

    for (int i = trans.lastPhrase->phrase.size() - 1; i >= 0; i--)
    {
        for (Uint j = 0; j < lmWeightsV.size(); j++)
        {
            // Compute score for j-th language model for word reverseArray[i]
            results[j] += lms[j]->wordProb(reverseArray[i], reverseArray + i + 1,context_len-i);
        } // for
    } // for

    if (trans.sourceWordsNotCovered.size() == 0)
    {
        for (Uint j = 0; j < lmWeightsV.size(); j++)
        {
            // Compute score for j-th language model for end-of-sentence
            results[j] += lms[j]->wordProb(tgt_vocab.index(PLM::SentEnd),
                reverseArray, context_len+1);
        } // for
    } // if
} // getRawLM

double BasicModelGenerator::dotProductLM(const vector<double> &weights,
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
    if (uPhrase.size() == 0) return;
    Uint i = 0;
    while (true)
    {
        assert(uPhrase[i] < tgt_vocab.size());
        s.append(tgt_vocab.word(uPhrase[i]));
        i++;
        if (i == uPhrase.size())
        {
            break;
        } // if
        s.append(" ");
    } // while
} // getStringPhrase

Uint BasicModelGenerator::getUintWord(const string &word)
{
    Uint index = tgt_vocab.index(word.c_str());
    if ( index == tgt_vocab.size() ) return PhraseDecoderModel::OutOfVocab;
    else return index;
} // getUintWord

double
BasicModelGenerator::uniformWeight() {
    return (double)rand() / RAND_MAX;
}

void
BasicModelGenerator::setRandomWeights() {
    for (vector<double>::iterator w=featureWeightsV.begin(); w!=featureWeightsV.end(); w++)
        (*w) = uniformWeight();
    for (vector<double>::iterator w=transWeightsV.begin(); w!=transWeightsV.end(); w++)
        (*w) = uniformWeight();
    for (vector<double>::iterator w=forwardWeightsV.begin(); w!=forwardWeightsV.end(); w++)
        (*w) = uniformWeight();
    for (vector<double>::iterator w=lmWeightsV.begin(); w!=lmWeightsV.end(); w++)
        (*w) = uniformWeight();
}


BasicModel::BasicModel(const vector<string> &src_sent,
                       vector<PhraseInfo *> **phrases,
                       double **futureScore,
                       BasicModelGenerator &parent) :
   src_sent(src_sent), lmWeights(parent.lmWeightsV),
   transWeights(parent.transWeightsV), forwardWeights(parent.forwardWeightsV),
   featureWeights(parent.featureWeightsV), distLimit(parent.distLimit),
   phrases(phrases), futureScore(futureScore), parent(parent) {}

BasicModel::~BasicModel()
{
    for (Uint i = 0; i < src_sent.size(); i++)
    {
        for (Uint j = 0; j < src_sent.size() - i; j++)
        {
            for (vector<PhraseInfo *>::iterator it = phrases[i][j].begin(); it !=
                    phrases[i][j].end(); it++)
            {
                delete *it;
            } // for
        } // for
        delete [] phrases[i];
        delete [] futureScore[i];
    } // for
    delete [] phrases;
    delete [] futureScore;
} // ~BasicModel


void BasicModel::setContext(const PartialTranslation& trans)
{
    for (vector<DecoderFeature *>::iterator it = parent.decoder_features.begin();
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
    double transScore = parent.dotProductTrans(transWeights, trans);
    if (verbosity >= 3) cerr << "\ttranslation score " << transScore << endl;

    // Forward translation model score
    double forwardScore = parent.dotProductForwardTrans(forwardWeights, trans);
    if (verbosity >= 3) cerr << "\tforward trans score " << forwardScore << endl;

    // LM score
    vector<double> lmVals;
    parent.getRawLM(lmVals, trans);
    double lmScore = dotProduct(lmVals, lmWeights, lmWeights.size());
    if (verbosity >= 3) cerr << "\tlanguage model score " << lmScore << endl;

    // Other feature scores
    double ffScore = parent.dotProductFeatures(featureWeights, trans);
    if (verbosity >= 3) cerr << "\tother features score " << ffScore << endl;

    return transScore + lmScore + ffScore;
} // scoreTranslation

/**
 * Estimate future score.  Returns the difference between the future score for the
 * given translation and the score for the translation.  Specifically, the future score
 * estimation takes is the maximum phrase translation score + unigram language model score
 * + distortion score for all possible translations of the remaining source words.  Note:
 * the computation here makes use of the condition that the ranges in
 * trans.sourceWordsNotCovered are in ascending order.
 * @param trans	The translation to score
 * @return	The incremental log future-probability.
 *
 * TODO: incorporate new distortion model score into this
 */
double BasicModel::computeFutureScore(const PartialTranslation &trans)
{
    // To prevent distortion limit violations later, it is checked that the distance from
    // the current position to the first untranslated word is at most the distortion
    // limit and that the distance between ranges of untranslated words is at most the
    // distortion limit.  This is not an iff condition - I am being more conservative and
    // penalizing some partial translations for which it is possible to complete without
    // violating the distortion limit.
    // I am confident however that there is an iff condition that could be checked in
    // O(trans.sourceWordsNotCovered.size()).

    double precomputedScore = 0;
    Uint lastEnd = trans.lastPhrase->src_words.end;
    for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin(); it !=
            trans.sourceWordsNotCovered.end(); it++)
    {
        assert(it->start < it->end);
        assert(it->end <= src_sent.size());
        precomputedScore += futureScore[it->start][it->end - it->start - 1];
        if (distLimit != NO_MAX_DISTORTION && abs((int)lastEnd - (int)it->start) > distLimit)
        {
            // May not be possible to finish this translation without violating distortion
            // limit, so return -INFINITY.
            return -INFINITY;
        } // if
        lastEnd = it->end;
    } // for

    // Other features
    double ffScore = 0;
    for (Uint k = 0; k < parent.decoder_features.size(); ++k) {
        ffScore += parent.decoder_features[k]->futureScore(trans) * featureWeights[k];
    }

    return precomputedScore + ffScore;
} // computeFutureScore

Uint BasicModel::computeRecombHash(const PartialTranslation &trans)
{
    // This might overflow result, but who cares.
    Uint result = 0;
    Phrase endPhrase;
    trans.getLastWords(endPhrase, parent.lm_numwords - 1);
    for (Phrase::const_iterator it = endPhrase.begin(); it != endPhrase.end(); it++)
    {
        result += *it;
        result = result * parent.tgt_vocab.size();
    } // for
    for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin(); it !=
            trans.sourceWordsNotCovered.end(); it++)
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
    if (trans.back == NULL)
    {
        vals.insert(vals.end(),
            parent.getNumFFs() + parent.getNumLMs() + parent.getNumTMs() + parent.getNumFTMs(),
            0);
    } else
    {
        parent.getRawFeatures(vals, trans);
        parent.getRawLM(vals, trans);
        parent.getRawTrans(vals, trans);
        parent.getRawForwardTrans(vals, trans);
    } // if
} // getFeatureFunctionVals

void BasicModel::getTotalFeatureFunctionVals(vector<double> &vals,
        const PartialTranslation &trans)
{
    assert(vals.size() == 0);
    getFeatureFunctionVals(vals, trans);
    if (trans.back != NULL)
    {
        vector<double> last;
        getTotalFeatureFunctionVals(last, *(trans.back));
        assert(vals.size() == last.size());
        for (Uint i = 0; i < last.size(); i++)
        {
            vals[i] += last[i];
        } // for
    } // if
} // getTotalFeatureFunctionVals
