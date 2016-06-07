/**
 * @author Aaron Tikuisis
 * @file phrasetable.cc  This file contains the implementation of the
 * PhraseTable class, which uses a trie to store mappings from source phrases
 * to target phrases.
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "logging.h"
#include "phrasetable.h"
#include "str_utils.h"
#include "file_utils.h"
#include "voc.h"
#include "tppt.h"
#include "pruning_style.h"
#include <cmath>
#include <algorithm>

#include "compact_phrase.h"
#include "string_hash.h"
#include "config_io.h"
#include "alignment_annotation.h"
#include "count_annotation.h"

namespace std { namespace tr1 {
   template<> class hash<CompactPhrase> {
      public:
         unsigned int operator()(const CompactPhrase& p) const {
            return hash<const char*>()(p.c_str());
         }
   };
}}

using namespace std;
using namespace Portage;

/// Logger for PhraseTable
Logging::logger ptLogger(Logging::getLogger("debug.canoe.phraseTable"));
/// Logger for filtering LMs with PhraseTable
Logging::logger ptLogger_filterLM(Logging::getLogger("debug.canoe.phraseTable.filterLM"));

const char *Portage::PHRASE_TABLE_SEP = " ||| ";

/********************* TScore **********************/
void TScore::print(ostream& os) const
{
   os << "forward: " << join(forward) << endl;
   os << "backward: " << join(backward) << endl;
   if (!adir.empty())
      os << "adirectional: " << join(adir) << endl;
   if (!lexdis.empty())
      os << "lexicalized distortion: " << join(lexdis) << endl;
   annotations.display(os);
}


/********************* TargetPhraseTable **********************/
void TargetPhraseTable::swap(TargetPhraseTable& o) {
   Parent::swap(o);
   input_sent_set.swap(o.input_sent_set);
}


void TargetPhraseTable::print(ostream& os, const VocabFilter* const tgtVocab) const {
   for (const_iterator it(begin()); it!=end(); ++it) {
      os << join(it->first) << nf_endl;
      if (tgtVocab) {
         for (Uint i(0); i<it->first.size(); ++i)
            os << tgtVocab->word(it->first[i]) << " ";
         os << "\n";
      }
      it->second.print(os);
   }
}


/********************* PhraseTable **********************/
double PhraseTable::log_almost_0 = LOG_ALMOST_0;

PhraseTable::PhraseTable(VocabFilter& _tgtVocab,
                         const string& pruningTypeStr,
                         bool appendJointCounts) :
   tgtVocab(_tgtVocab),
   numTextFilesRead(0),
   numTextTransModels(0),
   numTextAdirModels(0),
   numTransModels(0),
   numAdirTransModels(0),
   numLexDisModels(0),
   forwardsProbsAvailable(true),
   num_sents(tgtVocab.getNumSourceSents())
{
   if (pruningTypeStr == "forward-weights" || pruningTypeStr.empty()) {
      pruningType = FORWARD_WEIGHTS;
   } else if (pruningTypeStr == "backward-weights") {
      pruningType = BACKWARD_WEIGHTS;
   } else if (pruningTypeStr == "combined") {
      pruningType = COMBINED_SCORE;
   } else if (pruningTypeStr == "full") {
      pruningType = EXTERNAL_PRUNING;
   } else {
      assert(false);
   }

   if (appendJointCounts)
      CountAnnotation::setAppendJointCountsMode(this);
}


PhraseTable::~PhraseTable()
{
   annotatorRegistry.clear();
}

void PhraseTable::registerAnnotator(PhrasePairAnnotator* annotator)
{
   // If a previous instance of the same annotator exists, we just overwrite
   // it.  We don't delete the old one, because a model might depend on
   // using it.  For example, if there are two BiLMs, each will register its
   // own (identical) annotator.
   annotatorRegistry[annotator->getName()] = annotator;
}

void PhraseTable::clearCache()
{
   for ( Uint i = 0; i < tpptTables.size(); ++i ) {
      assert(tpptTables[i]);
      tpptTables[i]->clearCache();
   }
   for ( Uint i = 0; i < tpldmTables.size(); ++i ) {
      assert(tpldmTables[i]);
      tpldmTables[i]->clearCache();
   }
}


Uint PhraseTable::openTPPT(const char *tppt_filename)
{
   cerr << "opening TPPT phrase table " << tppt_filename << endl;
   shared_ptr<TPPT> p_tppt(new TPPT(tppt_filename));
   tpptTables.push_back(p_tppt);

   const Uint num_probs = p_tppt->numThirdCol();
   assert(num_probs % 2 == 0);
   const Uint model_count = num_probs / 2;
   numTransModels += model_count;

   const Uint adir_scores = p_tppt->numFourthCol();
   numAdirTransModels += adir_scores;

   ostringstream back_description, for_description, adir_description;
   for (Uint i = 0; i < model_count; ++i) {
      back_description << "TranslationModel:" << tppt_filename
         << "(col=" << i << ")" << endl;
      for_description << "ForwardTranslationModel:" << tppt_filename
         << "(col=" << (i + model_count) << ")" << endl;
   }
   for (Uint i = 0; i < adir_scores; ++i)
      adir_description << "AdirectionalModel:" << tppt_filename
         << "(col=" << i << ")" << endl;
   backwardDescription += back_description.str();
   forwardDescription  += for_description.str();
   adirDescription += adir_description.str();

   return num_probs;
}

void PhraseTable::openTPLDM(const char *lexicalized_dm_file)
{
   cerr << "opening TPLDM lexicalized distortion " << lexicalized_dm_file << endl;
   shared_ptr<TPPT> p_tppt(new TPPT(lexicalized_dm_file));
   tpldmTables.push_back(p_tppt);
}

