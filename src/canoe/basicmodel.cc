/**
 * @author Aaron Tikuisis
 * @file basicmodel.cc  This file contains the implementation of the BasicModel
 * class, which provides a log-linear model comprised of translation models,
 * language models, and built-in distortion and length penalties.
 *
 * Canoe Decoder
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
#include <cmath>
#include <numeric> // for accumulate
#include <algorithm>
#include "word_align_io.h" // for BiLMWriter::sep
#include "sparsemodel.h" // for describeModel()
#include "new_src_sent_info.h"
#include "lazy_stl.h"

using namespace Portage;
using namespace std;

Logging::logger bmgLogger(Logging::getLogger("debug.canoe.basicmodelgenerator"));

// Feature order is now strictly driven by the order in which features are
// found in c.features and the list of names in c.weight_params*.  Use these
// variables to iterate over features, do not write explicit lists of features!
void BasicModelGenerator::InitDecoderFeatures(const CanoeConfig& c)
{
   if (c.featureGroup("d")->empty())
      if (c.verbosity) cerr << "Not using any distortion model" << endl;

   for (CanoeConfig::FeatureGroupMap::const_iterator it(c.features.begin()),
        end(c.features.end()); it != end; ++it) {
      const CanoeConfig::FeatureGroup *f = it->second;
      assert(!f->need_args || f->weights.size() == f->args.size());
      for (Uint i(0); i < f->size(); ++i) {
         const string args = f->need_args ? f->args[i] : "";
         LOG_VERBOSE2(bmgLogger, "Creating a %s model with >%s<",
            f->group.c_str(), args.c_str());
         DecoderFeature* df = DecoderFeature::create(this, f->group, args, true, c.verbosity);
         lm_numwords = max(lm_numwords, df->lmLikeContextNeeded()+1);
         lm_min_context = max(lm_min_context, df->lmLikeContextNeeded());
         decoder_features.push_back(df);
         featureWeightsV.push_back(f->weights[i]);
         if (c.randomWeights)
            random_feature_weight.push_back(f->rnd_weights.get(i));
      }
   }

   for (vector<string>::const_iterator it(c.filterFeatures.begin()),
        end(c.filterFeatures.end()); it != end; ++it)
      filter_features.push_back(DecoderFeature::create(this, "DistortionModel", *it));
}


BasicModelGenerator* BasicModelGenerator::create(
      const CanoeConfig& c,
      const VectorPSrcSent *sents)
{
   BasicModelGenerator *result;

   // We don't need to call c.check_all_files() because it was already called
   // by c.check(), which must have been called to finish initializing the
   // canoe config prior to passing it to BasicModelGenerator::create.
   //c.check_all_files();
   assert (c.loadFirst || sents);

   if (c.backwards) {
      if (sents)
         result = new BackwardsModelGenerator(c, *sents);
      else
         result = new BackwardsModelGenerator(c);
   } else {
      if (sents)
         result = new BasicModelGenerator(c, *sents);
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
      result->phraseTable->readMultiProb(c.multiProbTMFiles[i].c_str(), result->limitPhrases, c.describeModelOnly);

   // We load TPPTs next, with their weights after the multi-prob weights.
   for ( Uint i = 0; i < c.tpptFiles.size(); ++i ) {
      result->vocab_read_from_TPPTs = false;
      result->phraseTable->openTPPT(c.tpptFiles[i].c_str());
   }

   //////////// loading LDMs
   if (!c.describeModelOnly) {
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

   // Apply -lm-context-size if specified.
   if (c.maxLmContextSize > -1)
      result->lm_numwords = min(result->lm_numwords, c.maxLmContextSize + 1);

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
   limitPhrases(false),
   lm_numwords(1),
   lm_min_context(1),
   futureScoreLMHeuristic(lm_heuristic_type_from_string(c.futLMHeuristic)),
   cubePruningLMHeuristic(lm_heuristic_type_from_string(c.cubeLMHeuristic)),
   vocab_read_from_TPPTs(true),
   addWeightMarked(log(c.weightMarked))
{
   LOG_VERBOSE1(bmgLogger, "BasicModelGenerator constructor with 2 args");

   GlobalVoc::set(&tgt_vocab);

   phraseTable = new PhraseTable(tgt_vocab, c.phraseTablePruneType.c_str(), c.appendJointCounts);

   InitDecoderFeatures(c);
}

BasicModelGenerator::BasicModelGenerator(
   const CanoeConfig &c,
   const VectorPSrcSent &sents
) :
   c(&c),
   verbosity(c.verbosity),
   tgt_vocab(c.loadFirst ? 0 : sents.size()),
   limitPhrases(!c.loadFirst),
   lm_numwords(1),
   lm_min_context(1),
   futureScoreLMHeuristic(lm_heuristic_type_from_string(c.futLMHeuristic)),
   cubePruningLMHeuristic(lm_heuristic_type_from_string(c.cubeLMHeuristic)),
   vocab_read_from_TPPTs(true),
   addWeightMarked(log(c.weightMarked))
{
   LOG_VERBOSE1(bmgLogger, "BasicModelGenerator construtor with 5 args");

   GlobalVoc::set(&tgt_vocab);

   phraseTable = new PhraseTable(tgt_vocab, c.phraseTablePruneType.c_str(), c.appendJointCounts);

   if (limitPhrases)
   {
      // Enter all source phrases into the phrase table, and into the vocab
      // in case they are OOVs we need to copy through to the output.
      LOG_VERBOSE2(bmgLogger, "Adding source sentences vocab");
      phraseTable->addSourceSentences(sents);
      if (!sents.empty()) {
         assert(tgt_vocab.size() > 0);
         assert(tgt_vocab.getNumSourceSents() == sents.size() ||
                tgt_vocab.getNumSourceSents() == VocabFilter::maxSourceSentence4filtering);
      }

      // Add target words from marks to the vocab
      LOG_VERBOSE2(bmgLogger, "Adding source sentences marked vocab: %d", sents.size());
      for (Uint sent_no = 0; sent_no < sents.size(); ++sent_no)
      {
         for (vector<MarkedTranslation>::const_iterator
                 jt(sents[sent_no]->marks.begin()),
                 j_end(sents[sent_no]->marks.end());
              jt != j_end; ++jt)
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

   for (PhraseTable::AnnotatorRegistry::const_iterator a_it(phraseTable->getAnnotators().begin()),
                                                       a_end(phraseTable->getAnnotators().end());
        a_it != a_end; ++a_it)
      a_it->second->addSourceSentences(sents);

} // BasicModelGenerator::BasicModelGenerator()

BasicModelGenerator::~BasicModelGenerator()
{
   LOG_VERBOSE1(bmgLogger, "Destroying a BasicModelGenerator");

   GlobalVoc::clear();

   // if the user doesn't want to clean up, exit.
   if (c != NULL && c->final_cleanup) {
      for ( vector<DecoderFeature *>::iterator it = decoder_features.begin();
            it != decoder_features.end(); ++it )
         delete *it;

      for ( vector<DecoderFeature *>::iterator it = filter_features.begin();
            it != filter_features.end(); ++it )
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

   // We don't need to call PLM::checkFileExists() for the lm file because
   // CanoeConfig::check_all_files() was already called by CanoeConfig::check(),
   // which must have been called to finish initializing the canoe config prior
   // to passing it to BasicModelGenerator::create.
   //if (!PLM::checkFileExists(lmFile))
   //   error(ETFatal, "Cannot read from language model file %s", lmFile);
   cerr << "loading language model from " << lmFile << endl;
   //time_t start_time = time(NULL);
   if (c->describeModelOnly) {
      lms.push_back(NULL);
   } else {
      PLM *lm = PLM::Create(lmFile, &tgt_vocab, PLM::SimpleAutoVoc, LOG_ALMOST_0,
                            limitPhrases, limit_order, os_filtered);
      //cerr << " ... done in " << (time(NULL) - start_time) << "s" << endl;
      assert(lm != NULL);
      lms.push_back(lm);

      // We trust the language model to tell us its order, not the other way around.
      lm_numwords = max(lm_numwords, lm->getOrder());
   }

   lmWeightsV.push_back(weight);
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
      description << "\t";
      if (c->describeModelOnly)
         description << "LanguageModel:" << c->lmFiles[i];
      else
         description << lms[i]->describeFeature();
      description << nf_endl;
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
   // sparse models
   featureIndex += 1;
   for (Uint i = 0; i < decoder_features.size(); ++i) {
      SparseModel* m = dynamic_cast<SparseModel*>(decoder_features[i]);
      if (m) {
         m->dump(description, featureIndex, c->verbosity);
         featureIndex += m->numFeatures();
      }
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

   if (!filter_features.empty())
      description << endl;
   for ( vector<DecoderFeature *>::const_iterator it = filter_features.begin();
         it != filter_features.end(); ++it )
      description << "Filter Feature:\t" << (*it)->describeFeature() << endl;
   return description.str();
} // describeModel

BasicModel *BasicModelGenerator::createModel(
   newSrcSentInfo& info, bool alwaysTryDefault)
{
   //LOG_VERBOSE2(bmgLogger, "Creating a model for: %s", join(src_sent).c_str());

   // Make sure the vocab is properly populated with given src and ref words
   typedef vector<string>::const_iterator IT;
   for (IT it(info.src_sent.begin()); it!=info.src_sent.end(); ++it)
      tgt_vocab.add(it->c_str());
   if (info.tgt_sent != NULL)
      for (IT it(info.tgt_sent->begin()); it!=info.tgt_sent->end(); ++it)
         tgt_vocab.add(it->c_str());

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
   for ( vector<DecoderFeature *>::iterator it = filter_features.begin();
         it != filter_features.end(); ++it)
      (*it)->newSrcSent(info);

   // Inform all LM models that we are about to process a new source sentence.
   for (Uint i=0;i<lms.size();++i) {
      lms[i]->clearCache();
      lms[i]->newSrcSent(info.src_sent, info.external_src_sent_id);
   }

   // Clear the phrase table caches every 10 sentences
   if ( info.internal_src_sent_seq % 10 == 0 )
      phraseTable->clearCache();

   // Compute and save the phrase heuristic for all phrase pairs kept
   computePhrasePartialScores(info.potential_phrases, info.src_sent.size());

   // -ttable-prune-type full is calculated here, on the phrase partial scores.
   if (c->phraseTablePruneType == "full")
      applyPhraseTablePruning(info.potential_phrases, info.src_sent.size());

   double **futureScores = precomputeFutureScores(info.potential_phrases,
         info.src_sent.size());

   info.model = new BasicModel(info, futureScores, *this);
   return info.model;
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
      // Create a PhraseInfo for each marked phrase
      PhraseInfo* newPI = new PhraseInfo;
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

      // Registered phrase pair annotators need to be called with the marked phrases
      for (PhraseTable::AnnotatorRegistry::const_iterator a_it(phraseTable->getAnnotators().begin()),
                                                          a_end(phraseTable->getAnnotators().end());
           a_it != a_end; ++a_it)
         a_it->second->annotateMarkedPhrase(newPI, &(*it), &info);

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
   PhraseInfo* newPI = new PhraseInfo;
   newPI->forward_trans_probs.insert(newPI->forward_trans_probs.end(),
         forwardWeightsV.size(), LOG_ALMOST_0);
   newPI->forward_trans_prob = dotProduct(forwardWeightsV,
         newPI->forward_trans_probs, forwardWeightsV.size());

   newPI->adir_probs.insert(newPI->adir_probs.end(),    //boxing
         adirTransWeightsV.size(), LOG_ALMOST_0);       //boxing
   newPI->adir_prob = dotProduct(adirTransWeightsV,     //boxing
         newPI->adir_probs, adirTransWeightsV.size());  //boxing

   newPI->src_words = range;
   newPI->phrase = VectorPhrase(1, tgt_vocab.add(word));
   newPI->phrase_trans_probs.insert(newPI->phrase_trans_probs.end(),
         transWeightsV.size(), LOG_ALMOST_0);
   newPI->phrase_trans_prob = dotProduct(transWeightsV,
	 newPI->phrase_trans_probs, transWeightsV.size());

   // Registered phrase pair annotators need to be called with the no trans phrases
   for (PhraseTable::AnnotatorRegistry::const_iterator a_it(phraseTable->getAnnotators().begin()),
                                                       a_end(phraseTable->getAnnotators().end());
        a_it != a_end; ++a_it)
      a_it->second->annotateNoTransPhrase(newPI, range, word);

   return newPI;
} // makeNoTransPhraseInfo

void BasicModelGenerator::computePhrasePartialScores(
   vector<PhraseInfo *> **phrases, Uint sentLength)
{
   for (Uint i = 0; i < sentLength; ++i) {
      for (int j = i; j >= 0; j--) {
         for ( vector<PhraseInfo*>::const_iterator it = phrases[j][i-j].begin();
               it != phrases[j][i-j].end(); ++it ) {
            (*it)->partial_score = phrasePartialScore(*it);
         }
      }
   }
}

void BasicModelGenerator::displaySPI(const char* msg, PhraseInfo* pi) {
   cerr << "\t" << msg << " " << pi->src_words << " "
        << getStringPhrase(pi->phrase) << " "
        << pi->partial_score << nf_endl;
}

void BasicModelGenerator::applyPhraseTablePruning(
   vector<PhraseInfo *> **phrases, Uint sentLength)
{
   const Uint limit = c->phraseTableSizeLimit;
   const double threshold = c->phraseTableThreshold;

   if (limit == NO_SIZE_LIMIT && threshold == 0) return;

   const double logThreshold = log(threshold);
   PhraseTable::PhraseScorePairLessThan pplt;

   for (Uint i = 0; i < sentLength; ++i) {
      for (int j = i; j >= 0; j--) {
         vector<PhraseInfo*>& in_phrases(phrases[j][i-j]);
         if (in_phrases.empty()) continue;

         // If required, apply the -ttable-threshold pruning criterion
         if (threshold) {
            vector<PhraseInfo*> to_delete, kept_phrases;
            to_delete.reserve(in_phrases.size());
            kept_phrases.reserve(min(in_phrases.size(),32));
            for (Uint k = 0; k < in_phrases.size(); ++k) {
               PhraseInfo* pi = in_phrases[k];
               if (pi->partial_score > logThreshold) {
                  kept_phrases.push_back(pi);
                  if (verbosity >= 4) displaySPI("Keeping(T)", pi);
               } else {
                  to_delete.push_back(pi);
                  //if (verbosity >= 4) displaySPI("Discarding(T)", pi);
               }
            }
            if (kept_phrases.empty()) {
               // Make sure we don't filter everything out - in particular, we don't want to
               // delete the no-trans phrase info added before this method is called.
               vector<PhraseInfo*>::iterator top_phrase =
                  std::max_element(in_phrases.begin(), in_phrases.end(), pplt);
               assert(top_phrase != in_phrases.end());
               kept_phrases.push_back(*top_phrase);
               if (verbosity >= 4) displaySPI("No phrase passed threshold, keeping top candidate", *top_phrase);
               // delete the rest here
               *top_phrase = NULL; // make sure the one we keep does not get deleted
               for (vector<PhraseInfo*>::iterator it = in_phrases.begin(); it < in_phrases.end(); ++it)
                  delete *it;
            } else {
               for (Uint k = 0; k < to_delete.size(); ++k)
                  delete to_delete[k];
            }
            // Put the results on top of the original phrase list
            in_phrases.swap(kept_phrases);
         }

         // If required, apply the -ttable-limit pruning criterion
         if (limit != NO_SIZE_LIMIT && in_phrases.size() > limit) {
            vector<PhraseInfo*> kept_phrases;
            kept_phrases.reserve(limit);
            make_heap(in_phrases.begin(), in_phrases.end(), pplt);
            for (Uint k = 0; k < limit; ++k)
            {
               PhraseInfo* pi = lazy::pop_heap(in_phrases, pplt);
               if (verbosity >= 4) displaySPI("Keeping(L)", pi);
               kept_phrases.push_back(pi);
            }
            // Delete the rest
            for (Uint k = 0; k < in_phrases.size(); ++k) {
               if (verbosity >= 4 && threshold) displaySPI("Discarding(L)", in_phrases[k]);
               delete in_phrases[k];
            }

            // Put the results on top of the original phrase list
            in_phrases.swap(kept_phrases);
         }
      }
   }
}

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
               it != phrases[j][i-j].end(); ++it )
         {
            double newScore = (*it)->partial_score;
            if (newScore == -INFINITY)
               continue;

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
         for (Uint k = j + 1; k < i + 1; ++k)
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

double BasicModelGenerator::rangePartialScore(const PartialTranslation& trans)
{
   // Filter features can cause early pruning here.
   for (Uint k(0), k_end(filter_features.size()); k < k_end; ++k)
      if (filter_features[k]->partialScore(trans) < 0.0)
         return -INFINITY;

   // Only "other features" come into play for this partial score.
   double result = 0.0;
   for (Uint k = 0; k < decoder_features.size(); ++k) {
      result += decoder_features[k]->partialScore(trans)
                * featureWeightsV[k];
   }
   return result;
}

double BasicModelGenerator::phrasePartialScore(const PhraseInfo* phrase)
{
   // This causes early pruning of a given phrase instead of a state.
   for (Uint k(0), k_end(filter_features.size()); k < k_end; ++k) {
      if (filter_features[k]->precomputeFutureScore(*phrase) < 0.0) {
         return -INFINITY;
      }
   }

   // Backward translation score - the main information source used here.
   double score = phrase->phrase_trans_prob;

   // Forward translation score (if used)
   score += phrase->forward_trans_prob;

   // Adirectional translation score (if used)
   score += phrase->adir_prob;

   // LM score: use a heuristic LM score, as specified by the user
   double lmScore = 0;
   if ( cubePruningLMHeuristic == LMH_UNIGRAM ) {
      // Our old future score LM heuristic: use the unigram LM score
      for ( Phrase::const_iterator jt = phrase->phrase.begin();
            jt != phrase->phrase.end(); ++jt )
         for (Uint k = 0; k < lmWeightsV.size(); ++k)
            lmScore += lms[k]->cachedWordProb(*jt, NULL, 0) * lmWeightsV[k];
   } else if ( cubePruningLMHeuristic == LMH_INCREMENTAL ) {
      Uint phrase_size(phrase->phrase.size());
      Uint reversed_phrase[phrase_size];
      for ( Uint i(0); i < phrase_size; ++i )
         reversed_phrase[i] = phrase->phrase[phrase_size - i - 1];
      for ( Uint j(0); j < lms.size(); ++j )
         for ( Uint i(0); i < phrase_size; ++i )
            lmScore += lms[j]->cachedWordProb(reversed_phrase[i],
                     &(reversed_phrase[i+1]), phrase_size-i-1)
                   * lmWeightsV[j];
   } else if ( cubePruningLMHeuristic == LMH_SIMPLE ) {
      // The "simple" heuristic is admissible by ignoring words with
      // insufficient context
      Uint phrase_size(phrase->phrase.size());
      Uint reversed_phrase[phrase_size];
      for ( Uint i(0); i < phrase_size; ++i )
         reversed_phrase[i] = phrase->phrase[phrase_size - i - 1];
      for ( Uint j(0); j < lms.size(); ++j )
         for ( Uint i(0); i < phrase_size - (lms[j]->getOrder()-1); ++i )
            lmScore += lms[j]->cachedWordProb(reversed_phrase[i],
                     &(reversed_phrase[i+1]), phrase_size-i-1)
                   * lmWeightsV[j];
   } // else LMH==NONE: nothing to do!

   score += lmScore;

   if (1&&c->verbosity >= 4)
      cerr << "Partially scoring phrase " << getStringPhrase(phrase->phrase) << endl
           << "\tphrase score           "
           << "B " << phrase->phrase_trans_prob
           << " + F " << phrase->forward_trans_prob
           << " + A " << phrase->adir_prob
           << " + h(LM) " << lmScore << endl
           << "\tother feature scores  ";

   // Other feature scores are available through precomputeFutureScore().
   for (Uint k(0); k < decoder_features.size(); ++k) {
      double fScore = decoder_features[k]->precomputeFutureScore(*phrase)
                      * featureWeightsV[k];
      score += fScore;
      if (1&&c->verbosity >= 4)
         cerr << " + ff_" << k << " " << fScore;
   }

   if (1&&c->verbosity >= 4)
      cerr << " = " << score << endl;

   return score;
} // phrasePartialScore

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
   Uint contextSize;
   if (c->minimizeLmContextSize) {
      assert(trans.back->lmContextSize >= 0 && "calling getRawLM() with uninitialized lmContextSize");
      contextSize = trans.back->lmContextSize;
   } else {
      contextSize = lm_numwords - 1;
   }
   const Uint num_words(last_phrase_size + contextSize);
   // Get the missing context words in the previous phrase(s);
   trans.getLastWords(endPhrase, num_words, true);

   if (endPhrase.size() < num_words)
   {
      // Start of sentence
      if (!c->nosent)
         endPhrase.push_back(tgt_vocab.index(PLM::SentStart));
   }
   assert(endPhrase.size() >= last_phrase_size);
   assert(endPhrase.size() <= num_words);

   const Uint context_len = endPhrase.size()-1;

   // Make room for results in vector
   lmVals.insert(lmVals.end(), lmWeightsV.size(), 0);
   // Create an iterator at the start of the results for convenience
   vector<double>::iterator results = lmVals.end() - lmWeightsV.size();

   for (int i = last_phrase_size - 1; i >= 0; --i)
   {
      for (Uint j = 0; j < lmWeightsV.size(); ++j)
      {
         // Compute score for j-th language model for word reverseArray[i]
         results[j] += lms[j]->cachedWordProb(endPhrase[i], &(endPhrase[i + 1]),
                                        context_len-i);
      }
   }

   if (trans.sourceWordsNotCovered.empty() && !c->nosent)
   {
      for (Uint j = 0; j < lmWeightsV.size(); ++j) {
         // Compute score for j-th language model for end-of-sentence
         results[j] += lms[j]->cachedWordProb(tgt_vocab.index(PLM::SentEnd),
               &(endPhrase[0]), context_len+1);
      }
   }

   // Calculate the lmContextSize for this partial translation if required
   if (c->minimizeLmContextSize && trans.lmContextSize == -1) {
      // lm_min_context is 1 by default because some sparse features depend on
      // the LM to make sure the last word of the previous phrase is taken
      // into account for recombination. It is 3 if NNJMs are used, because
      // NNJM don't enforce their own recombination themselves, instead rely
      // on the LM to keep three words of context.
      Uint contextAvailable = min(endPhrase.size(), lm_numwords-1);
      Uint requiredLMContextSize = min(lm_min_context, contextAvailable);
      for (Uint j = 0;
           requiredLMContextSize < contextAvailable && j < lmWeightsV.size();
           ++j) {
         Uint size = lms[j]->minContextSize(&(endPhrase[0]), contextAvailable);
         if (size > requiredLMContextSize) requiredLMContextSize = size;
      }
      trans.lmContextSize = requiredLMContextSize;
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
   // This comes directly from the PhraseInfo object.
   const PhraseInfo *phrase = trans.lastPhrase;
   assert(phrase != NULL);
   transVals.insert(transVals.end(), phrase->phrase_trans_probs.begin(),
         phrase->phrase_trans_probs.end());
} // getRawTrans

double BasicModelGenerator::dotProductTrans(const PartialTranslation &trans)
{
   // This value was already calculated by PhraseTable::getPhrases!!!
   return trans.lastPhrase->phrase_trans_prob;
}

void BasicModelGenerator::getRawForwardTrans(vector<double> &forwardVals,
        const PartialTranslation &trans)
{
   if (forwardWeightsV.empty()) return;

   // This comes from the PhraseInfo object, assuming we have forward weights,
   // as asserted above.
   const PhraseInfo *phrase = trans.lastPhrase;
   assert(phrase != NULL);
   forwardVals.insert(forwardVals.end(), phrase->forward_trans_probs.begin(),
         phrase->forward_trans_probs.end());
}

double BasicModelGenerator::dotProductForwardTrans(const PartialTranslation &trans)
{
   return trans.lastPhrase->forward_trans_prob;
}

//boxing getRawAdirTrans
void BasicModelGenerator::getRawAdirTrans(vector<double> &adirVals,
        const PartialTranslation &trans)
{
   if (adirTransWeightsV.empty()) return;

   // This comes from the PhraseInfo object, assuming we have adirectional
   // weights, as asserted above.
   const PhraseInfo* phrase = trans.lastPhrase;
   assert(phrase != NULL);
   adirVals.insert(adirVals.end(), phrase->adir_probs.begin(),
         phrase->adir_probs.end());

}

double BasicModelGenerator::dotProductAdirTrans(const PartialTranslation &trans)
{
   return trans.lastPhrase->adir_prob;
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


BasicModel::BasicModel(const newSrcSentInfo& info,
                       double **futureScore,
                       BasicModelGenerator &parent) :
   c(parent.c),
   info(info),
   src_len(info.src_sent.size()),
   lmWeights(parent.lmWeightsV),
   featureWeights(parent.featureWeightsV),
   phrases(info.potential_phrases),
   scored_phrases(NULL),
   futureScore(futureScore),
   parent(parent)
{}

BasicModel::~BasicModel()
{
   for (Uint i = 0; i < src_len; ++i) {
      for (Uint j = 0; j < src_len - i; j++) {
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

   // Filter feature scores - done first because if the result is negative,
   // we return -INFINITY and skip the rest of this function.
   for (Uint k(0), k_end(parent.filter_features.size()); k < k_end; ++k) {
      if (parent.filter_features[k]->score(trans) < 0.0) {
         if (verbosity >= 3)
            cerr << "\tfilter feature score < 0 => setting this state's score to -INFINITY" << endl;
         if (c->minimizeLmContextSize) trans.lmContextSize = 0;
         return -INFINITY;
      }
   }

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
   const double transScore = parent.dotProductTrans(trans);
   if (verbosity >= 3) {
      cerr << "\tbackward trans score  " << transScore;
      vals.clear();
      parent.getRawTrans(vals, trans);
      cerr << "  [ " << join(vals) << " ]" << endl;
   }

   // Forward translation model score
   const double forwardScore = parent.dotProductForwardTrans(trans);
   if (verbosity >= 3) {
      cerr << "\tforward trans score   " << forwardScore;
      vals.clear();
      parent.getRawForwardTrans(vals, trans);
      cerr << "  [ " << join(vals) << " ]" << endl;
   }

   // Adirectional translation model score
   const double adirScore = parent.dotProductAdirTrans(trans);
   if (verbosity >= 3) {
      cerr << "\tadir trans score      " << adirScore;
      vals.clear();
      parent.getRawAdirTrans(vals, trans);
      cerr << "  [ " << join(vals) << " ]" << endl;
   }

   if (verbosity >= 3) {
      if (trans.lmContextSize >= 0)
         cerr << "\tlm context size       " << trans.lmContextSize << endl;
   }

   return transScore + forwardScore + adirScore + lmScore + ffScore;
} // scoreTranslation

/*
 * Estimate future score.  Returns the difference between the future score for
 * the given translation and the score for the translation.  Specifically, the
 * future score estimation takes is the maximum phrase translation score +
 * heuristic language model score + distortion score for all possible
 * translations of the remaining source words.  Note: the computation here
 * makes use of the condition that the ranges in trans.sourceWordsNotCovered
 * are in ascending order.
 * @param trans   The translation to score
 * @return        The incremental log future-probability.
 */
