/**
 * @author Aaron Tikuisis
 * @file phrasetable.cc  This file contains the implementation of the
 * PhraseTable class, which uses a trie to store mappings from source phrases
 * to target phrases.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "phrasetable.h"
#include "str_utils.h"
#include "file_utils.h"
#include "voc.h"
#include <cmath>
#include <algorithm>
#include <map>


using namespace std;
using namespace Portage;

const char *Portage::PHRASE_TABLE_SEP = " ||| ";
double PhraseTable::log_almost_0 = LOG_ALMOST_0;

PhraseTable::PhraseTable(VocabFilter& _tgtVocab, const char* pruningTypeStr) :
   tgtVocab(_tgtVocab),
   numTransModels(0),
   forwardsProbsAvailable(true),
   num_sents(tgtVocab.getNumSourceSents())
{
   if (pruningTypeStr == NULL) {
      pruningType = FORWARD_WEIGHTS;
      //error(ETWarn, "No pruning type");
   } else if (0 == strcmp(pruningTypeStr, "forward-weights")) {
      pruningType = FORWARD_WEIGHTS;
   } else if (0 == strcmp(pruningTypeStr, "backward-weights")) {
      pruningType = BACKWARD_WEIGHTS;
   } else if (0 == strcmp(pruningTypeStr, "combined")) {
      pruningType = COMBINED_SCORE;
   } else {
      assert(false);
   }
}


PhraseTable::~PhraseTable() {}

void PhraseTable::read(const char *src_given_tgt_file,
                       const char *tgt_given_src_file,
                       bool limitPhrases)
{
   readFile(src_given_tgt_file, src_given_tgt, limitPhrases);

   ostringstream description;
   description << "TranslationModel:" << src_given_tgt_file << endl;
   backwardDescription += description.str();

   if (tgt_given_src_file != NULL) {
      readFile(tgt_given_src_file, tgt_given_src, limitPhrases);

      ostringstream description;
      description << "ForwardTranslationModel:" << tgt_given_src_file << endl;
      forwardDescription += description.str();
   }
   else {
      forwardsProbsAvailable = false;
   }
   ++numTransModels;
} // read


TargetPhraseTable* PhraseTable::getTargetPhraseTable(const Entry& entry, bool limitPhrases)
{
   TargetPhraseTable *tgtTable = NULL;
   char* tokens[1000]; // 1000 word-long phrase is considered "infinite"

   // Tokenize source
   // Destructive split: src becomes the memory holder for tokens.
   const Uint src_word_count = split(entry.src, tokens, 1000, " ");
   if(!(src_word_count > 0)) {
      error(ETWarn, "\nSuspicious entry in %s, there is no source in: %s\n",
            entry.file, entry.line->c_str());
      return NULL;
   }
   if ( src_word_count > 1000 )
      error(ETFatal, "Exceeded system limit of 1000-word long phrases");
   Uint srcWords[src_word_count];

   // Find in table
   if (limitPhrases) {
      bool contains_unknown_word = false;
      for (Uint i = 0; i < src_word_count; i++) {
         srcWords[i] = tgtVocab.index(tokens[i]);
         if (srcWords[i] == tgtVocab.size()) {
            contains_unknown_word = true;
            break;
         }
      }
      if ( contains_unknown_word ||
            ! textTable.find(srcWords, src_word_count, tgtTable) )
         tgtTable = NULL;
   }
   else {
      for (Uint i = 0; i < src_word_count; i++)
         srcWords[i] = tgtVocab.add(tokens[i]);
      textTable.find_or_insert(srcWords, src_word_count, tgtTable);
   }

   return tgtTable;
}


// Optimized implementation of readFile() - though uglier, it is almost twice
// as fast as the previous implementation.
Uint PhraseTable::readFile(const char *file, dir d, bool limitPhrases)
{
   Uint numKept(0);
   Uint numFiltered(0);
   Entry entry(d, file);
   iSafeMagicStream in(file);

   cerr << "loading phrase table from " << file << ": " << flush;
   time_t start_time = time(NULL);

   const Uint sep_len = strlen(PHRASE_TABLE_SEP);

   // Avoid unecessary alloc/realloc cycle by declaring tgtPhrase and line
   // outside the loop.
   string line;

   // Don't look the tgtTable up again if src hasn't changed
   string prev_src;
   TargetPhraseTable *prev_tgtTable = NULL;
   while (!in.eof())
   {
      getline(in, line);
      if (line == "") continue;
      ++entry.lineNum;

      // Look for two occurrences of |||.
      const string::size_type index1 = line.find(PHRASE_TABLE_SEP, 0);
      const string::size_type index2 = line.find(PHRASE_TABLE_SEP, index1 + sep_len);
      if (index2 == string::npos) {
         error(ETFatal, "Bad format in %s at line %d", file, entry.lineNum);
      }

      // Copy line into a buffer we can safely parse destructively.
      char line_buffer[line.size()+1];
      strcpy(line_buffer, line.c_str());
      entry.line = &line;

      line_buffer[index1] = '\0';
      entry.src        = trim(line_buffer, " ");
      line_buffer[index2] = '\0';
      entry.tgt        = trim(line_buffer + index1 + sep_len);
      entry.probString = trim(line_buffer + index2 + sep_len);

      if (entry.lineNum % 10000 == 0) {
         cerr << '.' << flush;
      }

      if (d == tgt_given_src || d == multi_prob_reversed) {
         // Account for the order of phrases in the table
         std::swap(entry.src, entry.tgt);
      }

      TargetPhraseTable *tgtTable = NULL;
      if ( entry.src == prev_src ) {
         tgtTable = prev_tgtTable;
      }
      else {
         //cerr << "NEW entry " << entry.src << " line " << entry.line << endl; //SAM DEBUG
         numFiltered += processTargetPhraseTable(prev_src, prev_tgtTable);

         prev_src      = entry.src;
         tgtTable      = getTargetPhraseTable(entry, limitPhrases);
         prev_tgtTable = tgtTable;
      }

      if (tgtTable != NULL) // i.e., if src in trie
      {
         // Tokenize target and add to tgt_vocab
         entry.tgtPhrase.clear();
         tgtStringToPhrase(entry.tgtPhrase, entry.tgt);
         if (entry.tgtPhrase.empty()) {
            error(ETWarn, "\nSuspicious entry in %s, there is no target in: %s\n",
		  entry.file, entry.line->c_str());
            continue;
         }
         if ( limitPhrases ) {
            tgtVocab.addPerSentenceVocab(entry.tgtPhrase, &tgtTable->input_sent_set);
         }

         // Adds entry to the trie's leaf
         if (processEntry(tgtTable, entry)) ++numKept;
      }
   }

   // In online processing we process after all the entries for a particular
   // source phrase were read thus it is necessary to process the last source
   // phrase as a particular case.
   numFiltered += processTargetPhraseTable(prev_src, prev_tgtTable);

   cerr << endl << entry.lineNum << " lines read, " << numKept << " entries kept.";
   if ( numFiltered > 0 )
      cerr << endl << (numKept - numFiltered) << "entries remaining after filtering.";
   cerr << endl << "Done in " << (time(NULL) - start_time) << "s" << endl;

   if ( entry.zero_prob_err_count ) {
      error(ETWarn, "%d 0 or negative probabilities found in %s - treated as missing entries",
         entry.zero_prob_err_count, file);
   }
   in.close();

   switch (d)
   {
      case src_given_tgt:
      case tgt_given_src:
         return 1;
      break;
      case multi_prob:
      case multi_prob_reversed:
         // Sometimes processing won't have counted columns - in that case we
         // do it here.
         if (entry.multi_prob_col_count == 0)
            return countProbColumns(file);
         else
            return entry.multi_prob_col_count;
      break;
      default:
         assert(false);
   }
   return 0;
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


bool PhraseTable::processEntry(TargetPhraseTable* tgtTable, Entry& entry)
{
   // ZERO's value depends on the subclass's implementation of convertFromRead
   const float ZERO(convertFromRead(0.0f));
   if ( entry.d == src_given_tgt || entry.d == tgt_given_src ) {
      // Determine probability for single-prob phrase table
      float prob;
      if (!conv(entry.probString, prob)) {
         error(ETFatal, "Invalid number format (%s) in %s at line %d",
               entry.probString, entry.file, entry.lineNum);
      }

      // Prevents against a bad phrase table.
      if (!isfinite(prob)) {
         error(ETWarn, "Invalid value of prob (%s) in %s at line %d",
               entry.probString, entry.file, entry.lineNum);
         prob = 0.0f;
      }

      // Get the vector of current probabilities
      vector<float> *curProbs;
      if (entry.d == src_given_tgt) {
         curProbs = &( (*tgtTable)[entry.tgtPhrase].backward );
      } else {
         curProbs = &( (*tgtTable)[entry.tgtPhrase].forward );
      }

      if (curProbs->size() > numTransModels)
      {
         error(ETWarn, "Entry %s ||| %s appears more than once in %s; new occurrence on line %d",
               entry.src, entry.tgt, entry.file, entry.lineNum);
      }
      else {
         //assert(curProbs->size() <= numTransModels);
         curProbs->resize(numTransModels, ZERO);
         if ( prob <= 0 ) {
            curProbs->push_back(ZERO);
            ++entry.zero_prob_err_count;
         }
         else {
            curProbs->push_back(convertFromRead(prob));
         }
      }
   }
   else {
      // Determine probabilities for multi-prob phrase table

      // On the first line, we need to figure out the number of
      // probability figures
      if (entry.multi_prob_col_count == 0) {
         // We only do this once, so it can be slow
         vector<string> prob_tokens;
         split(entry.probString, prob_tokens);
         entry.multi_prob_col_count = prob_tokens.size();
         if ( entry.multi_prob_col_count % 2 != 0 )
            error(ETFatal, "Multi-prob phrase table %s has an odd number of probability figures, must have matching backward and forward probs",
               entry.file);
         if ( entry.multi_prob_col_count == 0 )
            error(ETFatal, "Multi-prob phrase table %s has no probability figures, ill formatted",
               entry.file);
      }
      const Uint multi_prob_model_count(entry.multi_prob_col_count / 2); // == entry.multi_prob_col_count / 2

      char* prob_tokens[entry.multi_prob_col_count];
      // fast, destructive split
      const Uint actual_count = split(entry.probString, prob_tokens, entry.multi_prob_col_count);
      if ( actual_count != entry.multi_prob_col_count )
         error(ETFatal, "Wrong number of probabilities (%d instead of %d) in %s at line %d",
               actual_count, entry.multi_prob_col_count, entry.file, entry.lineNum);

      float probs[entry.multi_prob_col_count];
      for ( Uint i = 0 ; i < entry.multi_prob_col_count; ++i ) {
         if ( ! conv(prob_tokens[i], probs[i] ) )
            error(ETFatal, "Invalid number format (%s) in %s at line %d",
                  prob_tokens[i], entry.file, entry.lineNum);

         // Prevents against a bad phrase table.
         if (!isfinite(probs[i])) {
            error(ETWarn, "Invalid value of prob (%s) in %s at line %d",
                  prob_tokens[i], entry.file, entry.lineNum);
            probs[i] = 0.0f;
         }

         if ( probs[i] <= 0 ) {
            probs[i] = ZERO;
            ++entry.zero_prob_err_count;
         }
         else {
            probs[i] = convertFromRead(probs[i]);
         }
      }

      float *forward_probs, *backward_probs;
      if ( entry.d == multi_prob_reversed ) {
         backward_probs = &(probs[multi_prob_model_count]);
         forward_probs = &(probs[0]);
      }
      else {
         backward_probs = &(probs[0]);
         forward_probs = &(probs[multi_prob_model_count]);
      }

      // Get the vectors of current probabilities
      TScore* curProbs = &( (*tgtTable)[entry.tgtPhrase] );

      if (curProbs->backward.size() > numTransModels ||
          curProbs->forward.size() > numTransModels)
      {
         error(ETWarn, "Entry %s ||| %s appears more than once in %s; new occurrence on line %d",
               entry.src, entry.tgt, entry.file, entry.lineNum);
      }
      else {
         curProbs->backward.reserve(numTransModels + multi_prob_model_count);
         curProbs->forward.reserve(numTransModels + multi_prob_model_count);
         curProbs->backward.resize(numTransModels, ZERO);
         curProbs->forward.resize(numTransModels, ZERO);
         curProbs->backward.insert(curProbs->backward.end(), backward_probs,
                                   backward_probs + multi_prob_model_count);
         curProbs->forward.insert(curProbs->forward.end(), forward_probs,
                                  forward_probs + multi_prob_model_count);
      }
   } // if ; end deal with multi-prob line

   // In this class, the entry is always added to the TargetPhraseTable.
   return true;
}


bool PhraseTable::isReversed(const string& multi_prob_TM_filename,
                             string* physical_filename)
{
   static const string rev_suffix("#REVERSED");
   const Uint size(multi_prob_TM_filename.size());
   if ( size > rev_suffix.size() &&
        multi_prob_TM_filename.substr(size-rev_suffix.size()) == rev_suffix ) {
      if ( physical_filename )
         *physical_filename =
            multi_prob_TM_filename.substr(0, size - rev_suffix.size());
      return true;
   } else {
      if ( physical_filename )
         *physical_filename = multi_prob_TM_filename;
      return false;
   }
} // PhraseTable::isReversed()


Uint PhraseTable::countProbColumns(const char* multi_prob_TM_filename)
{
   string physical_filename;
   isReversed(multi_prob_TM_filename, &physical_filename);
   iMagicStream in(physical_filename, true);  // make this a silent stream
   if ( in.bad() ) return 0;
   string ph1, ph2, prob_str;
   bool blank = true;
   Uint lineNo;
   // This reads one line and returns the list of probs into prob
   while ( blank && !in.eof() )
      readLine(in, ph1, ph2, prob_str, blank, multi_prob_TM_filename, lineNo);
   if ( blank ) {
      error(ETWarn, "No data lines found in multi-prob phrase table %s",
            multi_prob_TM_filename);
      return 0;
   }
   vector<string> probs;
   split(prob_str, probs);
   return probs.size();
}


Uint PhraseTable::readMultiProb(const char* multi_prob_TM_filename,
   bool limitPhrases)
{
   string physical_filename;
   bool reversed = isReversed(multi_prob_TM_filename, &physical_filename);

   const Uint col_count = readFile(physical_filename.c_str(),
                           (reversed ? multi_prob_reversed : multi_prob ),
                           limitPhrases);

   const Uint model_count = col_count / 2;
   assert(col_count>0);
   assert(model_count>0);

   ostringstream back_description;
   ostringstream for_description;
   for (Uint i = 0; i < model_count; ++i) {
      back_description << "TranslationModel:" << multi_prob_TM_filename
         << "(col=" << (reversed ? i + model_count : i) << ")" << endl;
      for_description << "ForwardTranslationModel:" << multi_prob_TM_filename
         << "(col=" << (reversed ? i : i + model_count) << ")" << endl;
   }
   backwardDescription += back_description.str();
   forwardDescription  += for_description.str();

   numTransModels += model_count;

   return col_count;
}


void PhraseTable::tgtStringToPhrase(Phrase& tgtPhrase, const char* tgtString)
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


void PhraseTable::write(ostream* src_given_tgt_out, ostream* tgt_given_src_out)
{
   // 9 digits is enough to keep all the precision of a float
   if ( src_given_tgt_out ) src_given_tgt_out->precision(9);
   if ( tgt_given_src_out ) tgt_given_src_out->precision(9);
   vector<string> prefix;
   write(src_given_tgt_out, tgt_given_src_out,
         textTable.begin_children(), textTable.end_children(), prefix);
} // write (public method)

void PhraseTable::write(ostream& multi_src_given_tgt_out)
{
   // 9 digits is enough to keep all the precision of a float
   multi_src_given_tgt_out.precision(9);
   vector<string> prefix;
   write(multi_src_given_tgt_out,
      textTable.begin_children(), textTable.end_children(), prefix);
} // write (public method)


void PhraseTable::write(ostream& multi_src_given_tgt_out, const string& src,
                        const TargetPhraseTable& tgt_phrase_table)
{
   TargetPhraseTable::const_iterator tgt_p;
   for ( tgt_p  = tgt_phrase_table.begin();
         tgt_p != tgt_phrase_table.end();
         ++tgt_p) {

      // construct tgt phrase
      string tgt(getStringPhrase(tgt_p->first));

      // output
      multi_src_given_tgt_out << src << PHRASE_TABLE_SEP << tgt << PHRASE_TABLE_SEP;
      for (Uint i = 0; i < tgt_p->second.backward.size(); ++i) {
         multi_src_given_tgt_out << " " << convertToWrite(tgt_p->second.backward[i]);
      }
      for (Uint i(tgt_p->second.backward.size()); i<numTransModels; ++i) {
         multi_src_given_tgt_out << " " << 0.0f;
      }
      for (Uint i = 0; i < tgt_p->second.forward.size(); ++i) {
         multi_src_given_tgt_out << " " << convertToWrite(tgt_p->second.forward[i]);
      }
      for (Uint i(tgt_p->second.forward.size()); i<numTransModels; ++i) {
         multi_src_given_tgt_out << " " << 0.0f;
      }
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

      if ( it.is_leaf() ) {
         // write contents of this node, if any

         // construct source phrase
         string src;
         join(prefix.begin(), prefix.end(), src);

         write(multi_src_given_tgt_out, src, it.get_value());
      }

      if ( it.has_children() ) {
         // recurse over children, if any
         write(multi_src_given_tgt_out, it.begin_children(), it.end_children(), prefix);
      }

      prefix.pop_back();
   }
}

void PhraseTable::write(ostream* src_given_tgt_out, ostream* tgt_given_src_out,
                        PTrie<TargetPhraseTable>::iterator it,
                        const PTrie<TargetPhraseTable>::iterator& end,
                        vector<string>& prefix)
{
   for ( ; it != end; ++it ) {
      prefix.push_back(tgtVocab.word(it.get_key()));

      if ( it.is_leaf() ) {
         // write contents of this node, if any

         // construct source phrase
         string src;
         join(prefix.begin(), prefix.end(), src);

         const TargetPhraseTable& tgt_phrase_table = it.get_value();
         TargetPhraseTable::const_iterator tgt_p;
         for ( tgt_p = tgt_phrase_table.begin();
               tgt_p != tgt_phrase_table.end();
               ++tgt_p) {

            // construct tgt phrase
            string tgt(getStringPhrase(tgt_p->first));

            // output
            if (src_given_tgt_out) {
               (*src_given_tgt_out) << src << PHRASE_TABLE_SEP << tgt << PHRASE_TABLE_SEP;
               for (Uint i = 0; i < tgt_p->second.backward.size(); ++i) {
                  (*src_given_tgt_out) << convertToWrite(tgt_p->second.backward[i]);
                  if (i+1 < tgt_p->second.backward.size()) (*src_given_tgt_out) << ":";
               }
               for (Uint i = tgt_p->second.backward.size(); i < numTransModels; ++i) {
                  if (i > 0) (*src_given_tgt_out) << ":";
                  (*src_given_tgt_out) << 0.0f;
                  error(ETWarn, "Entry %s ||| %s missing from backward table",
                        src.c_str(), tgt.c_str());
               }
               (*src_given_tgt_out) << endl;
            }
            if (tgt_given_src_out && forwardsProbsAvailable) {
               (*tgt_given_src_out) << tgt << PHRASE_TABLE_SEP << src << PHRASE_TABLE_SEP;
               for (Uint i = 0; i < tgt_p->second.forward.size(); ++i) {
                  (*tgt_given_src_out) << convertToWrite(tgt_p->second.forward[i]);
                  if (i+1 < tgt_p->second.forward.size()) (*tgt_given_src_out) << ":";
               }
               for (Uint i = tgt_p->second.forward.size(); i < numTransModels; ++i) {
                  if (i > 0) (*tgt_given_src_out) << ":";
                  (*tgt_given_src_out) << 0.0f;
                  error(ETWarn, "Entry %s ||| %s missing from forward table",
                     tgt.c_str(), src.c_str());
               }
               (*tgt_given_src_out) << endl;
            }
         }
      }

      if ( it.has_children() ) {
         // recurse over children, if any
         write(src_given_tgt_out, tgt_given_src_out,
               it.begin_children(), it.end_children(), prefix);
      }

      prefix.pop_back();
   }
} // write (private recursive method)


void PhraseTable::readLine(istream &in, string &ph1, string &ph2, string &prob,
   bool &blank, const char *fileName, Uint &lineNum)
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
      // Look for (at least) two occurrences of |||.
      lineNum++;
      const string::size_type index1 = line.find(PHRASE_TABLE_SEP, 0);
      const string::size_type index2 = line.find(PHRASE_TABLE_SEP, index1 + strlen(PHRASE_TABLE_SEP));
      if (index2 == string::npos)
      {
         error(ETFatal, "Bad format in %s at line %d", fileName, lineNum);
      } // if

      ph1 = line.substr(0, index1);
      trim(ph1, " ");
      ph2 = line.substr(index1 + strlen(PHRASE_TABLE_SEP),
                        index2 - index1 - strlen(PHRASE_TABLE_SEP));
      trim(ph2, " ");
      prob = line.substr(index2 + strlen(PHRASE_TABLE_SEP));
      trim(prob, " ");
      blank = false;
   } // if
} // readLine

void PhraseTable::addSourceSentences(const vector<vector<string> >& sentences)
{
   tgtVocab.addSentences(sentences);
   for (Uint s(0); s<sentences.size(); ++s) {
      const vector<string>& src_sent = sentences[s];
      const Uint sent_no(s % VocabFilter::maxSourceSentence4filtering);
      assert(sent_no < VocabFilter::maxSourceSentence4filtering);

      const char* words[src_sent.size()];
      for (Uint i(0); i<src_sent.size(); ++i) {
         words[i] = src_sent[i].c_str();
      }

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
      if ( tgtTable == NULL ) {
         TargetPhraseTable newTgtTable;
         newTgtTable.input_sent_set.resize(num_sents);
         assert(sent_no < num_sents);
         newTgtTable.input_sent_set[sent_no] = true;
         textTable.insert(uint_phrase, len, newTgtTable);
      } else {
         if ( tgtTable->input_sent_set.empty() )
            tgtTable->input_sent_set.resize(num_sents);
         assert(tgtTable->input_sent_set.size() == num_sents);
         assert(sent_no < num_sents);
         tgtTable->input_sent_set[sent_no] = true;
      }
   }
} // addPhrase

void PhraseTable::getPhraseInfos(vector<PhraseInfo *> **phraseInfos, const
   vector<string> &sent, const vector<double> &weights, Uint pruneSize, double
   logPruneThreshold, const vector<Range> &rangesToSkip, Uint verbosity,
   const vector<double> *forward_weights)
{
   // Create a key to find phrases in the trie
   const char *s_curKey[sent.size()];
   Uint i_curKey[sent.size()];
   for (Uint i = 0; i < sent.size(); i++)
   {
      s_curKey[i] = sent[i].c_str();
      i_curKey[i] = tgtVocab.add(s_curKey[i]);
   } // for

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
            findInAllTables(s_curKey, i_curKey, curRange);

         if (tgtTable.get() != NULL)
         {
            if ( verbosity >= 4 ) {
               cerr << "Retrieving phrase table entries for "
                    << curRange.toString() << endl;
            }

            // Store all phrases into a vector, along with their weight.
            // Pruning based on the [ttable-threshold] parameter is done here.
            vector<pair<double, PhraseInfo *> > phrases;
            getPhrases(phrases, *tgtTable, curRange, numPrunedThreshold,
                       weights, logPruneThreshold, verbosity, forward_weights);

            // Do phrase table pruning based on the [ttable-limit] parameter
            vector<PhraseInfo *> &target =
               phraseInfos[curRange.start][curRange.end - curRange.start - 1];
            if (pruneSize == NO_SIZE_LIMIT || phrases.size() <= pruneSize)
            {
               // With no prune size, store all the phrases
               for (vector<pair<double, PhraseInfo *> >::const_iterator it =
                    phrases.begin(); it != phrases.end(); it++)
               {
                  target.push_back(it->second);
               } // for
            } else {
               if ( verbosity >= 4 ) {
                  cerr << "Pruning table entries (keeping only "
                       << pruneSize << ")" << endl;
               }
               // Use a heap to extract the pruneSize best items
               make_heap(phrases.begin(), phrases.end(), PhraseScorePairLessThan(*this));
               for (Uint i = 0; i < pruneSize; i++)
               {
                  pop_heap(phrases.begin(), phrases.end(), PhraseScorePairLessThan(*this));
                  if ( verbosity >= 4 ) {
                     cerr << "\tKeeping " << getStringPhrase(phrases.back().second->phrase)
                          << " " << phrases.back().first << endl;
                  }
                  target.push_back(phrases.back().second);
                  phrases.pop_back();
               } // for

               // Delete the remaining items
               numPrunedHist += phrases.size();
               for (vector<pair<double, PhraseInfo *> >::iterator it =
                    phrases.begin(); it != phrases.end(); it++)
               {
                  delete it->second;
               } // for
            } // if
         } // if
         else if ( verbosity >= 4 ) {
            cerr << "No phrase table entries for "
                 << curRange.toString() << endl;
         }
      } // for
   } // for
   // TODO: something with numPrunedThreshold, numPrunedHist
} // getPhraseInfos

shared_ptr<TargetPhraseTable> PhraseTable::findInAllTables(
   const char* s_key[], const Uint i_key[], Range range
) {
   TargetPhraseTable *textTgtTable(NULL);
   if ( ! textTable.find(i_key + range.start, range.end - range.start, textTgtTable) )
      textTgtTable = NULL;
   return shared_ptr<TargetPhraseTable>(textTgtTable, NullDeleter());
} // findInAllTables


void PhraseTable::getPhrases(vector<pair<double, PhraseInfo *> > &phrases,
   TargetPhraseTable &tgtTable, const Range &src_words, Uint &numPruned,
   const vector<double> &weights, double logPruneThreshold, Uint verbosity,
   const vector<double> *forward_weights)
{
   for (TargetPhraseTable::iterator it = tgtTable.begin(); it != tgtTable.end(); it++)
   {
      // Compute the pruning score (formerly forward prob, but now depends on
      // pruningType) for the given translation option
      TScore &tScores(it->second);
      assert(numTransModels >= tScores.backward.size());
      tScores.backward.resize(numTransModels, log_almost_0);
      double pruningScore;
      if (forwardsProbsAvailable) {
         assert(numTransModels >= tScores.forward.size());
         tScores.forward.resize(numTransModels, log_almost_0);

         if (forward_weights && pruningType == FORWARD_WEIGHTS) {
            pruningScore = dotProduct(*forward_weights, tScores.forward, forward_weights->size());
         } else if (forward_weights && pruningType == COMBINED_SCORE) {
            pruningScore = dotProduct(*forward_weights, tScores.forward, forward_weights->size())
                         + dotProduct(weights, tScores.backward, weights.size());
         } else {
            pruningScore = dotProduct(weights, tScores.forward, weights.size());
         }
      } else {
         pruningScore = dotProduct(weights, tScores.backward, weights.size());
      } // if

      if ( verbosity >= 4 ) {
         cerr << "\tConsidering " << src_words.toString()
              << " " << getStringPhrase(it->first) << " " << pruningScore;
      }
      if (pruningScore > logPruneThreshold)
      {
         // If it passes threshold pruning, add it to the vector phrases.
         MultiTransPhraseInfo *newPI;
         if (forward_weights) {
            ForwardBackwardPhraseInfo* newFBPI = new ForwardBackwardPhraseInfo;
            newFBPI->forward_trans_probs = tScores.forward;
            newFBPI->forward_trans_prob =
               dotProduct(*forward_weights, tScores.forward, forward_weights->size());
            newPI = newFBPI;
         } else {
            newPI = new MultiTransPhraseInfo;
         }
         newPI->src_words = src_words;
         newPI->phrase = it->first;
         newPI->phrase_trans_probs = tScores.backward;
         newPI->phrase_trans_prob = dotProduct(weights, tScores.backward, weights.size());
         phrases.push_back(make_pair(pruningScore, newPI));
         if ( verbosity >= 4 ) cerr << " Keeping it" << endl;
      } else
      {
         numPruned++;
         if ( verbosity >= 4 ) cerr << " Pruning it" << endl;
      } // if:
   } // for
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
   const pair<double, PhraseInfo *>& ph1, const pair<double, PhraseInfo *>& ph2
) const {
   // First comparison criterion: the score (forward probs)
   if ( ph1.first < ph2.first ) return true;
   else if ( ph1.first > ph2.first ) return false;
   else {
      // EJJ 2Feb2006: this code is replicated in tm/tmtext_filter.pl - if
      // you change it here, change it there too!!!

      // Second comparison criterion, in case of tie: the score backward probs
      // At first, the inequality may seem reversed, but it seems that the best
      // phrase is inversely correlated with its frequency as shown by
      // empirical results.
      if ( ph1.second->phrase_trans_prob > ph2.second->phrase_trans_prob )
         return true;
      else if ( ph1.second->phrase_trans_prob < ph2.second->phrase_trans_prob )
         return false;
      else {
         // For third criterion, hashing the phrase words to add some controlled randomness.
         const string s1 = parent.getStringPhrase(ph1.second->phrase);
         const string s2 = parent.getStringPhrase(ph2.second->phrase);
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
   if ( PhraseScorePairLessThan::operator()(ph1, ph2) )
      return false; // < holds
   else if ( ph1.first == ph2.first &&
             ph1.second->phrase == ph2.second->phrase )
      return false; // == holds
   else
      return true;  // > holds
}

string PhraseTable::getStringPhrase(const Phrase &uPhrase) const
{
   ostringstream s;
   bool first(true);
   for ( Phrase::const_iterator it(uPhrase.begin());
         it != uPhrase.end(); ++it ) {
      if ( first ) first = false; else s << " ";
      s << tgtVocab.word(*it);
   }
   return s.str();
} // getStringPhrase


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


Uint PhraseTable::processTargetPhraseTable(const string& src, TargetPhraseTable* tgtTable)
{
   return 0;
}
