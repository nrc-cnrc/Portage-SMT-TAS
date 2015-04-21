/**
 * @author George Foster
 * @file ngram_counts.h Read in SRILM-format ngram counts file, and provide
 * access to contents.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#ifndef __NGRAM_COUNTS_H__
#define __NGRAM_COUNTS_H__

#include "portage_defs.h"
#include "vocab_filter.h"
#include "trie.h"
#include <vector>

using namespace std;

namespace Portage
{

class NgramCounts
{
   Voc* voc;
   PTrie<Uint,Empty,false> trie;
   Uint maxlen;                 // longest ngram stored

public:

   /**
    * Construct from the contents of a file.
    * @param voc must point to existing Voc object - will add entries
    * @param file ngram count file, in SRILM format: tok1 [tok2...] count
    */
   NgramCounts(Voc* voc, const string& file);

   /**
    * Find the count of an ngram.
    * @param beg start of voc index array for ngram's tokens
    * @param n size of voc index array
    * @return count; 0 if not found
    */
   Uint count(const Uint* beg, Uint n) {
      Uint c = 0;
      trie.find(beg, n, c);
      return c;
   }
      
   /**
    * Find the count of an ngram.
    * @param ngram voc indexes for ngram's tokens
    * @return count; 0 if not found
    */
   Uint count(const vector<Uint>& ngram) {
      return count(&ngram[0], ngram.size());
   }

   /** 
    * Find the count of an ngram.
    * @param ngram ngram's tokens
    * @return count; 0 if not found
    */
   Uint count(vector<string>& ngram) {
      vector<Uint> ng(ngram.size());
      for (Uint i = 0; i < ngram.size(); ++i) {
         ng[i] = voc->index(ngram[i].c_str());
         if (ng[i] == voc->size()) return 0;
      }
      return count(ng);
   }

   /**
    * Index a sequence of tokens, for more efficient subsequent count calls.
    */
   void index(const vector<string>& toks, vector<Uint>& indexes) {
      indexes.clear();
      voc->index(toks, indexes);
   }

   /**
    * Find all ranges in a given sequence that match some ngram in the current set.
    * @param sequence to search
    * @param max maximum range size to consider
    * @param ranges set of ranges to return
    * @return number of ranges found
    */
    Uint findMatchingRanges(const vector<string>& seq, Uint max, 
                            vector< pair<Uint,Uint> >& ranges);

   /**
    * Return length of longest ngram.
    */
   Uint maxLen() {return maxlen;}
};

}

#endif // __NGRAM_COUNTS_H__
