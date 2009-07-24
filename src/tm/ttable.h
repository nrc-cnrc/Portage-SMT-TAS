/**
 * @author George Foster
 * @file ttable.h  A simple IBM1-style ttable, mapping src/tgt word pairs to
 *                 probabilities.
 *
 *
 * COMMENTS:
 *
 * A simple IBM1-style ttable, mapping src/tgt word pairs to probabilities. It
 * can be read from or written to file. This is the low-level guts of an IBM
 * one; see ibm.h for a higher-level view.
 *
 * NB: tgt means "conditioned variable" throughout! (not noisy channel)
 *
 * TODO: add read-from-stream constructor.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef TTABLE_H
#define TTABLE_H

#include "portage_defs.h"
#include "string_hash.h"
#include <algorithm>
#include <string>
#include <errors.h>
#include <quick_set.h>
#include <voc.h>

namespace Portage {

/// TTable
class TTable {

public:

   typedef pair<Uint,float> TIndexAndProb;  ///< target-word index & cond prob.
   typedef vector<TIndexAndProb> SrcDistn;  ///< Source distribution.

   /// Iterator over a TTable's source or target vocabulary
   typedef unordered_map<string,Uint>::const_iterator WordMapIter;

   /// Get a const iterator to the beginning of the source vocabulary
   WordMapIter beginSrcVoc() const { return sword_map.begin(); }
   /// Get a const iterator to the end of the source vocabulary
   WordMapIter endSrcVoc() const { return sword_map.end(); }

   /// Get a const iterator to the beginning of the target vocabulary
   WordMapIter beginTgtVoc() const { return tword_map.begin(); }
   /// Get a const iterator to the end of the target vocabulary
   WordMapIter endTgtVoc() const { return tword_map.end(); }

private:

   /// Token that separes fields in GIZA++ files
   static const string sep_str;
   Uint speed;                  ///< controls initial pass algorithm

   /// Definition of a source distribution iterator.
   typedef SrcDistn::const_iterator SrcDistnIter;

   /// Callable entity to sort on Probs in decreasing order.
   struct CompareProbs {
      /**
       * Compares x and y based on their probs (for decreasing order sort)
       * @param x,y TIndexAndProb objects to compare.
       * @return true iff x > y
       */
      bool operator()(const TIndexAndProb& x, const TIndexAndProb& y) const
      {return x.second > y.second;}
   };
   /// Callable entity to sort on indexes in increasing order.
   struct CompareTIndexes {
      /**
       * Compares x and y based on their indexes
       * @param x,y TIndexAndProb objects to compare.
       * @return true iff x < y
       */
      bool operator()(const TIndexAndProb& x, const TIndexAndProb& y) const
      {return x.first < y.first;}
   };
   /// Callabale entity that finds TIndexAndProb with an index lower then y.
   struct CmpTIndex {
      /**
       * Compares x and y based on their indexes
       * @param x  TIndexAndProb object to compare.
       * @param y  maximum index we are looking for.
       * @return true iff x < y
       */
      bool operator()(const TIndexAndProb& x, Uint y) const
      {return x.first < y;}
   };

   unordered_map<string,Uint> tword_map; ///< T-language vocab
   unordered_map<string,Uint> sword_map; ///< S-language vocab
   vector<string> twords; ///< T-language vocab (mapping index to word)
   vector<SrcDistn> src_distns;
   vector<vector<bool>*> src_distns_quick;
   QuickSet src_inds; ///< local variable to add(), kept persistent for speed
   QuickSet tgt_inds; ///< local variable to add(), kept persistent for speed

   SrcDistn empty_distn;  ///< Empty source distribution.

   void add(Uint src_index, Uint tgt_index);
   void read(const string& filename, const Voc* src_voc);

   void init();

