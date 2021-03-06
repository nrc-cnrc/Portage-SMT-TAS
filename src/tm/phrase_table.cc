/**
 * @author George Foster
 * @file phrase_table.cc  Implementation of PhraseTableBase.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <iostream>
#include <str_utils.h>
#include "phrase_table.h"

using namespace Portage;

const string PhraseTableBase::sep = " ";
const string PhraseTableBase::psep = "|||";
const string PhraseTableBase::psep_replacement = "___|||___";

void PhraseTableBase::compressPhrase(ToksIter beg, ToksIter end, string& coded, Voc& voc)
{
   coded.clear();
   coded.reserve((end-beg) * num_code_bytes + 1);
   for (; beg != end; ++beg) {
      Uint code = voc.add(*beg);
      assert (code <= max_code);
      pack(code, coded, code_base, num_code_bytes);
   }
}

void PhraseTableBase::compressPhrase(sToksIter beg, sToksIter end, string& coded, Voc& voc)
{
   coded.clear();
   coded.reserve((end-beg) * num_code_bytes + 1);
   for (; beg != end; ++beg) {
      Uint code = voc.add((*beg).c_str());
      assert (code <= max_code);
      pack(code, coded, code_base, num_code_bytes);
   }
}

void PhraseTableBase::compressPhrase(IndIter beg, IndIter end, string& coded, Voc& voc)
{
   coded.clear();
   coded.reserve((end-beg) * num_code_bytes + 1);
   for (; beg != end; ++beg) {
      assert (*beg <= max_code);
      pack(*beg, coded, code_base, num_code_bytes);
   }
}

void PhraseTableBase::decompressPhrase(const char* coded, vector<string>& toks, Voc& voc)
{
   toks.clear();
   const Uint len = strlen(coded);
   for (Uint pos = 0; pos < len; pos += num_code_bytes)
      toks.push_back(voc.word(unpack(coded, pos, num_code_bytes, code_base)));
}

void PhraseTableBase::decompressPhrase(const char* coded, vector<Uint>& toks, Voc& voc)
{
   toks.clear();
   const Uint len = strlen(coded);
   for (Uint pos = 0; pos < len; pos += num_code_bytes)
      toks.push_back(unpack(coded, pos, num_code_bytes, code_base));
}

string PhraseTableBase::recodePhrase(const char* coded, Voc& voc,
                                     const string& sep)
{
   // optimized to avoid temporary vectors as much as possible.
   string s;
   const Uint len = strlen(coded);
   s.reserve(len * 3); // pre-reserve 9 characters per token, including one for sep.
   if (len) s += voc.word(unpack(coded, 0, num_code_bytes, code_base));
   for (Uint pos = num_code_bytes; pos < len; pos += num_code_bytes) {
      s += sep;
      s += voc.word(unpack(coded, pos, num_code_bytes, code_base));
   }
   return s;

   // this old version was nicely functional, but expensive because of the temporary
   // structure.
   /*
   vector<string> toks;
   decompressPhrase(coded, toks, voc);
   return join(toks, sep);
   */
}

Uint PhraseTableBase::phraseLength(const char* coded)
{
   return strlen(coded) / num_code_bytes;
}

void PhraseTableBase::extractTokens(const string& line, char* buffer, vector<char*>& toks,
				    ToksIter& b1, ToksIter& e1,
				    ToksIter& b2, ToksIter& e2,
				    ToksIter& v, ToksIter& a, ToksIter& f,
                                    bool tolerate_multi_vals,
                                    bool allow_fourth_column)
{
   vector<Uint> seps;
   seps.reserve(3);
   destructive_splitZ(buffer, toks);
   for (Uint i = 0; i < toks.size(); ++i)
      if (toks[i] == psep) seps.push_back(i);
   if ((!allow_fourth_column && seps.size() != 2) ||
       ( allow_fourth_column && (seps.size() < 2 || seps.size() > 3)))
      error(ETFatal, "incorrect format in phrase table (wrong number of separators: found %d; this program %s columns): %s",
            seps.size(),
            (allow_fourth_column ? "only allows 3 or 4" : "requires exactly 3"),
            line.c_str());

   // Format is: ... val1 ... valn [a=...] [c=...]
   // Code below sets the 'a' pointer to the first a= or c= field.

   f = seps.size() == 3 ? toks.begin() + seps[2] : toks.end();
   assert(f > toks.begin() + 1);

   a = f;
   // look for a= and c= fields in any order
   while (true) {
      if (isPrefix("c=", *(a-1))) --a;
      else if (isPrefix("a=", *(a-1))) --a;
      else break;
   }

   int value_count((a - toks.begin()) - seps[1] - 1);
   if ( tolerate_multi_vals ) {
      if ( value_count < 1 )
         error(ETFatal, "incorrect format in phrase table (expected at least one value token after 2nd separator): %s", line.c_str());
   } else {
      if ( value_count != 1 )
         error(ETFatal, "incorrect format in phrase table (expected exactly one value token after 2nd separator): %s", line.c_str());
   }

   b1 = toks.begin();
   e1 = toks.begin() + seps[0];
   b2 = toks.begin() + seps[0] + 1;
   e2 = toks.begin() + seps[1];
   v =  toks.begin() + seps[1] + 1;
}


