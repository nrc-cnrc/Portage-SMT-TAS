/**
 * @author Aaron Tikuisis
 * @file phrasetable.h  This file contains the definition of the PhraseTable
 * class and some associated classes. PhraseTable is a bit of a misnomer,
 * because this is really a collection of heterogenous representations for 
 * mappings between souce/target phrase pairs and various kinds of scores,
 * including lexicalized distortion. It basically groups together many (but not
 * all) kinds of services for reading, writing, storing and accessing
 * phrasal information in the decoder.
 *
 * Here are some useful associated include files:
 * ../utils/tm_entry.h - parse all supported phrase-pair formats from text
 * ../tm/phrase_table.h - represention used during training, with read/write to
 *    text format - can be more convenient for standalone manipulation
 * - see below for others, especially ones used implicitly via the forward
 *   declarations right after "namespace Portage"
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef PHRASETABLE_H
#define PHRASETABLE_H

#include "canoe_general.h"
#include "phrasedecoder_model.h"
#include "vector_map.h"
#include "trie.h"
#include "vocab_filter.h"
#include "tm_entry.h"
#include "annotation_list.h"
#include "new_src_sent_info.h"
#include "phrasetable_feature.h"
#include <vector>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>

using namespace std;
using namespace boost;

namespace ugdiss {
   class TpPhraseTable;
}

namespace Portage
{
class TPPTFeature;
class Voc;
class CanoeConfig;
struct PhraseTableEntry;

/// Token that seperates items in a phrase table
extern const char *PHRASE_TABLE_SEP; // = " ||| "

/// Phrase Table structure - holds together info from all phrase tables
class PhraseTable : private NonCopyable
{
public:
   /// Log value to use for missing or 0-prob entries (default is LOG_ALMOST_0)
   static double log_almost_0;

   typedef vector_map<string, PhrasePairAnnotator*> AnnotatorRegistry;

protected:

   /// The vocab for the target and source languages combined
   VocabFilter &tgtVocab;

   /// The annotations that need to be initialized while loading the phrase table
   // Use of vector_map here is critical: it preserves elements in the order
   // they were first inserted, which is what we want and need.
   AnnotatorRegistry annotatorRegistry;

   /**
    * The mapping from source phrases to target phrases.
    * Includes only translation models that were loaded from text formatted
    * tables, i.e., excludes information from the TPPTs.
    */
   PTrie<TargetPhraseTable> textTable;

   /// Number of text files that have been read so far by readMultiProb().
   Uint numTextFilesRead;

   /// The number of translation models that have been loaded from text files.
   Uint numTextTransModels;

   /// The number of adirectional models that have been loaded from text files.
   Uint numTextAdirModels;

   /// Translation models of any kind in the PhraseTableFeature hierarchy
   vector<shared_ptr<PhraseTableFeature> > phraseTableFeatures;

   /// Lexicalized Distortion Models in TPLDM format.
   vector<shared_ptr<ugdiss::TpPhraseTable> > tpldmTables;

   /// The total number of translation models that have been loaded.
   Uint numTransModels;

   /// The total number of adirectional translation models that have been loaded (not included in numTransModels)
   Uint numAdirTransModels;

   /// The total number of lexicalized distortion models that have been loaded.
   Uint numLexDisModels;

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
      COMBINED_SCORE,
      /**
       * EXTERNAL_PRUNING means pruning is done by the BasicModelGenerator,
       * e.g., so it can take into account all the decoder features in "full"
       * mode.
       */
      EXTERNAL_PRUNING
   } PruningType;

   /// Type of pruning to use
   PruningType pruningType;

   /// For limitPhrases mode, the number of input sentences.  Used to determine
   /// the size of the input_sent_set bit vector for each TargetPhraseTable.
   const Uint num_sents;

   /// Human readable description of all backward phrase tables
   string backwardDescription;

   /// Human readable description of all forward phrase tables
   string forwardDescription;

   /// Human readable description of all adirectional phrase tables
   string adirDescription;

    /// Human readable description of all lexicalized distortion tables
   string lexdisDescription;

