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


void PhraseTableFilterGrep::filterSingleProb(const string& input, const string& output, bool reversed)
{
   //LOG_VERBOSE2(ptLogger_filter_grep, "Processing probs from: %s", input.c_str());
   os_filter_output.open(output);
   readFile(input.c_str(), reversed ? tgt_given_src : src_given_tgt, limitPhrases);
   os_filter_output.close();
}


void PhraseTableFilterGrep::processSingleProb(
   const string& src_given_tgt_file, 
   const string& backward_output,
   const char* tgt_given_src_file,
   const char* forward_output)
{
   //LOG_VERBOSE2(ptLogger_filter_grep, "Processing backward probs from: %s", src_given_tgt_file.c_str());
   os_filter_output.open(backward_output);
   readFile(src_given_tgt_file.c_str(), src_given_tgt, limitPhrases);
   os_filter_output.close();

   ostringstream description;
   description << "TranslationModel:" << src_given_tgt_file << endl;
   backwardDescription += description.str();

   if (tgt_given_src_file != NULL) {
      assert(forward_output != NULL);
      //LOG_VERBOSE2(ptLogger_filter_grep, "Processing forward probs from: %s", tgt_given_src_file);
      os_filter_output.open(forward_output);
      readFile(tgt_given_src_file, tgt_given_src, limitPhrases);
      os_filter_output.close();

      ostringstream description;
      description << "ForwardTranslationModel:" << tgt_given_src_file << endl;
      forwardDescription += description.str();
   }
   else {
      forwardsProbsAvailable = false;
   }
   
   ++numTransModels;
} // read


Uint PhraseTableFilterGrep::processMultiProb(const string& multi_prob_TM_filename, const string& filtered_output)
{
   string physical_filename;
   bool reversed = isReversed(multi_prob_TM_filename, &physical_filename);

   os_filter_output.open(filtered_output);
   const Uint col_count = readFile(physical_filename.c_str(),
                             (reversed ? multi_prob_reversed : multi_prob ),
                             limitPhrases);
   os_filter_output.close();

   const Uint model_count = col_count / 2;

   ostringstream back_description;
   ostringstream for_description;
   ostringstream adir_description;//boxing
   
   for (Uint i = 0; i < model_count; ++i) {
      back_description << "TranslationModel:" << multi_prob_TM_filename
         << "(col=" << (reversed ? i + model_count : i) << ")" << endl;
      for_description << "ForwardTranslationModel:" << multi_prob_TM_filename
         << "(col=" << (reversed ? i : i + model_count) << ")" << endl;
   }

   Uint adir_score_count = countAdirScoreColumns(physical_filename.c_str());
   for (Uint i = 0; i < adir_score_count; ++i)
      adir_description << "adirectionalModel:" << multi_prob_TM_filename
         << "(col=" << i << ")" << endl; //boxing
         
   backwardDescription += back_description.str();
   forwardDescription  += for_description.str();
   adirDescription     += adir_description.str(); //boxing

   numTransModels += model_count;
   numAdirTransModels += adir_score_count;

   return col_count;
}


bool PhraseTableFilterGrep::processEntry(TargetPhraseTable* tgtTable, Entry& entry)
{
   os_filter_output << *entry.line << endl;

   return false;
}
