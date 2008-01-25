/**
 * @author Samuel Larkin
 * @file phrasetable_filter_joint.cc  phrase table filter based on filter_joint
 *
 * $Id$
 *
 * Implements the phrase table filter based on filter_joint algorithm
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include <phrasetable_filter_joint.h>
#include <soft_filter_tm_visitor.h>
#include <hard_filter_tm_visitor.h>

using namespace Portage;

// Phrase Table filter joint logger
//Logging::logger ptLogger_filter_joint(Logging::getLogger("debug.canoe.phraseTable_filter_joint"));


PhraseTableFilterJoint::PhraseTableFilterJoint(bool _limitPhrases, 
   VocabFilter &tgtVocab, 
   const char* pruningTypeStr, 
   const vector<double>* const hard_filter_weights)
: Parent(_limitPhrases, tgtVocab, pruningTypeStr)
, visitor(NULL)
, tgtTable(NULL)
, online_filter_output(NULL)
, L(0)
{
   if (hard_filter_weights != NULL)   {   
      visitor = new Joint_Filtering::hardFilterTMVisitor(*this, log_almost_0, hard_filter_weights);
   }
   else {
      visitor = new Joint_Filtering::softFilterTMVisitor(*this, log_almost_0);
   }
   assert(visitor);

   //LOG_VERBOSE1(ptLogger_filter_joint, "Creating/Using PhraseTableFilterJoint %s mode", visitor->style);
}


PhraseTableFilterJoint::~PhraseTableFilterJoint()
{
   if (online_filter_output)   delete online_filter_output;   online_filter_output = NULL;
   if (visitor)  delete visitor;  visitor = NULL;
   if (tgtTable) delete tgtTable; tgtTable = NULL;
}
 

void PhraseTableFilterJoint::outputForOnlineProcessing(const string& filename, Uint limitsize)
{
   L = limitsize;
   online_filter_output = new oSafeMagicStream(filename);
   assert(online_filter_output);
   tgtTable = new TargetPhraseTable;
   assert(tgtTable);
   
   //LOG_VERBOSE2(ptLogger_filter_joint, "Setting output for online processing: %s, %d", filename.c_str(), L);
}


float PhraseTableFilterJoint::convertFromRead(const float& value) const
{
   // This class work in prob so no convertion here.
   return value;
}


float PhraseTableFilterJoint::convertToWrite(const float& value) const
{
   // This class work in prob so no convertion here.
   return value;
}


void PhraseTableFilterJoint::filter_joint(const string& filename, Uint L)
{
   assert(visitor);

   /*
   LOG_VERBOSE2(ptLogger_filter_joint, "Applying %s filter_joint to: %s, L=%d, n=%d",
      visitor->style, filename.c_str(), L, numTransModels);
   */

   visitor->set(L, numTransModels);
   visitor->numKeptEntry = 0;
   textTable.traverse(*visitor);

   oSafeMagicStream multi(filename);
   write(multi);
   fprintf(stderr, "There are %d entries left after applying %s filtering\n", visitor->numKeptEntry, visitor->style);

   //textTable.getSizeOfs();   // debugging
   //cerr << textTable.getStats() << endl;  // debugging
   visitor->display(cerr);
}


Uint PhraseTableFilterJoint::processTargetPhraseTable(const string& src, TargetPhraseTable* tgtTable)
{
   // if output is null it means we are not processing online thus we need to
   // return the number on entries in the TargetPhraseTable
   assert(visitor);

   // no TargetPhraseTable => Nothing to do
   if (!tgtTable) return 0;
   // Not processing online
   if (!online_filter_output) return 0;

   // Processing TargetPhraseTable online
   /*
   LOG_VERBOSE3(ptLogger_filter_joint, 
      "Online processing of one entry (%s) L=%d n=%d", 
      visitor->style, 
      L, 
      tgtTable->front().second.backward.size()); // => numTransModels
   */

   // The visitor holds the code to either do a soft or a hard filter on one leaf
   const Uint numBeforeFiltering(tgtTable->size());
   visitor->set(L, tgtTable->front().second.backward.size());
   (*visitor)(*tgtTable);

   // Write de results to a file and clear the leaf to save memory
   write(*online_filter_output, src, *tgtTable);
   const Uint numKeptEntry(tgtTable->size());
   tgtTable->clear();

   return numBeforeFiltering - numKeptEntry;
}


TargetPhraseTable* PhraseTableFilterJoint::getTargetPhraseTable(const Entry& entry, bool limitPhrases)
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

