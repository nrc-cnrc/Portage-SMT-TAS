/**
 * @author Eric Joanis
 * @file mixtm_feature.cc
 * @brief mixed phrase table, mixed dynamically in the decoder, as a decoder feature
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2017, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2017, Her Majesty in Right of Canada
 */

#include "mixtm_feature.h"
#include "count_annotation.h"
#include "alignment_annotation.h"
#include "file_utils.h"
#include "str_utils.h"
#include "multiprob_pt_feature.h"
#include <tr1/unordered_set>

// =============================================== MixTMFeature::Creator

MixTMFeature::Creator::Creator(const string& modelName)
   : PhraseTableFeature::Creator(modelName)
   , dirName(DirName(modelName))
{
   valid = false;

   iMagicStream in(modelName);
   if (!in) return;

   string line;
   if (!getline(in, line) || line != magicNumber) {
      error(ETWarn, "Invalid MixTM file %s: should start with magic number \"%s\"",
            modelName.c_str(), magicNumber.c_str());
      return;
   }

   Uint column_count = 0;
   while (getline(in, line)) {
      trim(line);
      if (line.empty() || line[0] == '#') continue;

      vector<string> tokens;
      if (split(line, tokens, "\t") != 2) {
         error(ETWarn, "Invalid MixTM file %s: invalid line \"%s\". Each line should have a component "
               "model name, a tab, and a space separated list of weights",
               modelName.c_str(), line.c_str());
         return;
      }
      string component = tokens[0];
      trim(component);
      component = adjustRelativePath(dirName, component);
      componentNames.push_back(component);

      vector<string> weightsStr;
      Uint count = split(tokens[1], weightsStr, " ");
      if (count == 0) {
         error(ETWarn, "Invalid MixTM file %s: line \"%s\" has a no weights.",
               modelName.c_str(), line.c_str());
         return;
      }

      if (column_count == 0) {
         column_count = count;
      } else if (count != column_count) {
         error(ETWarn, "Invalid MixTM file %s: line \"%s\" has a different number of weights than previous lines.",
               modelName.c_str(), line.c_str());
         return;
      }

      vector<double> weights(count, 0);
      for (Uint i = 0; i < count; ++i) {
         if (!conv(weightsStr[i], weights[i])) {
            error(ETWarn, "Invalid MixTM file %s: cannot convert weight \"%s\" to a double on line \"%s\".",
                  modelName.c_str(), weightsStr[i].c_str(), line.c_str());
            return;
         }
      }
      weightMatrix.push_back(weights);
   }

   if (componentNames.empty()) {
      error(ETWarn, "Invalid MixTM file %s has no components.", modelName.c_str());
      return;
   }

   assert(componentNames.size() == weightMatrix.size());

   valid = true;
}

void MixTMFeature::Creator::getNumScores(Uint& numModels, Uint& numAdir, Uint& numCounts, bool& hasAlignments)
{
   if (!valid || !checkFileExists(NULL)) {
      numModels = numAdir = numCounts = 0;
      hasAlignments = false;
   } else {
      PhraseTableFeature::getNumScores(componentNames[0], numModels, numAdir, numCounts, hasAlignments);
   }
}

MixTMFeature* MixTMFeature::Creator::create(const CanoeConfig &c, Voc &vocab)
{
   if (valid && checkFileExists(NULL))
      return new MixTMFeature(*this, c, vocab);
   else
      return NULL;
}

bool MixTMFeature::Creator::checkFileExists(vector<string>* list)
{
   if (!valid)
      return false;

   // Code written so we print warnings about all missing components - we
   // don't want to stop at the first one with a problem.
   bool ok = true;

   // We have checkFileExists() fail when a component's own checkFileExists() fails, as well as when
   // the number of model columns and weights are inconsistent, so that we catch problems in
   // "configtool check" instead of when we load the models.
   Uint numModels(0), numAdir(0), numCounts(0);
   bool hasAlignments(false);
   PhraseTableFeature::getNumScores(componentNames[0], numModels, numAdir, numCounts, hasAlignments);

   for (Uint i = 0; i < componentNames.size(); ++i) {
      if (!PhraseTableFeature::checkFileExists(componentNames[i], list)) {
         ok = false;
      } else if (MultiProbPTFeature::isA(componentNames[i])) {
         error(ETWarn, "Invalid MixTM file %s: component %s is a multi-prob phrase table; convert it to a tppt",
               modelName.c_str(), componentNames[i].c_str());
         ok = false;
      } else {
         Uint componentNumModels, componentNumAdir;
         PhraseTableFeature::getNumScores(componentNames[i], componentNumModels, componentNumAdir,
                                          numCounts, hasAlignments);
         if (2*componentNumModels + componentNumAdir != weightMatrix[0].size()) {
            error(ETWarn, "Invalid MixTM file %s: component %s has the wrong number of columns "
                  "for the weights provided.", modelName.c_str(), componentNames[i].c_str());
            ok = false;
         } else if (componentNumModels != numModels || componentNumAdir != numAdir) {
            error(ETWarn, "Invalid MixTM file %s: component %s has a different number of 3rd or 4th "
                  "column weights than the first component.", modelName.c_str(), componentNames[i].c_str());
            ok = false;
         }
      }
   }

   return ok;
}

Uint64 MixTMFeature::Creator::totalMemmapSize()
{
   Uint64 result = 0;
   if (valid && checkFileExists(NULL)) {
      for (Uint i = 0; i < componentNames.size(); ++i)
         result += PhraseTableFeature::totalMemmapSize(componentNames[i]);
   }
   return result;
}

