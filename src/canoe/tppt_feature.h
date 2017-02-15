/**
 * @author Eric Joanis
 * @file tppt_feature.h
 * @brief Wrapper around the TPPT class to meet the PhraseTableFeature requirements
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2016, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2016, Her Majesty in Right of Canada
 */

#ifndef __TPPT_FEATURE_H__
#define __TPPT_FEATURE_H__

#include "phrasetable.h"
#include "tppt.h"

namespace Portage {

class TPPTFeature: public PhraseTableFeature {
private:
   ugdiss::TpPhraseTable tppt;

   TPPTFeature(const string &fname, Voc &vocab)
      : PhraseTableFeature(vocab)
      , tppt(fname)
   {}

public:

   class Creator : public PhraseTableFeature::Creator
   {
   public:
      Creator(const string& modelName)
         : PhraseTableFeature::Creator(modelName) {}
      virtual void getNumScores(Uint& numModels, Uint& numAdir, Uint& numCounts, bool& hasAlignments);
      virtual TPPTFeature* create(const CanoeConfig &c, Voc& vocab);
      virtual bool checkFileExists(vector<string>* list);
      virtual Uint64 totalMemmapSize();
      virtual bool prime(bool full = false);
   }; // TPPTFeature::Creator
   static bool isA(const string& modelName);

   virtual Uint getNumModels()  const {
      const Uint num_probs = tppt.numThirdCol();
      assert(num_probs % 2 == 0);
      return num_probs / 2;
   }
   virtual Uint getNumAdir()    const { return tppt.numFourthCol(); }
   virtual Uint getNumCounts()  const { return tppt.numCounts(); }
   virtual bool hasAlignments() const { return tppt.hasAlignments(); }
   virtual void newSrcSent(const vector<string>& sentence);
   void clearCache() { tppt.clearCache(); }
   virtual shared_ptr<TargetPhraseTable> find(Range r);
}; // TPPTFeature

} // namespace Portage
#endif // __TPFEATURE_H__
