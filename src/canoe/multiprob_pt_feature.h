/**
 * @author Eric Joanis
 * @file multiprob_pt_feature.h
 * @brief Shell class for multi-prob phrase table feature, in particular for its Creator
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2016, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2016, Her Majesty in Right of Canada
 */

#ifndef __MULTIPROB_PT_FEATURE_H__
#define __MULTIPROB_PT_FEATURE_H__

#include "phrasetable_feature.h"

namespace Portage {

/**
 * The MultiProbPTFeature subclass of PhraseTableFeature is a stub with a queryable
 * Creator, for methods getNumCounts, checkFileExists, totalMemmapSize and
 * prime, but it asserts false if you try to create a MultiProbPTFeature since that's
 * not supported yet - plain-text Multi-Prob PTs are still loaded in memory by
 * PhraseTable itself
 */
class MultiProbPTFeature : public PhraseTableFeature {
private:
   MultiProbPTFeature(Voc &vocab) : PhraseTableFeature(vocab) {}
   ~MultiProbPTFeature() {}

public:
   class Creator : public PhraseTableFeature::Creator
   {
   public:
      Creator(const string& modelName)
         : PhraseTableFeature::Creator(modelName) {}
      /**
       * Count the number of various scores in a multi-prob phrase table file.
       * Reads the first line of a multi-prob phrase table file to determine its
       * the structure, and assumes the rest of the file matches it.
       */
      virtual void getNumScores(Uint& numModels, Uint& numAdir, Uint& numCounts, bool& hasAlignments);
      virtual MultiProbPTFeature* create(const CanoeConfig &c, Voc& vocab) { assert(false); }
      virtual bool checkFileExists(vector<string>* list);
      virtual Uint64 totalMemmapSize() { return 0; }
      virtual bool prime(bool full = false) { return true; }
   }; // MultiProbPTFeature::Creator
   static bool isA(const string& modelName);
   virtual shared_ptr<TargetPhraseTable> find(Range r);
}; // class MultiProbPTFeature

} // namespace Portage

#endif // __MULTIPROB_PT_FEATURE_H__
