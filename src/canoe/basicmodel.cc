/**
 * @author Aaron Tikuisis
 * @file basicmodel.cc  This file contains the implementation of the BasicModel
 * class, which provides a log-linear model comprised of translation models,
 * language models, and built-in distortion and length penalties.
 *
 * $Id$ *
 *
 * Canoe Decoder
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "logging.h"
#include "basicmodel.h"
#include "config_io.h"
#include "backwardsmodel.h"
#include "lm.h"
#include "errors.h"
#include "randomDistribution.h"
#include <math.h>
#include <numeric> // for accumulate
#include "word_align_io.h" // for BiLMWriter::sep

using namespace Portage;
using namespace std;

Logging::logger bmgLogger(Logging::getLogger("debug.canoe.basicmodelgenerator"));

// Feature order is not strictly driven by the order in which features are
// found in c.features and the list of names in c.weight_params*.  Use these
// variables to iterate over features, do not write explicit lists of features!
void BasicModelGenerator::InitDecoderFeatures(const CanoeConfig& c)
{
   if (c.feature("d")->empty())
      if (c.verbosity) cerr << "Not using any distortion model" << endl;

   for (CanoeConfig::FeatureMap::const_iterator it(c.features.begin()),
        end(c.features.end()); it != end; ++it) {
      const CanoeConfig::FeatureDescription *f = it->second;
      assert(!f->need_args || f->weights.size() == f->args.size());
      for (Uint i(0); i < f->size(); ++i) {
         string args = f->need_args ? f->args[i] : "";
         LOG_VERBOSE2(bmgLogger, "Creating a %s model with >%s<",
            f->group.c_str(), args.c_str());
         decoder_features.push_back(DecoderFeature::create(this, f->group, args));
         featureWeightsV.push_back(f->weights[i]);
         if (c.randomWeights)
            random_feature_weight.push_back(f->rnd_weights.get(i));
      }
   }
}


BasicModelGenerator* BasicModelGenerator::create(
      const CanoeConfig& c,
      const vector<vector<string> > *sents,
      const vector<vector<MarkedTranslation> > *marks)
{
   BasicModelGenerator *result;

   c.check_all_files();
   assert (c.loadFirst || (sents && marks));

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

   LOG_VERBOSE1(bmgLogger, "Creating bmg - loading TMs");

   // Copy all TM (backward, forward and adir) weights from the canoe.ini to
   // the result model.
   assert(c.transWeights.size() == c.getTotalBackwardModelCount());
   result->transWeightsV = c.transWeights;
   assert(c.forwardWeights.size() == 0 || c.forwardWeights.size() == c.transWeights.size());
   result->forwardWeightsV = c.forwardWeights;
   assert(c.adirTransWeights.size() == c.getTotalAdirectionalModelCount());
   result->adirTransWeightsV = c.adirTransWeights;

   // Initialize TM random weights
   if (c.randomWeights) {
      for (Uint i(0); i < c.transWeights.size(); ++i)
         result->random_trans_weight.push_back(c.rnd_transWeights.get(i));
      for (Uint i(0); i < c.forwardWeights.size(); ++i)
         result->random_forward_weight.push_back(c.rnd_forwardWeights.get(i));
      for (Uint i(0); i < c.adirTransWeights.size(); ++i)
         result->random_adir_weight.push_back(c.rnd_adirTransWeights.get(i));
   }

   // Load multi prob phrase tables
   for ( Uint i = 0; i < c.multiProbTMFiles.size(); ++i )
      result->phraseTable->readMultiProb(c.multiProbTMFiles[i].c_str(), result->limitPhrases);

   // We load TPPTs next, with their weights after the multi-prob weights.
   for ( Uint i = 0; i < c.tpptFiles.size(); ++i ) {
      result->vocab_read_from_TPPTs = false;
      result->phraseTable->openTPPT(c.tpptFiles[i].c_str());
   }

   //////////// loading LDMs
   for ( Uint i = 0; i < c.LDMFiles.size(); ++i ) {
      const string ldm_filename = c.LDMFiles[i];
      const bool isTPLDM = isSuffix(".tpldm", ldm_filename);
      if (isTPLDM)
         result->phraseTable->openTPLDM(ldm_filename.c_str());
      else
         result->phraseTable->readLexicalizedDist(ldm_filename.c_str(), true);

      for ( vector<DecoderFeature *>::iterator df_it = result->decoder_features.begin();
            df_it != result->decoder_features.end(); ++df_it ){

         LexicalizedDistortion *ldf = dynamic_cast<LexicalizedDistortion *>(*df_it);
         if ( ldf ) {
            if (isTPLDM)
               ldf->readDefaults((ldm_filename + "/bkoff").c_str());
            else
               ldf->readDefaults((removeZipExtension(ldm_filename) + ".bkoff").c_str());
         }
      }
   }

   //////////// loading LMs
   LOG_VERBOSE1(bmgLogger, "Creating bmg loading language models");

   // load language models
   for (Uint i = 0; i < c.lmFiles.size(); ++i) {
      LOG_VERBOSE3(bmgLogger, "Loading lm: %s", c.lmFiles[i].c_str());
      result->addLanguageModel(c.lmFiles[i].c_str(), c.lmWeights[i], c.lmOrder);

      if (c.randomWeights)
         result->random_lm_weight.push_back(c.rnd_lmWeights.get(i));
   }
   LOG_DEBUG(bmgLogger, "Finish loading language models");

   // We no longer need all that filtering data, since dynmamic LM filtering
   // has been completed by now.
   result->tgt_vocab.freePerSentenceData();

   // Finalize feature initialization
   for (Uint i = 0; i < result->decoder_features.size(); ++i)
      result->decoder_features[i]->finalizeInitialization();

   return result;
} // BasicModelGenerator::create()

BasicModelGenerator::BasicModelGenerator(const CanoeConfig& c) :
   c(&c),
   verbosity(c.verbosity),
   tgt_vocab(0),
   biPhraseVocab(0),
   limitPhrases(false),
   lm_numwords(1),
   futureScoreLMHeuristic(lm_heuristic_type_from_string(c.futLMHeuristic)),
   cubePruningLMHeuristic(lm_heuristic_type_from_string(c.cubeLMHeuristic)),
   futureScoreUseFtm(c.futScoreUseFtm),
   vocab_read_from_TPPTs(true),
   addWeightMarked(log(c.weightMarked))
{
   LOG_VERBOSE1(bmgLogger, "BasicModelGenerator constructor with 2 args");

   phraseTable = new PhraseTable(tgt_vocab, biPhraseVocab, c.phraseTablePruneType.c_str());
   InitDecoderFeatures(c);
}

BasicModelGenerator::BasicModelGenerator(
   const CanoeConfig &c,
   const vector<vector<string> > &src_sents,
   const vector<vector<MarkedTranslation> > &marks
) :
   c(&c),
   verbosity(c.verbosity),
   tgt_vocab(c.loadFirst ? 0 : src_sents.size()),
   biPhraseVocab(c.loadFirst ? 0 : src_sents.size()),
   limitPhrases(!c.loadFirst),
   lm_numwords(1),
   futureScoreLMHeuristic(lm_heuristic_type_from_string(c.futLMHeuristic)),
   cubePruningLMHeuristic(lm_heuristic_type_from_string(c.cubeLMHeuristic)),
   futureScoreUseFtm(c.futScoreUseFtm),
   vocab_read_from_TPPTs(true),
   addWeightMarked(log(c.weightMarked))
{
   LOG_VERBOSE1(bmgLogger, "BasicModelGenerator construtor with 5 args");

   phraseTable = new PhraseTable(tgt_vocab, biPhraseVocab, c.phraseTablePruneType.c_str(), !c.feature("bilm")->empty());

   if (limitPhrases)
   {
      // Enter all source phrases into the phrase table, and into the vocab
      // in case they are OOVs we need to copy through to the output.
      LOG_VERBOSE2(bmgLogger, "Adding source sentences vocab");
      phraseTable->addSourceSentences(src_sents);
      if (!src_sents.empty()) {
         assert(tgt_vocab.size() > 0);
         assert(tgt_vocab.getNumSourceSents() == src_sents.size() ||
                tgt_vocab.getNumSourceSents() == VocabFilter::maxSourceSentence4filtering);
      }

      // Add target words from marks to the vocab
      LOG_VERBOSE2(bmgLogger, "Adding source sentences marked vocab: %d", marks.size());
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
   LOG_VERBOSE1(bmgLogger, "Destroying a BasicModelGenerator");

   // if the user doesn't want to clean up, exit.
   if (c != NULL && c->final_cleanup) {
      for ( vector<DecoderFeature *>::iterator it = decoder_features.begin();
            it != decoder_features.end(); ++it )
         delete *it;

      if (phraseTable) {
         delete phraseTable;
         phraseTable = NULL;
      }

      for (vector<PLM *>::iterator it = lms.begin(); it != lms.end(); ++it)
         delete *it;
   }
} // ~BasicModelGenerator

void BasicModelGenerator::addLanguageModel(const char *lmFile, double weight,
   Uint limit_order, ostream *const os_filtered)
{
   extractVocabFromTPPTs();

   if (!PLM::checkFileExists(lmFile))
      error(ETFatal, "Cannot read from language model file %s", lmFile);
   cerr << "loading language model from " << lmFile << endl;
   //time_t start_time = time(NULL);
   PLM *lm = PLM::Create(lmFile, &tgt_vocab, PLM::SimpleAutoVoc, LOG_ALMOST_0,
                         limitPhrases, limit_order, os_filtered);
   //cerr << " ... done in " << (time(NULL) - start_time) << "s" << endl;
   assert(lm != NULL);
   lms.push_back(lm);
   lmWeightsV.push_back(weight);

   // We trust the language model to tell us its order, not the other way around.
   if ( lm->getOrder() > lm_numwords ) lm_numwords = lm->getOrder();
} // addLanguageModel

void BasicModelGenerator::extractVocabFromTPPTs()
{
   if ( limitPhrases && ! vocab_read_from_TPPTs ) {
      time_t start_time = time(NULL);
      cerr << "extracting target vocabulary from TPPTs";
      assert(phraseTable);
      phraseTable->extractVocabFromTPPTs(verbosity);
      vocab_read_from_TPPTs = true;
      cerr << " ... done in " << (time(NULL) - start_time) << "s" << endl;
   }
}

string BasicModelGenerator::describeModel() const
{
   // The order of the features here must be the same as in
   // BasicModel::getFeatureFunctionVals()
   ostringstream description;
   description << "index\tweight\tfeature description" << endl;
   Uint featureIndex(0);  // indicates column for the feature
   assert(decoder_features.size() == featureWeightsV.size());
   for (Uint i = 0; i < decoder_features.size(); ++i) {
      description << ++featureIndex << "\t";
      if (c->randomWeights) description << "random"; else description << featureWeightsV[i];
      description << "\t" << decoder_features[i]->describeFeature() << nf_endl;
   }
   assert(lms.size() == lmWeightsV.size());
   for (Uint i = 0; i < lms.size(); ++i) {
      description << ++featureIndex << "\t";
      if (c->randomWeights) description << "random"; else description << lmWeightsV[i];
      description << "\t" << lms[i]->describeFeature() << nf_endl;
   }
   string tm_desc = phraseTable->describePhraseTables(!forwardWeightsV.empty());
   vector<string> tm_desc_items;
   split(tm_desc, tm_desc_items, "\n");
   vector<double> all_tm_weights = transWeightsV;
   all_tm_weights.insert(all_tm_weights.end(), forwardWeightsV.begin(), forwardWeightsV.end());
   all_tm_weights.insert(all_tm_weights.end(), adirTransWeightsV.begin(), adirTransWeightsV.end());
   assert(all_tm_weights.size() == tm_desc_items.size());
   for (Uint i = 0; i < tm_desc_items.size(); ++i) {
      description << ++featureIndex << "\t";
      if (c->randomWeights) description << "random"; else description << all_tm_weights[i];
      description << "\t" << tm_desc_items[i] << nf_endl;
   }
   /*
   description << phraseTable->describePhraseTables(!forwardWeightsV.empty());
   description << "TM weights: tm: " << join(transWeightsV);
   if (!forwardWeightsV.empty())
      description << " ftm: " << join(forwardWeightsV);
   if (!adirTransWeightsV.empty())
      description << " atm: " << join(adirTransWeightsV);
   description << rnd << endl;
   */

   return description.str();
} // describeModel

