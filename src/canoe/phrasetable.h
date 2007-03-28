/**
 * @author Aaron Tikuisis
 * @file phrasetable.h  This file contains the definition of the PhraseTable
 * class, which uses a trie to store mappings from source phrases to target
 * phrases.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef PHRASETABLE_H
#define PHRASETABLE_H

#include "canoe_general.h"
#include "phrasedecoder_model.h"
#include <vector_map.h>
#include <trie.h>
#include <vector>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

using namespace std;
using namespace boost;


namespace Portage
{
class Voc;

/// Token that seperates items in a phrase table
extern const char *PHRASE_TABLE_SEP;


/// PhraseInfo with probabilities from multiple phrase tables
struct MultiTransPhraseInfo : public PhraseInfo
{
   vector<float> phrase_trans_probs;
}; // MultiTransPhraseInfo


/// PhraseInfo with forward and backward probs from multiple phrase tables
struct ForwardBackwardPhraseInfo : public MultiTransPhraseInfo
{
   double         forward_trans_prob;
   vector<float>  forward_trans_probs;
}; // ForwardBackwardPhraseInfo


/// Leaf sub-structure for phrase tables: holds the probs for the phrase pair.
struct TScore
{
   /// Forward probs from all TMText phrase tables
   vector<float> forward;
   /// Backward probs from all TMText phrase tables
   vector<float> backward;

   /// Constructor
   TScore() {}

   /// Resets all values.  Call clear() before re-using a TScore object.
   void clear() { forward.clear(); backward.clear(); }
}; // TScore

/// Leaf structure for phrase tables: map target phrase to probs.
typedef vector_map<Phrase, TScore> TargetPhraseTable;



/// Phrase Table structure - holds together info from all phrase tables
class PhraseTable
{
protected:
   /// The vocab for the target and source languages combined
   Voc &tgtVocab;

   /**
    * The mapping from source phrases to target phrases.
    */
   PTrie<TargetPhraseTable> textTable;

   /// The total number of translation models that have been loaded.
   Uint numTransModels;

   /// Whether forwards translation probabilities are available.
   bool forwardsProbsAvailable;

   /// Pruning types
   typedef enum {
      /**
       * Prune on the log-linear combination of forward probs with the forward
       * weights (default with forward weights)
       */
      FORWARD_WEIGHTS,
      /**
       * Prune on the log-linear combination of forward probs with the backward
       * weights (only possibility without forward weights)
       */
      BACKWARD_WEIGHTS,
      /**
       * Prune on the log-linear combination of forward and backward probs with
       * their respective weights
       */
      COMBINED_SCORE
   } PruningType;

   /// Type of pruning to use
   PruningType pruningType;

   /// Human readable description of all backward phrase tables
   string backwardDescription;

   /// Human readable description of all forward phrase tables
   string forwardDescription;