TargetPhraseTable* PhraseTable::getTargetPhraseTable(PhraseTableEntry& entry, bool limitPhrases)
{
   TargetPhraseTable *tgtTable = NULL;

   if(!(entry.src_word_count > 0)) {
      error(ETWarn, "\nSuspicious entry in %s, there is no source phrase in: %s\n",
            entry.File(), entry.line->c_str());
      return NULL;
   }

   Uint srcWords[entry.src_word_count];

   // Find in table
   if (limitPhrases) {
      bool contains_unknown_word = false;
      for (Uint i = 0; i < entry.src_word_count; i++) {
         srcWords[i] = tgtVocab.index(entry.src_tokens[i]);
         if (srcWords[i] == tgtVocab.size()) {
            contains_unknown_word = true;
            break;
         }
      }
      if (contains_unknown_word ||
            ! textTable.find(srcWords, entry.src_word_count, tgtTable))
         tgtTable = NULL;
   }
   else {
      for (Uint i = 0; i < entry.src_word_count; i++)
         srcWords[i] = tgtVocab.add(entry.src_tokens[i]);
      textTable.find_or_insert(srcWords, entry.src_word_count, tgtTable);
   }

   return tgtTable;
}


// Optimized implementation of readFile() - though uglier, it is almost twice
// as fast as the previous implementation.
Uint PhraseTable::readFile(const char *file, dir d, bool limitPhrases)
{
   assert(tpptTables.empty());
   Uint numKept(0);
   Uint numFiltered(0);
   PhraseTableEntry entry(d, limitPhrases, file);
   iSafeMagicStream in(file);

   if (d == lexicalized_distortion){
      cerr << "loading lexicalized distortion model from " << file << ": " << flush;
   } else{
      cerr << "loading phrase table from " << file << ": " << flush;
   }
   time_t start_time = time(NULL);

   // Avoid unecessary alloc/realloc cycle by declaring tgtPhrase and line
   // outside the loop.
   string line;

   // Don't look the tgtTable up again if src hasn't changed
   string prev_src;
   entry.tgtTable = NULL;
   while (in.good())
   {
      getline(in, line);
      if (line == "") continue;

      entry.newline(line);
      entry.line = &line;

      // Canoe can't do anything with an empty source thus skip it.
      if (entry.Src()[0] == '\0') {
         error(ETWarn, "SKIPPING empty source at line %d while loading tm: %s\n",
               entry.LineNo(), line.c_str());
         continue;
      }

      if (entry.Src() != prev_src) {
         //cerr << "NEW entry " << entry.Src() << " line " << entry << endl; //SAM DEBUG
         // The source phrase has changed, process the previous target table.
         numFiltered  += processTargetPhraseTable(prev_src, entry.src_word_count, entry.tgtTable);
         prev_src = entry.Src();

         // Tokenize the new source phrase
         Uint src_len = strlen(entry.Src())+1;
         if (src_len > entry.src_buffer_size) {
            delete [] entry.src_buffer;
            entry.src_buffer_size = max(2*entry.src_buffer_size, src_len);
            entry.src_buffer = new char[entry.src_buffer_size];
            entry.src_buffer[entry.src_buffer_size-1] = '\0';
         }
         strcpy(entry.src_buffer, entry.Src());
         assert(entry.src_buffer[entry.src_buffer_size-1] == '\0');
         entry.src_word_count = destructive_splitZ(entry.src_buffer, entry.src_tokens, " ");

         // Get the new target table.
         entry.tgtTable = getTargetPhraseTable(entry, limitPhrases);
      }

      if (entry.tgtTable != NULL) // i.e., if src in trie
      {
         // Tokenize target and add to tgt_vocab
         entry.tgtPhrase.clear();
         if (d == lexicalized_distortion)
            tgtStringToPhraseIndex(entry.tgtPhrase, entry.Tgt());
         else
            tgtStringToPhrase(entry.tgtPhrase, entry.Tgt());

         if (entry.tgtPhrase.empty()) {
            error(ETWarn, "\nSuspicious entry in %s at line %u, there is no target in: %s\n",
                  file, entry.LineNo(), line.c_str());
            continue;
         }
         if (limitPhrases && d != lexicalized_distortion) {
            tgtVocab.addPerSentenceVocab(entry.tgtPhrase, &entry.tgtTable->input_sent_set);
         }
         // Add entry to the trie's leaf
         if (processEntry(entry)) ++numKept;
      }
   }

   // In online processing we process after all the entries for a particular
   // source phrase were read thus it is necessary to process the last source
   // phrase as a particular case.
   numFiltered += processTargetPhraseTable(prev_src, entry.src_word_count, entry.tgtTable);

   cerr << endl << entry.LineNo() << " lines read, " << numKept << " entries kept.";
   if (numFiltered > 0)
      cerr << endl << (numKept - numFiltered) << " entries remaining after filtering.";
   cerr << endl << "Done in " << (time(NULL) - start_time) << "s" << endl;

   if (entry.zero_prob_err_count) {
      error(ETWarn, "%d 0 or negative probabilities found in %s - treated as missing entries",
         entry.zero_prob_err_count, file);
   }
   in.close();

   return entry.ThirdCount();
} // readFile


float PhraseTable::convertFromRead(float value) const
{
   // In this class, we take the log
   return shielded_log(value);
}

float PhraseTable::convertToWrite(float value) const
{
   return (value == log_almost_0) ? 0.0f : exp(value);
}

