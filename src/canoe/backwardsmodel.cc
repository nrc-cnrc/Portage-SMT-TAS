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
                         const vector< vector<string> > &src_sents,
                         const vector< vector<MarkedTranslation> > &marks)
   : BasicModelGenerator(c, src_sents, marks)
{}

BackwardsModelGenerator::~BackwardsModelGenerator()
{}

vector<PhraseInfo *> **BackwardsModelGenerator::createAllPhraseInfos(
   const vector<string> &src_sent,
   const vector<MarkedTranslation> &marks,
   bool alwaysTryDefault,
   vector<bool>* oovs)
{
   // Idea: get phrases from parent; reverse the phrase, modify the src_words,
   // and reorganize into a new triangular array
   Uint sentLength = src_sent.size();
   vector<PhraseInfo *> **forward =
      BasicModelGenerator::createAllPhraseInfos(src_sent, marks,
                                                alwaysTryDefault, oovs);
   vector<PhraseInfo *> **backward =
      TriangArray::Create<vector<PhraseInfo *> >() (sentLength);

   for (Uint i = 0; i < sentLength; i++) {
      for (Uint j = 0; j < sentLength - i; j++) {
         Range newRange(sentLength - i - j - 1, sentLength - i);
         for ( vector<PhraseInfo *>::iterator it = forward[i][j].begin();
               it != forward[i][j].end(); it++) {
            (*it)->src_words = newRange;
            Phrase tmp;
            tmp.insert(tmp.end(), (*it)->phrase.rbegin(), (*it)->phrase.rend());
            (*it)->phrase.swap(tmp);
         }
         backward[sentLength - i - j - 1][j] = forward[i][j];
      }
      delete [] forward[i];
   }
   delete [] forward;
   return backward;
} // createAllPhraseInfos

void BackwardsModelGenerator::getStringPhrase(string &s, const Phrase &uPhrase)
{
   s.clear();
   if (uPhrase.empty()) return;
   bool first(true);
   for ( Phrase::const_reverse_iterator word(uPhrase.rbegin());
         word != uPhrase.rend(); ++word ) {
      assert(*word < tgt_vocab.size());
      if ( first )
         first = false;
      else
         s.append(" ");
      s.append(tgt_vocab.word(*word));
   }
} // getStringPhrase