double BasicModel::computeFutureScore(const PartialTranslation &trans)
{
   // Filter features - this is an inefficient place for filter feature to
   // reject a state, but in case a feature does it, do the test anyway.
   for (Uint k(0), k_end(parent.filter_features.size()); k < k_end; ++k)
      if (parent.filter_features[k]->futureScore(trans) < 0.0)
         return -INFINITY;

   double precomputedScore = 0;
   for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin(); it !=
         trans.sourceWordsNotCovered.end(); it++)
      precomputedScore += futureScore[it->start][it->end - it->start - 1];

   // Other features
   double ffFutureScore = 0;
   for (Uint k = 0; k < parent.decoder_features.size(); ++k)
      ffFutureScore += parent.decoder_features[k]->futureScore(trans) * featureWeights[k];

   if (trans.sourceWordsNotCovered.empty()) {
      if (ffFutureScore != 0.0) {
         for (Uint k = 0; k < parent.decoder_features.size(); ++k)
            if (0.0 != parent.decoder_features[k]->futureScore(trans))
               error(ETWarn, "Feature %s gave a non-zero future score on a complete translation.",
                     parent.decoder_features[k]->describeFeature().c_str());
         error(ETFatal, "Non-zero future score on a complete translation violates "
               "the definition of decoder features.");
      }
      assert(precomputedScore == 0);
   }

   return precomputedScore + ffFutureScore;
} // computeFutureScore