bool PhraseTable::processEntry(PhraseTableEntry& entry)
{
   // ZERO's value depends on the subclass's implementation of convertFromRead
   const float ZERO(convertFromRead(0.0f));

   if (entry.d == lexicalized_distortion) {
      TargetPhraseTable::iterator it = entry.tgtTable->find(entry.tgtPhrase);
      if (it == entry.tgtTable->end()) {
         return false;
      }
      else {
         TScore* curProbs = &(it->second);

         if (entry.ThirdCount() != 6)
            error(ETFatal, "Wrong nubmer of lexicalized distortion probabilities (%u instead of 6) in first line of %s",
                  entry.ThirdCount(), entry.File());

         float lexdis_probs[6];
         entry.parseThird(lexdis_probs);

         for (Uint i = 0; i < 6; ++i)
            lexdis_probs[i] = convertFromRead(lexdis_probs[i]);

         if (curProbs->lexdis.size() > numLexDisModels)
         {
            error(ETWarn, "Entry %s ||| %s appears more than once in %s; new occurrence on line %d",
                  entry.Src(), entry.Tgt(), entry.File(), entry.LineNo());
         }
         else {
            curProbs->lexdis.resize(numLexDisModels, ZERO);
            curProbs->lexdis.insert(curProbs->lexdis.end(), lexdis_probs, lexdis_probs + 6);
         }
      }
   }
   else {
      // Get the object with all the information about this phrase pair.
      TScore* curProbs = &( (*entry.tgtTable)[entry.tgtPhrase] );

      // Determine probabilities for multi-prob phrase table, possibly with
      // named annotation fields.
      const Uint col_count = entry.ThirdCount();
      float probs[col_count];
      entry.parseThird(probs, curProbs->annotations);

      for ( Uint i = 0 ; i < col_count; ++i ) {
         if (probs[i] <= 0) {
            probs[i] = ZERO;
            ++entry.zero_prob_err_count;
         }
         else {
            probs[i] = convertFromRead(probs[i]);
         }
      }

      const Uint multi_prob_model_count = col_count / 2;
      float *forward_probs, *backward_probs;
      assert(entry.d == multi_prob);
      backward_probs = &(probs[0]);
      forward_probs = &(probs[multi_prob_model_count]);

      if (curProbs->backward.size() > numTextTransModels ||
          curProbs->forward.size() > numTextTransModels)
      {
         error(ETWarn, "Entry %s ||| %s appears more than once in %s; new occurrence on line %d",
               entry.Src(), entry.Tgt(), entry.File(), entry.LineNo());
      }
      else {
         curProbs->backward.reserve(numTextTransModels + multi_prob_model_count);
         curProbs->forward.reserve(numTextTransModels + multi_prob_model_count);
         curProbs->backward.resize(numTextTransModels, ZERO);
         curProbs->forward.resize(numTextTransModels, ZERO);
         curProbs->backward.insert(curProbs->backward.end(), backward_probs,
                                   backward_probs + multi_prob_model_count);
         curProbs->forward.insert(curProbs->forward.end(), forward_probs,
                                  forward_probs + multi_prob_model_count);
      }

      //boxing
      const Uint adir_count = entry.FourthCount();
      if (adir_count > 0) {
         float ascores[adir_count];
         entry.parseFourth(ascores);
         for (Uint i = 0 ; i < adir_count; ++i)
            ascores[i] = convertFromRead(ascores[i]);

         if (curProbs->adir.size() > numTextAdirModels)
         {
            error(ETWarn, "Entry %s ||| %s appears more than once in %s; new occurrence on line %d",
                  entry.Src(), entry.Tgt(), entry.File(), entry.LineNo());
         }
         else {
            curProbs->adir.reserve(numTextAdirModels + adir_count);
            curProbs->adir.resize(numTextAdirModels, ZERO);
            curProbs->adir.insert(curProbs->adir.end(), ascores,
                  ascores + adir_count);
         }
      }
      //boxing

      // Call all the registered annotators to do what they need with the phrase pair
      for (AnnotatorRegistry::iterator it(annotatorRegistry.begin()), end(annotatorRegistry.end());
           it != end; ++it)
         it->second->annotate(curProbs, entry);

   } // if ; end deal with multi-prob line

   // In this class, the entry is always added to the TargetPhraseTable.
   return true;
}


bool PhraseTable::isReversed(const string& multi_prob_TM_filename,
                             string* physical_filename)
{
   static const string rev_suffix("#REVERSED");
   const Uint size(multi_prob_TM_filename.size());
   if (size > rev_suffix.size() &&
       multi_prob_TM_filename.substr(size-rev_suffix.size()) == rev_suffix) {
      if (physical_filename)
         *physical_filename =
            multi_prob_TM_filename.substr(0, size - rev_suffix.size());
      return true;
   } else {
      if (physical_filename)
         *physical_filename = multi_prob_TM_filename;
      return false;
   }
} // PhraseTable::isReversed()


Uint PhraseTable::countProbColumns(const char* multi_prob_TM_filename)
{
   iMagicStream in(multi_prob_TM_filename, true);  // make this a silent stream
   if (in.fail()) return 0;
   TMEntry entry(multi_prob_TM_filename);
   string line;
   if (!getline(in, line)) {
      error(ETWarn, "Multi-prob phrase table %s is empty.", multi_prob_TM_filename);
      return 0;
   }
   entry.newline(line);
   return entry.ThirdCount();
}

Uint PhraseTable::countAdirScoreColumns(const char* multi_prob_TM_filename)
{
   iMagicStream in(multi_prob_TM_filename, true);  // make this a silent stream
   if (in.fail()) return 0;
   TMEntry entry(multi_prob_TM_filename);
   string line;
   if (!getline(in, line)) {
      error(ETWarn, "Multi-prob phrase table %s is empty.", multi_prob_TM_filename);
      return 0;
   }
   entry.newline(line);
   return entry.FourthCount();
}