public:

   /**
    * Constructor. GIZA++ format: src_word tgt_word p(tgt_word|src_word).
    * The file can also be in PORTAGE Bin TTable format as written by
    * write_bin().
    * @param filename  file containing the GIZA++ info or binary TTable.
    * @param src_voc   if not null, causes the ttable to be filtered while
    *                  loading, keeping only lines where the source word exists
    *                  in *src_voc.
    */
   TTable(const string& filename, const Voc* src_voc = NULL);

   /// Constructor that creates an empty ttable. Use add() to populate, then
   /// makeDistnsUniform().
   TTable();

   /// Destructor.
   ~TTable();

   /**
    * Remove all pairs with probability < thresh, and renormalize afterwards
    * return size.
    * @param thresh  Threshold value to prune.
    * @param null_thresh  Threshold value to use for pruning NULL
    * @param null_word    The null word string, typically "NULL"
    * @return the number of pruned entries.
    */
   Uint prune(double thresh, double null_thresh, const string& null_word);

   /**
    * Writes the ttable to a stream.
    * @param os  An opened stream to output the ttable.
    */
   void write(ostream& os) const;

   /**
    * Writes the TTable in binary format to a stream
    * @param os  An opened stream to output the binary TTable to.
    */
   void write_bin(ostream& os) const;

   /**
    * Reads the TTable in binary format from a stream
    * @param filename File to read the binary TTable from.
    * @param src_voc  if not null, causes the ttable to be filtered while
    *                 loading, keeping only entries where the source word
    *                 exists in *src_voc.
    */
   void read_bin(const string& filename, const Voc* src_voc = NULL);

   /**
    * if src_word/tgt_word not in table, add it & set prob to 0
    * @param src_word
    * @param tgt_word
    */
   void add(const string& src_word, const string& tgt_word);

   /**
    * Add cartesian produce of words in src_sent and tgt_sent to table.
    * @param src_sent
    * @param tgt_sent
    */
   void add(const vector<string>& src_sent, const vector<string>& tgt_sent);

   /// Makes the distributions uniform. Call after add()'s are done.
   void makeDistnsUniform();

   /// @name Get the number of source/target words.
   /// @return the number of source/target words.
   //@{
   Uint numSourceWords() const {return src_distns.size();}
   Uint numTargetWords() const {return twords.size();}
   //@}

   /**
    * Fetches the internal source vocabulary.
    * @param src_words  will be cleared and filled with the source vocabulary.
    */
   void getSourceVoc(vector<string>& src_words) const;

   /**
    * Converts an index to its target string representation.
    * @param index  the index to convert.
    * @return  Returns the string representing index in the target vocabulary.
    */
   const string& targetWord(Uint index) const {return twords[index];}

   /**
    * Convert a string to its index value in the target vocabulary.
    * @param tgt_word  string to convert into an index.
    * @return tgt_word index or  numTargetWords() if unknown.
    */
   Uint targetIndex(const string& tgt_word) {
      WordMapIter p = tword_map.find(tgt_word);
      return p == tword_map.end() ? numTargetWords() : p->second;
   }

   /**
    * Converts src_word to its index value in the source vocabulary.
    * @param src_word  String to convert to its index value in the source
    * vocabulary.
    * @return the index of src_word in the source vocabulary or
    *         numSourceWords() if unknown
    */
   Uint sourceIndex(const string& src_word) {
      WordMapIter p = sword_map.find(src_word);
      return p == sword_map.end() ? numSourceWords() : p->second;
   }

   /**
    * Get the source distribution from an index.
    * @param src_index  index to retrieve.
    * @return the source distribution associated with src_index or the
    * empty distribution if not found.
    */
   const SrcDistn& getSourceDistn(Uint src_index) const {
      return src_index == numSourceWords() ? empty_distn : src_distns[src_index];
   }

   /**
    * Get the source distribution from a word.
    * @param src_word  source string to retrieve.
    * @return the source distribution associated with src_index or the
    *         empty distribution if not found.
    */
   const SrcDistn& getSourceDistn(const string& src_word) const {
      WordMapIter p = sword_map.find(src_word);
      return p == sword_map.end() ? empty_distn : src_distns[p->second];
   }

   /**
    * Find offset of given target index within src_distn.
    * @param target_index  target index
    * @param src_distn     source distribution
    * @return the offset of given target index within src_distn, or
    *         return -1 if not there.
    */
   int targetOffset(Uint target_index, const SrcDistn& src_distn);

   /**
    * Get the source distribution in decreasing order of probabilities.
    * @param src_word  source word to find.
    * @param distn     will contain the source distribution in decreasing order
    *                  of probabilities.
    */
   void getSourceDistnByDecrProb(const string& src_word, SrcDistn& distn) const;

   /**
    * A sugary & inefficient version of
    * void getSourceDistnByDecrProb(const string& src_word, SrcDistn& distn)
    * @param src_word  source word to find
    * @param tgt_words
    * @param tgt_probs
    */
   void getSourceDistnByDecrProb(const string& src_word,
                                 vector<string>& tgt_words,
                                 vector<float>& tgt_probs) const;

   /**
    * Gets the best translation for a given source word.
    * @param src_word
    * @param[out] best_trans
    * @return the probability for the best translation for a given source word.
    *         or -1 if src_word has no translations.
    */
   double getBestTrans(const string& src_word, string& best_trans);

   /**
    * p(tgt_word|src_word).
    * @return p(tgt_word|src_word), or smooth if no such pair exists in the
    * table.
    */
   double getProb(const string& src_word, const string& tgt_word,
                  double smooth = -1) const;

   /**
    * Get the probability of src_index and target_offset been aligned.
    * @param src_index      index of source word we are looking for.
    * @param target_offset  index of target word we are looking for.
    * @return  Returns the probability of src_index and target_offset been aligned.
    */
   float& prob(Uint src_index, Uint target_offset) {
      return src_distns[src_index][target_offset].second;
   }

   /**
    * Set speed used for initial add() operations; valid values are 1 (slower,
    * less mem) or 2 (faster, more mem)
    * @param sp  speed value.
    */
   void setSpeed(Uint sp) {speed = sp;}

   /**
    * Test write_bin() and read_bin() by writing self to filename and reading
    * the result in another TTable, then making sure the two are identical.
    * @param filename Filename for test bin file.
    */
   void test_read_write_bin(const string& filename);
};
}

#endif
