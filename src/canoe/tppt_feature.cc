/**
 * @author Eric Joanis
 * @file tppt_feature.cc
 * @brief Wrapper around the TPPT class to meet the PhraseTableFeature requirements
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2016, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2016, Her Majesty in Right of Canada
 */

#include "tppt_feature.h"
#include "count_annotation.h"

// =============================================== TPPTFeature::Creator

void TPPTFeature::Creator::getNumScores(Uint& numModels, Uint& numAdir, Uint& numCounts, bool& hasAlignments)
{
   Uint thirdCol;
   ugdiss::TpPhraseTable::numScores(modelName, thirdCol, numAdir, numCounts, hasAlignments);
   if (thirdCol % 2 != 0)
      error(ETFatal, "Uneven number of probability columns in TPPT file %s.", modelName.c_str());
   numModels = thirdCol / 2;
}

TPPTFeature* TPPTFeature::Creator::create(const CanoeConfig &c, Voc &vocab)
{
   return new TPPTFeature(modelName, vocab);
}

bool TPPTFeature::Creator::checkFileExists(vector<string>* list)
{
   if (list) list->push_back(modelName);
   return ugdiss::TpPhraseTable::checkFileExists(modelName);
}

Uint64 TPPTFeature::Creator::totalMemmapSize()
{
   return ugdiss::TpPhraseTable::totalMemmapSize(modelName);
}

bool TPPTFeature::Creator::prime(bool full)
{
   cerr << "\tPriming: " << modelName << endl;  // SAM DEBUGGING
   gulpFile(modelName + "/trg.repos.dat");
   gulpFile(modelName + "/cbk");
   if (full) gulpFile(modelName + "/tppt");
   return true;
}


// =============================================== TPPTFeature

bool TPPTFeature::isA(const string& modelName)
{
   return isSuffix(".tppt", modelName) || isSuffix(".tppt/", modelName);
}

void TPPTFeature::newSrcSent(const vector<string>& sentence)
{
   PhraseTableFeature::newSrcSent(sentence);
}

shared_ptr<TargetPhraseTable> TPPTFeature::find(Range r)
{
   shared_ptr<TargetPhraseTable> tgtTable(new TargetPhraseTable);
   assert(tgtTable);

   const Uint numModels = getNumModels();
   const Uint numAdir = getNumAdir();
   const Uint numCounts = getNumCounts();
   const bool hasAl = hasAlignments();
   VectorPhrase tgtPhrase;
   ugdiss::TpPhraseTable::val_ptr_t targetPhrases = tppt.lookup(sourceSent, r.start, r.end);
   if (targetPhrases) {
      // results are not empty.
      for ( vector<ugdiss::TpPhraseTable::TCand>::iterator
               it(targetPhrases->begin()), end(targetPhrases->end());
            it != end; ++it ) {
         // Convert target phrase to global vocab
         tgtPhrase.resize(it->words.size());
         for ( Uint j = 0; j < tgtPhrase.size(); ++j )
            tgtPhrase[j] = vocab.add(it->words[j].c_str());

         // insert the values into tgtTable
         TScore* tScores(&(*tgtTable)[tgtPhrase]);
         assert(tScores);
         assert(it->score.size() == 2*numModels+numAdir);
         tScores->backward.assign(it->score.begin(), it->score.begin()+numModels);
         tScores->forward.assign(it->score.begin()+numModels, it->score.begin()+2*numModels);
         tScores->adir.assign(it->score.begin()+2*numModels, it->score.begin()+2*numModels+numAdir);

         /*
         // DONE: replace the next two blocks by three calls to Vector::assign()
         tScores->backward.resize(numModels);
         tScores->forward.resize(numModels);
         if (numAdir)
            tScores->adir.resize(numAdir);

         assert(it->score.size() == 2*numModels+numAdir);
         for (Uint j = 0; j < numModels; ++j) {
            tScores->backward[j] = it->score[j];
            tScores->forward[j] = it->score[j+numModels];
         }
         for (Uint j = 0; j < numAdir; ++j)
            tScores->adir[j] = it->score[2*numModels+j];
         */

         if (numCounts) {
            // Joint counts have a vector addition semantics, if they appear
            // in multiple phrase tables.
            assert(it->counts.size() == numCounts);
            CountAnnotation::getOrCreate(tScores->annotations)->updateValue(it->counts);
         }

         if (hasAl && !it->alignment.empty())
            tScores->annotations.initAnnotation("a", it->alignment.c_str());
      }
   }

   return tgtTable;
}