// Not inlined so as to keep TPPT code completely encapsulated within this .cc
// file, and thus avoid propagating dependencies unecessarily.
Uint PhraseTable::countTPPTProbModels(const char* tppt_filename)
{
   Uint third, fourth, counts;
   bool al;
   TPPT::numScores(tppt_filename, third, fourth, counts, al);
   return third;
}

Uint PhraseTable::countTPPTAdirModels(const char* tppt_filename)
{
   Uint third, fourth, counts;
   bool al;
   TPPT::numScores(tppt_filename, third, fourth, counts, al);
   return fourth;
}

Uint PhraseTable::readMultiProb(const char* multi_prob_TM_filename,
   bool limitPhrases, bool describeModelOnly)
{
   assert(!isReversed(multi_prob_TM_filename, NULL) && "#REVERSED is no longer support - configtool check should have already said so and we should not get here in the code");

   Uint col_count;
   if (describeModelOnly)
      col_count = countProbColumns(multi_prob_TM_filename);
   else
      col_count = readFile(multi_prob_TM_filename, multi_prob, limitPhrases);

   const Uint model_count = col_count / 2;
   //assert(col_count>0);
   //assert(model_count>0); // no longer required - file can have only adir or alignment info.

   ostringstream back_description;
   ostringstream for_description;
   ostringstream adir_description; //boxing
   for (Uint i = 0; i < model_count; ++i) {
      back_description << "TranslationModel:" << multi_prob_TM_filename
         << "(col=" << i << ")" << endl;
      for_description << "ForwardTranslationModel:" << multi_prob_TM_filename
         << "(col=" << (i + model_count) << ")" << endl;
   }

   //boxing
   Uint adir_score_count = countAdirScoreColumns(multi_prob_TM_filename);
   for (Uint i = 0; i < adir_score_count; ++i) {
      adir_description << "AdirectionalModel:" << multi_prob_TM_filename
                       << "(col=" << i << ")" << endl; //boxing
   }//boxing

   backwardDescription += back_description.str();
   forwardDescription  += for_description.str();
   adirDescription += adir_description.str(); //boxing

   numTransModels += model_count;
   numTextTransModels += model_count;
   numAdirTransModels += adir_score_count; //boxing
   numTextAdirModels += adir_score_count;
   numTextFilesRead += 1;

   return col_count;
}

////
Uint PhraseTable::readLexicalizedDist(const char* lexicalized_DM_filename,
                                       bool limitPhrases)
{
   cerr << "opening text lexicalized distortion " << lexicalized_DM_filename << endl;

   const Uint model_count = readFile(lexicalized_DM_filename,
                                     lexicalized_distortion,
                                     limitPhrases);
   numLexDisModels += model_count;
   return model_count;
}
////


void PhraseTable::extractVocabFromTPPTs(Uint verbosity) {
   if (!tpptTables.empty())
      error(ETFatal, "operation not implemented yet: can't extract vocab from TPPTs");
}

void PhraseTable::tgtStringToPhraseIndex(VectorPhrase& tgtPhrase, const char* tgtString)
{
   char buffer[strlen(tgtString)+1];
   strcpy(buffer, tgtString);
   char* strtok_state;
   char* tok = strtok_r(buffer, " ", &strtok_state);
   while ( tok != NULL ) {
      tgtPhrase.push_back(tgtVocab.index(tok));
      tok = strtok_r(NULL, " ", &strtok_state);
   }
}


void PhraseTable::tgtStringToPhrase(VectorPhrase& tgtPhrase, const char* tgtString)
{
   char buffer[strlen(tgtString)+1];
   strcpy(buffer, tgtString);
   char* strtok_state;
   char* tok = strtok_r(buffer, " ", &strtok_state);
   while ( tok != NULL ) {
      tgtPhrase.push_back(tgtVocab.add(tok));
      tok = strtok_r(NULL, " ", &strtok_state);
   }
}


void PhraseTable::write(ostream& multi_src_given_tgt_out)
{
   LOG_VERBOSE2(ptLogger, "PhraseTable::write multi-prob");
   // 9 digits is enough to keep all the precision of a float; but it's
   // artificial precision we don't need.
   multi_src_given_tgt_out.precision(9);
   vector<string> prefix;
   write(multi_src_given_tgt_out,
      textTable.begin_children(), textTable.end_children(), prefix);
} // write (public method)


void PhraseTable::write(ostream& multi_src_given_tgt_out, const string& src,
                        const TargetPhraseTable& tgt_phrase_table)
{
   LOG_VERBOSE2(ptLogger, "PhraseTable::write multi-prob one TargetPhraseTable");
   // Fix for [BUG 1067]
   multi_src_given_tgt_out.precision(9);

   TargetPhraseTable::const_iterator tgt_p;
   for ( tgt_p  = tgt_phrase_table.begin();
         tgt_p != tgt_phrase_table.end();
         ++tgt_p) {

      // construct tgt phrase
      const string tgt(getStringPhrase(tgt_p->first));

      // output
      multi_src_given_tgt_out << src << PHRASE_TABLE_SEP << tgt << PHRASE_TABLE_SEP;

      for (Uint i = 0; i < tgt_p->second.backward.size(); ++i) {
         multi_src_given_tgt_out << " " << convertToWrite(tgt_p->second.backward[i]);
      }
      for (Uint i(tgt_p->second.backward.size()); i<numTextTransModels; ++i) {
         multi_src_given_tgt_out << " " << 0.0f;
      }
      for (Uint i = 0; i < tgt_p->second.forward.size(); ++i) {
         multi_src_given_tgt_out << " " << convertToWrite(tgt_p->second.forward[i]);
      }
      for (Uint i(tgt_p->second.forward.size()); i<numTextTransModels; ++i) {
         multi_src_given_tgt_out << " " << 0.0f;
      }

      // Write all the annotations out
      tgt_p->second.annotations.write(multi_src_given_tgt_out);

      //boxing
      if (numAdirTransModels > 0 || !tgt_p->second.adir.empty()) {
         multi_src_given_tgt_out << PHRASE_TABLE_SEP;

         for (Uint i = 0; i < tgt_p->second.adir.size(); ++i)
            multi_src_given_tgt_out << " " << convertToWrite(tgt_p->second.adir[i]);

         for (Uint i(tgt_p->second.adir.size()); i < numTextAdirModels; ++i)
            multi_src_given_tgt_out << " " << 0.0f;
      }//boxing

      multi_src_given_tgt_out << endl;
   }
}


