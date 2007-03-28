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
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "phrasetable.h"
#include "canoe_general.h"
#include "phrasedecoder_model.h"
#include <portage_defs.h>
#include <str_utils.h>
#include <file_utils.h>
#include <math.h>
#include <vector>
#include <utility>
#include <algorithm>
#include <map>
#include <ext/hash_map>
#include <string>
#include <voc.h>
#include <boost/shared_ptr.hpp>

using namespace std;
using namespace Portage;
using namespace __gnu_cxx;

const char *Portage::PHRASE_TABLE_SEP = " ||| ";

PhraseTable::PhraseTable(Voc &tgtVocab, const char* pruningTypeStr) :
   tgtVocab(tgtVocab), numTransModels(0), forwardsProbsAvailable(true)
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
                       bool limitPhrases,
                       ostream *const src_filtered,
                       ostream *const tgt_filtered)
{
   readFile(src_given_tgt_file, src_given_tgt, limitPhrases, src_filtered);

   ostringstream description;
   description << "TranslationModel:" << src_given_tgt_file << endl;
   backwardDescription += description.str();

   if (tgt_given_src_file != NULL)
   {
      readFile(tgt_given_src_file, tgt_given_src, limitPhrases, tgt_filtered);

      ostringstream description;
      description << "ForwardTranslationModel:" << tgt_given_src_file << endl;
      forwardDescription += description.str();
   } else
   {
      forwardsProbsAvailable = false;
   } // if
   numTransModels++;
} // read