BasicModel *BasicModelGenerator::createModel(
   newSrcSentInfo& info, bool alwaysTryDefault)
{
   //LOG_VERBOSE2(bmgLogger, "Creating a model for: %s", join(src_sent).c_str());

   // make sure all class names are valid.
   const vector<MarkedTranslation>& marks = info.marks;
   const vector<string>& rc = c->rule_classes;
   for (Uint i(0); i<marks.size(); ++i) {
      const string& cn = marks[i].class_name;
      // Only marks with a non empty class_name are considered rules.
      if (!cn.empty()) {
         if (find(rc.begin(), rc.end(), cn) == rc.end()) {
            error(ETWarn, "%s is an invalid class in sentence %d",
               cn.c_str(), info.external_src_sent_id);
         }
      }
   }

   // Get the candidate translation phrases for this source sentence.
   info.potential_phrases = createAllPhraseInfos(info, alwaysTryDefault);

   // Transform the string target sentence into a Uint target sentence
   info.convertTargetSentence(get_voc());

   // Inform all the decoder features we are about to process another source sentence.
   for ( vector<DecoderFeature *>::iterator it = decoder_features.begin();
         it != decoder_features.end(); ++it)
      (*it)->newSrcSent(info);

   // Inform all LM models that we are about to process a new source sentence.
   for (Uint i=0;i<lms.size();++i) {
      lms[i]->clearCache();
   }

   // Clear the phrase table caches every 10 sentences
   if ( info.internal_src_sent_seq % 10 == 0 )
      phraseTable->clearCache();


   double **futureScores = precomputeFutureScores(info.potential_phrases,
         info.src_sent.size());

   return new BasicModel(info.src_sent, info.tgt_sent,
                         info.potential_phrases, futureScores, *this);
} // createModel

