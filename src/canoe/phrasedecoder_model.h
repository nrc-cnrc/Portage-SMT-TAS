/**
 * @author George Foster, Aaron Tikuisis
 * @file phrasedecoder_model.h  This file contains the declaration of
 * PhraseDecoderModel, which is an abstraction of the model (translation model
 * + language model) to be used in decoding.  It also contains the declaration
 * of some general structures used: PhraseInfo and PartialTranslation.
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef PHRASEDECODER_MODEL_H
#define PHRASEDECODER_MODEL_H

#include "canoe_general.h"
#include "shift_reducer.h"
#include "annotation_list.h"
#include <vector>
#include <cmath> // NAN
#include <boost/dynamic_bitset.hpp>
#include "toJSON.h"

using namespace std;

namespace Portage
{

   /**
    * Utility class to pack 8 4-bit Uints into 4 bytes, with array-like access.
    */
   class ArrayUint4 {
      Uint _storage;
    public:
      static const Uint MAX = 15; ///< Greatest value that can be stored: 0b1111
      static const Uint MAXI = 7; ///< Greatest index i that can be used
      /// Default and one-parameter construction sets all 8 values to init [0]
      /// (You can use -1 for MAX, which can be used to mean uninitialized)
      ArrayUint4(Uint init = 0) : _storage(init) {
         if (init != Uint(-1) && init != 0)
            for (Uint i = 0; i <= MAXI; ++i) set(i, init);
      }

      Uint get(Uint i) const { return (_storage >> (4*i)) & MAX; }
      void set(Uint i, Uint v) {
         assert(i <= MAXI);
         _storage &= ~(MAX << (4*i));
         _storage |= (v&MAX) << (4*i);
      }
   };

   /**
    * Structure to represent a phrase translation option.
    */
   struct PhraseInfo
   {
      /// Default constructor makes an empty phrase info
      PhraseInfo()
         : src_words(0,0)
         , phrase_trans_prob(0)
         , forward_trans_prob(0)
         , adir_prob(0)
         , partial_score(NAN)
      {}

      /// This class has some subclasses, so let's be cosher and give it a
      /// virtual destructor. Necessary to make g++ 4.7.4 or more recent happy,
      /// too...
      virtual ~PhraseInfo() {}

      /// The source words that this is a translation of
      Range src_words;

      /// The target phrase.
      /// The front of the vector should contain the leftmost
      /// word of the phrase.
      Phrase phrase;

      /// The backward translation model probability of this phrase.
      double         phrase_trans_prob;
      /// Backward probabilities for this phrase
      vector<float>  phrase_trans_probs;

      /// forward probability score
      double         forward_trans_prob;
      /// forward probabilities for this phrase
      vector<float>  forward_trans_probs;

      /// adirectional probability score
      double         adir_prob; //boxing
      /// adirectional probabilities for this phrase
      vector<float>  adir_probs; //boxing

      /// lexicalized distortion probabilities for this phrase
      vector<float>  lexdis_probs; //boxing

      /// cache for phrasePartialScore(): Partial score combining the heuristic
      /// precomputeFutureScore() from all features.
      double partial_score;

      /// Annotation trail left by various features, to find their way later, stored
      /// for each phrase pair in an annotation list
      AnnotationList annotations;

      ostream& toJSON(ostream& out, Voc const * const voc = NULL) const {
         out << '{';
         out << to_JSON("src_words", src_words);
         if (voc != NULL) {
            out << ',';
            out << keyJSON("phrase");
            // Since Phrase has no method .toJSON, we must print it this way.
            to_JSON(out, phrase, *voc);
         }
         else {
            //  This outputs the phrase as a vector of ints.
            out << ',';
            out << to_JSON("phrase", (vector<Uint>)phrase);
         }
         out << ',';
         out << to_JSON("phrase_trans_prob", phrase_trans_prob);
         out << ',';
         out << to_JSON("phrase_trans_probs", phrase_trans_probs);
         out << ',';
         out << to_JSON("forward_trans_prob", forward_trans_prob);
         out << ',';
         out << to_JSON("forward_trans_probs", forward_trans_probs);
         out << ',';
         out << to_JSON("adir_prob", adir_prob);
         out << ',';
         out << to_JSON("adir_probs", adir_probs);
         out << ',';
         out << to_JSON("lexdis_probs", lexdis_probs);
         out << ',';
         out << to_JSON("partial_score", partial_score);
	 /*
         out << ',';
         out << to_JSON("annotations", annotations);
	 */
         out << '}';
         return out;
      }

      virtual void display() const {
         cerr << src_words << " " << phrase_trans_prob;
         cerr << " " << forward_trans_prob << " " << adir_prob;
      }

      virtual void printCPT(ostream& out, const vector<string>& src_sent) const {
         const Voc& voc = *GlobalVoc::get();
         out << join(src_sent.begin()+src_words.start, src_sent.begin()+src_words.end, " ");
         out << " ||| " << phrase2string(phrase, voc);
         //out << " ||| " << join(phrase_trans_probs);
         out << " |||";
         for (Uint i(0); i<phrase_trans_probs.size(); ++i)
            out << " " <<  exp(phrase_trans_probs[i]);
         //out << " ||| " << join(forward_trans_probs);
         for (Uint i(0); i<forward_trans_probs.size(); ++i)
            out << " " <<  exp(forward_trans_probs[i]);
         out << endl;
      }
   }; // PhraseInfo

   /**
    * Structure to represent a partial translation.
    * Note: This is implemented in partialtranslation.cc.
    */
   class PartialTranslation : private NonCopyable
   {
      static const PhraseInfo EmptyPhraseInfo;

   public:
      ///  Placeholder for levenshtein
      struct levenshteinInfo
      {
         /// The ngramDist so far. Used to optimize calculations 
         vector <double> ngramDistance;

         /// The levDist so far. Used to optimize calculations 
         int levDistance;

         /// Positions with minimum Levenshtein distance in ref so far. Used to
         /// optimize calculations 
         boost::dynamic_bitset<> min_positions;

         /// Default constructor
         levenshteinInfo() : levDistance(-1) {}
      };

      /// The previous partial translation.
      const PartialTranslation* const back;

      /// The right-most phrase in the translation.
      const PhraseInfo* const lastPhrase;

      /// get the phrase covered by the current part of this PartialTranslation
      // EJJ:  This method may seem pointless, but it helps with consistency of
      // code between the gappy-branch and the trunk.
      const Phrase& getPhrase() const {
         if (lastPhrase) return lastPhrase->phrase;
         else return EmptyPhraseInfo.phrase;
      }

      /**
       * A set of ranges representing all the source words not covered.  This
       * should be a minimal set, ie. no overlap and no [a, b) [b, c).
       * Moreover, the ranges must be in order from least to greatest (since
       * there is no overlap, this is a well-defined ordering).
       */
      UintSet sourceWordsNotCovered;

      /// The number of source words that have been covered.
      Uint numSourceWordsCovered;

      /**
       * The right context sizes, following Li & Khudanpur, 2008: A Scalable
       * Decoder for Parsing-based Machine Translation with Equivalent Language
       * Model State Maintenance.
       * recombination and LM queries on subsequent partial translations only
       * need to consider the last lm context size of this PT.
       *
       * contextSizes[0] is the max required size over all LMs (coarse,
       * regular, mix, whatever) (i.e. max_LM(min context size(LM))
       * contextSizes[1..7] are the required context sizes for the BiLMs
       */
      mutable ArrayUint4 contextSizes;
      Uint getLmContextSize() const { return contextSizes.get(0); }
      void setLmContextSize(Uint size) const;
      bool isLmContextSizeSet() const { return contextSizes.get(0) != ArrayUint4::MAX; }

      /**
       * The right context size for BiLMs
       */
      Uint getBiLMContextSize(Uint biLM_ID) const { return contextSizes.get(biLM_ID); }
      void setBiLMContextSize(Uint biLM_ID, Uint size) const;

      /// Set all LM and BiLM context sizes to the same value given
      void setAllContextSizes(Uint size) const;

      // EJJ Packing note: numSourceWordsCovered and contextSizes are placed
      // together to pack well, filling exactly a Word slot between pointers
      // (assuming a 64 bit word, which we now always use).

      /// Info to calculate the levenshtein distance.
      levenshteinInfo* levInfo;

      /**
       * On-board shift-reduce parser tracks the largest-possible
       * previous phrase
       */
      ShiftReducer* shiftReduce;

      /**
       * Constructor, creates a new partial translation object, intended for
       * creating the initial empty PartialTranslation.
       * @param sourceLen Length of source sentence
       * @param usingLev  Specifies if the levenshteinInfo is required
       * @param usingSR   Specifies if state includes a shift-reduce parser
       */
      explicit PartialTranslation(Uint sourceLen, bool usingLev = false, bool usingSR = false);

      /**
       * Constructor, creates a new partial translation object, intended for
       * creating artificial debugging states.
       * Will create an inconsistent state by default (coverage vector and
       * numCovered will not match the provided phrase)
       * @param phrase to be initially covered
       */
      explicit PartialTranslation(PhraseInfo* phrase);

      /**
       * Constructor extends trans0 by adding phrase at the end of it.
       * Used mainly by extendDecoderState, but can be called on its own.
       * @param trans0 Partial translation to extend (cannot be NULL)
       * @param phrase Phrase to add to trans (cannot be NULL)
       * @param preCalcSourceWordsCovered (optional) if specified, indicates
       *               that the coverage set of the resulting translation has
       *               already been calculated, and provides it.
       */
      explicit PartialTranslation(const PartialTranslation* trans0, const PhraseInfo* phrase,
            const UintSet* preCalcSourceWordsCovered = NULL);

      /// Destructor.
      ~PartialTranslation();

      /**
       * Get the last num words of partial translation hypothesis.
       * Puts the last num words of the target partial-sentence into the
       * vector words (which should be empty).  The front of the vector will
       * contain the leftmost word.  If the partial-sentence contains fewer
       * than num words, all the words will be entered into the vector.
       * @param[out] words  The vector to be filled with the last num target
       *                    words in the translation.
       * @param[in]  num    The number of words to get.
       * @param[in]  backward  If true, fill the vector backwards, with the
       *                    leftmost word at the back of the vector.
       */
      void getLastWords(VectorPhrase &words, Uint num,
            bool backward = false) const;

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
      void getEntirePhrase(VectorPhrase &words) const;

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
      void _getLastWords(VectorPhrase &words, Uint num) const;

      /**
       * Does the real work for getLastWords backward.
       * A separate implementation is provided because this method is called
       * millions of time, so it is work optimizing it, as benchmarking has
       * shown.
       * @param[out] words  The vector to be filled with the last num target
       *                    words in the translation, in reverse order.
       * @param[in] num     The number of words to get.
       */
      void _getLastWordsBackward(VectorPhrase &words, Uint num) const;

   }; // PartialTranslation

   /**
    * Abstract base class for phrase decoder models.
    * Abstract class to encapsulate model information passed to a basic
    * phrase-based decoder a la Koehn. Currently, each model is specific to
    * the current source sentence.
    *
    * This seemingly pointless abstract base class exists to enable unit
    * testing using stubs for the real model.  See tests/legacy/testdecoder.cc
    * and testhypstack.cc.
    */
   class PhraseDecoderModel
   {
   public:
      /// Destructor
      virtual ~PhraseDecoderModel() {}

      /**
       * Converts a phrase consisting of Uint "word"s into a printable
       * phrase string.
       * @param[in]  uPhrase   The phrase using Uint's to represent words.
       */
      virtual string getStringPhrase(const Phrase &uPhrase) = 0;

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

      /**
       * Get the feature weights
       * @param[out] wts      vector to receive the feature weights
       */
      virtual void getFeatureWeights(vector<double> &wts) = 0;

   }; // PhraseDecoderModel

} // Portage

#endif