public:

   /**
    * Constructor creates a new PhraseTable using the given vocab.
    * @param tgtVocab vocab object to used - can be shared with other models
    * @param pruningTypeStr "forward-weights" or "backward-weights", "combined",
    *                    or "full" - see PruningType enum documentation for
    *                    details.
    * @param appendJointCounts Whether to append joint counts from different
    *        incoming tables (default is to add them element-wise).
    */
   PhraseTable(VocabFilter &tgtVocab,
               const string& pruningTypeStr = "",
               bool appendJointCounts = false);

   /**
    * Destructor.
    */
   virtual ~PhraseTable();

   /**
    * Register that a particular annotation type should be initialized while
    * loading a phrase table.  Annotations are created in the order in which
    * annotators are registered, so if your annotator needs another annotation
    * to have been initialized first, register that annotator first.
    * Registering the same annotator (i.e., with same answer to
    * annotator->getName()) multiple times is OK and will result in only the
    * last instance being kept.
    * Object ownership: PhraseTable will not delete the annotator, it is up to
    * the caller to do so.
    * @param annotator  Factory class used to create the desired annotation
    *                   for each phrase pair.
    */
   void registerAnnotator(PhrasePairAnnotator* annotator);

   /// Get access to the registered annotators, to run them
   const AnnotatorRegistry& getAnnotators() { return annotatorRegistry; }

   /**
    * Clear all caches kept by this or submodels.
    */
   void clearCache();

   /**
    * Set the log value to use for missing or 0-prob entries (default is LOG_ALMOST_0)
    */
   void setLogAlmostZero(double val) {log_almost_0 = val;}

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
      return backwardDescription + (forwardWeights ? forwardDescription : "") + adirDescription;
   }

   /**
    * open a phrase table in any format that is part of the PhraseTableFeature
    * hierarchy, i.e., all but plain-text, multi-prob phrase tables
    * Note: all text based phrase tables must be loaded first.
    * @param modelName  Name or descriptor of the phrase table or model to open
    * @param c          CanoeConfig object
    */
   void openTTable(const string &modelName, const CanoeConfig &c);

   /**
    * Open a Lexicalized Distortion Model in memory map form.
    * @param  lexicalized_dm_file  Lexicalized Distortion Model's filename.
    */
   void openTPLDM(const char *lexicalized_dm_file);

   /**
    * Extract all target language vocabulary from all opened TPPTs.
    * This method considers only source phrases in the in the Trie, i.e., added
    * with addPhrase() or found in any phrase tables loaded using read() with
    * limitPhrases = false.  Intended for use when limitPhrases is true.
    * @param verbosity  Verbosity level - only >= 4 has an impact here and
    *                   causes lots of output about src/tgt phrases being
    *                   looked at.
    */
   void extractVocabFromTPPTs(Uint verbosity);

   /**
    * OBSOLETE - #REVERSED is no longer supported. This method is only here to
    * facilitate issueing meaningful error messages when that format is used.
    *
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
    * Count the number of probabilities in phrase table in any format.
    * @param modelName file name
    * @return the number of probabilities per row, i.e., the number of models;
    *         0 if multi_prob_TM_filename can't be opened or if it is not valid.
    */
   static Uint countProbColumns(const string& modelName);

   /**
    * Count the number of adirectional scores in a phrase table in any format.
    * @param modelName file name
    * @return the number of "4th column" scores per row, i.e., the number of
    *         adirectional models;
    *         0 if modelName can't be opened or if it is not valid.
    */
   static Uint countAdirScoreColumns(const string& modelName);

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
    * @param multi_prob_TM_filename  file name
    * @param limitPhrases   Whether to store all phrase translations or only
    *                       those for source phrases that are already in the
    *                       table.
    * @param describeModelOnly  If true, don't load anything, just fill the model
    *                       description variables.
    * @return The number of probability columns in the file.
    */
   virtual Uint readMultiProb(const char* multi_prob_TM_filename,
                              bool limitPhrases, bool describeModelOnly);

   /**
    * Read a lexicalized distortion model file.  The file must be formatted as for
    * read().  All lines must have the same number of probabilities.
    *
    * Any probability value <= 0 is considered illegal (because you can't take
    * its log), and will be replaced by ALMOST_0, with a warning to the user.
    *
    * @param lexicalized_DM_filename  file name
    * @param limitPhrases   Whether to store all phrase translations or only
    *                       those for source phrases that are already in the
    *                       table.
    * @return The number of probability columns in the file.
    */
   virtual Uint readLexicalizedDist(const char* lexicalized_DM_filename, bool limitPhrases);

   /**
    * Write a multiprob translation table from the trie
    * @param  multi_src_given_tgt_out  opened stream to output multiprob TM
    */
   void write(ostream& multi_src_given_tgt_out);

   /**
    * Adds the given phrase and all its prefixes to the table.
    * This may be used to populate the table with phrases ahead of calling read
    * with limitPhrases = true.
    * @param phrase     The phrase, stored as an array of words.
    * @param phrase_len The number of words in phrase.
    * @param sent_no    The index of the sentence for which this phrase is
    *                   being added.
    */
   void addPhrase(const char * const *phrase, Uint phrase_len, Uint sent_no);

   /**
    * Adds all sentences to the VocabFilter object which will take care of
    * building structures needed for LM filtering based on per sentence vocab.
    * @param  sentences  tokenized sentences to be added to the vocab.
    */
   virtual void addSourceSentences(const VectorPSrcSent& sentences);

   /**
    * Get all phrase translations from all phrase tables for a given sentence.
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
    * @param adir_weights  If adirectional models are used, the weight of each
    *                   adirectional model.
    */
   void getPhraseInfos(vector<PhraseInfo *> **phraseInfos,
                       const vector<string> &sent,
                       const vector<double> &weights,
                       Uint pruneSize, double logPruneThreshold,
                       const vector<Range> &rangesToSkip,
                       Uint verbosity,
                       const vector<double> *forward_weights = NULL,
                       const vector<double> *adir_weights = NULL);

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
   string getStringPhrase(const Phrase &uPhrase) const {
      return phrase2string(uPhrase, tgtVocab);
   }

   /**
    * Determine if the table contains a given source phrase.
    * This looks only in the text table, i.e., it doesn't check TPPTs.
    * @param num_tokens number of tokens in the phrase
    * @param tokens the phrase
    * @return true if phrase is a source phrase in the table
    */
   bool containsSrcPhrase(Uint num_tokens, char* tokens[]);

   /**
    * Find out how many text files have been read so far
    */
   Uint getNumTextFilesRead() const { return numTextFilesRead; }