void PhraseTable::write(ostream& multi_src_given_tgt_out,
                        PTrie<TargetPhraseTable>::iterator it,
                        const PTrie<TargetPhraseTable>::iterator& end,
                        vector<string>& prefix)
{
   for ( ; it != end; ++it ) {
      prefix.push_back(tgtVocab.word(it.get_key()));

      if (it.is_leaf()) {
         // write contents of this node, if any

         // construct source phrase
         string src = join(prefix);

         write(multi_src_given_tgt_out, src, it.get_value());
      }

      if (it.has_children()) {
         // recurse over children, if any
         write(multi_src_given_tgt_out, it.begin_children(), it.end_children(), prefix);
      }

      prefix.pop_back();
   }
}

void PhraseTable::readLine(istream &in, string &ph1, string &ph2, string &prob,
   string &ascore, bool &blank, const char *fileName, Uint &lineNum)
{
   string line;
   // EJJ Pre-allocation of memory results in a significant speed-up:
   line.reserve(80);
   getline(in, line);

   if (line == "")
   {
      blank = true;
   } else
   {
      const Uint sep_len = strlen(PHRASE_TABLE_SEP);
      // Look for two or three occurrences of |||. //boxing
      lineNum++;
      const string::size_type index1 = line.find(PHRASE_TABLE_SEP, 0);
      const string::size_type index2 = line.find(PHRASE_TABLE_SEP, index1 + sep_len);
      if (index2 == string::npos)
      {
         error(ETFatal, "Bad format in %s at line %d", fileName, lineNum);
      }
      ph1 = line.substr(0, index1);
      trim(ph1, " ");
      ph2 = line.substr(index1 + sep_len,
                        index2 - index1 - sep_len);
      trim(ph2, " ");

      //boxing
      const string::size_type index3 = line.find(PHRASE_TABLE_SEP, index2 + sep_len - 1);
      if (index3 == string::npos)
      {
         prob = line.substr(index2 + sep_len);
         ascore = "";
      } else
      {
         if (index3 > index2 + sep_len)
            prob   = line.substr(index2 + sep_len,
                                 index3 - index2 - sep_len);
         else
            prob = ""; // empty 3rd column
         ascore = line.substr(index3 + sep_len);
      }
      trim(prob, " ");
      trim(ascore, " ");

      blank = false;
   }
} // readLine


void PhraseTable::addSourceSentences(const VectorPSrcSent& sentences)
{
   LOG_VERBOSE2(ptLogger, "Adding %d source sentences", sentences.size());
   for (Uint s(0); s<sentences.size(); ++s) {
      const vector<string>& src_sent = sentences[s]->src_sent;

      tgtVocab.addSentence(src_sent, s);

      // Add each sub sequence of the source sentence as a possible source phrase
      const Uint sent_no(s % VocabFilter::maxSourceSentence4filtering);
      assert(sent_no < VocabFilter::maxSourceSentence4filtering);

      const char* words[src_sent.size()];
      for (Uint i(0); i<src_sent.size(); ++i)
         words[i] = src_sent[i].c_str();

      for (Uint i(0); i<src_sent.size(); ++i)
         addPhrase(words + i, src_sent.size() - i, sent_no);
   }
}

void PhraseTable::addPhrase(const char * const *phrase, Uint phrase_len,
                            Uint sent_no)
{
   Uint uint_phrase[phrase_len];
   for ( Uint i = 0; i < phrase_len; ++i )
      uint_phrase[i] = tgtVocab.add(phrase[i]);
   for ( Uint len = phrase_len; len > 0; --len ) {
      TargetPhraseTable * tgtTable(NULL);
      textTable.find(uint_phrase, len, tgtTable);
      if (tgtTable == NULL) {
         TargetPhraseTable newTgtTable;
         newTgtTable.input_sent_set.resize(num_sents);
         assert(sent_no < num_sents);
         newTgtTable.input_sent_set[sent_no] = true;
         textTable.insert(uint_phrase, len, newTgtTable);
      } else {
         if (tgtTable->input_sent_set.empty())
            tgtTable->input_sent_set.resize(num_sents);
         assert(tgtTable->input_sent_set.size() == num_sents);
         assert(sent_no < num_sents);
         tgtTable->input_sent_set[sent_no] = true;
      }
   }
} // addPhrase

