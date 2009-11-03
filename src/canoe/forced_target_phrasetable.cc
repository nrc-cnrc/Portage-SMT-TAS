/**
 * @author Eric Joanis
 * @file forced_target_phrasetable.cc  Implementation of ForcedTargetPhraseTable
 *
 * $Id$
 * vim:sw=3:
 * 
 * THIS FILE IS NOT USED ANYWHERE YET - PREPARED FOR A CHANGE TO
 * PHRASE_TM_ALIGN THAT WAS DEFERRED INDEFINITELY.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "forced_target_phrasetable.h"
#include "basicmodel.h"

using namespace Portage;
using namespace std;


void ForcedTargetPhraseTable::setTargetSentence ( const vector<string> &tgt_sent )
{ 
   this->tgt_sent.resize(tgt_sent.size());
   for (Uint i = 0; i < tgt_sent.size(); i++) {
      this->tgt_sent[i] = tgtVocab.index(tgt_sent[i].c_str());
      if ( this->tgt_sent[i] == tgtVocab.size() ) 
         this->tgt_sent[i] = PhraseDecoderModel::OutOfVocab;
   } // for
}

void ForcedTargetPhraseTable::setTargetSentence ( const vector<Uint> &tgt_sent )
{ 
   this->tgt_sent = tgt_sent;
}

void ForcedTargetPhraseTable::getPhrases(vector<pair<double, PhraseInfo *> > &phrases,
         TargetPhraseTable &tgtTable, const Range &src_words, Uint &numPruned,
         const vector<double> &weights, double logPruneThreshold, Uint verbosity,
         const vector<double> *forward_weights)
{
   TargetPhraseTable filteredTgtTable;
   for (TargetPhraseTable::iterator phrase_it = tgtTable.begin();
        phrase_it != tgtTable.end(); phrase_it++)
   {
      // Check if the phrase exists in tgt_sent
      for (Uint i = 0; i < tgt_sent.size(); i++)
      {
         Phrase &phrase = (*phrase_it).first;
         bool match = (tgt_sent.size() - i >= phrase.size());
         Phrase::const_iterator w_it = phrase.begin();
         for (Uint j = 0; match && (w_it != phrase.end()); ++j, ++w_it) {
            match = (*w_it == tgt_sent[i+j]);
         }
         if (match) {
            filteredTgtTable.push_back(*phrase_it);
            break;
         } // if
      } // for tgt_sent start index
   } // for target phrase

   PhraseTable::getPhrases(phrases, filteredTgtTable, src_words, numPruned,
                           weights, logPruneThreshold, verbosity, forward_weights);
} // getPhrases

