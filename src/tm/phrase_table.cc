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

void PhraseTableBase::codePhrase(ToksIter beg, ToksIter end, string& coded, const string& sep) 
{
   coded.clear();
   if (beg < end) 
      coded += *beg++;
   while (beg < end) 
      coded += sep + *beg++;
}

void PhraseTableBase::decodePhrase(const string& coded, vector<string>& toks, const string& sep) 
{
   toks.clear();
   string::size_type beg = 0, end;
   while ((end = coded.find(sep, beg)) != string::npos) {
      toks.push_back(coded.substr(beg, end-beg));
      beg = end + sep.length();
   }
   toks.push_back(coded.substr(beg, coded.length()-beg));
}

void PhraseTableBase::compressPhrase(ToksIter beg, ToksIter end, string& coded, Voc& voc) 
{
   coded.clear();
   for (; beg != end; ++beg) {
      Uint code = voc.add((*beg).c_str());
      assert (code <= max_code);
      pack(code, coded, code_base, num_code_bytes);
   }
}

void PhraseTableBase::decompressPhrase(const string& coded, vector<string>& toks, Voc& voc)
{
   toks.clear();
   for (Uint pos = 0; pos < coded.length(); pos += num_code_bytes)
      toks.push_back(voc.word(unpack(coded, pos, num_code_bytes, code_base)));
}

string PhraseTableBase::recodePhrase(const string& coded, Voc& voc,
                                     const string& sep)
{
   vector<string> toks;
   decompressPhrase(coded, toks, voc);
   string s1;
   join(toks.begin(), toks.end(), s1, sep);
   return s1;
}

Uint PhraseTableBase::phraseLength(const char* coded)
{
   return strlen(coded) / num_code_bytes;
}

void PhraseTableBase::extractTokens(const string& line, vector<string>& toks,
				    ToksIter& b1, ToksIter& e1,
				    ToksIter& b2, ToksIter& e2,
				    ToksIter& v, bool tolerate_multi_vals)
{
   toks.clear();
   vector<Uint> seps;
   split(line, toks);
   for (Uint i = 0; i < toks.size(); ++i)
      if (toks[i] == psep) seps.push_back(i);
   if (seps.size() != 2 || (!tolerate_multi_vals && seps[1]+2 != toks.size()))
      error(ETFatal, "incorrect format in phrase table: %s", line.c_str());

   b1 = toks.begin();
   e1 = toks.begin() + seps[0];
   b2 = toks.begin() + seps[0] + 1;
   e2 = toks.begin() + seps[1];
   v =  toks.begin() + seps[1] + 1;
}


