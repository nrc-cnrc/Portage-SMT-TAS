/**
 * @author Eric Joanis
 * @file lmtext.cc - in memory representation for a language model.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lmtext.h"
#include <file_utils.h>
#include <stdio.h>
#include <stdlib.h>
//#include <dmalloc.h>

using namespace Portage;
using namespace std;

//template<class F, class S>
//pair<F,S> operator+(const pair<F,S> &x, const pair<F,S> &y) {
//   return make_pair<F,S>(x.first + y.first, x.second + y.second);
//}

//template<class F, class S>
//ostream& operator<<(ostream &os, const pair<F,S> &x) {
//   os << "<" << x.first << "," << x.second << ">";
//   return os;
//}


float LMText::wordProb(Uint word, const Uint context[], Uint context_length)
{
   // Reminder: context is backwards!
   // This is desirable since we work backwards in the trie.

   TrieKeyT query[context_length+1];
   query[0] = word;
   for (Uint i = 0; i < context_length; ++i)
      query[i+1] = context[i];

   // For open-vocabulary language models, map unknown words to UNK_Symbol
   if ( complex_open_voc_lm && vocab ) {
      Uint UNK_index = vocab->index(UNK_Symbol);
      float dummy;
      for (Uint i = 0; i < context_length + 1; ++i)
         if ( ! trie.find(query+i, 1, dummy) )
            query[i] = UNK_index;
   }

   return wordProbQuery(query, context_length+1);
} // LMText::wordProb()

float LMText::wordProbQuery(const Uint query[], Uint query_length) {
   assert(query_length > 0);

   // p(w|context) is defined recursively as follows:
   // p(w|w1,..,wn) =
   //    if (exists "w1,..,wn,w")    prob("w1,..,wn,w")
   //    else if (exists "w1,..,wn") bo("w1,..,wn") + p(w|w2,..,wn)
   //    else                        p(w|w2,..,wn)
   // with bottom of the recursion defined as:
   // p(w) = if (exists "w") prob(w) else oov_unigram_prob;

   // We can simplify the recursive rule by definig bo("w1,..,wn") = 0 whenever
   // "w1,..,wn" doesn't exist.  Then we get:
   // p(w|w1,..,wn) =
   //    if (exists "w1,..,wn,w")    prob("w1,..,wn,w")
   //    else                        bo("w1,..,wn") + p(w|w2,..,wn)

   // Let i be the largest value such that "wi,..,wn,w" exists (with i==n+1
   // meaning either that only "w" exists or that even "w" doesn't exist).
   // Let prob("wi,..,w") be p("w") as defined above when i==n+1.
   // We can then unroll the recursion and obtain the following:
   //    p(w|w1,..,wn) = prob("wi,..,w") + sum j=1..i-1 (bo("wj,..,wn"))

   // The above formula is what is implemented below.

   float prob(0);
   Uint depth;
   if ( trie.find(query, query_length, prob, &depth) ) {
      return prob;
   } else {
      // if depth==0, we need to handle oov_unigram_prob here
      if ( depth == 0 ) {
         prob = oov_unigram_prob;
         depth = 1;
      }

      // depth == length of longest found prefix, i.e., n+1 - (i-1) = n+2-i
      // context_length == n
      // so i = n+2-depth = context_length + 2 - depth
      // but we don't need to calculate this, since j = 1 .. i-1 is more simply
      // expressed as bo_depth = context_length down to depth

      Uint bo_min_depth = depth;
      Uint bo_max_depth = query_length - 1;
      float bo_sum_value = trie.sum_internal_node_values(
         query+1, bo_min_depth, bo_max_depth);

      return
         prob +          // prob("wi,..,w")
         bo_sum_value;   // sum j=1..i-1 (bo("wj,..,wn"))
   }
} // LMText::wordProbQuery

LMText::LMText(Voc *vocab, OOVHandling oov_handling,
               double oov_unigram_prob)
   : PLM(vocab, oov_handling, oov_unigram_prob)
{ }

Uint LMText::global_index(Uint local_index) {
   return local_index;
}


LMText::LMText(const string& lm_file_name, Voc *vocab,
               OOVHandling oov_handling, double oov_unigram_prob,
               bool limit_vocab, Uint limit_order,
               ostream *const os_filtered, bool quiet)
   : PLM(vocab, oov_handling, oov_unigram_prob)
   , trie(12)
{
   assert(vocab);
   read(lm_file_name, limit_vocab, limit_order, os_filtered, quiet);
}

void LMText::read(const string& lm_file_name, bool limit_vocab,
                  Uint limit_order, ostream *const os_filtered, bool quiet)
{
   OMagicStream* tmp_os_filtered = NULL;
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
      tmp_os_filtered = new OMagicStream(tmp_fileN);
      assert(tmp_os_filtered != NULL);
   }

   time_t start_overall = time(NULL);
   vector<Uint> ngram_counts;
   IMagicStream in(lm_file_name);
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
      vector<string> tokens;
      split(line, tokens);
      if ( tokens.size() != 2 || tokens[0] != "ngram" ||
           2 != sscanf(tokens[1].c_str(), "%u=%u", &order, &count) )
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

   //cerr << trie.getSizeOfs() << endl;

   vector<Uint> sentenceCount(ngram_counts.size());
   // Now for each order N we expect, go down to the \N-grams: section and read
   // it
   for (Uint order = 1; order <= ngram_counts.size(); ++order) {
      time_t start = time(NULL);
      if (!quiet) cerr << "Reading " << ngram_counts[order-1] << " " << order << "-grams.";
      if ( in.eof() )
         error(ETFatal, "EOF before \\%u-grams: sections in %s",
            lm_file_name.c_str(), order);
      stringstream ss;
      ss << "\\" << order << "-grams:";
      string ngram_line = ss.str();

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
      bool blank;
      bool bo_present;
      while(!in.eof()) {
         readLine(in, prob, order, reversed_phrase, bo_wt, blank, bo_present,
                  limit_vocab, tmp_os_filtered);
         if ( blank ) break;
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
      IMagicStream  tmp_is_filtered(tmp_fileN);
      *os_filtered << tmp_is_filtered.rdbuf();
      delete_if_exists(tmp_fileN.c_str(), "Deleting tmp file used during filtering (NORMAL message)");

      *os_filtered << endl << "\\end\\" << endl;
   }
} // LMText::read

// Ugly, yes, but must be lightning fast, so too bad, that's the way it is!
// This function gave me a 33% speed-up of LM load time over the PLM::readLine
// alternative, which makes it well worth the ugly code.
void LMText::readLine(
   istream &in, float &prob, Uint order, Uint phrase[/*order*/],
   float &bo_wt, bool &blank, bool &bo_present, bool limit_vocab,
   ostream* const os_filtered
) {
   static string line;
   // Loop until a line with all known vocab is found (only once unless
   // limit_vocab is set)
   for(;;) {
      bool eof = ! getline(in, line);
      //cerr << "READ: " << line << endl;
      if (eof || line == "") {
         blank = true;
         return;
      } else {
         // This was done using split, but that's way too slow -- so we do it
         // the old but fast C way instead.
         // WARNING: this code writes over the const char* returned by
         // line.c_str().  This is OK because we don't touch line again until
         // the next getline(), which resets it.
         blank = false;
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
         bool oov_found = false;
         while (s_tok != NULL) {
            ++tok_count;
            if ( tok_count > order )
               error(ETFatal,
                  "N-gram of greater order in %d-gram section: %s ,%d, %d",
                  order, line.c_str(), tok_count, order);
            if ( limit_vocab ) {
               phrase[order-tok_count] = vocab->index(s_tok);
               if ( phrase[order-tok_count] == vocab->size() ) {
                  // OOV found - skip this line
                  oov_found = true;
                  break;
               }
            }
            else {
               phrase[order-tok_count] = vocab->add(s_tok);
            }

            s_tok = strtok_r(NULL, " ", &strtok_state);
         } // while

         // If we found an OOV, we have to try again
         if ( oov_found ) {
            assert (limit_vocab);
            continue;
         }

         if (os_filtered != 0) *os_filtered << line << endl;

         if ( tok_count != order )
            error(ETFatal, "N-gram of order %d in %d-gram section",
               tok_count, order);

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
} // LMText::readLine


void LMText::write_binary(const string& binlm_file_name) const
{
   cerr << trie.getStats() << endl;

   // We use a regular output file stream because we need a seekable output
   // stream.  Can be compressed separately, after, if desired.
   ofstream ofs(binlm_file_name.c_str());
   if (!ofs)
      error(ETFatal, "unable to open %s for writing", binlm_file_name.c_str());

   // Write the "magic number" (or string...)
   ofs << "Portage BinLM file, format v1.0" << endl;

   // Write out the order of the model
   ofs << "Order = " << gram_order << endl;

   // Write out the vocabulary in plain text
   ofs << "Vocab size = " << vocab->size() << endl;
   vocab->write(ofs);
   ofs << endl;

   // Write out the trie itself - this part of the file is binary
   Uint nodes_written = trie.write_binary(ofs);

   ofs << endl << "End of Portage BinLM file.  Internal node count="
       << nodes_written << endl;

   cerr << "Wrote out " << nodes_written << " internal nodes" << endl;

} // LMText::write_binary

