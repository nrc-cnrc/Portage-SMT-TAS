/**
 * @author Aaron Tikuisis
 * @file backwardsmodel.cc  This file contains the implementation of the
 * BackwardsPhrases{Un,}KnownBMG classes, used to create BasicModel objects to
 * decode phrases backwards.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "backwardsmodel.h"
#include "basicmodel.h"
#include "phrasedecoder_model.h"
#include <string>
#include <vector>

using namespace std;
using namespace Portage;



BackwardsModelGenerator::BackwardsModelGenerator(const CanoeConfig& c)
   : BasicModelGenerator(c)
{}

BackwardsModelGenerator::BackwardsModelGenerator(
                         const CanoeConfig& c,
                         const VectorPSrcSent &sents)
   : BasicModelGenerator(c, sents)
{}

BackwardsModelGenerator::~BackwardsModelGenerator()
{}

vector<PhraseInfo *> **BackwardsModelGenerator::createAllPhraseInfos(
      const newSrcSentInfo& info,
      bool alwaysTryDefault)
{
   // Idea: get phrases from parent; reverse the phrase, modify the src_words,
   // and reorganize into a new triangular array
   Uint sentLength = info.src_sent.size();
   vector<PhraseInfo *> **forward =
      BasicModelGenerator::createAllPhraseInfos(info, alwaysTryDefault);
   vector<PhraseInfo *> **backward =
      TriangArray::Create<vector<PhraseInfo *> >() (sentLength);

   for (Uint i = 0; i < sentLength; i++) {
      for (Uint j = 0; j < sentLength - i; j++) {
         Range newRange(sentLength - i - j - 1, sentLength - i);
         for ( vector<PhraseInfo *>::iterator it = forward[i][j].begin();
               it != forward[i][j].end(); it++) {
            (*it)->src_words = newRange;
            VectorPhrase tmp;
            tmp.insert(tmp.end(), (*it)->phrase.rbegin(), (*it)->phrase.rend());
            (*it)->phrase = tmp;
         }
         backward[sentLength - i - j - 1][j] = forward[i][j];
      }
      delete [] forward[i];
   }
   delete [] forward;
   return backward;
} // createAllPhraseInfos

string BackwardsModelGenerator::getStringPhrase(const Phrase &uPhrase) const
{
   if (uPhrase.empty()) return "";
   bool first(true);
   string s;
   for ( Phrase::const_reverse_iterator word(uPhrase.rbegin());
         word != uPhrase.rend(); ++word ) {
      assert(*word < tgt_vocab.size());
      if ( first )
         first = false;
      else
         s.append(" ");
      s.append(tgt_vocab.word(*word));
   }
   return s;
} // getStringPhrase