public:
   /// Direction of phrase table to load
   enum dir {
      multi_prob,            ///< src ||| tgt ||| backward probs forward probs
      //multi_prob_reversed,   ///< tgt ||| src ||| forward probs backward probs -- OBSOLETE
      lexicalized_distortion ///< src ||| tgt ||| pm ps pd nm ns nd
   };

protected:
   /**
    * Helper for readFile, with changing behaviour in filtering subclasses.
    * When doing a hard filtering, this will process one block of entries at a
    * time and then flush the ptrie to keep the memory footprint low.
    * The online processing can only be done if there is only one multiprob TM.
    * @param src             The source phrase to process
    * @param src_word_count  The number of tokens in src
    * @param tgtTable        The TargetPhraseTable for src
    * @return Returns the number of entries in tgtTable
    */
   virtual Uint processTargetPhraseTable(const string& src,
         Uint src_word_count,
         TargetPhraseTable* tgtTable);

   /**
    * Helper for readFile, with changing behaviour in filtering subclasses.
    * In this class, get the TargetPhraseTable object from the trie for the
    * phrase in Entry, creating it if limitPhrases is false.
    * @param[in/out] entry  information about phrase table line being processed
    * @param[out] entry.src_word_count  return the number of source words in entry.src.
    * @param limitPhrases whether to restrict the processing to phrases
    *                     pre-entered into the trie.
    */
   virtual TargetPhraseTable* getTargetPhraseTable(PhraseTableEntry& entry, bool limitPhrases);

   /**
    * Search for a src phrase in text format PTs (in the Trie) and in TPPT
    * format PTs.
    * @param str_key    the key in in a string vector
    * @param s_key      the key in a c string array
    * @param i_key      the key in tgtVocab mapped Uints.  Must correspond to
    *                   the same tokens as s_key.
    * @param range      the subrange of s_key/i_key to query for
    * @return a shared_ptr to a TargetPhraseTable if found, a NULL shared_ptr
    * otherwise (test with ret_val.get() == NULL or ret_val.use_count() == 0).
    */
   shared_ptr<TargetPhraseTable> findInAllTables(
         const vector<string>& str_key,
         const char* s_key[], const Uint i_key[], Range range);

   /**
    * Reads all phrase mappings from the given file.
    * @param file       The file to read from ("-" for stdin).
    * @param d          The direction of the probabilities in the file (this
    *                   determines the order of the phrases as well).
    * @param limitPhrases  Whether to store only the phrases already in the
    *                   table (as opposed to storing everything).
    * @return The number of probability columns in the file: 1 for obsolete
    *         1-figure files, the number of third-column prob figures for
    *         multi-prob files.
    */
   Uint readFile(const char *file, dir d, bool limitPhrases);

   /**
    * Recursively write out all source phrases stored below a given trie node.
    * @param multi_src_given_tgt_out  output stream for multi probs
    * @param it         iterator at the beginning of the current node to explore
    * @param end        iterator at the end of the current node to explore
    * @param prefix_stack  the source words of the parent nodes in trie
    */
   void write(ostream& multi_src_given_tgt_out,
              PTrie<TargetPhraseTable>::iterator it,
              const PTrie<TargetPhraseTable>::iterator& end,
              vector<string>& prefix_stack);

   /**
    */
   void write(ostream& multi_src_given_tgt_out, const string& src,
              const TargetPhraseTable& tgt_phrase_table);

   /**
    * Convert a string to a Phrase (Uint vector).
    * @param tgtPhrase  Destination Phrase for the conversion.
    * @param tgtString  String to convert.
    */
   void tgtStringToPhrase(VectorPhrase& tgtPhrase, const char* tgtString);

   /// Same as tgtStringToPhrase(), but does not add previously unseen words
   /// to the vocabulary.  (Needed for loading lexicalized distortion models)
   void tgtStringToPhraseIndex(VectorPhrase& tgtPhrase, const char* tgtString);