void PhraseTable::getPhraseInfos(vector<PhraseInfo *> **phraseInfos,
      const vector<string> &sent,
      const vector<double> &weights,
      Uint pruneSize,
      double logPruneThreshold,
      const vector<Range> &rangesToSkip,
      Uint verbosity,
      const vector<double> *forward_weights,
      const vector<double> *adir_weights)
{
   // Create a key to find phrases in the trie
   const char *s_curKey[sent.size()];
   Uint i_curKey[sent.size()];
   for (Uint i = 0; i < sent.size(); i++)
   {
      s_curKey[i] = sent[i].c_str();
      i_curKey[i] = tgtVocab.add(s_curKey[i]);
   }

   // Create an iterator to track which phrases not to look for.  Since we will
   // find phrases from the end of the sentence first, we iterate through the
   // ranges in reverse.
   vector<Range>::const_reverse_iterator rangeIt = rangesToSkip.rbegin();

   Uint numPrunedThreshold = 0;
   Uint numPrunedHist = 0;

   Range curRange;
   for (curRange.end = sent.size(); curRange.end > 0; curRange.end--)
   {
      // Proceed to the next applicable range
      while (rangeIt != rangesToSkip.rend() && rangeIt->start >= curRange.end)
      {
         rangeIt++;
      }

      for (curRange.start = (rangeIt == rangesToSkip.rend()) ? 0 : rangeIt->end;
           curRange.start < curRange.end; curRange.start++)
      {
         // get all possible phrases for current range form all tables
         shared_ptr<TargetPhraseTable> tgtTable =
            findInAllTables(sent, s_curKey, i_curKey, curRange);

         if (tgtTable.get() != NULL)
         {
            // TPLDM DEBUGGING STARTS
            if (false){
               cerr << "Range: " << curRange.toString() << nf_endl;
               cerr << "src: " << join(sent.begin()+curRange.start, sent.begin()+curRange.end) << nf_endl;
               cerr << "tgtTable: ";
               tgtTable->print(cerr, &tgtVocab);
            }
            // TPLDM DEBUGGING ENDS

            if (verbosity >= 4) {
               cerr << "Retrieving phrase table entries for "
                    << curRange.toString() << endl;
            }

            // Store all phrases into a vector, along with their weight.
            // Pruning based on the [ttable-threshold] parameter is done here.
            vector<pair<double, PhraseInfo *> > phrases;
            getPhrases(phrases, *tgtTable, curRange, numPrunedThreshold,
                       weights, logPruneThreshold, verbosity, forward_weights,
                       adir_weights); //boxing

            // Do phrase table pruning based on the [ttable-limit] parameter
            vector<PhraseInfo *> &target =
               phraseInfos[curRange.start][curRange.end - curRange.start - 1];
            if (pruningType == EXTERNAL_PRUNING || pruneSize == NO_SIZE_LIMIT ||
                phrases.size() <= pruneSize)
            {
               // With no prune size, store all the phrases
               for (vector<pair<double, PhraseInfo *> >::const_iterator it =
                    phrases.begin(); it != phrases.end(); it++)
               {
                  target.push_back(it->second);
               }
            } else {
               if (verbosity >= 4) {
                  cerr << "Pruning table entries (keeping only "
                       << pruneSize << ")" << endl;
               }
               // Use a heap to extract the pruneSize best items
               make_heap(phrases.begin(), phrases.end(), PhraseScorePairLessThan());
               for (Uint i = 0; i < pruneSize; i++)
               {
                  pop_heap(phrases.begin(), phrases.end(), PhraseScorePairLessThan());
                  if (verbosity >= 4) {
                     cerr << "\tKeeping " << getStringPhrase(phrases.back().second->phrase)
                          << " " << phrases.back().first << endl;
                  }
                  target.push_back(phrases.back().second);
                  phrases.pop_back();
               }

               // Delete the remaining items
               numPrunedHist += phrases.size();
               for (vector<pair<double, PhraseInfo *> >::iterator it =
                    phrases.begin(); it != phrases.end(); it++)
               {
                  delete it->second;
               }
            }
         }
         else if (verbosity >= 4) {
            cerr << "No phrase table entries for "
                 << curRange.toString() << endl;
         }
      }
   }
   // TODO: something with numPrunedThreshold, numPrunedHist
} // getPhraseInfos

