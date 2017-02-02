/**
 * @author Eric Joanis
 * @file phrasetable_feature.h
 * @brief Abstract base class for standardized decoder-feature interface to phrase tables.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2016, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2016, Her Majesty in Right of Canada
 */

#ifndef __PHRASETABLE_FEATURE_H__
#define __PHRASETABLE_FEATURE_H__

#include "canoe_general.h"
#include "voc.h"
#include "annotation_list.h"
#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>

namespace Portage {

class VocabFilter;
class CanoeConfig;

/// Leaf sub-structure for phrase tables: holds the probs for the phrase pair.
struct TScore
{
   // Code smell: forward, backward, adir and lexdis contain probs when returned
   // by PhraseTableFeature::find(), but log probs when returned by
   // PhraseTable::findInAllTables() or when stored in the in-memory phrase
   // table. Yuck.

   /// Forward probs from all TMText phrase tables
   vector<float> forward;
   /// Backward probs from all TMText phrase tables
   vector<float> backward;
   /// Adirectional scores from all TMText phrase tables
   vector<float> adir;
   /// lexicalized distortion scores from all DM tables
   vector<float> lexdis;

   /// Annotation trail left by various features, to find their way later, stored
   /// for each phrase pair in an annotation list.
   AnnotationList annotations;

   /**
    * Display the content of a TScore in ascii format.
    * Mainly for debugging purposes.
    * @param  os  where to output this TScore.
    */
   void print(ostream& os = cerr) const;

   /// Resets all values.  Call clear() before re-using a TScore object.
   void clear() {
      forward.clear();
      backward.clear();
      adir.clear();
      lexdis.clear();
      annotations.clear();
   }
}; // TScore

/// Leaf structure for phrase tables: map target phrase to probs.
class TargetPhraseTable : public unordered_map<Phrase, TScore> {
private:
   typedef unordered_map<Phrase, TScore> Parent;
public:
   /// Set of input sentences in which the source phrase occurs.
   /// Only relevant in limitPhrases mode, will be empty otherwise.
   boost::dynamic_bitset<> input_sent_set;

   /// Swaps two TargetPhraseTables.
   /// @param o  other TargetPhraseTables.
   void swap(TargetPhraseTable& o);

   /**
    * Display the content of a TargetPhraseTable in ascii format, for debugging
    * purposes.
    * @param  os  where to output this TargetPhraseTable.
    * @param  tgtVocab  the vocabulary to be able to output in a Human redable
    *         form.
    */
   void print(ostream& os = cerr, const VocabFilter* const tgtVocab = NULL) const;
};
//typedef vector_map<Phrase, TScore> TargetPhraseTable;

/// Stardardized abstract base PhraseTable class, for querying PTs as decoder features
class PhraseTableFeature : private NonCopyable
{
protected:
   vector<string> sourceSent;   ///< The current source sentence, for find() queries
   vector<Uint> sourceSentUint; ///< The currnet src sent, mapped through vocab

   /// The global decoder vocab, for the target and source languages combined
   Voc &vocab;

   /// The Creator class is used to query PTs without opening them, and to create them later.
   class Creator {
   protected:
      string modelName;     ///< model name or descriptor

      /// Construct a PhraseTableFeature::Creator for model modelName
      /// @param modelName Model file name or descriptor
      Creator(const string& modelName)
         : modelName(modelName) {}

   public:
      /// See PhraseTableFeature::getNumScores() for documentation
      virtual void getNumScores(Uint& numModels, Uint& numAdir, Uint& numCounts, bool& hasAlignments) = 0;

      /**
       * Create the phrase table object this creator describes
       * @param c     global decoder configuration
       * @param vocab global decoder vocab
       */
      virtual PhraseTableFeature* create(const CanoeConfig &c, Voc &vocab) = 0;

      /**
       * Checks that all of a model's files exist
       * @param list  if list is non-NULL, names of all physical files and subfiles must be added to *list
       * @return Returns true if the file exists (and associated files, if any)
       */
      virtual bool checkFileExists(vector<string>* list) = 0;

