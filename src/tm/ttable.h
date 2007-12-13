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
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef TTABLE_H
#define TTABLE_H

#include <portage_defs.h>
#include <ext/hash_map>
#include <map>
#include <algorithm>
#include <string>
#include <errors.h>
#include <quick_set.h>
#include <voc.h>

namespace Portage {

using namespace __gnu_cxx;

/// Callable entity that returns and index for a string.
class StringHash : public unary_function<string, size_t>
{
public:
   /**
    * Calculates the hash value of key.
    * @param key  string that we want to hash.
    * @return Returns the hash value of key.
    */
   size_t operator() (const string& key) const {
      hash<const char*> h;
      return h(key.c_str());
   }
};


/// TTable
class TTable {

public:

   typedef pair<Uint,float> TIndexAndProb;  ///< target-word index & cond prob.
   typedef vector<TIndexAndProb> SrcDistn;  ///< Source distribution.

private:

   /// Token that separes fields in GIZA++ files
   static const string sep_str;
   Uint speed;                  ///< controls initial pass algorithm

   typedef hash_map<string,Uint,StringHash>::const_iterator WordMapIter;
   /// Definition of a source distribution iterator.
   typedef SrcDistn::const_iterator SrcDistnIter;

   /// Callable entity to sort on Probs in decreasing order.
   struct CompareProbs {
      /**
       * Compares x and y based on their probs (for decreasing order sort)
       * @param x,y TIndexAndProb objects to compare.
       * @return Returns true if x > y
       */
      bool operator()(const TIndexAndProb& x, const TIndexAndProb& y) const
      {return x.second > y.second;}
   };
   /// Callable entity to sort on indexes in increasing order.
   struct CompareTIndexes {
      /**
       * Compares x and y based on their indexes
       * @param x,y TIndexAndProb objects to compare.
       * @return Returns true if x < y
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
       * @return Returns true if x < y
       */
      bool operator()(const TIndexAndProb& x, const Uint& y) const
      {return x.first < y;}
   };

   hash_map<string,Uint,StringHash> tword_map; ///< T-language vocab
   hash_map<string,Uint,StringHash> sword_map; ///< S-language vocab
   vector<string> twords; ///< T-language vocab (mapping index to word)
   vector<SrcDistn> src_distns;
   vector<vector<bool>*> src_distns_quick;
   QuickSet src_inds; ///< local variable to add(), kept persistent for speed
   QuickSet tgt_inds; ///< local variable to add(), kept persistent for speed

   SrcDistn empty_distn;  ///< Empty source distribution.

   void add(Uint src_index, Uint tgt_index);
   void read(const string& filename, const Voc* src_voc);

public:

   /**
    * Constructor. GIZA++ format: src_word tgt_word p(tgt_word|src_word).
    * @param filename  file containing the GIZA++ info.
    * @param src_voc   if not null, causes the ttable to be filtered while
    *                  loading, keeping only lines where the source word exists
    *                  in *src_voc.
    */
   TTable(const string& filename, const Voc* src_voc = NULL);

   /// Constructor that creates an empty ttable. Use add() to populate, then
   /// makeDistnsUniform().
   TTable() {speed = 2;}

   /// Destructor.
   ~TTable() {}

   /**
    * Remove all pairs with probability < thresh, and renormalize afterwards
    * return size.
    * @param thresh  Threshold value to prune.
    * @return Returns the number of pruned entries.
    */
   Uint prune(double thresh);

   /**
    * Writes the ttable to a stream.
    * @param os  An opened stream to output the ttable.
    */
   void write(ostream& os) const;

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
   /// @return Returns the number of source/target words.
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
    * @return Returns tgt_word index or  numTargetWords() if unknown.
    */
   Uint targetIndex(const string& tgt_word) {
      WordMapIter p = tword_map.find(tgt_word);
      return p == tword_map.end() ? numTargetWords() : p->second;
   }

   /**
    * Converts src_word to its index value in the source vocabulary.
    * @param src_word  String to convert to its index value in the source
    * vocabulary.
    * @return Returns the index of src_word in the source vocabulary or
    * numSourceWords() if unknown
    */
   Uint sourceIndex(const string& src_word) {
      WordMapIter p = sword_map.find(src_word);
      return p == sword_map.end() ? numSourceWords() : p->second;
   }

   /**
    * Get the source distribution from an index.
    * @param src_index  index to retrieve.
    * @return Returns the source distribution associated with src_index or the
    * empty distribution if not found.
    */
   const SrcDistn& getSourceDistn(Uint src_index) const {
      return src_index == numSourceWords() ? empty_distn : src_distns[src_index];
   }

   /**
    * Get the source distribution from a word.
    * @param src_word  source string to retrieve.
    * @return Returns the source distribution associated with src_index or the
    * empty distribution if not found.
    */
   const SrcDistn& getSourceDistn(const string& src_word) const {
      WordMapIter p = sword_map.find(src_word);
      return p == sword_map.end() ? empty_distn : src_distns[p->second];
   }

   /**
    * Find offset of given target index within src_dist.
    * @param target_index  target index
    * @param src_distn     source distribution
    * @return  Returns the offset of given target index within src_dist, or
    * return -1 if not there.
    */
   int targetOffset(Uint target_index, const SrcDistn& src_distn);

   /**
    * Get the source distribution in decreasing order of probabilities.
    * @param src_word  source word to find.
    * @param distn     will contain the source distribution in decreasing order of probabilities.
    */
   void getSourceDistnByDecrProb(const string& src_word, SrcDistn& distn) const;

   /**
    * A sugary & inefficient version of the above
    * void getSourceDistnByDecrProb(const string& src_word, SrcDistn& distn) const
    * @param src_word
    * @param tgt_words
    * @param tgt_probs
    */
   void getSourceDistnByDecrProb(const string& src_word,
                                 vector<string>& tgt_words,
                                 vector<float>& tgt_probs) const;

   /**
    * Gets the best translation for a given source word.
    * @param src_word
    * @param best_trans
    * @return Returns the probability for the best translation for a given source word.
    */
   double getBestTrans(const string& src_word, string& best_trans);

   /**
    * p(tgt_word|src_word).
    * @param src_word
    * @param tgt_word
    * @return Returns p(tgt_word|src_word), or -1 if no such pair exists in the table.
    */
   double getProb(const string& src_word, const string& tgt_word) const;

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

};
}

#endif