protected:
   /**
    * When reading a file containing a phrase table, process one entry.
    * Depending on normal mode or filtering mode, we handle entries differently.
    * @param entry.tgtTable  target Table (trie leaf) to which entry belongs.
    * @param entry  current entry we are processing.
    * @return Returns true if the entry was added to the TargetPhraseTable.
    */
   virtual bool processEntry(PhraseTableEntry& entry);

   /**
    * Converts a float value to its value in log prob if needed.
    * This method is overriden in filtering subclasses as appropriate.
    * @param value value to process
    * @return Returns value appropriately converted for in-memory storage.
    */
   virtual float convertFromRead(float value) const;

   /**
    * Converts a float to a prob before writing to a file.
    * This method is overriden in filtering subclasses as appropriate.
    * @param value  value to convert.
    * @return Returns value appropriately converted for writing, i.e., always a
    *         prob, reversing whatever convertFromRead() did.
    */
   virtual float convertToWrite(float value) const;

private:
   /// Returns log(x) unless x <= 0, in which case returns log_almost_0.
   /// Use this if you know you need to take the log, convertFromRead() for
   /// sub-class dependent behaviour.
   inline float shielded_log (float x) const {
      return x <= 0 ? log_almost_0 : log(x);
   }

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
    * @param ascore     The adirectional probability string is stored here.
    * @param blank      If the line is blank, this is set to true.
    * @param fileName   The name of the file that is being used (for a possible
    *                   error message).
    * @param lineNum    The line number, which is incremented (for a possible
    *                   error message).
    */
   static void readLine(istream &in, string &ph1, string &ph2, string &prob,
                        string &ascore, bool &blank, const char *fileName,
                        Uint &lineNum);

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
                           const vector<double> *forward_weights = NULL,
                           const vector<double> *adir_weights = NULL); //boxing