double BasicModel::computePartialFutureScore(const PartialTranslation &trans)
{
   // Filter features - this is an inefficient place for filter feature to
   // reject a state, but in case a feature does it, do the test anyway.
   for (Uint k(0), k_end(parent.filter_features.size()); k < k_end; ++k)
      if (parent.filter_features[k]->partialFutureScore(trans) < 0.0)
         return -INFINITY;

   double precomputedScore = 0;
   for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin(); it !=
         trans.sourceWordsNotCovered.end(); it++)
      precomputedScore += futureScore[it->start][it->end - it->start - 1];

   // Other features
   double ffPartialFutureScore = 0;
   for (Uint k = 0; k < parent.decoder_features.size(); ++k)
      ffPartialFutureScore += parent.decoder_features[k]->partialFutureScore(trans) * featureWeights[k];

   if (trans.sourceWordsNotCovered.empty()) {
      if (ffPartialFutureScore != 0.0) {
         for (Uint k = 0; k < parent.decoder_features.size(); ++k)
            if (0.0 != parent.decoder_features[k]->partialFutureScore(trans))
               error(ETWarn, "Feature %s gave a non-zero partial future score on a complete translation.",
                     parent.decoder_features[k]->describeFeature().c_str());
         error(ETFatal, "Non-zero partial future score on a complete translation violates "
               "the definition of decoder features.");
      }
      assert(precomputedScore == 0);
   }

   return precomputedScore + ffPartialFutureScore;
} // computePartialFutureScore