public:

   /**
    * Creates a new PhraseTable using the given vocab.
    * @param tgtVocab vocab object to used - can be shared with other models
    * @param pruningTypeStr NULL or "forward-weights" or "backward-weights"
    *                    or "combined" - see PruningType enum documentation for
    *                    details.
    */
   PhraseTable(Voc &tgtVocab, const char* pruningTypeStr = NULL);

   /**
    * Destructor.
    */
   virtual ~PhraseTable();

   /**
    * Get a human readable description of the phrase table model.
    * Unlike PLM::describeFeature() and DecoderFeature::describeFeature(),
    * this function describes the whole phrase translation model in one call,
    * since it is a much more complicated model to describe.
    *
    * @param  forwardWeights indicates whether the forward TM features are use
    *                        in the model
    * @return A string describing all phrase tables
    */
   string describePhraseTables(bool forwardWeights) const {
      return backwardDescription + (forwardWeights ? forwardDescription : "");
   }

   /**
    * Read a pair of phrase table files and stores the translations.
    * The file contents should be of the standard
    * "phrase_lang1 ||| phrase_lang2 ||| p(phrase_lang1 | phrase_lang2)".
    * If the files cannot be opened or if this format is not followed, this
    * function may terminate the execution with an error.  Note that it is not
    * checked whether the same phrase mappings appear in the two files; if one
    * appears in one file but not the other, the missing probability is
    * assigned the value exp(LOG_ALMOST_0).
    * @param src_given_tgt_file The path of the phrase table file, with
    *                           source phrases in the leftmost column.
    *                           The probabilities here are what are
    *                           actually used in decoding.
    * @param tgt_given_src_file The path of the phrase table file, with
    *                           target phrases in the leftmost column.
    *                           The probabilities here may be used for pruning
    *                           and/or as the forward probability score.
    *                           This may be NULL, but then it is recommended
    *                           that size-pruning not be done.
    * @param limitPhrases       Whether to store all phrase translations or
    *                           only those for source phrases that are already
    *                           in the table.
    * @param src_filtered       if not NULL, will filter the backward phrase
    *                           table to this stream instead of loading it in
    *                           memory.
    * @param tgt_filtered       if not NULL, will filter the forward phrase
    *                           table to this stream instead of loading it in
    *                           memory.
    */
   void read(const char *src_given_tgt_file, const char *tgt_given_src_file,
             bool limitPhrases,
             ostream *const src_filtered = NULL,
             ostream *const tgt_filtered = NULL);

   /**
    * Determine if a multi-prob file name says it's reversed.
    * A normal multi-prob files has lines like:
    *    "src ||| tgt ||| backward_probs forward_probs"
    * A reversed multi-prob was generated for translating in the other
    * direction, and therefore contains lines that look like this instead:
    *    "tgt ||| src ||| forward_probs backward_probs"
    * Reversed files must be specified with "#REVERSED" appended to the file
    * name.  This method looks for this suffix to determine of the file is
    * reversed.
    * @param multi_prob_TM_filename     file name string, possibly ending with
    *                                   "#REVERSED"
    * @param physical_filename          if not NULL, will be set to the
    *                                   physical file name, without the
    *                                   "#REVERSED" suffix if present.
    * @return true iff multi_prob_TM_filename ends in "#REVERSED".
    */
   static bool isReversed(const string& multi_prob_TM_filename,
                          string* physical_filename = NULL);

   /**
    * Count the number of probabilities in a multi-prob phrase table file.
    * Reads the first line of a multi-prob phrase table file to determine the
    * number of probability columns it contains, and therefore the number of
    * models is contains.
    * @param multi_prob_TM_filename file name; a "#REVERSED" suffix will be
    *                               removed if present.
    * @return the number of probabilities per row, i.e., the number of models;
    *         0 if multi_prob_TM_filename can't be opened or if its first line
    *         is not valid.
    */
   static Uint countProbColumns(const char* multi_prob_TM_filename);

   /**
    * Read a multi-prob phrase table file.  The file must be formatted as for
    * read(), except that the probability section of each line must contain an
    * even number of probabilities, separated by whitespace.  All lines must
    * have the same number of probabilities.  The first half are considered to
    * be backward probabilities (p(phrase_lang1 | phrase_lang2)) estimated in
    * various ways; the second half are considered to be forward probabilities
    * (p(phrase_lang2 | phrase_lang1)) estimated in various ways, and should
    * correspond one-to-one with the backward probabilities.
    *
    * Any probability value <= 0 is considered illegal (because you can't take
    * its log), and will be replaced by ALMOST_0, with a warning to the user.
    *
    * @param multi_prob_TM_filename  file name; if the file name ends with
    *                       "#REVERSED", it will be considered a reversed
    *                       phrase table.  See isReversed() for details.
    * @param limitPhrases   Whether to store all phrase translations or only
    *                       those for source phrases that are already in the
    *                       table.
    * @param filtered  Opened stream to prints the kept entries went filtering
    * @return The number of probability colums in the file.
    */
   Uint readMultiProb(const char* multi_prob_TM_filename, bool limitPhrases,
                      ostream *const filtered = NULL);

   /**
    * Write the contents to backward and/or forward streams.
    * @param src_given_tgt_out Pointer to backward stream; write nothing if null
    * @param tgt_given_src_out Pointer to forward stream; write nothing if
    * null, or if table doesn't contain forward probs.
    */
   void write(ostream* src_given_tgt_out, ostream* tgt_given_src_out);

   /**
    * Adds the given phrase and all its prefixes to the table.
    * This may be used to populate the table with phrases ahead of calling read
    * with limitPhrases = true.
    * @param phrase     The phrase, stored as an array of words.
    * @param phrase_len The number of words in phrase.
    */
   void addPhrase(const char * const *phrase, Uint phrase_len);

   /**
    * Get phrase translations from the phrase table for a given sentence.
    * Creates PhraseInfo objects for them, storing the results in the
    * triangular array phraseInfos.
    * @param phraseInfos  A triangular array into which the PhraseInfo's
    *                   created are stored.  Phrase translations for the range
    *                   [i, j) are put into the (i, j - i - 1)-th entry.
    * @param sent       The source sentence to get translations for.
    * @param weights    The translation model weights, used to compute the total
    *                   translation probability, which is used for pruning.
    * @param pruneSize  The maximum number of translation options to allow for
    *                   each source phrase, or NO_SIZE_LIMIT for no limit.  The
    *                   forward probabilities, p(tgt|src), are used to
    *                   determine which options are the best.  If pruneSize is
    *                   different from NO_SIZE_LIMIT, then it is highly
    *                   recommended that forward probabilities be available (or
    *                   translation quality may severely suffer).
    * @param logPruneThreshold  A threshold on the forward log probability;
    *                   only translation options whose log p(tgt|src), or log
    *                   p(src|tgt) if the former is unavailable, is greater
    *                   than logPruneThreshold are kept.
    * @param rangesToSkip  An ordered set of ranges to not find translation
    *                   options for; naturally, phrases are not found for
    *                   subranges of the given ranges either.
    * @param verbosity  Verbosity level - only >= 4 has an impact here and
    *                   causes lots of output about phrases being considered.
    * @param forward_weights  If forward translation probabilities are used,
    *                   the weight of each phrase table's forward probs.
    */
   void getPhraseInfos(vector<PhraseInfo *> **phraseInfos,
                       const vector<string> &sent,
                       const vector<double> &weights,
                       Uint pruneSize, double logPruneThreshold,
                       const vector<Range> &rangesToSkip, Uint verbosity,
                       const vector<double> *forward_weights = NULL);


   /**
    * Look up a given phrase pair.
    * @param src Source half, encoded as a space separated token string.
    * @param tgt Target half, encoded as a space separated token string.
    * @param score will receive backward (i.e., p(src|tgt)) and forward
    * conditional score(s); forward score vector will be empty if table
    * contains only backward scores.
    * @return true if phrase pair exists in table
    */
   bool getPhrasePair(const string& src, const string& tgt, TScore& score);

   /**
    * Produces a string representing the given Uint-vector phrase.
    * EJJ This is redundant with BasicModel(Generator)::getStringPhrase, and
    * furthermore ignores the model's forward or backward word ordering, but
    * needs to be available here to produce legible verbosity 4 output without
    * creating too many nasty interdependencies.  It is also needed for the
    * lexicographic sorting of phrases (see class PhraseScorePairLessThan
    * below).
    *
    * @param uPhrase    The phrase as a vector of Uint's.
    * @return           The string representation of uPhrase.
    */
   string getStringPhrase(const Phrase &uPhrase);

