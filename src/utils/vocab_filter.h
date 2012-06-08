/**
 * @author Samuel Larkin
 * @file vocab_filter.h   Defines Word <-> index mapping, with filtering
 *                        operations for LMs.
 *
 * COMMENTS:
 *
 * This is pretty specific to LMs, so maybe it should be part of the LM module.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __VOCAB_FILTER_H__
#define __VOCAB_FILTER_H__

#include "voc.h"
#include "multi_voc.h"
#include <vector>
#include <string>

namespace Portage {

 /**
  * This class regroups all that is related to filtering and the vocab.
  */
 class VocabFilter : public Voc {
 private:

    /// Keeps track of how many entries are discarded in the LMs and by which technique.
    struct discardingCounter {
       vector<Uint> oov;              ///< number of discarded entries by oov per ngrams
       vector<Uint> perSentenceVocab; ///< number of discarded entries by perSentenceVocab per ngrams
       /// Constructor.
       discardingCounter()
       {}
       /// Prepares the counters for a specific N in ngram.
       /// @param  ngram  maximum value of N in ngram
       void setMaxNgram(Uint ngram) {
	  if (ngram>oov.size())              oov.resize(ngram, 0);
	  if (ngram>perSentenceVocab.size()) perSentenceVocab.resize(ngram, 0);
       }
       /// Resets the counters.
       void reset() {
	  for (Uint i(0); i<oov.size(); ++i)
	     oov[i] = perSentenceVocab[i] = 0;
       }
       /// Displays to stderr the gathered stats on entries filtering.
       void print() const {
	  Uint coov(0);
	  Uint cperSentenceVocab(0);
	  fprintf(stderr, "| ngram |       oov | perSentenceVocab |\n");
	  for (Uint i(0); i<oov.size(); ++i) {
	     fprintf(stderr, "|     %d | %9d |        %9d |\n", i+1, 
		     oov[i], perSentenceVocab[i]);
	     coov += oov[i];
	     cperSentenceVocab += perSentenceVocab[i];
	  }
	  fprintf(stderr, "| TOTAL | %9d |        %9d |\n", 
		  coov, cperSentenceVocab);
       }
    } discarding;

 public:

    /// To keep the per sentence components small, we limit them to mod
    /// maxSourceSentence4filtering
    static Uint maxSourceSentence4filtering;

    MultiVoc*  per_sentence_vocab;   ///< vocabulary for each source sentences

 private:

    /// Deactivated default constructor.
    VocabFilter();

 public:

   /// Functor to split string to a vector of uint using a vocab and adding the
   /// per_sentence_vocab.
   struct addConverter
   {
      VocabFilter& voc;             ///< Vocabulary used
      Uint         index;           ///< source sentence index
      const bool   per_sent_limit;  ///< Do we need the per sentence vocab
      /// Default constructor.
      /// @param voc vocabulary to use
      /// @param per_sent_limit  should we limit per sentence?
      addConverter(VocabFilter& voc, bool per_sent_limit)
      : voc(voc), index(0), per_sent_limit(per_sent_limit)
      {
         if (per_sent_limit)
            assert(voc.per_sentence_vocab != NULL);
      }

      /**
       * Make the object a functor to map a string its uint representation
       * @param s   source word
       * @param val uint representation of s
       * @return  Always return true
       */
      bool operator()(const char* s, Uint& val) {
         val = voc.add(s);
         if (per_sent_limit) {
            voc.per_sentence_vocab->add(val,
               index % VocabFilter::maxSourceSentence4filtering);
         }
         return true;
      }

      /// Tells the converter we are processing a new source sentence.
      void next() { ++index; }
   };

    /**
     * Constructor.
     * @param number_source_sents approx number of source sentences on which
     *        per_sentence_vocab is to operate, or 0 for no per-sentence
     *        filtering.
     */
    VocabFilter(Uint number_source_sents);

    /**
     * Construct from an existing object, converting the vocabulary by applying
     * a given many-to-one mapping over strings.  The intent is that this be
     * used after the add*() operations and before keepLMentry() calls.
     * @param vf object to be remapped
     * @param vmap functor to map vf strings, eg:
     *        struct VMap {const string& operator()(const string& s){...}};
     * @param index_map for each index i in vf, index_map[i] will be set to the
     *        corresponding index in the new VocabFilter.
     */
    template<typename VMap>
    VocabFilter(const VocabFilter& vf, VMap& vmap, vector<Uint>& index_map);

    /// Destructor.
    ~VocabFilter();

    /**
     * When you know that you don't need the per sentence infos after
     * filtering you can free up some memory.
     */
    void freePerSentenceData();

    /**
     * Adds a words belonging to a sentence sent_no.
     * @param word  the word to be added
     * @param sent_no  the source sentence number to which word belongs
     */
    void addWord(const string& word, Uint sent_no = 0);

    /**
     * Adds a sentences with sent_no.
     * @param sentence  sentence to add all of its words
     * @param sent_no  sentence number
     */
    void addSentence(const vector<string> sentence, Uint sent_no = 0);

    /**
     * Adds a group of sentences, assumed to start at number 0.
     * @param sentences  sentences to add
     */
    void addSentences(const vector<vector<string> > sentences);

    /**
     * When adding SentStart or SentEnd or UNK_Symbol.  This symbol should
     * belong to all sentences by default.
     * @param symbol  symbol to add
     */
    void addSpecialSymbol(const char* symbol);

    /**
     * Returns the number of vocabs aka number of source sentences tracked by
     * the Multi_Voc
     * @return  Returns the number of source sentences tracked by Multi_Voc
     */
    Uint getNumSourceSents() const;

    /**
     * Determines if an LM entry should be kept or filtered.
     * @param phrase  phrase to evaluate
     * @param tok_count  size of phrase aka order
     * @param order
     * @return  true if phrase should be kept or false otherwise
     */
    bool keepLMentry(const Uint phrase[/*order*/], Uint tok_count, Uint order);

    /**
     * When using the discading counters, sets the maximum N in ngram
     * @param ngram  maximum value of N in ngram
     */
    void setMaxNgram(Uint ngram) { discarding.setMaxNgram(ngram); }
    /**
     * Reset the counters when processing a new LM
     */
    void resetDiscardingCount() { discarding.reset(); }
    /**
     * Displays the cumulates stats to stderr.
     */
    void printDiscardingCount() const { discarding.print(); }

    /**
     * Takes care of adding vocab to the per_sentence_vocab.
     * This is used for filtring lm_phrases based on the vocabulary generated
     * per source sentences.
     * @param tgtPhrase       target phrase to add
     * @param input_sent_set  source sentence to which tgtPhrase belongs to
     */
    void addPerSentenceVocab(vector<Uint>& tgtPhrase,
                             const boost::dynamic_bitset<>* const input_sent_set = NULL);

 }; //ends class VocabFilter


template<class VMap>
VocabFilter::VocabFilter(const VocabFilter& vf, VMap& vmap, vector<Uint>& index_map) :
   per_sentence_vocab(NULL)
{
   if (vf.per_sentence_vocab)
      per_sentence_vocab = new MultiVoc(*this, vf.per_sentence_vocab->get_num_vocs());

   index_map.resize(vf.size());
   for (Uint i = 0; i < vf.size(); ++i) {  // for each word in original voc
      string s = vf.word(i);
      index_map[i] = add(vmap(s).c_str());
      if (per_sentence_vocab)
         per_sentence_vocab->add(index_map[i], vf.per_sentence_vocab->get_vocs(i));
   }
}

} // ends namespace Portage

#endif // __VOCAB_FILTER_H__