bool MixTMFeature::Creator::prime(bool full)
{
   bool ok = true;
   if (valid && checkFileExists(NULL)) {
      for (Uint i = 0; i < componentNames.size(); ++i)
         if (!PhraseTableFeature::prime(componentNames[i], full))
            ok = false;
   } else {
      ok = false;
   }
   return ok;
}


// =============================================== MixTMFeature

string MixTMFeature::magicNumber = "Portage dynamic MixTM v1.0";

bool MixTMFeature::isA(const string& modelName)
{
   return isSuffix(".mixtm", modelName);
}

Uint MixTMFeature::getNumModels()  const
{
   assert(!components.empty());
   return components[0]->getNumModels();
}

Uint MixTMFeature::getNumAdir()    const
{
   assert(!components.empty());
   return components[0]->getNumAdir();
}

Uint MixTMFeature::getNumCounts()  const
{
   Uint numCounts = 0;
   for (Uint i = 0; i < components.size(); ++i)
      numCounts = max(numCounts, components[i]->getNumCounts());
   return numCounts;
}

bool MixTMFeature::hasAlignments() const
{
   bool hasAl = false;
   for (Uint i = 0; i < components.size(); ++i)
      hasAl = hasAl || components[i]->hasAlignments();
   return hasAl;
}

void MixTMFeature::newSrcSent(const vector<string>& sentence)
{
   for (Uint i = 0; i < components.size(); ++i)
      components[i]->newSrcSent(sentence);
}

void MixTMFeature::clearCache()
{
   for (Uint i = 0; i < components.size(); ++i)
      components[i]->clearCache();
}

shared_ptr<TargetPhraseTable> MixTMFeature::find(Range r)
{
   shared_ptr<TargetPhraseTable> tgtTable(new TargetPhraseTable);

   // The original design looped over components, then phrases, but rounded too
   // much as it used the floats in TScore to store intermediate results.
   // The difference between dynamic MixTMs and static ones was just too great,
   // and we could not reproduce results, or BLEU scores in tuning.

   // The new design first lists all the target phrases, so that we can loop
   // over phrases, then loop over components, and store the intermediate
   // results in doubles, casting back to float at the end only.  It's still
   // not magic, though: the MixTM calculated this way is closer to the static
   // one, but decoder output is still not identical and BLEU tuning is fickle
   // as usual, sensitive to even the tinyest variability in decoding.

   tr1::unordered_set<Phrase> tgtPhrases;
   vector<shared_ptr<TargetPhraseTable> > t(components.size());
   for (Uint i = 0; i < components.size(); ++i) {
      t[i] = components[i]->find(r);
      for (TargetPhraseTable::iterator iter(t[i]->begin()); iter != t[i]->end(); ++iter)
         tgtPhrases.insert(iter->first);
   }

   for (tr1::unordered_set<Phrase>::iterator phraseIter(tgtPhrases.begin()); phraseIter != tgtPhrases.end(); ++phraseIter) {
      vector<double> backward(numModels, 0);
      vector<double> forward(numModels, 0);
      vector<double> adir(numAdir, 0);
      TScore* tScore = &((*tgtTable)[*phraseIter]);

      for (Uint i = 0; i < components.size(); ++i) {
         TargetPhraseTable::iterator iter = t[i]->find(*phraseIter);
         if (iter != t[i]->end()) {
            for (Uint j = 0; j < numModels; ++j) {
               backward[j] += iter->second.backward[j] * creator.weightMatrix[i][j];
               forward[j] += iter->second.forward[j] * creator.weightMatrix[i][j+numModels];
            }

            for (Uint j = 0; j < numAdir; ++j)
               adir[j] += iter->second.adir[j] * creator.weightMatrix[i][j+2*numModels];

            if (numCounts) {
               CountAnnotation *counts = CountAnnotation::get(iter->second.annotations);
               if (counts)
                  CountAnnotation::getOrCreate(tScore->annotations)->updateValue(counts->joint_counts);
            }

            if (hasAl) {
               AlignmentAnnotation *al = AlignmentAnnotation::get(iter->second.annotations);
               if (al)
                  tScore->annotations.initAnnotation("a", al->getAlignment());
            }
         }
      }

      tScore->backward.resize(numModels);
      tScore->forward.resize(numModels);
      for (Uint j = 0; j < numModels; ++j) {
         tScore->backward[j] = backward[j];
         tScore->forward[j] = forward[j];
      }
      tScore->adir.resize(numAdir);
      for (Uint j = 0; j < numAdir; ++j)
         tScore->adir[j] = adir[j];
   }

   return tgtTable;
}

MixTMFeature::MixTMFeature(Creator &creator, const CanoeConfig &c, Voc &vocab)
   : PhraseTableFeature(vocab)
   , creator(creator)
{
   for (Uint i = 0; i < creator.componentNames.size(); ++i) {
      PhraseTableFeature* component = PhraseTableFeature::create(creator.componentNames[i], c, vocab);
      if (!component)
         error(ETFatal, "Error loading mixtm %s: cannot load component %s.",
               creator.modelName.c_str(), creator.componentNames[i].c_str());
      components.push_back(shared_ptr<PhraseTableFeature>(component));
   }

   numModels = getNumModels();
   numAdir = getNumAdir();
   numCounts = getNumCounts();
   hasAl = hasAlignments();
}