shared_ptr<TargetPhraseTable> PhraseTable::findInAllTables(
   const vector<string>& str_key,
   const char* s_key[], const Uint i_key[], Range range
) {
   TargetPhraseTable *textTgtTable(NULL);
   if (!textTable.find(i_key + range.start, range.end - range.start, textTgtTable))
      textTgtTable = NULL;

   if (tpptTables.empty() && tpldmTables.empty()) {
      // If there are no TPPTs/TPLDMs, this function is a mere wrapper around
      // textTable.find().  We use NullDeleter as the custom deleter for the
      // returned shared_ptr, which does nothing and will therefore not affect
      // the in memory phrase table.
      return shared_ptr<TargetPhraseTable>(textTgtTable, NullDeleter());
   }

   // Structure to return
   shared_ptr<TargetPhraseTable> tgtTable(new TargetPhraseTable);
   assert(tgtTable);
   if (textTgtTable != NULL) {
      *tgtTable = *textTgtTable;
      for (TargetPhraseTable::iterator iter(tgtTable->begin());
           iter != tgtTable->end(); ++iter) {
         TScore &tScores(iter->second);
         assert(numTransModels >= tScores.backward.size());
         tScores.backward.resize(numTransModels, log_almost_0);
         if (forwardsProbsAvailable) {
            tScores.forward.resize(numTransModels, log_almost_0);
         }
      }
   }

   VectorPhrase tgtPhrase;

   // For each TPPT phrase table, find each tgt canditate in the TPPT phrase
   // table for src_phrase and merge it into tgtTable
   Uint prob_offset = numTextTransModels;
   Uint adir_offset = numTextAdirModels;
   for ( Uint i = 0; i < tpptTables.size(); ++i ) {
      const Uint numModels = tpptTables[i]->numThirdCol() / 2;
      const Uint numAdir = tpptTables[i]->numFourthCol();
      const Uint numCounts = tpptTables[i]->numCounts();
      const bool hasAlignments = tpptTables[i]->hasAlignments();
      assert(tpptTables[i]);
      TPPT::val_ptr_t targetPhrases =
         tpptTables[i]->lookup(str_key, range.start, range.end);
      if (targetPhrases) {
         // results are not empty.
         for ( vector<TPPT::TCand>::iterator
                  it(targetPhrases->begin()), end(targetPhrases->end());
               it != end; ++it ) {
            // Convert target phrase to global vocab
            tgtPhrase.resize(it->words.size());
            for ( Uint j = 0; j < tgtPhrase.size(); ++j )
               tgtPhrase[j] = tgtVocab.add(it->words[j].c_str());

            // merge and/or insert the values into tgtTable
            TScore* tScores(&(*tgtTable)[tgtPhrase]);
            assert(tScores);
            tScores->backward.resize(numTransModels, log_almost_0);
            if (forwardsProbsAvailable)
               tScores->forward.resize(numTransModels, log_almost_0);
            if (numAdir)
               tScores->adir.resize(numAdirTransModels, log_almost_0);

            assert(it->score.size() == 2*numModels+numAdir);
            for (Uint j = 0; j < numModels; ++j) {
               tScores->backward[prob_offset+j] = shielded_log(it->score[j]);
               if (forwardsProbsAvailable)
                  tScores->forward[prob_offset+j] = shielded_log(it->score[j+numModels]);
            }
            for (Uint j = 0; j < numAdir; ++j)
               tScores->adir[adir_offset+j] = shielded_log(it->score[2*numModels+j]);

            if (numCounts) {
               // Joint counts have a vector addition semantics, if they appear
               // in multiple phrase tables.
               assert(it->counts.size() == numCounts);
               CountAnnotation::getOrCreate(tScores->annotations)->updateValue(it->counts);
            }

            if (hasAlignments && !it->alignment.empty())
               tScores->annotations.initAnnotation("a", it->alignment.c_str());

            // Call all the registered annotators to do what they need with the phrase pair
            for (AnnotatorRegistry::iterator it(annotatorRegistry.begin()), end(annotatorRegistry.end());
                 it != end; ++it)
               it->second->annotate(tScores, s_key + range.start, range.size(), tgtPhrase);
         }
      }
      prob_offset += numModels;
      adir_offset += numAdir;
   }
   assert (prob_offset == numTransModels);

   // ZERO's value depends on the subclass's implementation of convertFromRead
   const float ZERO(convertFromRead(0.0f));

   // Lexicalized Distortion models.
   for (Uint tpldm = 0; tpldm < tpldmTables.size(); ++tpldm) {
      const Uint currentNumLexDisModels = numLexDisModels + tpldm*6;
      // Get all lexicalized distortion score for the source phrase.
      assert(range.start <= range.end);
      TPPT::val_ptr_t targetPhrases =
         tpldmTables[tpldm]->lookup(str_key, range.start, range.end);

      if (targetPhrases) {
         // Ok, this tpldm has some values for this source phrase, let's keep
         // the values for the target phrases that our cpts know about.
         TScore* tScores(NULL);
         for ( vector<TPPT::TCand>::iterator
                  it(targetPhrases->begin()), end(targetPhrases->end());
               it != end; ++it ) {
            // Convert target phrase to global vocab
            tgtPhrase.resize(it->words.size());
            for ( Uint w = 0; w < tgtPhrase.size(); ++w )
               tgtPhrase[w] = tgtVocab.index(it->words[w].c_str());

            // Is this a target phrase previously seen by the cpts (thus we
            // will want to keep this source/target phrase pair)?  If so,
            // attach the lexicalized distortion scores to that TScore.
            TargetPhraseTable::iterator tgt_iter = tgtTable->find(tgtPhrase);
            if (tgt_iter != tgtTable->end()) {
               tScores = &(tgt_iter->second);
               // If we have found the target phrase in the
               // TargetPhraseTable, there must be a tScore with it.
               assert(tScores != NULL);
               if (tScores->lexdis.size() > currentNumLexDisModels) {
                  error(ETWarn, "Entry src phrase %s appears to have the wrong number of lexical score",
                        join(str_key).c_str());
               }
               else {
                  tScores->lexdis.reserve(currentNumLexDisModels+6);
                  tScores->lexdis.resize(currentNumLexDisModels, ZERO);
                  for (Uint p(0); p<it->score.size(); ++p) {
                     // Make the probs log_probs.
                     tScores->lexdis.push_back(convertFromRead(it->score[p]));
                  }
                  assert(tScores->lexdis.size() == currentNumLexDisModels+6);
               }
            } //target phrase found.
         } // For every candidates in targetPhrases.
      } // If there are targetPhrases.
   } // Foreach tpldm.

   return tgtTable;
} // findInAllTables