public:
   /**
    * Callable entity ("Function object") to sort phrases for pruning.
    * Uses parent to access getStringPhrase and translate each phrase to its
    * string representation of phrases.
    */
   class PhraseScorePairLessThan
   : public binary_function<pair<double, PhraseInfo *>, pair<double, PhraseInfo *>, bool>
   {
      private:
         /**
          * Hash function for strings which should be stable between 32/64 bits.
          * @param  param the string to hash.
          * @return Returns a stable hash of param.
          */
         Uint mHash(const string& param) const;

      public:
         /**
          * Less ("smaller than") operator for phrases.
          * Returns true iff ph1 is "smaller than" ph2, for the purpose of
          * pruning phrases in getPhrases.  The sort order considers each of
          * these values in sequence, considering the next only in case of a
          * tie of the previous values:
          *
          * 1) score forward probs (i.e., the first element of the pair)
          *
          * 2) score backward probs (which is inversely correlated to the
          * frequency, as shown by empirical results)
          *
          * 3) hash on phrase words (adds a controlled randomness)
          *
          * 4) lexicographic (arbitrary but stable tie-breaker)
          *
          * Criteria 2 to 4 are somewhat arbitrary, and possibly questionable,
          * but they guarantee a stable ordering which will not change when
          * various options to canoe change.
          *
          * @param ph1        left-hand side operand
          * @param ph2        right-hand side operand
          * @return           true iff ph1 < ph2
          */
         bool operator() (const pair<double, PhraseInfo *> ph1,
                          const pair<double, PhraseInfo *> ph2) const;
         /// This variant uses ph{1,2}->partial_score as the primary score
         bool operator() (PhraseInfo* ph1, PhraseInfo* ph2) const {
            return operator()(make_pair(ph1->partial_score, ph1),
                              make_pair(ph2->partial_score, ph2));
         }
   };

   /// Same as PhraseScorePairLessThan but for sorting in descending order.
   class PhraseScorePairGreaterThan : public PhraseScorePairLessThan {
      public:
         /// Greater than operator for phrases.
         /// See PhraseScorePairLessThan::operator()() for details.
         /// @return true iff ph1 > ph2
         bool operator() (const pair<double, PhraseInfo *>& ph1,
                          const pair<double, PhraseInfo *>& ph2) const;
   };

}; // PhraseTable

/**
 * Container for holding one entry when reading a either a single or multi probs.
 * The code to parse a line in a TM is in TMEntry, this class just adds other
 * variables that PhraseTable methods need to pass around with entries.
 */
struct PhraseTableEntry : public TMEntry
{
   const PhraseTable::dir d;   ///< direction of the prob file
   const bool limitPhrases;    ///< parameter copied from PhraseTable::readFile()'s
   string* line;               ///< raw entry
   char* src_buffer;           ///< storage for src_tokens
   Uint src_buffer_size;       ///< size of src_buffer
   vector<char*> src_tokens;   ///< source tokens, split from Src() -- reset when Src() changes, not at every entry.
   VectorPhrase tgtPhrase;     ///< phrase representation of the target string (vector<Uint>)
   Uint src_word_count;        ///< the number of words in src
   Uint zero_prob_err_count;   ///< Number of prob <= 0 errors detected
   TargetPhraseTable *tgtTable;///< the target phrase table object for this entry

   /**
    * Constructor.
    * @param d     direction of _file
    * @param limitPhrases  whether we're filtering as we're loading.
    * @param file  current file we are processing
    */
   PhraseTableEntry(PhraseTable::dir d, bool limitPhrases, const string& file)
   : TMEntry(file)
   , d(d)
   , limitPhrases(limitPhrases)
   , line(NULL)
   , src_buffer(NULL)
   , src_buffer_size(0)
   , src_word_count(0)
   , zero_prob_err_count(0)
   {}
};

} // Portage

#endif // PHRASETABLE_H