Uint BasicModel::computeRecombHash(const PartialTranslation &trans)
{
   // This might overflow result, but who cares.
   Uint result = 0;
   // make this endPhrase static so we don't have to reallocate this vector
   // millions of times.  (this method gets called ridiculously often...)
   static VectorPhrase endPhrase;
   endPhrase.clear();
   if (c->minimizeLmContextSize) {
      assert(trans.lmContextSize >= 0 && "lmContextSize not properly initialized before calling computeRecombHash()");
      result = (result + trans.lmContextSize) * 7;
      trans.getLastWords(endPhrase, trans.lmContextSize);
   } else {
      trans.getLastWords(endPhrase, parent.lm_numwords - 1);
   }
   for (VectorPhrase::iterator it = endPhrase.begin();
         it != endPhrase.end(); ++it) {
      result += *it;
      result *= 2551; // yes, 2551 is prime; big because vocabularies are large.
   }

   for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin();
         it != trans.sourceWordsNotCovered.end(); it++) {
      result += it->start + it->end * src_len;
      result *= 17;
   }

   // As in isRecombinable(), we now handle the shift-reducer here instead of
   // doing so repeatedly in features that use it.
   if (trans.shiftReduce) {
      result += trans.shiftReduce->computeRecombHash();
      result *= 17;
   }

   for (vector<DecoderFeature *>::iterator it = parent.decoder_features.begin();
         it != parent.decoder_features.end(); ++it) {
      result += (*it)->computeRecombHash(trans);
      result *= 17;
   }

   for (vector<DecoderFeature *>::iterator it = parent.filter_features.begin();
         it != parent.filter_features.end(); ++it) {
      result += (*it)->computeRecombHash(trans);
      result *= 17;
   }

   return result;
} // computeRecombHash