vector<PhraseInfo *> **BasicModelGenerator::createAllPhraseInfos(
   const newSrcSentInfo& info,
   bool alwaysTryDefault)
{
   assert(transWeightsV.size() > 0);
   // Initialize the triangular array
   vector<PhraseInfo *> **result =
      TriangArray::Create<vector<PhraseInfo *> >()(info.src_sent.size());

   if (info.oovs) info.oovs->assign(info.src_sent.size(), false);

   // Keep track of which ranges to skip (because they are marked)
   vector<Range> rangesToSkip;
   addMarkedPhraseInfos(info, result, rangesToSkip);
   sort(rangesToSkip.begin(), rangesToSkip.end());

   phraseTable->getPhraseInfos(result, info.src_sent, transWeightsV,
      c->phraseTableSizeLimit, log(c->phraseTableThreshold),
      rangesToSkip, verbosity,
      (forwardWeightsV.empty() ? NULL : &forwardWeightsV),
      (adirTransWeightsV.empty() ? NULL : &adirTransWeightsV));

   // Use an iterator to go through the ranges to skip, so that we avoid these
   // when creating default translations
   vector<Range>::const_iterator rangeIt = rangesToSkip.begin();

   for (Uint i = 0; i < info.src_sent.size(); ++i)
   {
      while (rangeIt != rangesToSkip.end() && rangeIt->end <= i)
         ++rangeIt;

      if ( ( result[i][0].empty() &&
             (rangeIt == rangesToSkip.end() || i < rangeIt->start) )
           || alwaysTryDefault )
      {
         // The word has no translation and is not in a range to skip, so
         // create a default translation (use the source word as itself).
         Range curRange(i, i + 1);
         result[i][0].push_back(makeNoTransPhraseInfo(curRange,
            info.src_sent[i].c_str()));
         if (info.oovs) (*info.oovs)[i] = true;
      }
   }

   if ( verbosity >= 3 )
      for ( Uint i = 0; i < info.src_sent.size(); ++i )
         for ( Uint j = i; j < info.src_sent.size(); ++j )
            if ( ! result[i][j-i].empty() )
               cerr << result[i][j-i].size() << " candidate phrases for range "
                    << Range(i,j+1).toString() << endl;

   return result;
} // createAllPhraseInfos

