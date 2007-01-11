/**
 * @author George Foster, Aaron Tikuisis
 * @file phrasedecoder_model.h  This file contains the declaration of
 * PhraseDecoderModel, which is an abstraction of the model (translation model
 * + language model) to be used in decoding.  It also contains the declaration
 * of some general structures used: PhraseInfo and PartialTranslation.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 */

#ifndef PHRASEDECODER_MODEL_H
#define PHRASEDECODER_MODEL_H

#include "canoe_general.h"
#include <vector>

using namespace std;

namespace Portage
{

    /**
     * Structure to represent a phrase translation option.  We may need to
     * rethink what probabilities are stored here when we implement the actual
     * scoring functions.
     */
    struct PhraseInfo
    {
        /// Destructor
        virtual ~PhraseInfo() {}

        /// The source words that this is a translation of
        Range src_words;

        /// The phrase.  The front of the vector should contain the / leftmost
        /// word of the phrase.
        Phrase phrase;

        /// The translation model probability of this phrase.
        double phrase_trans_prob;
    }; // PhraseInfo

    /**
     * Structure to represent a partial translation.
     * Note: This is implemented in partialtranslation.cc.
     */
    class PartialTranslation
    {
    public:
        /**
         * The previous partial translation.
         */
        PartialTranslation *back;

        /**
         * The right-most phrase in the translation.
         */
        PhraseInfo *lastPhrase;

        /**
         * A set of ranges representing all the source words not covered.  This
         * should be a minimal set, ie. no overlap and no [a, b) [b, c).
         * Moreover, the ranges must be in order from least to greatest (since
         * there is no overlap, this is a well-defined ordering).
         */
        UintSet sourceWordsNotCovered;

        /**
         * The number of source words that have been covered.
         */
        Uint numSourceWordsCovered;

        /**
         * Constructor, creates a new partial translation object.
         */
        PartialTranslation();

        /**
         * Get the last num words of partial translation hypothesis.
         * Puts the last num words of the target partial-sentence into the
         * vector words (which should be empty).  The front of the vector will
         * contain the leftmost word.  If the partial-sentence contains fewer
         * than num words, all the words will be entered into the vector.
         * @param[out] words  The vector to be filled with the last num target
         *                    words in the translation.
         * @param[in]  num    The number of words to get.
         */
        void getLastWords(Phrase &words, Uint num) const;

        /**
         * Compares the num last words of this with that.
         * @param[in] that     The other partial translation for the comparison
         * @param[in] num        The number of words to compare
         * @param[in] verbosity  Debug output level
         * @return             true iff the num last words of this and that are
         *                     the same
         */
        bool sameLastWords(const PartialTranslation &that, Uint num,
            Uint verbosity = 0) const;

        /**
         * Get the whole partial translation hypothesis.
         * Puts all the words of the target partial-sentence into the vector
         * words (which should be empty).  The front of the vector will contain
         * the first word in the partial-sentence.
         * @param[out] words  The vector to be filled with the words in the
         *                    translation.
         */
        void getEntirePhrase(Phrase &words) const;

        /**
         * Determines the target length of this partial translation.
         * @return  The length of the target partial-sentence.
         */
        Uint getLength() const;

    private:
        /**
         * Does the real work for getLastWords.
         * See getLastWords(Phrase &words, Uint num) const
         * @param[out] words  The vector to be filled with the last num target
         *                    words in the translation.
         * @param[in] num     The number of words to get.
         */
        void _getLastWords(Phrase &words, Uint num) const;

    }; // PartialTranslation

    /**
     * Abstract base class for phrase decoder models.
     * Abstract class to encapsulate model information passed to a basic
     * phrase-based decoder a la Koehn. Currently, each model is specific to
     * the current source sentence.
     */
    class PhraseDecoderModel
    {
    public:
        /// Destructor
        virtual ~PhraseDecoderModel() {}

        /**
         * Converts a phrase consisting of Uint "word"'s into a printable
         * phrase string.
         * @param[out] s         The string to put the phrase into.
         * @param[in]  uPhrase   The phrase using Uint's to represent words.
         */
        virtual void getStringPhrase(string &s, const Phrase &uPhrase) = 0;

        /**
         * Get the Uint representation of word.
         * @param[in] word  The string representation of the word
         * @return          The Uint representation of word, or OutOfVocab if
         *                  word is not in the current vocabulary.
         */
        virtual Uint getUintWord(const string &word) = 0;
        /// Uint representation for out-of-vocabulary words.
        Uint static const OutOfVocab = (Uint)-1;

        /**
         * Get the length of the source sentence being translated.
         * @return          The number of words in the source sentence.
         */
        virtual Uint getSourceLength() = 0;

        /**
         * Get all the phrase options.
         * @return The phrase options, organized into a triangular array by the
         * source words covered.  The (i, j)-th entry of the array returned
         * will contain all phrase translation options for the source range
         * [i, i + j + 1).
         */
        virtual vector<PhraseInfo *> **getPhraseInfo() = 0;

        /**
         * Inform the model of the current PartialTranslation context.
         * Between calls to this function, the "<trans>" arguments to
         * scoreTranslation() and computeFutureScore() are guaranteed to differ
         * only in their lastPhrase members, ie in their last source/target
         * phrase pairs. The purpose is to give the model a chance to factor
         * out computations that depend only on this pair (added for the new
         * tree-based distortion models).
         * @param[in] trans  the current context
         */
        virtual void setContext(const PartialTranslation& trans) {}

        /**
         * Main scoring function.
         * Should return the incremental score, log P(trans | *trans.back) =
         * log P(trans) - log P(*trans.back).
         * @param[in] trans     The translation to score
         * @param[in] verbosity Verbose level
         * @return          The incremental log probability.
         */
        virtual double scoreTranslation(const PartialTranslation &trans, Uint
                verbosity = 1) = 0;

        /**
         * Estimate future score.
         * Should return the difference between the future score for the given
         * translation and the score for the translation.
         * @param[in] trans  The translation to score
         * @return           The incremental log future-probability.
         */
        virtual double computeFutureScore(const PartialTranslation &trans) = 0;

        /** -- Functions to manage recombining hypotheses. -- */

        /**
         * Compute a hash value for this partial translation.
         * Two translations that may be recombined (ie., that can be treated
         * the same for scoring in the future) should have the same hash value.
         * @param[in] trans  The translation to compute the hash value for.
         * @return           The hash value.
         */
        virtual Uint computeRecombHash(const PartialTranslation &trans) = 0;

        /**
         * Determine whether two partial translations may be recombined.
         * @param[in] trans1  The first translation to compare.
         * @param[in] trans2  The second translation to compare.
         * @return            true iff trans1 and trans2 can be recombined.
         */
        virtual bool isRecombinable(const PartialTranslation &trans1, const
                PartialTranslation &trans2) = 0;

        /**
         * Get the feature function values, for the last phrase in the
         * translation.  Returns the marginal feature function values.
         * @param[out] vals      vector to receive the feature values
         * @param[in]  trans     translation to score
         */
        virtual void getFeatureFunctionVals(vector<double> &vals,
                const PartialTranslation &trans) = 0;

    }; // PhraseDecoderModel

} // Portage

#endif