bool BasicModel::isRecombinable(const PartialTranslation &trans1,
        const PartialTranslation &trans2)
{
   if (trans1.sourceWordsNotCovered != trans2.sourceWordsNotCovered)
      return false;
   if (c->minimizeLmContextSize) {
      assert(trans1.lmContextSize >= 0 && trans2.lmContextSize >= 0 && "lmContextSize not properly initialized before calling isRecombinable()");
      if (trans1.lmContextSize != trans2.lmContextSize ||
          !trans1.sameLastWords(trans2, trans1.lmContextSize))
         return false;
   } else {
      if (!trans1.sameLastWords(trans2, parent.lm_numwords - 1))
         return false;
   }

   // Moved ShiftReducer::isRecombinable here because -dist-limit-itg and DHDM
   // without HDM both have the SR stack in place, but not SR::isR() testing.
   // SR::isR() handles the case where no SR stack is in use cleanly.
   if (!ShiftReducer::isRecombinable(trans1.shiftReduce, trans2.shiftReduce))
      return false;

   for (vector<DecoderFeature *>::iterator it = parent.filter_features.begin();
        it != parent.filter_features.end(); ++it)
      if (!(*it)->isRecombinable(trans1, trans2))
         return false;

   for (vector<DecoderFeature *>::iterator it = parent.decoder_features.begin();
        it != parent.decoder_features.end(); ++it)
      if (!(*it)->isRecombinable(trans1, trans2))
         return false;

   return true;
} // isRecombinable