// Optimized implementation of readFile() - though uglier, it is almost twice as
// fast as the implementation above.
Uint PhraseTable::readFile(const char *file, dir d, bool limitPhrases, ostream * const filtered_output)
{
   IMagicStream in(file);
   if ( filtered_output )
      cerr << "filtering phrase table " << file << flush;
   else
      cerr << "loading phrase table from " << file << flush;
   time_t start_time = time(NULL);

   char* tgt;
   char* src;
   char* probString;
   Uint lineNum = 0;
   Uint numKept = 0;
   const Uint sep_len = strlen(PHRASE_TABLE_SEP);

   // Avoid unecessary alloc/realloc cycle by declaring tgtPhrase and line
   // outside the loop.
   Phrase tgtPhrase;
   string line;
   char* tokens[1000]; // 1000 word-long phrase is considered "infinite"

   // For multi-prob phrase tables, we keep here the number of columns in the table
   Uint multi_prob_col_count = 0;
   Uint multi_prob_model_count = 0; // == multi_prob_col_count / 2

   Uint zero_prob_err_count = 0;

   // Don't look the tgtTable up again if src hasn't changed
   string prev_src;
   TargetPhraseTable *prev_tgtTable = NULL;
   while (!in.eof())
   {
      getline(in, line);
      if (line == "") continue;
      lineNum++;

      // Look for two occurrences of |||.
      string::size_type index1 = line.find(PHRASE_TABLE_SEP, 0);
      string::size_type index2 = line.find(PHRASE_TABLE_SEP, index1 + sep_len);
      if (index2 == string::npos)
      {
         error(ETFatal, "Bad format in %s at line %d", file, lineNum);
      } // if

      // Copy line into a buffer we can safely parse destructively.
      char line_buffer[line.size()+1];
      strcpy(line_buffer, line.c_str());
      line_buffer[index1] = '\0';
      src = trim(line_buffer, " ");
      line_buffer[index2] = '\0';
      tgt = trim(line_buffer + index1 + sep_len);
      probString = trim(line_buffer + index2 + sep_len);

      if (lineNum % 10000 == 0)
      {
         cerr << '.' << flush;
      }
      if (d == tgt_given_src || d == multi_prob_reversed)
      {
         // Account for the order of phrases in the table
         std::swap(src, tgt);
      }

      TargetPhraseTable *tgtTable = NULL;
      if ( src == prev_src ) {
         tgtTable = prev_tgtTable;
      } else {
         prev_src = src;

         // Tokenize source
         // Destructive split: src becomes the memory holder for tokens.
         Uint src_word_count = split(src, tokens, 1000, " ");
         if ( src_word_count > 1000 )
            error(ETFatal, "Exceeded system limit of 1000-word long phrases");
         Uint srcWords[src_word_count];
         for (Uint i = 0; i < src_word_count; i++)
            srcWords[i] = tgtVocab.add(tokens[i]);

         // Find in table
         if (limitPhrases) {
            if ( ! textTable.find(srcWords, src_word_count, tgtTable) )
               tgtTable = NULL;
         } else {
            textTable.find_or_insert(srcWords, src_word_count, tgtTable);
         }

         prev_tgtTable = tgtTable;
      }

      if (tgtTable != NULL) // i.e., if src in trie
      {
         // Tokenize target and add to tgt_vocab
         tgtPhrase.clear();
         tgtStringToPhrase(tgtPhrase, tgt);

         // Are we filtering => output kept prob and don't load it in memory
         if (filtered_output != NULL) {
            *filtered_output << line << endl;
            ++numKept;
         }
         else {
            if ( d == src_given_tgt || d == tgt_given_src ) {
               // Determine probability for single-prob phrase table
               float prob;
               if (!conv(probString, prob))
               {
                  error(ETFatal, "Invalid number format (%s) in %s at line %d",
                        probString, file, lineNum);
               } // if

               // Get the vector of current probabilities
               vector<float> *curProbs;
               if (d == src_given_tgt)
               {
                  curProbs = &( (*tgtTable)[tgtPhrase].backward );
               } else
               {
                  curProbs = &( (*tgtTable)[tgtPhrase].forward );
               } // if

               if (curProbs->size() > numTransModels)
               {
                  error(ETWarn, "Entry %s ||| %s appears more than once in %s; new occurrence on line %d",
                        src, tgt, file, lineNum);
               } else
               {
                  numKept++;

                  //assert(curProbs->size() <= numTransModels);
                  curProbs->resize(numTransModels, LOG_ALMOST_0);
                  if ( prob <= 0 ) {
                     zero_prob_err_count++;
                     curProbs->push_back(LOG_ALMOST_0);
                  } else {
                     curProbs->push_back(log(prob));
                  }
               } // if
            } else {
               // Determine probabilities for multi-prob phrase table

               // On the first line, we need to figure out the number of
               // probability figures
               if (multi_prob_col_count == 0) {
                  // We only do this once, so it can be slow
                  vector<string> prob_tokens;
                  split(probString, prob_tokens);
                  multi_prob_col_count = prob_tokens.size();
                  multi_prob_model_count = multi_prob_col_count / 2;
                  if ( multi_prob_col_count % 2 != 0 )
                     error(ETFatal, "Multi-prob phrase table %s has an odd number of probability figures, must have matching backward and forward probs",
                        file);
                  if ( multi_prob_col_count == 0 )
                     error(ETFatal, "Multi-prob phrase table %s has no probability figures, ill formatted",
                        file);
               }

               char* prob_tokens[multi_prob_col_count];
               // fast, destructive split
               Uint actual_count = split(probString, prob_tokens, multi_prob_col_count);
               if ( actual_count != multi_prob_col_count )
                  error(ETFatal, "Wrong number of probabilities (%d instead of %d) in %s at line %d",
                        actual_count, multi_prob_col_count, file, lineNum);

               float probs[multi_prob_col_count];
               for ( Uint i = 0 ; i < multi_prob_col_count; ++i ) {
                  if ( ! conv(prob_tokens[i], probs[i] ) )
                     error(ETFatal, "Invalid number format (%s) in %s at line %d",
                           prob_tokens[i], file, lineNum);

                  if ( probs[i] <= 0 ) {
                     zero_prob_err_count++;
                     probs[i] = LOG_ALMOST_0;
                  } else {
                     probs[i] = log(probs[i]);
                  }
               }

               float *forward_probs, *backward_probs;
               if ( d == multi_prob_reversed ) {
                  backward_probs = &(probs[multi_prob_model_count]);
                  forward_probs = &(probs[0]);
               } else {
                  backward_probs = &(probs[0]);
                  forward_probs = &(probs[multi_prob_model_count]);
               }

               // Get the vectors of current probabilities
               vector<float> *curBackProbs = &( (*tgtTable)[tgtPhrase].backward );
               vector<float> *curForProbs = &( (*tgtTable)[tgtPhrase].forward );

               if (curBackProbs->size() > numTransModels ||
                   curForProbs->size() > numTransModels)
               {
                  error(ETWarn, "Entry %s ||| %s appears more than once in %s; new occurrence on line %d",
                        src, tgt, file, lineNum);
               } else
               {
                  numKept++;

                  curBackProbs->reserve(numTransModels + multi_prob_model_count);
                  curForProbs->reserve(numTransModels + multi_prob_model_count);
                  curBackProbs->resize(numTransModels, LOG_ALMOST_0);
                  curForProbs->resize(numTransModels, LOG_ALMOST_0);
                  curBackProbs->insert(curBackProbs->end(), backward_probs,
                                       backward_probs + multi_prob_model_count);
                  curForProbs->insert(curForProbs->end(), forward_probs,
                                      forward_probs + multi_prob_model_count);
               } // if
            } // if ; end deal with multi-prob line
         } // if ; end deal with probs
      } // if (tgtTable != NULL)
   } // while(!in.eof())
   cerr << lineNum << " lines read, " << numKept << " entries kept.  Done in "
        << (time(NULL) - start_time) << "s" << endl;

   if ( zero_prob_err_count ) {
      error(ETWarn, "%d 0 or negative probabilities found in %s - treated as missing entries",
         zero_prob_err_count, file);
   }

   if ( multi_prob_col_count == 0 )
      return 1;
   else
      return multi_prob_col_count;
} // readFile

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
   bool limitPhrases, ostream *const filtered)
{
   string physical_filename;
   bool reversed = isReversed(multi_prob_TM_filename, &physical_filename);
   Uint model_count = countProbColumns(multi_prob_TM_filename) / 2;

   ostringstream back_description;
   ostringstream for_description;
   for (Uint i = 0; i < model_count; ++i) {
      back_description << "TranslationModel:" << multi_prob_TM_filename
         << "(col=" << (reversed ? i + model_count : i) << ")" << endl;
      for_description << "ForwardTranslationModel:" << multi_prob_TM_filename
         << "(col=" << (reversed ? i : i + model_count) << ")" << endl;
   }
   backwardDescription += back_description.str();
   forwardDescription += for_description.str();

   Uint col_count = readFile(physical_filename.c_str(),
                             (reversed ? multi_prob_reversed : multi_prob ),
                             limitPhrases, filtered);

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
            string tgt;
            const Phrase& phrase = tgt_p->first;
            for (Uint i = 0; i < phrase.size(); ++i) {
               tgt += tgtVocab.word(phrase[i]);
               if (i + 1 < phrase.size()) tgt += " ";
            }

            // output
            if (src_given_tgt_out) {
               (*src_given_tgt_out) << src << PHRASE_TABLE_SEP << tgt << PHRASE_TABLE_SEP;
               for (Uint i = 0; i < tgt_p->second.backward.size(); ++i) {
                  (*src_given_tgt_out) << exp(tgt_p->second.backward[i]);
                  if (i+1 < tgt_p->second.backward.size()) (*src_given_tgt_out) << ":";
               }
               // EJJ: Output "ALMOST_0" for missing entries
               for (Uint i = tgt_p->second.backward.size(); i < numTransModels; ++i) {
                  if (i > 0) (*src_given_tgt_out) << ":";
                  (*src_given_tgt_out) << exp(LOG_ALMOST_0);
                  error(ETWarn, "Entry %s ||| %s missing from backward table",
                     src.c_str(), tgt.c_str());
               }
               (*src_given_tgt_out) << endl;
            }
            if (tgt_given_src_out && forwardsProbsAvailable) {
               (*tgt_given_src_out) << tgt << PHRASE_TABLE_SEP << src << PHRASE_TABLE_SEP;
               for (Uint i = 0; i < tgt_p->second.forward.size(); ++i) {
                  (*tgt_given_src_out) << exp(tgt_p->second.forward[i]);
                  if (i+1 < tgt_p->second.forward.size()) (*tgt_given_src_out) << ":";
               }
               // EJJ: Output "ALMOST_0" for missing entries
               for (Uint i = tgt_p->second.forward.size(); i < numTransModels; ++i) {
                  if (i > 0) (*tgt_given_src_out) << ":";
                  (*tgt_given_src_out) << exp(LOG_ALMOST_0);
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
      string::size_type index1 = line.find(PHRASE_TABLE_SEP, 0);
      string::size_type index2 = line.find(PHRASE_TABLE_SEP, index1 + strlen(PHRASE_TABLE_SEP));
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

void PhraseTable::addPhrase(const char * const *phrase, Uint phrase_len)
{
   Uint uint_phrase[phrase_len];
   for ( Uint i = 0; i < phrase_len; ++i )
      uint_phrase[i] = tgtVocab.add(phrase[i]);
   for ( Uint len = phrase_len; len > 0; --len )
      textTable.insert(uint_phrase, len, TargetPhraseTable());
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
      // Mark the end of the key
      //Map_noKey(curKey[curRange.end]);

      // Proceed to the next applicable range
      while (rangeIt != rangesToSkip.rend() && rangeIt->start >= curRange.end)
      {
         rangeIt++;
      } // while

      for (curRange.start = (rangeIt == rangesToSkip.rend()) ? 0 : rangeIt->end;
           curRange.start < curRange.end; curRange.start++)
      {
         shared_ptr<TargetPhraseTable> tgtTable =
            findInAllTables(s_curKey, i_curKey, curRange);
         //TargetPhraseTable *tgtTable = textTable.find(curKey + curRange.start);
         if (tgtTable.get() != NULL)
         {
            if ( verbosity >= 4 ) {
               cerr << "Retrieving phrase table entries for "
                    << curRange.toString() << endl;
            }

            // Store all phrases into a vector, along with their weight
            vector<pair<double, PhraseInfo *> > phrases;
            getPhrases(phrases, *tgtTable, curRange, numPrunedThreshold,
                       weights, logPruneThreshold, verbosity, forward_weights);

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
            } else
            {
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


// returns log(x) unless x <= 0, in which case returns LOG_ALMOST_0
namespace Portage{
   inline double shielded_log (double x) {
      if ( x <= 0 ) return LOG_ALMOST_0;
      else return log(x);
   }
}

shared_ptr<TargetPhraseTable> PhraseTable::findInAllTables(
   const char* s_key[], const Uint i_key[], Range range
) {
   TargetPhraseTable *textTgtTable;
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
      // Compute the forwards probability for the given translation option
      TScore &tScores(it->second);
      assert(numTransModels >= tScores.backward.size());
      tScores.backward.resize(numTransModels, LOG_ALMOST_0);
      double pruningScore;
      if (forwardsProbsAvailable) {
         assert(numTransModels >= tScores.forward.size());
         tScores.forward.resize(numTransModels, LOG_ALMOST_0);

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


bool PhraseTable::getPhrasePair(const string& src, const string& tgt, TScore& score)
{
   TargetPhraseTable *tgtTable = NULL;

   vector<string> srcWordsV;
   split(src, srcWordsV, " ");
   /*
   const char *srcWords[srcWordsV.size()];
   for (Uint i = 0; i < srcWordsV.size(); i++)
      srcWords[i] = srcWordsV[i].c_str();
   */
   Uint srcWords[srcWordsV.size()];
   for (Uint i = 0; i < srcWordsV.size(); i++)
      srcWords[i] = tgtVocab.index(srcWordsV[i].c_str());

   bool found = false;

   // Search for the phrase pair in the phrase tables
   if ( textTable.find(srcWords, srcWordsV.size(), tgtTable) ) {
      vector<string> tgtWordsV;
      split(tgt, tgtWordsV, " ");
      Phrase tgtPhrase(tgtWordsV.size());
      bool hasOutOfVocab = false;
      for (Uint i = 0; i < tgtWordsV.size(); i++) {
         tgtPhrase[i] = tgtVocab.index(tgtWordsV[i].c_str());
         if (tgtPhrase[i] == tgtVocab.size()) {
            hasOutOfVocab = true;
            break;
         }
      }
      if ( !hasOutOfVocab ) {
         TargetPhraseTable::iterator p = tgtTable->find(tgtPhrase);
         if (p != tgtTable->end()) {
            found = true;
            score = p->second;
         }
      }
   }

   if ( !found ) score.clear();

   return found;
} // getPhrasePair

bool PhraseTable::PhraseScorePairLessThan::operator()(
   pair<double, PhraseInfo *> ph1, pair<double, PhraseInfo *> ph2
) {
   // First comparison criterion: the score
   if ( ph1.first < ph2.first ) return true;
   else if ( ph1.first > ph2.first ) return false;
   else {
      // EJJ 2Feb2006: this code is replicated in tm/tmtext_filter.pl - if
      // you change it here, change it there too!!!

      // Second comparison criterion, in case of tie: the phrase length
      if ( ph1.second->phrase.size() < ph2.second->phrase.size() ) return true;
      else if ( ph1.second->phrase.size() > ph2.second->phrase.size() ) return false;
      else {
         // Final tie breaker: ascii-betic sort
         return parent.getStringPhrase(ph1.second->phrase) <
                parent.getStringPhrase(ph2.second->phrase);
      }
   }
} // phraseScorePairLessThan

string PhraseTable::getStringPhrase(const Phrase &uPhrase)
{
   ostringstream s;
   if (uPhrase.size() == 0) return s.str();
   Uint i = 0;
   while (true)
   {
      assert(uPhrase[i] < tgtVocab.size());
      s << tgtVocab.word(uPhrase[i]);
      ++i;
      if (i == uPhrase.size())
      {
         break;
      } // if
      s << " ";
   } // while
   return s.str();
} // getStringPhrase



