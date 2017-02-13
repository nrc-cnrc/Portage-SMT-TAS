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

#include "multiprob_pt_feature.h"
#include "count_annotation.h"
#include "alignment_annotation.h"
#include "tm_entry.h"


void MultiProbPTFeature::Creator::getNumScores(Uint& numModels, Uint& numAdir, Uint& numCounts, bool& hasAlignments)
{
   numModels = numAdir = numCounts = 0;
   hasAlignments = false;
   iMagicStream in(modelName, true);  // make this a silent stream
   if (in.fail()) return;
   TMEntry entry(modelName);
   string line;
   if (!getline(in, line)) {
      error(ETWarn, "Multi-prob phrase table %s is empty.", modelName.c_str());
      return;
   }
   entry.newline(line);
   if (entry.ThirdCount() % 2 != 0)
      error(ETFatal, "Uneven number of probability columns in ttable-multi-prob file %s.", modelName.c_str());
   numModels = entry.ThirdCount() / 2;
   numAdir = entry.FourthCount();

   // Flawed assumption: the first line may not reflect the a= and c= field
   // structure of the file, since each line can have or omit the a= or c=
   // field, and have its own number of values in the c= field.
   TScore tScore;
   float probs[entry.ThirdCount()];
   entry.parseThird(probs, tScore.annotations);
   hasAlignments = (AlignmentAnnotation::get(tScore.annotations) != NULL);
   numCounts = CountAnnotation::getOrCreate(tScore.annotations)->joint_counts.size();
}

bool MultiProbPTFeature::Creator::checkFileExists(vector<string>* list)
{
   if (list) list->push_back(modelName);
   if (!check_if_exists(modelName)) {
      error(ETWarn, "Can't access phrase table file %s", modelName.c_str());
      return false;
   }
   return true;
}

bool MultiProbPTFeature::isA(const string& modelName)
{
   PCreator creator = PhraseTableFeature::getCreator(modelName);
   return (dynamic_cast<MultiProbPTFeature::Creator*>(creator.get()) != NULL);
}

shared_ptr<TargetPhraseTable> MultiProbPTFeature::find(Range r)
{
   assert(false);
   return shared_ptr<TargetPhraseTable>();
}
