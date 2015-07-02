/**
 * @author Eric Joanis
 * @file bilm_model.cc  BiLM decoder feature implementation
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include "bilm_model.h"
#include "bilm_annotation.h"
#include "lm.h"
#include "vocab_filter.h"
#include "phrasetable.h"
#include "basicmodel.h"

using namespace Portage;

Uint BiLMModel::nextBiLM_ID = 1; // starts at 1; 0 is for the regular LMs

void BiLMModel::getLastBiWordsBackwards(VectorPhrase &biWords, Uint num, const PartialTranslation& trans)
{
   biWords.clear();
   const PartialTranslation* pt = &trans;
   // End condition is pt->back != NULL, which flags the PartialTranslation
   // associated with the initial decoder state, because it has a plain
   // PhraseInfo object with an empty phrase, and thus no bi_phrase.
   while (pt->back != NULL && num > 0) {
      BiLMAnnotation* ann = annotator->get(pt->lastPhrase->annotations);
      assert(ann && "if this assertion fails, a phrase pair did not get its BiLMAnnotation initialized correctly");
      const Phrase& bi_phrase(ann->getBiPhrase());
      Phrase::const_reverse_iterator w_it(bi_phrase.rbegin());
      Phrase::const_reverse_iterator w_end(bi_phrase.rend());
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

static const bool debug_coarse_bilm = false;

bool BiLMModel::checkFileExists(const string& bilm_specification, vector<string>* list)
{
   vector<string> tokens;
   split(bilm_specification, tokens, ";");
   if (tokens.empty()) {
      cerr << "empty BiLM specification: " << bilm_specification << endl;
      return false;
   }
   if (tokens.size() > 4) {
      cerr << "BiLM specification has superfluous options: " << bilm_specification << endl;
      return false;
   }
   bool ok = true;
   if (!PLM::checkFileExists(tokens[0])) {
      cerr << "Can't access LM " << tokens[0] << " in BiLM " << bilm_specification << endl;
      ok = false;
   }
   if (list)
      PLM::checkFileExists(tokens[0], list);
   for (Uint i = 1; i < tokens.size(); ++i) {
      vector<string> keyvalue;
      split(tokens[i], keyvalue, "=", 2);
      if (keyvalue.size() != 2) {
         cerr << "Bad class file specification: " << tokens[i] << " in BiLM " << bilm_specification << endl;
         ok = false;
         continue;
      }
      const string class_file = keyvalue[1];
      if (!check_if_exists(class_file)) {
         cerr << "Can't access class file " << class_file << " in BiLM " << bilm_specification << endl;
         ok = false;
      }
      if (list) list->push_back(class_file);
      const string key=keyvalue[0];
      if (key != "cls(src)" && key != "cls(tgt)" && key != "cls(tgt/src)") {
         cerr << "Don't know how to interpret " << tokens[i] << " in BiLM " << bilm_specification << endl;
         ok = false;
      }
   }
   return ok;
}

Uint64 BiLMModel::totalMemmapSize(const string& bilm_specification)
{
   vector<string> tokens;
   split(bilm_specification, tokens, ";");
   assert((tokens.size() >= 1 && tokens.size() <= 4) && "config_io::check should have caught this and issued a user error");
   string filename = tokens[0];
   return PLM::totalMemmapSize(filename);
}

string BiLMModel::fix_relative_path(const string& path, string file)
{
   vector<string> tokens;
   vector<string> fixed_tokens;
   split(file, tokens, ";");
   if (!tokens.empty()) {
      if (!tokens[0].empty() && tokens[0][0] != '/')
         fixed_tokens.push_back(path + "/" + tokens[0]);
      else
         fixed_tokens.push_back(tokens[0]);
   }
   for (Uint i = 1; i < tokens.size(); ++i) {
      vector<string> keyvalue;
      split(tokens[i], keyvalue, "=", 2);
      if (keyvalue.size() != 2) {
         fixed_tokens.push_back(tokens[i]);
      } else {
         const string class_file = keyvalue[1];
         if (!class_file.empty() && class_file[0] != '/')
            fixed_tokens.push_back(keyvalue[0] + "=" + path + "/" + class_file);
         else
            fixed_tokens.push_back(tokens[i]);
      }
   }
   return join(fixed_tokens, ";");
}

BiLMModel::BiLMModel(BasicModelGenerator* bmg, const string& model_string)
: bmg(bmg)
, annotator(NULL)
, model_string(model_string)
, bilm(NULL)
, order(0)
, biVoc(NULL)
, sentStartID(Uint(-1)) // "uninit"
, minimizeLmContextSize(bmg->c->minimizeLmContextSize)
, biLM_ID(nextBiLM_ID++)
{
   // This feature requires that the phrase table have BiLM Annotations in place
   vector<string> tokens;
   split(model_string, tokens, ";");
   assert((tokens.size() >= 1 && tokens.size() <= 4) && "config_io::check should have caught this and issued a user error");
   filename = tokens[0];
   if (tokens.size() == 1) {
      annotator = new BiLMAnnotation::Annotator(bmg->get_voc());
   } else {
      tokens.erase(tokens.begin());
      annotator = new CoarseBiLMAnnotation::Annotator(bmg->get_voc(), tokens);
   }
   biVoc = annotator->getVoc();
   if (debug_coarse_bilm) cerr << "biVoc " << biVoc << " .size() = " << biVoc->size() << endl;
   bmg->getPhraseTable().registerAnnotator(annotator);

   if (minimizeLmContextSize) {
      if (biLM_ID > ArrayUint4::MAXI)
         error(ETFatal, "With -minimize-lm-context-size, a maximum of %u BiLMs are supported.", ArrayUint4::MAXI);
      if (order > ArrayUint4::MAX)
         error(ETFatal, "With -minimize-lm-context-size, BiLMs of a maximum order of %u are supported.", ArrayUint4::MAX);
   }
}

void BiLMModel::finalizeInitialization()
{
   assert(!bilm);
   if (bmg->limitPhrases) {
      if (debug_coarse_bilm) cerr << "biVoc " << biVoc << " .size() = " << biVoc->size() << endl;
      assert(biVoc->size() > 0);
      assert(biVoc->getNumSourceSents() > 0);
   }
   cerr << "Loading BiLM model " << model_string << endl;
   bilm = PLM::Create(filename, biVoc, PLM::SimpleAutoVoc, LOG_ALMOST_0,
      bmg->limitPhrases, 0, NULL, false, "BiLMModel");
   assert(bilm);
   order = bilm->getOrder();
   if (order == 0)
      error(ETFatal, "Got an order 0 bilm infile %s; this makes no sense.", filename.c_str());
   sentStartID = biVoc->index(PLM::SentStart);
   assert(sentStartID != biVoc->size());
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
   // We give a detailed diagnostic message here - this is the first method
   // called with any given phrase_info; other methods can just assume or
   // assert correct input
   BiLMAnnotation* ann = annotator->get(phrase_info.annotations);
   if (!ann || ann->getBiPhrase().empty()) {
      error(ETWarn, "Phrase pair without a bi-phrase.  Something is wrong in the code, but here is the phrase pair to give us a chance to figure out where the problem comes from:");
      phrase_info.display();
      error(ETFatal, "Stopping because of missing bi-phrase.");
   }
   const Phrase& bi_phrase(ann->getBiPhrase());

   // Adapted from BasicModelGenerator::precomputeFutureScores() with the LMH_INCREMENTAL heuristic
   Uint bi_phrase_size(bi_phrase.size());
   Uint reversed_bi_phrase[bi_phrase_size];
   for (Uint i = 0; i < bi_phrase_size; ++i)
      reversed_bi_phrase[i] = bi_phrase[bi_phrase_size - i - 1];
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
   BiLMAnnotation* ann = annotator->get(pt.lastPhrase->annotations);
   assert(ann);
   const Uint bi_phrase_size = ann->getBiPhrase().size();

   const Uint back_context_size = minimizeLmContextSize
      ? pt.back->getBiLMContextSize(biLM_ID)
      : order-1;

   static VectorPhrase endBiPhrase; // non-reentrant!
   getLastBiWordsBackwards(endBiPhrase, back_context_size + bi_phrase_size, pt);
   Uint total_context_length = endBiPhrase.size()-1;

   double result = 0;
   for (int i = bi_phrase_size - 1; i >= 0; --i)
      result += bilm->cachedWordProb(endBiPhrase[i], &(endBiPhrase[i+1]), total_context_length-i);

   if (minimizeLmContextSize && pt.getBiLMContextSize(biLM_ID) == ArrayUint4::MAX) {
      Uint contextAvailable = min(endBiPhrase.size(), order-1);
      Uint size = bilm->minContextSize(&(endBiPhrase[0]), contextAvailable);
      assert(size < ArrayUint4::MAX);
      pt.setBiLMContextSize(biLM_ID, size);
   }

   return result;
}

double BiLMModel::partialScore(const PartialTranslation &trans)
{
   return 0; // The whole partial score is done in precomputeFutureScores()
}

Uint BiLMModel::computeRecombHash(const PartialTranslation &pt)
{
   Uint result = 0;

   Uint context_size = order-1;
   if (minimizeLmContextSize) {
      context_size = pt.getBiLMContextSize(biLM_ID);
      assert(context_size != ArrayUint4::MAX);
      result = context_size * 2551;
   }

   // We need to go over the order-1 last bi_phrase elements
   // non-reentrant!
   static VectorPhrase endBiPhrase;
   getLastBiWordsBackwards(endBiPhrase, context_size, pt);
   for (Uint i = 0; i < endBiPhrase.size(); ++i) {
      result += endBiPhrase[i];
      result = result * biVoc->size();
   }
   return result;
}

bool BiLMModel::isRecombinable(const PartialTranslation &pt1,
                               const PartialTranslation &pt2)
{
   Uint context_size = order-1;
   if (minimizeLmContextSize) {
      context_size = pt1.getBiLMContextSize(biLM_ID);
      if (context_size != pt2.getBiLMContextSize(biLM_ID))
         return false;
   }

   // We need to compare the order-1 last bi_phrase elements
   // non-reentrant!
   static VectorPhrase endBiPhrase1, endBiPhrase2;
   getLastBiWordsBackwards(endBiPhrase1, context_size, pt1);
   getLastBiWordsBackwards(endBiPhrase2, context_size, pt2);
   return endBiPhrase1 == endBiPhrase2;
}
