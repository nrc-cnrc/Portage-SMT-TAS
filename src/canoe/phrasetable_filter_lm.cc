/**
 * @author Samuel Larkin
 * @file phrasetable_filter_lm.cc  Implements phrase table for filtering LMs.
 *
 * $Id$
 *
 * Special phrase table which only creates the vocab in memory without
 * filtering.  Used for filtering LMs.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
          
#include "phrasetable_filter_lm.h"

using namespace Portage;

// Phrase Table filter LM logger
//Logging::logger ptLogger_filter_LM(Logging::getLogger("debug.canoe.phraseTable_filter_LM"));


PhraseTableFilterLM::PhraseTableFilterLM(bool _limitPhrases, VocabFilter &tgtVocab, const char* pruningTypeStr)
: Parent(_limitPhrases, tgtVocab, pruningTypeStr)
{
   //LOG_VERBOSE2(ptLogger_filter_LM, "Creating/Using PhraseTableFilterLM");
}


PhraseTableFilterLM::~PhraseTableFilterLM()
{}


bool PhraseTableFilterLM::processEntry(TargetPhraseTable* tgtTable, Entry& entry)
{
   return false;
}