void BasicModelGenerator::addMarkedPhraseInfos(
   const newSrcSentInfo& info,
   vector<PhraseInfo *> **result,
   vector<Range> &rangesToSkip)
{
   const vector<MarkedTranslation> &marks(info.marks);

   // Compute the total weight on phrase translation models, in order to
   // denormalize the probabilities
   double totalWeight = accumulate(transWeightsV.begin(), transWeightsV.end(), 0.0);
   double totalForwardWeight = accumulate(forwardWeightsV.begin(), forwardWeightsV.end(), 0.0);
   double totalAdirWeight = accumulate(adirTransWeightsV.begin(), adirTransWeightsV.end(), 0.0);

   for ( vector<MarkedTranslation>::const_iterator it = marks.begin();
         it != marks.end(); it++)
   {
      // Create a ForwardBackwardPhraseInfo for each marked phrase
      ForwardBackwardPhraseInfo* newPI = new ForwardBackwardPhraseInfo;
      newPI->forward_trans_prob =
         (it->log_prob + addWeightMarked) * totalForwardWeight;
      newPI->forward_trans_probs.insert(newPI->forward_trans_probs.end(),
         forwardWeightsV.size(), it->log_prob + addWeightMarked);

      //boxing
      newPI->adir_prob =
         (it->log_prob + addWeightMarked) * totalAdirWeight;
      newPI->adir_probs.insert(newPI->adir_probs.end(),
         adirTransWeightsV.size(), it->log_prob + addWeightMarked);
      //boxing

      //EJJ Oct2010 - why don't we set lexdis_probs, as we do in
      //PhraseTable::getPhrases???  Because we don't need to: when missing,
      //the global defaults (i.e., the backoff scores) are used.

      newPI->alignment = 0;
      if (!c->feature("bilm")->empty()) {
         static bool warning_printed = false;
         if (!warning_printed) {
            error(ETWarn, "Using BiLMs with marks is OK but not completely supported: even if the BiLM contained the relevant marks, they might have gotten filtered out while loading the model.");
            // To provide full support, canoe would have to add the marks to
            // biPhraseVocab *before* loading the BiLM files, which is tricky
            // to do.  I did not do it right away because this is a rare use
            // case - we can deal with it later if a) BiLMs prove to work well
            // *and* b) we want to use marks with them.
            warning_printed = true;
         }

         VectorPhrase bi_phrase;

         for ( vector<string>::const_iterator jt = it->markString.begin();
               jt != it->markString.end(); jt++) {
            bi_phrase.push_back(
               biPhraseVocab.add(
                  (jt->c_str() +
                   BiLMWriter::sep +
                   join(info.src_sent.begin()+it->src_words.start, info.src_sent.begin()+it->src_words.end, BiLMWriter::sep)
                  ).c_str()));
         }
         newPI->bi_phrase = bi_phrase;
      }

      newPI->src_words = it->src_words;
      newPI->phrase_trans_prob = (it->log_prob + addWeightMarked) * totalWeight;
      newPI->phrase_trans_probs.insert(newPI->phrase_trans_probs.end(),
         transWeightsV.size(), it->log_prob + addWeightMarked);
      // Construct the Phrase into a VectorPhrase, because CompactPhrase does
      // not support .push_back().
      VectorPhrase newPI_phrase;
      for ( vector<string>::const_iterator jt = it->markString.begin();
            jt != it->markString.end(); jt++) {
         newPI_phrase.push_back(tgt_vocab.add(jt->c_str()));
      }
      newPI->phrase = newPI_phrase;

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
   ForwardBackwardPhraseInfo* newPI = new ForwardBackwardPhraseInfo;
   newPI->forward_trans_probs.insert(newPI->forward_trans_probs.end(),
         forwardWeightsV.size(), LOG_ALMOST_0);
   newPI->forward_trans_prob = dotProduct(forwardWeightsV,
         newPI->forward_trans_probs, forwardWeightsV.size());

   newPI->adir_probs.insert(newPI->adir_probs.end(),    //boxing
         adirTransWeightsV.size(), LOG_ALMOST_0);       //boxing
   newPI->adir_prob = dotProduct(adirTransWeightsV,     //boxing
         newPI->adir_probs, adirTransWeightsV.size());  //boxing

   newPI->joint_counts.push_back(0); // OOV
   newPI->alignment = 0;
   if (!c->feature("bilm")->empty())
      newPI->bi_phrase = VectorPhrase(1, biPhraseVocab.add((word + BiLMWriter::sep + word).c_str()));

   newPI->src_words = range;
   newPI->phrase = VectorPhrase(1, tgt_vocab.add(word));
   newPI->phrase_trans_probs.insert(newPI->phrase_trans_probs.end(),
         transWeightsV.size(), LOG_ALMOST_0);
   newPI->phrase_trans_prob = dotProduct(newPI->phrase_trans_probs,
         transWeightsV, transWeightsV.size());
   return newPI;
} // makeNoTransPhraseInfo

double **BasicModelGenerator::precomputeFutureScores(
   vector<PhraseInfo *> **phrases, Uint sentLength)
{
   double **result = TriangArray::Create<double>()(sentLength);
   for (Uint i = 0; i < sentLength; ++i)
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
            ForwardBackwardPhraseInfo *FBPI = dynamic_cast<ForwardBackwardPhraseInfo*>(*it);
            assert(FBPI);
            double newScore = FBPI->phrase_trans_prob + FBPI->adir_prob;
            if (futureScoreUseFtm)
               newScore += FBPI->forward_trans_prob;

            // Add heuristic LM score
            if ( futureScoreLMHeuristic == LMH_UNIGRAM ) {
               // Our old way: use the unigram LM score
               for ( Phrase::const_iterator jt = (*it)->phrase.begin();
                     jt != (*it)->phrase.end(); ++jt ) {
                  for (Uint k = 0; k < lmWeightsV.size(); k++) {
                     newScore += lms[k]->cachedWordProb(*jt, NULL, 0) * lmWeightsV[k];
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
                     newScore += lms[j]->cachedWordProb(reversed_phrase[i],
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
               cerr << "    src_words " << (*it)->src_words
                    << " phrase " << getStringPhrase((*it)->phrase)
                    << " score " << newScore << endl;
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

   if (!(sentLength == 0 || result[0][sentLength - 1] > -INFINITY))
      cerr << "result[0]: " << result[0][sentLength - 1];
   assert(sentLength == 0 || result[0][sentLength - 1] > -INFINITY);

   if (verbosity >= 3)
   {
      for (Uint i = 0; i < sentLength; ++i)
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
   // caveat: as a consequence, this method is not re-entrant
   static VectorPhrase endPhrase;
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
   assert(endPhrase.size() >= last_phrase_size);
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
         results[j] += lms[j]->cachedWordProb(endPhrase[i], &(endPhrase[i + 1]),
                                        context_len-i);
      }
   }

   if (trans.sourceWordsNotCovered.empty())
   {
      for (Uint j = 0; j < lmWeightsV.size(); j++)
      {
         // Compute score for j-th language model for end-of-sentence
         results[j] += lms[j]->cachedWordProb(tgt_vocab.index(PLM::SentEnd),
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
   // This comes directly from the ForwardBackwardPhraseInfo object.
   const ForwardBackwardPhraseInfo *phrase = dynamic_cast<const ForwardBackwardPhraseInfo *>(trans.lastPhrase);
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
   const ForwardBackwardPhraseInfo *phrase =
      dynamic_cast<const ForwardBackwardPhraseInfo *>(trans.lastPhrase);
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
   const ForwardBackwardPhraseInfo *phrase =
      dynamic_cast<const ForwardBackwardPhraseInfo *>(trans.lastPhrase);
   assert(phrase != NULL);
   return phrase->forward_trans_prob;
}

//boxing getRawAdirTrans
void BasicModelGenerator::getRawAdirTrans(vector<double> &adirVals,
        const PartialTranslation &trans)
{
   if (adirTransWeightsV.empty()) return;

   // This comes from the ForwardBackwardPhraseInfo object, assuming
   // we have adirectional weights, as asserted above.
   const ForwardBackwardPhraseInfo *phrase =
      dynamic_cast<const ForwardBackwardPhraseInfo *>(trans.lastPhrase);
   assert(phrase != NULL);
   adirVals.insert(adirVals.end(), phrase->adir_probs.begin(),
         phrase->adir_probs.end());

}

double BasicModelGenerator::dotProductAdirTrans(const vector<double> &weights,
        const PartialTranslation &trans)
{
   if (weights.empty()) return 0.0;

   // This comes from the ForwardBackwardPhraseInfo object, assuming
   // we have forward weights, as checked above.
   const ForwardBackwardPhraseInfo *phrase =
      dynamic_cast<const ForwardBackwardPhraseInfo *>(trans.lastPhrase);
   assert(phrase != NULL);
   return phrase->adir_prob;
}
//boxing

void BasicModelGenerator::getRawFeatures(vector<double> &ffVals,
        const PartialTranslation &trans)
{
   // The other features' weights come from their individual objects
   for (Uint k = 0; k < decoder_features.size(); ++k)
      ffVals.push_back(decoder_features[k]->score(trans));
}

double BasicModelGenerator::dotProductFeatures(const vector<double> &weights,
        const PartialTranslation &trans)
{
   double result = 0.0;
   for (Uint k = 0; k < decoder_features.size(); ++k)
      result += decoder_features[k]->score(trans) * weights[k];
   return result;
}

string BasicModelGenerator::getStringPhrase(const Phrase &uPhrase) const
{
   return phrase2string(uPhrase, tgt_vocab);
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
   seed_helper(random_adir_weight, seed);    //boxing
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

   //boxing
   assert(random_adir_weight.size() == adirTransWeightsV.size());
   transform(random_adir_weight.begin(), random_adir_weight.end(),
         adirTransWeightsV.begin(), mem_fun(&rnd_distribution::operator()));
   //boxing

   assert(random_lm_weight.size() == lmWeightsV.size());
   transform(random_lm_weight.begin(), random_lm_weight.end(),
         lmWeightsV.begin(), mem_fun(&rnd_distribution::operator()));
}

void BasicModelGenerator::setWeightsFromString(const string& s)
{
   // Put the weights into a temporary copy of the CanoeConfig
   CanoeConfig tempConfig(*c);
   tempConfig.setFeatureWeightsFromString(s);

   // Now set weights in the BMG from corresponding weights in CanoeConfig.

   // 1) "Normal" features
   vector<double> all_weights;
   tempConfig.getFeatureWeights(all_weights);
   assert(all_weights.size() > featureWeightsV.size());
   featureWeightsV.assign(all_weights.begin(), all_weights.begin()+featureWeightsV.size());

   // 2) TMs and LMs
   transWeightsV = tempConfig.transWeights;
   forwardWeightsV = tempConfig.forwardWeights;
   adirTransWeightsV = tempConfig.adirTransWeights;
   lmWeightsV = tempConfig.lmWeights;

   assert(join(all_weights) ==
          join(featureWeightsV)+" "+join(transWeightsV)+" "+join(forwardWeightsV)
          +" "+join(adirTransWeightsV)+" "+join(lmWeightsV));
}

void BasicModelGenerator::displayLMHits(ostream& out) {
   PLM::Hits hits;
   typedef vector<PLM*>::iterator LM_IT;
   for (LM_IT it(lms.begin()); it!= lms.end(); ++it) {
      hits += (*it)->getHits();
   }
   out << "Overall LM hits:" << endl;
   hits.display(out);
}


BasicModel::BasicModel(const vector<string> &src_sent,
                       const vector<string> *tgt_sent,
                       vector<PhraseInfo *> **phrases,
                       double **futureScore,
                       BasicModelGenerator &parent) :
   c(parent.c), tgt_sent(tgt_sent), src_sent(src_sent),
   lmWeights(parent.lmWeightsV), transWeights(parent.transWeightsV),
   forwardWeights(parent.forwardWeightsV),
   adirTransWeights(parent.adirTransWeightsV),
   featureWeights(parent.featureWeightsV),
   phrases(phrases), futureScore(futureScore), parent(parent) {}

BasicModel::~BasicModel()
{
   for (Uint i = 0; i < src_sent.size(); ++i) {
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

double BasicModel::scoreTranslation(const PartialTranslation &trans, Uint verbosity)
{
   assert(trans.lastPhrase != NULL);
   assert(trans.back != NULL);
   assert(trans.back->lastPhrase != NULL);
   vector<double> vals;

   // Other feature values - done first because they are first in the ffvals vector
   const double ffScore = parent.dotProductFeatures(featureWeights, trans);
   if (verbosity >= 3) {
      cerr << "\tother feature values  " << ffScore;
      vals.clear();
      parent.getRawFeatures(vals, trans);
      cerr << "  [ " << join(vals) << " ]" << endl;
   }

   // LM score
   static vector<double> lmVals;
   lmVals.clear();
   parent.getRawLM(lmVals, trans);
   const double lmScore = dotProduct(lmVals, lmWeights, lmWeights.size());
   if (verbosity >= 3) {
      cerr << "\tlanguage model score  " << lmScore;
      cerr << "  [ " << join(lmVals) << " ]" << endl;
   }

   // (Backward) translation model score
   const double transScore = parent.dotProductTrans(transWeights, trans);
   if (verbosity >= 3) {
      cerr << "\tbackward trans score  " << transScore;
      vals.clear();
      parent.getRawTrans(vals, trans);
      cerr << "  [ " << join(vals) << " ]" << endl;
   }

   // Forward translation model score
   const double forwardScore = parent.dotProductForwardTrans(forwardWeights, trans);
   if (verbosity >= 3) {
      cerr << "\tforward trans score   " << forwardScore;
      vals.clear();
      parent.getRawForwardTrans(vals, trans);
      cerr << "  [ " << join(vals) << " ]" << endl;
   }

   // Adirectional translation model score
   const double adirScore = parent.dotProductAdirTrans(adirTransWeights, trans);
   if (verbosity >= 3) {
      cerr << "\tadir trans score      " << adirScore;
      vals.clear();
      parent.getRawAdirTrans(vals, trans);
      cerr << "  [ " << join(vals) << " ]" << endl;
   }

   return transScore + forwardScore + adirScore + lmScore + ffScore;
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
   // The distortion limit test is now done when considering candidate phrases,
   // so it is no longer necessary to do it again here.

   double precomputedScore = 0;
   for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin(); it !=
         trans.sourceWordsNotCovered.end(); it++)
   {
      assert(it->start < it->end);
      assert(it->end <= src_sent.size());
      precomputedScore += futureScore[it->start][it->end - it->start - 1];
   } // for

   // Other features
   double ffScore = 0;
   for (Uint k = 0; k < parent.decoder_features.size(); ++k)
      ffScore += parent.decoder_features[k]->futureScore(trans)
                 * featureWeights[k];

   if (trans.sourceWordsNotCovered.empty()) {
      if (ffScore != 0.0) {
         for (Uint k = 0; k < parent.decoder_features.size(); ++k)
            if (0.0 != parent.decoder_features[k]->futureScore(trans))
               error(ETWarn, "Feature %s gave a non-zero future score on a complete translation.",
                     parent.decoder_features[k]->describeFeature().c_str());
         error(ETFatal, "Non-zero future score on a complete translation violates "
               "the definition of decoder features.");
      }
      assert(precomputedScore == 0);
   }

   return precomputedScore + ffScore;
} // computeFutureScore

double BasicModel::rangePartialScore(const PartialTranslation& trans)
{
   // Only "other features" come into play for this partial score.
   double result = 0.0;
   for (Uint k = 0; k < parent.decoder_features.size(); ++k) {
      result += parent.decoder_features[k]->partialScore(trans)
                * featureWeights[k];
   }
   return result;
}

double BasicModel::phrasePartialScore(const PhraseInfo* phrase)
{
   // Backward translation score - the main information source used here.
   double score = phrase->phrase_trans_prob;

   // Forward translation score (if used)
   if ( ! forwardWeights.empty() )
      score += ((const ForwardBackwardPhraseInfo*)phrase)->forward_trans_prob;

   // Adirectional translation score (if used)
   if ( ! adirTransWeights.empty() )
      score += ((const ForwardBackwardPhraseInfo*)phrase)->adir_prob;

   // LM score: use a heuristic LM score, as specified by the user
   if ( parent.cubePruningLMHeuristic == parent.LMH_UNIGRAM ) {
      // Our old future score LM heuristic: use the unigram LM score
      for ( Phrase::const_iterator jt = phrase->phrase.begin();
            jt != phrase->phrase.end(); ++jt )
         for (Uint k = 0; k < lmWeights.size(); k++)
            score += parent.lms[k]->cachedWordProb(*jt, NULL, 0) * lmWeights[k];
   } else if ( parent.cubePruningLMHeuristic == parent.LMH_INCREMENTAL ) {
      Uint phrase_size(phrase->phrase.size());
      Uint reversed_phrase[phrase_size];
      for ( Uint i(0); i < phrase_size; ++i )
         reversed_phrase[i] = phrase->phrase[phrase_size - i - 1];
      for ( Uint j(0); j < parent.lms.size(); ++j )
         for ( Uint i(0); i < phrase_size; ++i )
            score += parent.lms[j]->cachedWordProb(reversed_phrase[i],
                     &(reversed_phrase[i+1]), phrase_size-i-1)
                   * lmWeights[j];
   } else if ( parent.cubePruningLMHeuristic == parent.LMH_SIMPLE ) {
      // The "simple" heuristic is admissible by ignoring words with
      // insufficient context
      Uint phrase_size(phrase->phrase.size());
      Uint reversed_phrase[phrase_size];
      for ( Uint i(0); i < phrase_size; ++i )
         reversed_phrase[i] = phrase->phrase[phrase_size - i - 1];
      for ( Uint j(0); j < parent.lms.size(); ++j )
         for ( Uint i(0); i < phrase_size - (parent.lms[j]->getOrder()-1); ++i )
            score += parent.lms[j]->wordProb(reversed_phrase[i],
                     &(reversed_phrase[i+1]), phrase_size-i-1)
                   * lmWeights[j];
   } // else LMH==NONE: nothing to do!

   // Other feature scores are available through precomputeFutureScore().
   for ( Uint k(0); k < parent.decoder_features.size(); ++k )
      score += parent.decoder_features[k]->precomputeFutureScore(*phrase)
               * featureWeights[k];

   return score;
} // phrasePartialScore

Uint BasicModel::computeRecombHash(const PartialTranslation &trans)
{
   // This might overflow result, but who cares.
   Uint result = 0;
   // make this endPhrase static so we don't have to reallocate this vector
   // millions of times.  (this method gets called ridiculously often...)
   static VectorPhrase endPhrase;
   endPhrase.clear();
   trans.getLastWords(endPhrase, parent.lm_numwords - 1);
   for (VectorPhrase::iterator it = endPhrase.begin();
         it != endPhrase.end(); ++it)
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
            parent.getNumFFs() + parent.getNumLMs() +
            parent.getNumTMs() + parent.getNumFTMs() +
            parent.getNumATMs(),
            0);
   } else {
      parent.getRawFeatures(vals, trans);
      parent.getRawLM(vals, trans);
      parent.getRawTrans(vals, trans);
      parent.getRawForwardTrans(vals, trans);
      parent.getRawAdirTrans(vals, trans);
   }
} // getFeatureFunctionVals

void BasicModel::getFeatureWeights( vector<double> &wts )
{
   // The order of the feature weights is the same as for getFeatureFunctionVals
   wts.clear();
   wts.insert( wts.end(), featureWeights.begin(), featureWeights.end() );
   wts.insert( wts.end(), lmWeights.begin(), lmWeights.end() );
   wts.insert( wts.end(), transWeights.begin(), transWeights.end() );
   wts.insert( wts.end(), forwardWeights.begin(), forwardWeights.end() );
   wts.insert( wts.end(), adirTransWeights.begin(), adirTransWeights.end() );
} // getFeatureWeights

void BasicModel::getTotalFeatureFunctionVals(vector<double> &vals,
        const PartialTranslation &trans)
{
   assert(vals.empty());
   getFeatureFunctionVals(vals, trans);
   if (trans.back != NULL) {
      vector<double> last;
      getTotalFeatureFunctionVals(last, *(trans.back));
      assert(vals.size() == last.size());
      for (Uint i = 0; i < last.size(); ++i) {
         vals[i] += last[i];
      }
   }
} // getTotalFeatureFunctionVals

