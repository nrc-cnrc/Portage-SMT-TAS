/**
 * @author Eric Joanis
 * @file forced_target_model.cc  Translation model generator (based on
 * BasicModelGenerator) which takes into consideration the reference sentences
 * in the target language.
 *
 * $Id$
 * vim:sw=3:
 *
 *
 * THIS FILE IS NOT USED ANYWHERE YET - PREPARED FOR A CHANGE TO
 * PHRASE_TM_ALIGN THAT WAS DEFERRED INDEFINITELY.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "forced_target_model.h"
#include "forced_target_phrasetable.h"
#include <vector>
#include <string>

using namespace Portage;
using namespace std;

ForcedTargetModelGenerator::ForcedTargetModelGenerator(
      const CanoeConfig &c,
      const vector< vector<string> > &src_sents,
      const vector< vector<string> > &tgt_sents,
      const vector< vector<MarkedTranslation> > &marks) :
  BasicModelGenerator(c, src_sents, marks,
                      new ForcedTargetPhraseTable(tgt_vocab))
{
   if (!c.loadFirst)
   {
      // Enter all target phrase into the target language phrase table, used
      // only for pruning the regular phrase table.
      for (vector<vector<string> >::const_iterator it = tgt_sents.begin(); 
           it != tgt_sents.end(); it++)
      {
         Uint phrase[it->size()];
         for (Uint i = 0; i < it->size(); i++)
         {
            phrase[i] = tgt_vocab.add((*it)[i].c_str());
         }

         for (Uint i = 0; i < it->size(); i++)
         {
            targetPhraseTable.insert(phrase, it->size() - i, true);
         }
      } // for tgt_sents 
   }
}