private:
   /**
    * Search for a src phrase in all phrase tables.
    * @param s_key      the key in character strings
    * @param i_key      the key in tgtVocab mapped Uints.  Must correspond to
    *                   the same tokens as s_key.
    * @param range      the subrange of s_key/i_key to query for
    * @return a shared_ptr to a TargetPhraseTable if found, a NULL shared_ptr
    * otherwise (test with ret_val.get() == NULL or ret_val.use_count() == 0).
    */
   shared_ptr<TargetPhraseTable> findInAllTables(
      const char* s_key[], const Uint i_key[], Range range);

   /// Direction of phrase table to load
   enum dir {
      src_given_tgt,        ///< src ||| tgt ||| p(src|tgt)
      tgt_given_src,        ///< tgt ||| src ||| p(tgt|src)
      multi_prob,           ///< src ||| tgt ||| backward probs forward probs
      multi_prob_reversed   ///< tgt ||| src ||| forward probs backward probs
   };

   /**
    * Reads all phrase mappings from the given file.
    * @param file       The file to read from.
    * @param d          The direction of the probabilities in the file (this
    *                   determines the order of the phrases as well).
    * @param limitPhrases  Whether to store only the phrases already in the
    *                   table (as opposed to storing everything).
    * @param filtered_output  if not NULL, we filter phrases based on vocab and
    *                   write to this stream, instead of actually loading the
    *                   phrase table.
    * @return The number of probability columns in the file: 1 for traditional
    *         1 figure ones, the number of actual prob figures for multi-prob
    *         files.
    */
   Uint readFile(const char *file, dir d, bool limitPhrases,
                 ostream *const filtered_output);

   /**
    * Recursively write out all source phrases stored below a given trie node.
    * @param src_given_tgt_out  output stream for backward probs
    * @param tgt_given_src_out  output stream for forward probs
    * @param it         iterator at the beginning of the current node to explore
    * @param end        iterator at the end of the current node to explore
    * @param prefix_stack  the strouce words of the parent nodes in trie
    */
   void write(ostream* src_given_tgt_out, ostream* tgt_given_src_out,
              PTrie<TargetPhraseTable>::iterator it,
              const PTrie<TargetPhraseTable>::iterator& end,
              vector<string>& prefix_stack);

   /**
    * Convert a string to a Phrase (Uint vector)
    * @param tgtPhrase  Destination Phrase for the conversion.
    * @param tgtString  String to convert.
    */
   void tgtStringToPhrase(Phrase& tgtPhrase, const char* tgtString);

