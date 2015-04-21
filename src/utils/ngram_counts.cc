/**
 * @author George Foster
 * @file ngram_counts.cc
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include "ngram_counts.h"

using namespace std;
using namespace Portage;

namespace Portage {

NgramCounts::NgramCounts(Voc* voc, const string& file) :
   voc(voc), maxlen(0)
{
   assert(voc);
   
   iSafeMagicStream is(file);
   string line;
   vector<string> toks;
   vector<Uint> ng;
   while (getline(is, line)) {
      splitZ(line, toks);
      if (toks.size() < 2)
         error(ETFatal, 
               "NgramCounts: expected line in format word1 [word2...] count");
      ng.resize(toks.size()-1);
      for (Uint i = 0; i < toks.size()-1; ++i)
         ng[i] = voc->add(toks[i].c_str());
      Uint* pc;
      if (trie.find_or_insert(&ng[0], ng.size(), pc))
         error(ETFatal, "NgramCounts: duplicate ngram: %s", line.c_str());
      *pc = conv<Uint>(toks.back());
      if (ng.size() > maxlen)
         maxlen = ng.size();
   }
}

Uint NgramCounts::findMatchingRanges(const vector<string>& seq, Uint mxlen, 
                                     vector< pair<Uint,Uint> >& ranges) 
{
   vector<Uint> indseq(seq.size());
   for (Uint i = 0; i < seq.size(); ++i)
      indseq[i] = voc->index(seq[i].c_str());
   Uint count;
   for (Uint i = 0; i < indseq.size(); ++i) {
      for (Uint j = 1; j < min(mxlen,indseq.size()-i)+1; ++j) {
         if (trie.find(&indseq[0]+i, j, count))
            ranges.push_back(make_pair(i, i+j));
         else
            break; // assume no longer prefixes exist
      }
   }
   return ranges.size();
}


};
