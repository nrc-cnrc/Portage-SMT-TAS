/**
 * @author Aaron Tikuisis
 * @file basicmodel.h  This file contains the declaration of the basic phrase
 * decoder model, which uses an n-gram language model for language probability,
 * phrase translation probability, and distortion probability in order to
 * compute the overall probability.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef BASICMODEL_H
#define BASICMODEL_H

#include "canoe_general.h"
#include "phrasedecoder_model.h"
#include "phrasetable.h"
#include "distortionmodel.h"
#include "decoder_feature.h"
#include "config_io.h"
#include "vocab_filter.h"
#include "marked_translation.h"
#include "new_src_sent_info.h"


using namespace std;

namespace Portage
{
   class PLM;
   class BasicModel;
   class rnd_distribution;


   /**
    * Core of the decoder model.
    * The basic model generator holds all the structures needed for decoding,
    * and is used to create specific instances of decoder models for each
    * sentence.
    */
   class BasicModelGenerator : private NonCopyable
   {
   public:
      /// Global configuration object
      const CanoeConfig* const c;

   protected:
      /**
       * Assemble all hypotheses from the phrase table(s) for a sentence.
       * Creates a triangular array of all the phrase options for the given
       * sentence.  The (i, j)-th entry of the array returned will contain
       * all phrase translation options for the source range [i, i + j + 1).
       * @param info.src_sent  The source sentence.
       * @param info.marks     The marked translations for the sentence.
       * @param alwaysTryDefault  If true, the default translation (source
       *                  word translates as itself) will be included in the
       *                  phrase options for all phrases; if false, the
       *                  default translation is included only when there is
       *                  no other option.
       * @param info.oovs If not NULL, will be set to true for each position
       *                  in src_sent that contains an out-of-vocabulary word.
       * @return          A triangular array of translation options,
       *                  organized by the source range covered.
       */
      virtual vector<PhraseInfo *> **createAllPhraseInfos(
            const newSrcSentInfo& info,
            bool alwaysTryDefault);

      /**
       * @brief add marked phrases to the phrase table info.
       *
       * Adds all the marked phrases to the triangular array result.  Ranges
       * that should be skipped (ranges that have marked translations when
       * c.bypassMarked is false) are stored in rangesToSkip.
       * @param info.marks    The marked translations to add.
       * @param result        The triangular array in which to enter the
       *                      PhraseInfo object pointers for the marked phrases.
       *                      The (i, j)-th entry will contain phrases for the
       *                      source range [i, i + j + 1).
       * @param rangesToSkip  Ranges that should be skipped are put here.
       * @param info.src_sent The source sentence in context.
       */
      virtual void addMarkedPhraseInfos(
            const newSrcSentInfo& info,
            vector<PhraseInfo *> **result,
            vector<Range> &rangesToSkip);

      /**
       * Produce a copy-as-is hypothesis.
       *
       * Produces a non-translation PhraseInfo for the given range; that is,
       * the word is translated as itself according to this translation
       * option.  This should be used only when there is no alternative (ie.
       * the word has no translation in the phrase table).  The word is also
       * added to the language model if necessary, so that it doesn't give
       * -INFINITY as a language model score.
       * @param range     The source range to use.  Since this is a
       *                  single-word translation, this should be a range of
       *                  the form [i, i + 1).
       * @param word      The word to use as the translation.
       * @return          A pointer to a new PhraseInfo that represents the
       *                  translation of the given word to itself.
       */
      virtual PhraseInfo *makeNoTransPhraseInfo(const Range &range,
            const char *word);

      /**
       * Future score pre-computation using dynamic programming.
       *
       * Computes the estimated future translation probability for each range
       * of the source sentence and creates a triangular array to store the
       * values.  The (i, j)-th entry of the array returned contains the
       * estimated future score for the source range [i, i + j + 1).  Each
       * future score estimate is the maximum score possible, where score is
       * computed using translation probability, unigram language model
       * probability, and length penalty.
       * @param phrases   The triangular array of phrase options for the
       *                  sentence; phrases[i][j] should contain the phrase
       *                  translation options for the range [i, i + j + 1).
       * @param sentLength  The number of words in the source sentence.
       * @return          A triangular array of estimated future scores by
       *                  source range.
       */
      virtual double **precomputeFutureScores(vector<PhraseInfo *> **phrases,
            Uint sentLength);

      /**
       * @brief Initialize all the decoder features other than LMs and TMs.
       *
       * Initialize the decoder features other than LMs and TMs.
       * Called by the constructor, creates the features found in the
       * decoder_features vector
       *
       * Any new features created should be initialized here, and should
       * have its parameters added to CanoeConfig.
       *
       * @param c  Global canoe configuration object
       */
      void InitDecoderFeatures(const CanoeConfig& c);

      /**
       * Verbosity level (between 1 and 4)
       */
      Uint verbosity;

      /**
       * The vocabulary for the target language.  (Also includes words from
       * the source language, for various purposes.)
       */
      VocabFilter tgt_vocab;

      /**
       * The vocab to store bi-words (for BiLM queries) that will constitute bi_phrase
       */
      VocabFilter biPhraseVocab;

      /**
       * All decoder features other than LMs and TMs (e.g., distortion,
       * length, segmentation, and any feature added in the future).
       */
      vector<DecoderFeature*> decoder_features;

      /**
       * The language model(s)
       */
      vector<PLM *> lms;

      /**
       * The phrase table.
       */
      PhraseTable *phraseTable;

      vector<rnd_distribution*> random_feature_weight;
      vector<rnd_distribution*> random_trans_weight;
      vector<rnd_distribution*> random_forward_weight;
      vector<rnd_distribution*> random_adir_weight; //boxing
      vector<rnd_distribution*> random_lm_weight;

   public:
      /**
       * Whether reading tables should be limited to vocab/phrases
       * already there.
       */
      const bool limitPhrases;

   protected:
      /**
       * Language model weights, one entry per LM.
       */
      vector<double> lmWeightsV;

      /**
       * Translation model weights.  If a mix of TM types are used, this vector
       * contains first the multi-prob model weights, then the TPPT model
       * weights.
       */
      vector<double> transWeightsV;

      /**
       * Forward translation weights, in the same order as in transWeightsV.
       */
      vector<double> forwardWeightsV;

      /**
       * adirectional translation weights.
       */
      vector<double> adirTransWeightsV; //boxing

      /**
       * Weight of the other decoder features, typically but not
       * necessarily distortion, length and segmentation, in that order.
       * This vector will always have the same number of element as the
       * decoder_features vector.
       */
      vector<double> featureWeightsV;

   public:
      /**
       * The window size (number of words) used by the language model.
       * Probably between 2 and 5.  (Typically 3)
       */
      Uint lm_numwords;

   protected:
      /// Possible LM Heuristic types
      enum LMHeuristicType { LMH_NONE, LMH_UNIGRAM, LMH_INCREMENTAL,
                             LMH_SIMPLE };

      /**
       * Convenient method to encapsulate converting "none", "unigram", "incr",
       * and "simple" to the enum, since we need to do it in several places.
       */
      static LMHeuristicType lm_heuristic_type_from_string(const string& s);

      /// LM heuristic to use to compute future scores
      LMHeuristicType futureScoreLMHeuristic;
      /// LM heuristic to use in cube pruning
      LMHeuristicType cubePruningLMHeuristic;
      /// future Score Use Forward tm
      bool futureScoreUseFtm;

      // These compute the raw feature function values of different sorts.
      /**
       * @brief Get raw language models probabilities
       * @param lmVals        vector to which will be appended the probs
       * @param trans         partial translation to score
       */
      virtual void getRawLM(vector<double> &lmVals,
            const PartialTranslation &trans);
      /**
       * @brief Get raw translation models probabilities
       * @param transVals     vector to which will be appended the probs
       * @param trans         partial translation to score
       */
      virtual void getRawTrans(vector<double> &transVals,
            const PartialTranslation &trans);
      /**
       * @brief Get raw forward translation models scores
       * @param forwardVals   vector to which will be appended the scores
       * @param trans         partial translation to score
       */
      virtual void getRawForwardTrans(vector<double> &forwardVals,
            const PartialTranslation &trans);

      //boxing
      /**
       * @brief Get raw adirectional translation model scores
       * @param adirVals      vector to which will be appended the scores
       * @param trans         partial translation to score
       */
      virtual void getRawAdirTrans(vector<double> &adirVals,
            const PartialTranslation &trans);
      //boxing

      /**
       * @brief Get raw forward scores from other features
       * @param ffVals        vector to which will be appended the scores
       * @param trans         partial translation to score
       */
      virtual void getRawFeatures(vector<double> &ffVals,
            const PartialTranslation &trans);

      // Optimized versions of the getRaw* functions, for when you're only
      // going to call dotproduct on the result, i.e., the typical use, in
      // which case it's silly to allocate space in a vector for it!
      /**
       * @brief Get weighted, combined language model score.
       * @param weights       individual model weights.
       * @param trans         partial translation to score.
       * @return              the combined score.
       */
      virtual double dotProductLM(const vector<double> &weights,
            const PartialTranslation &trans);
      /**
       * @brief Get weighted, combined translation model score.
       * @param weights       individual model weights.
       * @param trans         partial translation to score.
       * @return              the combined score.
       */
      virtual double dotProductTrans(const vector<double> &weights,
            const PartialTranslation &trans);
      /**
       * @brief Get weighted, combined forward translation model score.
       * @param weights       individual model weights.
       * @param trans         partial translation to score.
       * @return              the combined score.
       */
      virtual double dotProductForwardTrans(const vector<double> &weights,
            const PartialTranslation &trans);

      //boxing
      /**
       * @brief Get weighted, combined adirectional translation model score.
       * @param weights       individual model weights.
       * @param trans         partial translation to score.
       * @return              the combined score.
       */
      virtual double dotProductAdirTrans(const vector<double> &weights,
            const PartialTranslation &trans);
      //boxing

      /**
       * @brief Get weighted, combined score from all other features
       * @param weights       individual model weights.
       * @param trans         partial translation to score.
       * @return              the combined score.
       */
      virtual double dotProductFeatures(const vector<double> &weights,
            const PartialTranslation &trans);

      /**
       * Convert a phrase from vector<Uint> representation to string.
       * Subclasses may override as needed to change the semantics of a phrase.
       * @param uPhrase   The phrase as a vector of Uint's.
       * @return the result
       */
      virtual string getStringPhrase(const Phrase &uPhrase) const;

      /**
       * @brief Get the Uint representation of word
       * @param word      The string representation of the word
       * @return          The Uint representation of word, or
       *                  PhraseDecoderModel::OutOfVocab if word is not
       *                  in the current vocabulary.
       */
      virtual Uint getUintWord(const string &word);

      /**
       * When TPPTs are used with limitPhrases, addLanguageModel
       * calls phraseTable->extractVocabFromTPPTs() before loading the
       * language models, because opening a TPPT does not involve
       * populating tgt_vocab, the way opening a text phrase table does.
       *
       * This flag is set iff extractVocabFromTPPTs() has been
       * called since the last TPPT was opened.
       */
      bool vocab_read_from_TPPTs;

      friend class BasicModel;
   public:
      /**
       * Creates a basic model generator will all models loaded in the
       * default way.  If your program loads a canoe.ini config file and
       * simply loads all the models accordingly, you can call this static
       * method instead of using the constructors and various add methods
       * below.
       *
       * @param c             Canoe configuration object.  See the
       *                      documention of the constructors for this class
       *                      for details of relevant member variables.
       * @param sents         The source sentences, used to determine the
       *                      phrases to limit to (if c.loadFirst is
       *                      false).  (Can be NULL if c.loadFirst is true)
       * @param marks         All the marked translations. (Can be NULL if
       *                      c.loadFirst is true)
       * @pre c.loadFirst || sents != NULL && marks != NULL
       */
      static BasicModelGenerator* create(const CanoeConfig& c,
            const vector<vector<string> > *sents = NULL,
            const vector<vector<MarkedTranslation> > *marks = NULL);

      /**
       * Constructor, creates a new BasicModelGenerator.
       * Since the source phrases are not prespecified, this version cannot
       * allow the phrases read to be limited.  Note that all the parameters
       * here, except verbosity, may be changed before creating a model
       * (since they are stored as public instance variables).
       *
       * NOTE: Most of paramemeters described below are now members of the
       *       CanoeConfig object, but the documentation is kept here, where
       *       it is relevant.
       *
       * @param c                 Global canoe configuration object
       *
       * c.bypassMarked    Whether to use translations from the phrase
       *            table in addition to marked translations, when marked
       *            translations are provided.<BR>
       * c.addWeightMarked The value to add to each marked
       *            translation's log probability.  This has a real effect
       *            only if bypassMarked is true.<BR>
       * c.*Weight         The weights for various models.<BR>
       * c.phraseTableSizeLimit  The maximum number of translations
       *            per source phrase, or NO_SIZE_LIMIT for no limit.  If
       *            available, the forward probability is used to determine
       *            which are best.  It is recommended that no limit should
       *            be used if forward probabilities are not available, since
       *            quality may suffer substantially.<BR>
       * c.phraseTableThreshold  The probability threshold for phrases
       *            in the phrase table.  This should be a probability (not a
       *            log probability).<BR>
       * c.distLimit       The maximum distortion length allowed
       *            between two phrases.  This is used to detect whether
       *            partial translations can be completed without violating
       *            this limit.<BR>
       * c.levLimit The maximum levenshtein distance allowed for the current
       *            translation.<BR>
       * c.distModelName   name of distortion model to use - see
       *            DistortionModel::create() in distortion.cc for choices.<BR>
       * c.distModelArg    argument to pass to distortion model.<BR>
       * c.segModelName    name of segmentation model to use - see
       *            SegmentationModel::create() in segmentmodel.cc for
       *            choices.<BR>
       * c.segModelArg     argument to pass to segmentation model.<BR>
       * c.verbosity       The verbosity level [1].<BR>
       */
      BasicModelGenerator(const CanoeConfig &c);

      /**
       * constructor to use when the source text is already known.
       *
       * Creates a new BasicModelGenerator.  This version optionally allows
       * the phrases loaded to be limited to phrases in the sentences given,
       * and the language model words loaded to be limited.
       *
       * Note that all the parameters here, except verbosity, may be changed
       * before creating a model (since they are stored as public instance
       * variables).
       *
       * @param c             Global canoe configuration object.  See the
       *                      documention of the other constructor for this
       *                      class for details of relevant member variables.
       * @param src_sents     The source sentences, used to determine the
       *                      phrases to limit to (if c.loadFirst is
       *                      false).
       * @param marks         All the marked translations.
       * c.loadFirst          If false, limit the phrases loaded to ones
       *                      prespecified using src_sents.  The language
       *                      model is then also limited to words found in
       *                      the translation options for these sentences;
       *                      thus, if this option is used, it is imperative
       *                      that the translation model be loaded BEFORE the
       *                      language model.
       */
      BasicModelGenerator(const CanoeConfig &c,
            const vector< vector<string> > &src_sents,
            const vector< vector<MarkedTranslation> > &marks);

      /**
       * Destructor.
       */
      virtual ~BasicModelGenerator();

   private:
      /**
       * Add a language model.  Adds a language model, loaded from the given
       * file.  Note that the weight may be subsequently changed using the
       * public instance variable lmWeights.
       * @param file          The language model file.
       * @param weight        The weight for this model.
       * @param limit_order   If non zero, limits the LM to this order
       * @param os_filtered   An opened stream to output the filtered LM.
       */
      virtual void addLanguageModel(const char *file, double weight,
            Uint limit_order = 0, ostream *const os_filtered = 0);

      /**
       * Extracts the vocabulary from the TPPTs.
       */
      void extractVocabFromTPPTs();

   public:
      /**
       * @brief describe the model in human readable format.
       *
       * Display in human readable format all the components of the model,
       * which will show all the features in the same order they'll occur in
       * any ff value output vector.
       * @return model description
       */
      virtual string describeModel() const;

      /**
       * @brief Create a model to translate one sentence.
       *
       * Creates a model with the current parameters.  Note: if the
       * parameters (below) are changed after this is called, they do not
       * change for the created model.
       * @param new_src_sent_info  Contains all source/target sentence info.
       *                           See newSrcSentInfo.
       * @param alwaysTryDefault   If true, the default translation (source
       *                  word translates as itself) will be included in the
       *                  phrase options for all phrases; if false, the
       *                  default translation is included only when there is
       *                  no other option.
       */
      virtual BasicModel *createModel(newSrcSentInfo& new_src_sent_info,
            bool alwaysTryDefault = false);

      /**
       * Gets a reference to the phrase table object of this model.
       * @return the phrase table.
       */
      PhraseTable& getPhraseTable() { return *phraseTable; }

      /// The value to add to each marked translation's log probability.
      /// This is log(c->weightMarked).
      double addWeightMarked;

      /// Get the number of language models
      Uint getNumLMs() const { return lmWeightsV.size(); }
      /// Get the number of translation models
      Uint getNumTMs() const { return transWeightsV.size(); }
      /// Get the number of forward translation models
      Uint getNumFTMs() const { return forwardWeightsV.size(); }
      /// Get the number of forward translation models
      Uint getNumATMs() const { return adirTransWeightsV.size(); } //boxing
      /// Get the number of other decoder features
      Uint getNumFFs() const { return featureWeightsV.size(); }

      /**
       * Get the target word corresponding to an index.
       * @param target_index  index of a word to find in the vocabulary
       * @param word          returned word for target_index
       * @return Returns word
       */
      string& getTargetWord(Uint target_index, string& word) {
         const char* res = tgt_vocab.word(target_index);
         word = (res == 0 ? "" : res);
         return word;
      }

      /**
       * Initializes all models weights randomly, with seed.
       * @param seed Seed for the random number generator, for reproducible
       * results.
       */
      void setRandomWeights(unsigned int seed);
      /// Initialize all models weights randomly
      void setRandomWeights();

      /// Set model weights from a string specification in the format defined
      /// by CanoeConfig::setFeatureWeightsFromString(). The string need not
      /// contain a weight for every feature - weights that are not specified
      /// in the string will retain their current values.
      void setWeightsFromString(const string& s);

      /// Get a read-only reference to the vocab, which is a source-language
      /// vocab initially, until the phrase tables are loaded at which point
      /// it contains the union of the source and target language
      /// vocabularies.
      const Voc& get_voc() const { return tgt_vocab; }

      // So much for read-only.
      Voc* getOpenVoc() {return &tgt_vocab;}

      /// Get a reference to the bi-phrase vocabulary
      VocabFilter& getBiPhraseVoc() { return biPhraseVocab; }

      /// Prints how many times each N where hit with the lm queries.
      /// @param out  where to display the hits
      void displayLMHits(ostream& out = cerr);

   }; // BasicModelGenerator

   /// Decoder model for one sentence.
   class BasicModel: public PhraseDecoderModel, private NonCopyable
   {
   public:
      /// Glocal configuration object
      const CanoeConfig* const c;

      /// Optional target sentence.
      const vector<string>* const tgt_sent;

   private:
      /// The source sentence (in the order it occurs in the input).
      const vector<string> &src_sent;

      /// Language model weights
      const vector<double> lmWeights;
      /// Translation model weights
      const vector<double> transWeights;
      /// Forward translation model weights
      const vector<double> forwardWeights;
      /// Adirectional translation model weights
      const vector<double> adirTransWeights; //boxing
      /// Other feature weights
      const vector<double> featureWeights;

      /**
       * @brief A triangular array containing all the phrases.
       *
       * The (i, j)-th entry contains the phrase translation options for the
       * range [i, i + j + 1), 0 <= i < src_sent.size(), 0 <= j < (size-i).
       */
      vector<PhraseInfo *> **phrases;

      /**
       * @brief A triangular array containing precomputed future scores.
       *
       * The (i, j)-th entry contains the estimated future score for
       * translating the range [i, i + j + 1).  The future score for a
       * partial translation is equal to it's score plus the future score on
       * each maximal contiguous range of untranslated source words (we use
       * addition because it is a log-probability), plus the maximum future
       * distortion.
       */
      double **futureScore;

      /// Model generator which was used to create this model instance
      BasicModelGenerator &parent;

   protected:
      /**
       * @brief Constructor, creates a BasicModel.
       *
       * @param src_sent  The source sentence.
       * @param tgt_sent  Optional reference sentence in the target language.
       * @param phrases   The triangular array of phrase translation options;
       *                  the (i, j)-th entry contains phrase translation
       *                  options for the source range [i, i + j + 1).
       * @param futureScore  The triangular array of future scores.  The (i,
       *                  j)-th entry contains the estimated future score for
       *                  translating the range [i, i + j + 1).
       * @param parent    Parent model generator, with all decoder features
       *                  assumed to be initialized for current sentence.
       *                  All model weights will be initialized from parent.
       */
      BasicModel(const vector<string> &src_sent,
            const vector<string> *tgt_sent,
            vector<PhraseInfo *> **phrases,
            double **futureScore,
            BasicModelGenerator &parent);

      friend class BasicModelGenerator;

   public:
      /// Destructor
      virtual ~BasicModel();

      virtual string getStringPhrase(const Phrase &uPhrase) {
         return parent.getStringPhrase(uPhrase);
      }

      virtual Uint getUintWord(const string &word) {
         return parent.getUintWord(word);
      }

      /**
       * Gets a reference to the phrase table object of this model.
       * @return the phrase table.
       */
      PhraseTable& getPhraseTable() { return *(parent.phraseTable); }

      /// Get a reference to the bi-phrase vocabulary
      VocabFilter& getBiPhraseVoc() { return parent.biPhraseVocab; }

      /**
       * Get the length of the source sentence being translated.
       * @return          The number of words in the source sentence.
       */
      virtual Uint getSourceLength() { return src_sent.size(); }

      /**
       * @brief Get all the phrase options.
       * @return All phrase options, organized into a triangular array by the
       * source words covered.  The (i, j)-th entry of the array returned
       * will contain all phrase translation options for the source range
       * [i, i + j + 1).
       */
      virtual vector<PhraseInfo *> **getPhraseInfo() { return phrases; }

      /**
       * Main scoring function.
       *
       * Calculates the incremental score, log P(trans | *trans.back) =
       * log P(trans) - log P(*trans.back).
       * @param trans  The translation to score
       * @param verbosity  Verbose level
       * @return       The incremental log score
       */
      virtual double scoreTranslation(const PartialTranslation &trans,
            Uint verbosity = 1);

      /**
       * Estimate future score.
       * Returns the difference between the future score for the given
       * translation and the score for the translation, i.e., the estimated
       * cost of translating the rest of the sentence.
       * @param trans     The translation to score
       * @return          The remaining log future-probability.
       */
      virtual double computeFutureScore(const PartialTranslation &trans);

      /**
       * Partial scoring function based on range, for cube pruning.
       *
       * Estimates the incremental score, in so far as can be calculated if one
       * only takes into account the source range of the last phrase in trans.
       * Does not consider TMs and LMs - only considers features that override
       * DecoderFeature::partialScore(), e.g., distortion.
       *
       * The sum rangePartialScore(trans) + phrasePartialScore(trans.src_words)
       * is used as a heuristic for scoreTranslation(trans).
       * @param trans           The previous partial translation
       * @return  the part of the incremental score than can be estimated
       */
      virtual double rangePartialScore(const PartialTranslation& trans);

      /**
       * Partial scoring function based on the phrase, for cube pruning.
       * Only considers the TM and other features that implement
       * precomputeFutureScore().
       * @param phrase  The phrase to score
       * @return the future score for phrase_info, taking into account all
       *         models except the LMs.
       */
      virtual double phrasePartialScore(const PhraseInfo* phrase);

      /** -- Functions to manage recombining hypotheses. -- */

      /**
       * @brief Compute a hash value for this partial translation.
       *
       * Two translations that may be recombined (ie., that can be treated
       * the same for scoring in the future) should have the same hash value.
       * @param trans     The translation to compute the hash value for.
       * @return          The hash value.
       */
      virtual Uint computeRecombHash(const PartialTranslation &trans);

      /**
       * @brief Determine whether two partial translations may be recombined.
       * @param trans1    The first translation to compare.
       * @param trans2    The second translation to compare.
       * @return          true iff trans1 and trans2 can be recombined.
       */
      virtual bool isRecombinable(const PartialTranslation &trans1,
            const PartialTranslation &trans2);

      /** -- Bonus functions -- */
      /**
       * @brief Get the feature function values, for the last phrase in the
       * translation
       *
       * Returns the marginal feature function values
       * @param vals      vector to receive the feature values
       * @param trans     translation to score
       */
      virtual void getFeatureFunctionVals(vector<double> &vals,
            const PartialTranslation &trans);

      /**
       * @brief Get the feature weights in the conventional order
       *
       * Returns the feature weights
       * @param wts      vector to receive the feature values
       */
      virtual void getFeatureWeights(vector<double> &wts);

      /**
       * @brief Get the total feature function values
       *
       * Returns the summed (not marginal) feature function values for the
       * whole translation hypothesis.
       * @param vals      vector to receive the feature values
       * @param trans     translation to score
       */
      virtual void getTotalFeatureFunctionVals(vector<double> &vals,
            const PartialTranslation &trans);

   }; // BasicModel

} // Portage

#endif // BASICMODEL_H
