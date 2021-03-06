/**
 * @author Samuel Larkin
 * @file phrasetable_filter_joint.cc  phrase table filter based on filter_joint
 *
 * Implements the phrase table filter based on filter_joint algorithm
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "phrasetable_filter_joint.h"
#include "soft_filter_tm_visitor.h"
#include "hard_filter_tm_visitor.h"
#include "logging.h"

using namespace Portage;

// Phrase Table filter joint logger
Logging::logger ptLogger_filter_joint(Logging::getLogger("debug.canoe.phraseTable_filter_joint"));


PhraseTableFilterJoint::PhraseTableFilterJoint(bool limitPhrases,
                                               VocabFilter &tgtVocab,
                                               const pruningStyle* const pruning_style,
                                               const string& pruningTypeStr,
                                               const vector<double>& forwardWeights,
                                               const vector<double>& transWeights,
                                               bool tm_hard_limit,
                                               bool appendJointCounts) : 
   Parent(limitPhrases, tgtVocab, pruningTypeStr, appendJointCounts)
   , visitor(NULL)
   , tgtTable(NULL)
   , pruning_style(pruning_style)
   , online_filter_output(NULL)
{
   // Keep a reference on the pruning style.
   assert(pruning_style);

   if (tm_hard_limit) {
      visitor = new Joint_Filtering::hardFilterTMVisitor(*this, log_almost_0, pruningTypeStr,
                                                         forwardWeights, transWeights);
   }
   else {
      visitor = new Joint_Filtering::softFilterTMVisitor(*this, log_almost_0, pruningTypeStr);
   }
   assert(visitor);

   LOG_VERBOSE1(ptLogger_filter_joint, "Creating/Using PhraseTableFilterJoint %s mode", visitor->style.c_str());
}


PhraseTableFilterJoint::~PhraseTableFilterJoint()
{
   if (online_filter_output) delete online_filter_output;
   online_filter_output = NULL;
   if (visitor) delete visitor;
   visitor = NULL;
   if (tgtTable) delete tgtTable;
   tgtTable = NULL;
}


void PhraseTableFilterJoint::outputForOnlineProcessing(const string& filtered_TM_filename)
{
   online_filter_output = new oSafeMagicStream(filtered_TM_filename);
   assert(online_filter_output);
   tgtTable = new TargetPhraseTable;
   assert(tgtTable);

   LOG_VERBOSE2(ptLogger_filter_joint,
        "Setting output for online processing: %s, %s",
        filtered_TM_filename.c_str(),
        pruning_style->description().c_str());
}


float PhraseTableFilterJoint::convertFromRead(float value) const
{
   // This class work in prob so no convertion here.
   return value;
}


float PhraseTableFilterJoint::convertToWrite(float value) const
{
   // This class work in prob so no convertion here.
   return value;
}


// When processing a trie in memory
void PhraseTableFilterJoint::filter(const string& filtered_TM_filename)
{
   assert(visitor);

   // Keep a reference on the pruning style.
   assert(pruning_style);

   LOG_VERBOSE2(ptLogger_filter_joint,
        "Applying %s filter_joint to: %s, pruningStyle=%s, n=%d",
        visitor->style.c_str(),
        filtered_TM_filename.c_str(),
        pruning_style->description().c_str(),
        numTextTransModels);

   visitor->set(pruning_style, numTextTransModels);
   visitor->numKeptEntry = 0;
   textTable.traverse(*visitor);

   oSafeMagicStream multi(filtered_TM_filename);
   write(multi);
   fprintf(stderr, "There are %d entries left after applying %s filtering\n", visitor->numKeptEntry, visitor->style.c_str());

   //textTable.getSizeOfs();   // debugging
   //cerr << textTable.getStats() << endl;  // debugging
   visitor->display(cerr);
}


// When processing online or on the fly.
Uint PhraseTableFilterJoint::processTargetPhraseTable(const string& src,
      Uint src_word_count,
      TargetPhraseTable* tgtTable)
{
   // if output is null it means we are not processing online thus we need to
   // return the number on entries in the TargetPhraseTable
   assert(visitor);

   // no TargetPhraseTable => Nothing to do
   if (!tgtTable) return 0;
   // Not processing online
   if (!online_filter_output) return 0;

   // Calculate the proper L for this source sentence.
   assert(pruning_style);
   const Uint L = (*pruning_style)(src_word_count);

   // Processing TargetPhraseTable online
   LOG_VERBOSE3(ptLogger_filter_joint,
      "Online processing of one entry (%s) L=%d n=%d",
      visitor->style.c_str(),
      L,
      tgtTable->begin()->second.backward.size()); // => numTextTransModels

   // The visitor holds the code to either do a soft or a hard filter on one
   // leaf
   const Uint numBeforeFiltering(tgtTable->size());
   visitor->set(L, tgtTable->begin()->second.backward.size());
   (*visitor)(*tgtTable);

   // Write the results to a file and clear the leaf to save memory
   write(*online_filter_output, src, *tgtTable);
   const Uint numKeptEntry(tgtTable->size());
   tgtTable->clear();

   return numBeforeFiltering - numKeptEntry;
}


TargetPhraseTable* PhraseTableFilterJoint::getTargetPhraseTable(PhraseTableEntry& entry, bool limitPhrases)
{
   // if we are in offline processing or online but not complete we need to
   // fill out the trie
   if (!online_filter_output || tgtVocab.getNumSourceSents() > 0) {
      return PhraseTable::getTargetPhraseTable(entry, limitPhrases);
   }
   else {
      // When processing online, every TargetPhraseTable should be empty
      assert(tgtTable);
      tgtTable->clear();

      return tgtTable;
   }
}

