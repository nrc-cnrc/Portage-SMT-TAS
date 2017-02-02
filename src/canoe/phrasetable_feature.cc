/**
 * @author Eric Joanis
 * @file phrasetable_feature.cc
 * @brief Abstract base class for standardized decoder-feature interface to phrase tables.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2016, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2016, Her Majesty in Right of Canada
 */

#include "phrasetable_feature.h"
#include "vocab_filter.h"
#include "config_io.h"
#include "tppt_feature.h"
#include "mixtm_feature.h"
#include "multiprob_pt_feature.h"

/********************* TScore **********************/
void TScore::print(ostream& os) const
{
   os << "forward: " << join(forward) << endl;
   os << "backward: " << join(backward) << endl;
   if (!adir.empty())
      os << "adirectional: " << join(adir) << endl;
   if (!lexdis.empty())
      os << "lexicalized distortion: " << join(lexdis) << endl;
   annotations.display(os);
}


/********************* TargetPhraseTable **********************/
void TargetPhraseTable::swap(TargetPhraseTable& o)
{
   Parent::swap(o);
   input_sent_set.swap(o.input_sent_set);
}


void TargetPhraseTable::print(ostream& os, const VocabFilter* const tgtVocab) const
{
   for (const_iterator it(begin()); it!=end(); ++it) {
      os << join(it->first) << nf_endl;
      if (tgtVocab) {
         for (Uint i(0); i<it->first.size(); ++i)
            os << tgtVocab->word(it->first[i]) << " ";
         os << "\n";
      }
      it->second.print(os);
   }
}


/********************* PhraseTableFeature **********************/
PhraseTableFeature::PCreator PhraseTableFeature::getCreator(const string& modelName)
{
   if (TPPTFeature::isA(modelName)) {
      return PCreator(new TPPTFeature::Creator(modelName));
   } else if (MixTMFeature::isA(modelName)) {
      return PCreator(new MixTMFeature::Creator(modelName));
   } else if (MultiProbPTFeature::isA(modelName)) {
      return PCreator(new MultiProbPTFeature::Creator(modelName));
   } else {
      error(ETFatal, "Cannot determine Creator type for phrase table model %s", modelName.c_str());
      return PCreator();
   }
}

PhraseTableFeature* PhraseTableFeature::create(const string &modelName, const CanoeConfig &c, Voc &vocab)
{
   PCreator cr = getCreator(modelName);
   if (cr)
      return cr->create(c, vocab);
   else
      return NULL;
}

void PhraseTableFeature::getNumScores(const string& modelName, Uint& numModels, Uint& numAdir,
      Uint& numCounts, bool& hasAlignments)
{
   PCreator cr = getCreator(modelName);
   if (cr) {
      cr->getNumScores(numModels, numAdir, numCounts, hasAlignments);
   } else {
      numModels = numAdir = numCounts = 0;
      hasAlignments = false;
   }
}

bool PhraseTableFeature::checkFileExists(const string& modelName, vector<string>* list)
{
   PCreator cr = getCreator(modelName);
   if (cr) {
      return cr->checkFileExists(list);
   } else {
      error(ETWarn, "%s is not a known PhraseTableFeature type", modelName.c_str());
      return false;
   }
}

Uint64 PhraseTableFeature::totalMemmapSize(const string& modelName)
{
   PCreator cr = getCreator(modelName);
   if (cr)
      return cr->totalMemmapSize();
   else
      return 0;
}

bool PhraseTableFeature::prime(const string& modelName, bool full)
{
   PCreator cr = getCreator(modelName);
   if (cr)
      return cr->prime(full);
   else
      return false;
}

void PhraseTableFeature::newSrcSent(const vector<string>& sentence)
{
   sourceSent = sentence;
   Uint sSize = sentence.size();
   sourceSentUint.clear();
   sourceSentUint.resize(sSize);
   for (Uint i = 0; i < sSize; ++i) {
      sourceSentUint[i] = vocab.add(sourceSent[i].c_str());
   }
}

