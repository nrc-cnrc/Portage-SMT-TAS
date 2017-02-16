/**
 * @author Eric Joanis
 * @file mixtm_feature.h
 * @brief mixed phrase table, mixed dynamically in the decoder, as a decoder feature
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2017, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2017, Her Majesty in Right of Canada
 */

#ifndef __MIXTM_FEATURE_H__
#define __MIXTM_FEATURE_H__

#include "phrasetable_feature.h"

namespace Portage {

class MixTMFeature: public PhraseTableFeature {
public:
   static string magicNumber;

   class Creator : public PhraseTableFeature::Creator
   {
      friend class MixTMFeature;
      string dirName; ///< directory part of modelName, for relative-path modifications
      vector<string> componentNames; ///< Names of the components, read from modelName
      vector<vector<double> > weightMatrix; ///< All weights read from modelName

      /// Set by constructor iff modelName was read successfully and parsed correctly.
      /// When valid, dirName, componentNames and weightMatrix are initialized.
      /// When !valid, those variables have undefined values.
      bool valid;

   public:
      Creator(const string& modelName);
      virtual void getNumScores(Uint& numModels, Uint& numAdir, Uint& numCounts, bool& hasAlignments);
      virtual MixTMFeature* create(const CanoeConfig &c, Voc& vocab);
      virtual bool checkFileExists(vector<string>* list);
      virtual Uint64 totalMemmapSize();
      virtual bool prime(bool full = false);
   }; // MixTMFeature::Creator

   static bool isA(const string& modelName);

   virtual Uint getNumModels()  const;
   virtual Uint getNumAdir()    const;
   virtual Uint getNumCounts()  const;
   virtual bool hasAlignments() const;
   virtual void newSrcSent(const vector<string>& sentence);
   virtual void clearCache();
   virtual shared_ptr<TargetPhraseTable> find(Range r);

private:
   vector<shared_ptr<PhraseTableFeature> > components;
   Creator creator;
   Uint numModels;
   Uint numAdir;
   Uint numCounts;
   bool hasAl;
   MixTMFeature(Creator &creator, const CanoeConfig &c, Voc &vocab);
}; // MIXTMFeature

} // namespace Portage
#endif // __MIXTM_FEATURE_H__
