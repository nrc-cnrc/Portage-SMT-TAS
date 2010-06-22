/**
 * @author Eric Joanis
 * @file lmtext.cc - in memory representation for a language model.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lmtext.h"
#include "file_utils.h"

using namespace Portage;
using namespace std;


LMText::LMText(const string& lm_file_name, VocabFilter *vocab,
               OOVHandling oov_handling, double oov_unigram_prob,
               bool limit_vocab, Uint limit_order,
               ostream *const os_filtered, bool quiet)
   : LMTrie(vocab, oov_handling, oov_unigram_prob)
{
   assert(vocab);
   read(lm_file_name, limit_vocab, limit_order, os_filtered, quiet);
   //dump_trie_to_cerr();
}


void LMText::read(const string& lm_file_name, bool limit_vocab,
                  Uint limit_order, ostream *const os_filtered, bool quiet)
{
   oSafeMagicStream* tmp_os_filtered = NULL;
   string tmp_fileN;
   if (os_filtered != NULL)
   {
      if (!quiet) cerr << "Filtering LM " << lm_file_name << endl;
      tmp_fileN = extractFilename(lm_file_name) + ".tmp.filtering";
      const char* const portage = getenv("TMPDIR");
      if (portage != NULL) {
         tmp_fileN = string(portage) + string("/") + tmp_fileN;
      }
      else {
         tmp_fileN = string("/tmp/") + tmp_fileN;
      }
      ostringstream pid;
      pid << "." << getpid();
      tmp_fileN += pid.str();
      tmp_os_filtered = new oSafeMagicStream(tmp_fileN);
      assert(tmp_os_filtered != NULL);
   }

   time_t start_overall = time(NULL);
   vector<Uint> ngram_counts;
   iSafeMagicStream in(lm_file_name);
   string line;
   // Look for \data\ line
   while (getline(in, line)) {
      if ( line == "\\data\\" ) break;
   }
   if ( in.eof() )
      error(ETFatal, "EOF before \\data\\ section in %s", lm_file_name.c_str());

   // read the blocks showing how many lines to expect for each order
   while (getline(in, line)) {
      line = trim(line);
      if ( line.substr(0,1) == "\\" )
         break;
      if ( line == "" )
         continue;
      Uint order;
      Uint count;
      if ( sscanf(line.c_str(), "ngram %u=%u", &order, &count) != 2 )
         error(ETFatal, "Bad \\data\\ section in %s: %s not in 'ngram N=M' format",
            lm_file_name.c_str(), line.c_str());

      if ( order != ngram_counts.size() + 1 )
         error(ETFatal, "Bad \\data\\ section in %s: %s should have been %u-gram counts",
            lm_file_name.c_str(), line.c_str(), (ngram_counts.size() + 1));
      ngram_counts.push_back(count);
   }

   gram_order = ngram_counts.size();
   if ( limit_order && limit_order < gram_order ) {
      error(ETWarn, "Treating order %d LM %s as order %d",
         gram_order, lm_file_name.c_str(), limit_order);
      gram_order = limit_order;
   }
   vocab->setMaxNgram(getOrder());
   vocab->resetDiscardingCount();
   hits.init(getOrder());

   //cerr << trie.getSizeOfs() << endl;

   vector<Uint> sentenceCount(ngram_counts.size());
   // Now for each order N we expect, go down to the \N-grams: section and read
   // it
   for (Uint order = 1; order <= getOrder(); ++order) {
      time_t start = time(NULL);
      if (!quiet) cerr << "Reading " << ngram_counts[order-1] << " " << order << "-grams.";
      if ( in.eof() )
         error(ETFatal, "EOF before \\%u-grams: sections in %s",
            lm_file_name.c_str(), order);
      stringstream ss;
      ss << "\\" << order << "-grams:";
      const string ngram_line = ss.str();

      do {
         line = trim(line);
         if ( line == ngram_line ) break;
      } while (getline(in, line));

      if ( in.eof() )
         error(ETFatal, "EOF before \\%u-grams: sections in %s",
            lm_file_name.c_str(), order);

      if (os_filtered != NULL && tmp_os_filtered != NULL)
         *tmp_os_filtered << endl << line << endl;

      // Now we can use readLine to read all the order-gram lines
      float prob;
      Uint reversed_phrase[order];
      float bo_wt;
      bool bo_present;
      while(!in.eof()
         && readLine(in, prob, order, reversed_phrase, bo_wt, bo_present,
                  limit_vocab, tmp_os_filtered)) {
         // if we are not filtering add to trie
         if (os_filtered == NULL)
         {
            trie.insert(reversed_phrase, order, prob);

            // Insert the back-off weight into the internal node value, but only
            // if absolutely necessary, i.e., if
            //  1) it was on the input line AND
            //  2) it wasn't 0 (the default value when not present) AND
            //  3) the LM order isn't limited to order or less.
            if ( bo_present && bo_wt != 0.0 && order < gram_order )
               trie.set_internal_node_value(reversed_phrase, order, bo_wt);
         }
         ++sentenceCount[order-1];  // Counting the number of kept lines
      }

      if (!quiet) cerr << " ... kept: " << sentenceCount[order-1] << " in " << (time(NULL) - start) << "s." << endl;

      // Useful statistics, but expensive to calculate - do only for optimizing
      // memory structures!  Or maybe it could be included in verbose 2 output.
      //cerr << trie.getStats() << endl;
   }
   if (!quiet) cerr << "LM loading completed in: " << (time(NULL) - start_overall) << "s." << endl;
   // This following is good for knowing how much is pruned during loading.
   // Manages per ngrams per technic pruning of lm_phrases.
   //vocab->printDiscardingCount();  // MAINLY FOR DEBUGGING

   // If we are filtering the LM we must cat the proper header with the proper
   // stats with the tmp_file containing the kept lines.
   if (os_filtered != NULL)
   {
      if (tmp_os_filtered) delete tmp_os_filtered;

      // Building the Header
      *os_filtered << "\\data\\" << endl;

      for (Uint i(0); i<sentenceCount.size(); ++i)
         *os_filtered << "ngram " << i+1 << "=" << sentenceCount[i] << endl;
      *os_filtered << endl;

      // Quickly copies the header + temp file in the final output
      iSafeMagicStream  tmp_is_filtered(tmp_fileN);
      *os_filtered << tmp_is_filtered.rdbuf();
      delete_if_exists(tmp_fileN.c_str(), "Deleting tmp file used during filtering (NORMAL message)");

      *os_filtered << endl << "\\end\\" << endl;
   }
} // LMText::read


// Ugly, yes, but must be lightning fast, so too bad, that's the way it is!
// This function gave me a 33% speed-up of LM load time over the PLM::readLine
// alternative, which makes it well worth the ugly code.
bool LMText::readLine(
   istream &in, float &prob, Uint order, Uint phrase[/*order*/],
   float &bo_wt, bool &bo_present, bool limit_vocab,
   ostream* const os_filtered
) {
   static string line;
   // Loop until a line with all known vocab is found (only once unless
   // limit_vocab is set).  If limit_vocab and per_sentence_vocab != NULL, the
   // intersection of sentence sets for each word must also be non empty.
   for(;;) {
      if (in.peek() == '\\') {
         //cerr << "Next n" << endl;  // SAM DEBUG
         return false;
      }

      const bool eof = ! getline(in, line);
      //cerr << "READ: " << line << endl;
      if (eof || line.empty()) {
         continue;
      } else {
         // This was done using split, but that's way too slow -- so we do it
         // the old but fast C way instead.
         // WARNING: this code writes over the const char* returned by
         // line.c_str().  This is OK because we don't touch line again until
         // the next getline(), which resets it.
         char* phrase_pos;

         // read the prob at the beginning of the line
         char buffer[line.size()+1];
         strcpy(buffer, line.c_str());
         prob = strtof(buffer, &phrase_pos);
         if (phrase_pos[0] != '\t')
            error(ETWarn, "Expected tab character after prob in LM input file");
         else
            phrase_pos++;

         // find the back off weight at the end of the line and, at the same
         // time, the end of the phrase
         char* bo_wt_pos = strchr(phrase_pos, '\t');
         if ( bo_wt_pos == NULL ) {
            bo_present = false;
            bo_wt = 0.0;
         } else {
            bo_present = true;
            bo_wt = strtof(bo_wt_pos+1, NULL);
            bo_wt_pos[0] = '\0'; // null terminate phrase so we can use strtok_r
         }

         // convert the phrase into a Uint array
         char* strtok_state; // state variable for strtok_r
         Uint tok_count = 0;
         char* s_tok = strtok_r(phrase_pos, " ", &strtok_state);
         bool order_error(false);
         while (s_tok != NULL) {
            ++tok_count;
            if ( tok_count > order ) {
               error(ETWarn,
                  "Ignoring N-gram of higher order in %d-gram section: %s",
                  order, line.c_str());
               order_error = true;
               break;
            }
            if ( limit_vocab ) {
               phrase[order-tok_count] = vocab->index(s_tok);
            }
            else {
               phrase[order-tok_count] = vocab->add(s_tok);
            }

            s_tok = strtok_r(NULL, " ", &strtok_state);
         } // while
         if ( order_error ) continue;

         // If we found an OOV or fair another filtering test, we skip this
         // entry and read another line
         if ( limit_vocab && !vocab->keepLMentry(phrase, tok_count, order) ) {
            continue;
         }

         if (os_filtered != 0) *os_filtered << line << endl;

         if ( tok_count != order ) {
            error(ETWarn, "Ignoring N-gram of order %d in %d-gram section: %s",
               tok_count, order, line.c_str());
            continue;
         }

         // Either limit_vocab is false, or we found no OOV, so we're done
         break;
      } // else clause - i.e., getline got a non-blank line
   } // for(;;)
   /*
   cerr << "PARSED: prob=" << prob << " order=" << order << " bo_wt=" << bo_wt
        << " blank=" << blank << " bo_present=" << bo_present << endl;
   cerr << "PHRASE:";
   for ( Uint i = 0; i < order; ++i )
      cerr << " " << phrase[i] << "/" << vocab->word(phrase[i]) ;
   cerr << endl;
   */

   return true;
} // LMText::readLine