bool BasicModel::earlyFilterFeatureViolation(const PartialTranslation& prev_pt, Range src_words)
{
   if (parent.filter_features.empty()) return false;

   // this is implemented for regular decoding, which does not call
   // rangePartialScore()
   static PhraseInfo dummy_info;
   dummy_info.src_words = src_words;
   PartialTranslation temp_pt(&prev_pt, &dummy_info);
   for (Uint k(0), k_end(parent.filter_features.size()); k < k_end; ++k)
      if (parent.filter_features[k]->partialScore(temp_pt) < 0.0)
         return true;
   return false;
}

bool BasicModel::respectsDistortionLimit(const PartialTranslation& prev_pt, Range src_words, const UintSet* out_cov)
{
   const UintSet& cov = prev_pt.sourceWordsNotCovered;
   const Range last_phrase = prev_pt.lastPhrase->src_words;
   return
      DistortionModel::respectsDistLimit(cov, last_phrase, src_words,
         c->distLimit, src_len, c->distLimitSimple, c->distLimitExt, out_cov)
   ||
      (c->distPhraseSwap &&
       (scored_phrases
          ? DistortionModel::isPhraseSwap(cov, last_phrase, src_words, src_len, scored_phrases)
          : DistortionModel::isPhraseSwap(cov, last_phrase, src_words, src_len, phrases)));
}

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
   wts.insert( wts.end(), parent.transWeightsV.begin(), parent.transWeightsV.end() );
   wts.insert( wts.end(), parent.forwardWeightsV.begin(), parent.forwardWeightsV.end() );
   wts.insert( wts.end(), parent.adirTransWeightsV.begin(), parent.adirTransWeightsV.end() );
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

