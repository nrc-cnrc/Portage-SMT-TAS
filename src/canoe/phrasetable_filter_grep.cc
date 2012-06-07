/**
 * @author Samuel Larkin
 * @file phrasetable_filter_grep.cc  phrase table filtering based on source phrases
 *
 * $Id$
 *
 * Implement phrase table filtering based on source phrases
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
          
#include "phrasetable_filter_grep.h"

using namespace Portage;

// Phrase Table filter grep logger
//Logging::logger ptLogger_filter_grep(Logging::getLogger("debug.canoe.phraseTable_filter_grep"));

PhraseTableFilterGrep::PhraseTableFilterGrep(bool _limitPhrases, VocabFilter &tgtVocab, const char* pruningTypeStr)
: Parent(_limitPhrases, tgtVocab, pruningTypeStr)
{
   //LOG_VERBOSE2(ptLogger_filter_grep, "Creating/Using a PhraseTableFilterGrep");
}


PhraseTableFilterGrep::~PhraseTableFilterGrep()
{
}


Uint PhraseTableFilterGrep::filter(const string& TM_filename, const string& filtered_TM_filename)
{
   string physical_filename;
   const bool reversed = isReversed(TM_filename, &physical_filename);

   // os_filter_output will be use by processEntry since processEntry gets called during readFile.
   os_filter_output.open(filtered_TM_filename);
   const Uint col_count = readFile(physical_filename.c_str(),
                             (reversed ? multi_prob_reversed : multi_prob ),
                             limitPhrases);
   os_filter_output.close();

   const Uint model_count = col_count / 2;

   ostringstream back_description;
   ostringstream for_description;
   ostringstream adir_description;//boxing
   
   for (Uint i = 0; i < model_count; ++i) {
      back_description << "TranslationModel:" << TM_filename
         << "(col=" << (reversed ? i + model_count : i) << ")" << endl;
      for_description << "ForwardTranslationModel:" << TM_filename
         << "(col=" << (reversed ? i : i + model_count) << ")" << endl;
   }

   const Uint adir_score_count = countAdirScoreColumns(physical_filename.c_str());
   for (Uint i = 0; i < adir_score_count; ++i)
      adir_description << "adirectionalModel:" << TM_filename
         << "(col=" << i << ")" << endl; //boxing
         
   backwardDescription += back_description.str();
   forwardDescription  += for_description.str();
   adirDescription     += adir_description.str(); //boxing

   numTransModels     += model_count;
   numTextTransModels += model_count;
   numAdirTransModels += adir_score_count;

   return col_count;
}


bool PhraseTableFilterGrep::processEntry(TargetPhraseTable* tgtTable, Entry& entry)
{
   os_filter_output << *entry.line << endl;

   return true;
}