      /**
       * Calculate the total size of memory mapped files in the model, if any.
       * @return total size of memory mapped files associated with lm_filename;
       *   0 in case of problems or if the model does not use memory mapped IO.
       */
      virtual Uint64 totalMemmapSize() = 0;

      /**
       * Efficiently pre-load model in memory. This only has an effect for memory-
       * mapped models and should be a no-op for models that load in memory.
       * @param full load all model's parts in memory, not just the most
       *             essential ones.
       * @return true unless there was a problem.
       */
      virtual bool prime(bool full = false) = 0;

   }; // class PhraseTableFeature::Creator
   typedef shared_ptr<Creator> PCreator;

   // Each subclass PhraseTableFeature must implement
   //    static bool isA(modelName);
   // which will return true iff modelName describes a model of that subclass's type

private:
   static PCreator getCreator(const string& modelName);

protected:
   /// Constructor
   PhraseTableFeature(Voc &vocab) : vocab(vocab) {}

public:
   /// Destructor
   virtual ~PhraseTableFeature() {};

   /**
    * Create a PhraseTableFeature
    * @param modelName  Name of model to inspect
    * @param c     global decoder configuration
    * @param vocab global decoder vocab
    */
   static PhraseTableFeature* create(const string &modelName, const CanoeConfig &c, Voc &vocab);

   /**
    * Count the number of score columns in model modelName, without fully opening it.
    * @param modelName  Name of model to inspect
    * @param numModels  Will be set to the number of backward and forward models,
    *                   i.e., half the number of 3rd column scores.
    * @param numAdir    Will be set to the number of adirectional - 4th column - scores
    * @param numCounts  Will be set of the number of count values (c=)
    * @param hasAlignments  Will be set iff alignments are stored (a=)
    */
   static void getNumScores(const string& modelName, Uint& numModels, Uint& numAdir,
         Uint& numCounts, bool& hasAlignments);

   /**
    * Check if a phrase table's file and all its related files (if any) exist
    * @param modelName  Name of model to check
    * @param list       If non-NULL, names of files required will be added to *list
    * @return true iff the file and all its subfiles exist
    */
   static bool checkFileExists(const string& modelName, vector<string>* list = NULL);

   /**
    * Calculate the total size of memory mapped files in modelName, if any.
    * @param modelName  model whose memory-mapped size is to be determined
    * @return total size of memory mapped files associated with modelName; 0
    *         in case of problems or if the model does not use memory mapped IO.
    */
   static Uint64 totalMemmapSize(const string& modelName);

   /**
    * Efficiently load model in memory, if they are memory mapped.
    * @param full load all model's parts in memory not just the most
    *             essential ones.
    * @return true unless there was a problem.
    */
   static bool prime(const string& modelName, bool full = false);

   //@{
   /**
    * Subclasses must override getNumModels(), getNumAdir(), getNumCounts() and
    * hasAlignments() to return how many forward/backward models it has, how
    * many phrase pair counts it has, and whether it has alignments, for the
    * elements it actually provides.
    * These methods apply to a model that has been opened already. Use
    * getNumModels() to get the same information from a model without loading it.
    */
   virtual Uint getNumModels()  const { return 0; }
   virtual Uint getNumAdir()    const { return 0; }
   virtual Uint getNumCounts()  const { return 0; }
   virtual bool hasAlignments() const { return false; }
   //@}

   /// Provide the source sentence for the next set of queries
   virtual void newSrcSent(const vector<string>& sentence);

   /// Clear all caches kept by this or submodels.
   virtual void clearCache() {};

   /**
    * Lookup a source phrase and return all target phrases and scores for it.
    * @param r  query range within the sentence last provided vias newSrcSent()
    * @return The target phrases for the source phrase, with all scores, adir,
    *         counts, alignments.
    *         Horrible code smell warning: find() returns straight probs, as stored
    *         in the various ttable formats, but when PhraseTable::findInAllTables()
    *         returns the same structure, it will contain log probs. Yuck.
    */
   virtual shared_ptr<TargetPhraseTable> find(Range r) = 0;

}; // PhraseTableFeature


} // namespace Portage

#endif // __PHRASETABLE_FEATURE_H__