public:
   /**
    * Read a line from the given input stream and splits it into it into three
    * parts.
    *
    * Equivalent to the following perl/pseudocode:
    *   $line = "<in>";
    *   if ($line == "")
    *   {
    *       $blank = true;
    *   } else
    *   {
    *       $lineNum++;
    *       if ($line =~ /^(.*) ||| (.*) ||| (.*)$/)
    *       {
    *           $ph1 = $1;
    *           $ph2 = $2;
    *           $prob = $3;
    *           $blank = false;
    *       } else
    *       {
    *           error(ETFatal, "Bad format in $fileName at line $lineNum");
    *       } # if
    *  } # if
    *
    * EJJ: Made public so that other programs can also use it.  Since this
    * function is static, making it public doesn't really expose anything.
    *
    * This method is loosing its relevance as we replaced it with strtok_r in
    * readFile(), but it is still correct, if much slower.
    *
    * @param in         The input stream to read from.
    * @param ph1        The first phrase is stored here.
    * @param ph2        The second phrase is stored here.
    * @param prob       The probability string is stored here.
    * @param blank      If the line is blank, this is set to true.
    * @param fileName   The name of the file that is being used (for a possible
    *                   error message).
    * @param lineNum    The line number, which is incremented (for a possible
    *                   error message).
    */
   static void readLine(istream &in, string &ph1, string &ph2, string &prob,
                        bool &blank, const char *fileName, Uint &lineNum);

protected:
   /**
    * Extract all phrases from tgtTable that pass the pruning criteria.
    * Iterates through the given target phrase table, adding all phrases above
    * the threshold and their forwards probabilities to the vector phrases.
    *
    * @param phrases    The vector into which the forwards probabilities and
    *                   PhraseInfo object pointers are stored.  All PhraseInfo
    *                   pointers stored by this function point to objects
    *                   created by this function.
    * @param tgtTable   The table containing target phrases.
    * @param src_words  The range of source words, used for initializing the
    *                   PhraseInfo objects.
    * @param numPruned  For each phrase that is below the threshold, this value
    *                   is incremented.
    * @param weights    The weights on the different phrase tables, used to
    *                   compute the forward probabilities.
    * @param logPruneThreshold  The log of the pruning threshold.  All phrases
    *                   with forward log probability at or below this threshold
    *                   are pruned.
    * @param verbosity  Sets the verbosity level - only >= 4 has an impact here
    *                   and causes lots of output about phrases being
    *                   considered.
    * @param forward_weights  If forward translation probabilities are used,
    *                   the weight of each phrase table's forward probs.
    */
   virtual void getPhrases(vector<pair<double, PhraseInfo *> > &phrases,
      TargetPhraseTable &tgtTable, const Range &src_words, Uint &numPruned,
      const vector<double> &weights, double logPruneThreshold, Uint verbosity,
      const vector<double> *forward_weights = NULL);

private:
   /**
    * Callable entity ("Function object") to sort phrases for pruning.
    * Uses parent to access getStringPhrase and translate each phrase to its
    * string representation of phrases.
    */
   class PhraseScorePairLessThan {
      private:
         PhraseTable &parent;   ///< parent PhraseTable object
      public:
         /**
          * Constructor.
          * @param parent    The parent phrase table, whose getStringPhrase
          *                  function will be used to convert Uint sequence to
          *                  readable, and lexicographically comparable, text
          */
         PhraseScorePairLessThan(PhraseTable &parent) : parent(parent) {}

         /**
          * Less ("smaller than") operator for phrases.
          * Returns true iff ph1 is "smaller than" ph2, for the purpose of
          * pruning phrases in getPhrases.  The sort order considers each of
          * these values in sequence, considering the next only in case of a
          * tie of the previous values:
          *
          * 1) score (i.e., the first element of the pair)
          *
          * 2) phrase length (longer is "greater than" shorter)
          *
          * 3) lexicographic (arbitrary but stable tie-breaker)
          *
          * The 2nd and 3rd criteria are somewhat arbitrary, and possibly
          * questionable, but they guarantee a stable ordering which will not
          * change when various options to canoe change.
          *
          * @param ph1        left-hand side operand
          * @param ph2        right-hand side operand
          * @return           true iff ph1 < ph2
          */
         bool operator() (pair<double, PhraseInfo *> ph1,
                          pair<double, PhraseInfo *> ph2);
   };
}; // PhraseTable
} // Portage

#endif // PHRASETABLE_H