void PhraseTable::getPhrases(vector<pair<double, PhraseInfo *> > &phrases,
   TargetPhraseTable &tgtTable, const Range &src_words, Uint &numPruned,
   const vector<double> &weights, double logPruneThreshold, Uint verbosity,
   const vector<double> *forward_weights, const vector<double> *adir_weights)
{
   for (TargetPhraseTable::iterator it = tgtTable.begin(); it != tgtTable.end(); it++)
   {
      // Compute the pruning score (formerly forward prob, but now depends on
      // pruningType) for the given translation option
      TScore &tscore(it->second);
      assert(numTransModels >= tscore.backward.size());
      tscore.backward.resize(numTransModels, log_almost_0);
      if (forwardsProbsAvailable) {
         assert(numTransModels >= tscore.forward.size());
         tscore.forward.resize(numTransModels, log_almost_0);
      }
      tscore.adir.resize(numAdirTransModels, log_almost_0); //boxing

      double pruningScore = 0;
      if (pruningType != EXTERNAL_PRUNING) {
         if (forwardsProbsAvailable) {
            if (forward_weights && pruningType == FORWARD_WEIGHTS) {
               pruningScore = dotProduct(*forward_weights, tscore.forward, forward_weights->size());
            } else if (forward_weights && pruningType == COMBINED_SCORE) {
               pruningScore = dotProduct(*forward_weights, tscore.forward, forward_weights->size())
                            + dotProduct(weights, tscore.backward, weights.size());
            } else {
               pruningScore = dotProduct(weights, tscore.forward, weights.size());
            }
         } else {
            pruningScore = dotProduct(weights, tscore.backward, weights.size());
         }
      }

      if (verbosity >= 4 && pruningType != EXTERNAL_PRUNING) {
         cerr << "\tConsidering " << src_words.toString()

              << " " << getStringPhrase(it->first) << " " << pruningScore;
      }
      if (pruningScore > logPruneThreshold || pruningType == EXTERNAL_PRUNING)
      {
         // If it passes threshold pruning, add it to the vector phrases.
         PhraseInfo* newPI = new PhraseInfo;
         if (forward_weights) {
            newPI->forward_trans_probs = tscore.forward;
            newPI->forward_trans_prob =
               dotProduct(*forward_weights, tscore.forward, forward_weights->size());
         } else
            newPI->forward_trans_prob = 0;

         if (adir_weights) {
            newPI->adir_probs = tscore.adir; //boxing
            newPI->adir_prob =
               dotProduct(*adir_weights, tscore.adir, adir_weights->size());   //boxing
         } else
            newPI->adir_prob = 0;

         newPI->lexdis_probs = tscore.lexdis; //boxing
         newPI->annotations  = tscore.annotations;

         newPI->src_words = src_words;
         newPI->phrase = it->first;
         newPI->phrase_trans_probs = tscore.backward;
         newPI->phrase_trans_prob = dotProduct(weights, tscore.backward, weights.size());
         phrases.push_back(make_pair(pruningScore, newPI));
         if (verbosity >= 4 && pruningType != EXTERNAL_PRUNING) cerr << " Keeping it" << endl;
      } else
      {
         numPruned++;
         if (verbosity >= 4) cerr << " Pruning it" << endl;
      } // if:
   }
} // getPhrases

Uint PhraseTable::PhraseScorePairLessThan::mHash(const string& param) const
{
   Uint hash = 0;
   const char* sz = param.c_str();
   for ( ; *sz; ++sz)
      hash = *sz + 5*hash;
   return ((hash>>16) + (hash<<16));
}

bool PhraseTable::PhraseScorePairLessThan::operator()(
   const pair<double, PhraseInfo *> ph1, const pair<double, PhraseInfo *> ph2
) const {
   // First comparison criterion: the score (forward probs)
   if (ph1.first < ph2.first) return true;
   else if (ph1.first > ph2.first) return false;
   else {
      // EJJ 2Feb2006: this code is replicated in tm/tmtext_filter.pl - if
      // you change it here, change it there too!!!

      // Second comparison criterion, in case of tie: the score backward probs
      // At first, the inequality may seem reversed, but it seems that the best
      // phrase is inversely correlated with its frequency as shown by
      // empirical results.
      if (ph1.second->phrase_trans_prob > ph2.second->phrase_trans_prob)
         return true;
      else if (ph1.second->phrase_trans_prob < ph2.second->phrase_trans_prob)
         return false;
      else {
         // For third criterion, hashing the phrase words to add some controlled randomness.
         const Voc* global_voc = GlobalVoc::get();
         const string s1 = phrase2string(ph1.second->phrase, *global_voc);
         const string s2 = phrase2string(ph2.second->phrase, *global_voc);
         const unsigned int h1 = mHash(s1);
         const unsigned int h2 = mHash(s2);
         if (h1 > h2) {
            return true;
         } else if (h1 < h2) {
            return false;
         } else {
            // Final tie breaker: ascii-betic sort
            return s1 < s2;
         }
      }
   }
} // phraseScorePairLessThan

bool PhraseTable::PhraseScorePairGreaterThan::operator()(
   const pair<double, PhraseInfo *>& ph1, const pair<double, PhraseInfo *>& ph2
) const {
   return PhraseScorePairLessThan::operator()(ph2, ph1);
}


vector<string>&
PhraseTable::getVectorStringPhrase(const Phrase& p, vector<string>& res) const
{
   res.resize(p.size());
   for (Uint i = 0; i < p.size(); ++i)
      res[i] = tgtVocab.word(p[i]);
   return res;
}

// partially redundant with some code in readFile(), but kept separate to avoid
// de-optimizing that method.
bool PhraseTable::containsSrcPhrase(Uint num_tokens, char* tokens[])
{
   Uint srcWords[num_tokens];
   for (Uint i = 0; i < num_tokens; ++i)
      if ((srcWords[i] = tgtVocab.index(tokens[i])) == tgtVocab.size())
         return false;
   TargetPhraseTable *tgtTable = NULL;
   return textTable.find(srcWords, num_tokens, tgtTable);
}


Uint PhraseTable::processTargetPhraseTable(const string&, Uint, TargetPhraseTable*)
{
   return 0;
}
