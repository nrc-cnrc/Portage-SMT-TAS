/**
 * @author George Foster
 * @file ttable.cc  Implementation of TTable.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "ttable.h"
#include "str_utils.h"
#include <file_utils.h>
#include <cmath>

using namespace Portage;

const string TTable::sep_str = "|||";

TTable::TTable(const string& filename, const Voc* src_voc)
{
   IMagicStream ifs(filename);

   vector<string> toks;
   string line;
   while (getline(ifs, line)) {
      if (splitZ(line, toks) != 3)
         error(ETFatal, "line not in format <src tgt p(tgt|src)>: %s",
               line.c_str());

      // If src_voc specified, ignore source words not in it.
      if (src_voc && src_voc->index(toks[0].c_str()) == src_voc->size())
         continue;

      pair<WordMapIter,bool> res;

      float pr;
      conv(toks[2], pr);

      res = tword_map.insert(make_pair(toks[1], twords.size()));
      if (res.second) twords.push_back(toks[1]);
      Uint tindex = res.first->second;

      res = sword_map.insert(make_pair(toks[0], src_distns.size()));
      if (res.second) src_distns.push_back(empty_distn);
      src_distns[res.first->second].push_back(make_pair(tindex,pr));

   }

   bool ok = true;

   for (WordMapIter p = sword_map.begin(); p != sword_map.end(); ++p) {
      sort(src_distns[p->second].begin(), src_distns[p->second].end(), CompareTIndexes());
      double sum = 0.0;
      for (Uint i = 0; i < src_distns[p->second].size(); ++i) {
         sum += src_distns[p->second][i].second;
      }
      if (ok && abs(sum - 1.0) > .05) {
         error(ETWarn, "hmm, distributions don't look normalized %s",
               "- are you sure this is an inverted table?");
         ok = false;
      }
   }

   src_distns_quick.resize(src_distns.size()); // in case we want to add later
}

void TTable::makeDistnsUniform()
{
   // Copy contents of src_distns_quick, if any, to src_distns, deallocating as
   // we go.

   for (Uint i = 0; i < src_distns_quick.size(); ++i)
      if (src_distns_quick[i]) {
         SrcDistn& distn = src_distns[i];
         distn.clear();         // anything already there got copied to quick
         for (Uint j = 0; j < src_distns_quick[i]->size(); ++j)
            if ((*src_distns_quick[i])[j])
               distn.push_back(make_pair(j,0.0));
         delete src_distns_quick[i];
         src_distns_quick[i] = NULL;
      }
   src_distns_quick.resize(src_distns.size()); // for future add()'s

   // sort and do normalization on regular src_distns tables

   for (Uint i = 0; i < src_distns.size(); ++i) {
      sort(src_distns[i].begin(), src_distns[i].end(), CompareTIndexes());
      for (Uint j = 0; j < src_distns[i].size(); ++j)
         src_distns[i][j].second = 1.0 / (float) src_distns[i].size();
   }
}

void TTable::add(Uint sindex, Uint tindex)
{
   // check for current src/tgt pair in quick table first then in slow table
   // (quick table entry will only be non-null if speed == 2)

   if (src_distns_quick[sindex]) {
      if (src_distns_quick[sindex]->size() < tindex)
         src_distns_quick[sindex]->resize(2 * tindex);
      (*src_distns_quick[sindex])[tindex] = true;
   } else {
      SrcDistn& distn = src_distns[sindex];
      SrcDistn::iterator sp = lower_bound(distn.begin(), distn.end(),
                                          make_pair(tindex,0.0), CompareTIndexes());
      if (sp == distn.end() || sp->first != tindex)
         distn.insert(sp, make_pair(tindex,(float)0.0));

      if (speed == 2 && distn.size() > 2000) { // convert to quick table if warranted
         src_distns_quick[sindex] = new vector<bool>(max(200000,tword_map.size()+1000));
         for (sp = distn.begin(); sp != distn.end(); ++sp)
            (*src_distns_quick[sindex])[sp->first] = true;
      }
   }
}

void TTable::add(const string& src_tok, const string& tgt_tok)
{
   pair<WordMapIter,bool> res;

   // lookup or add target word
   res = tword_map.insert(make_pair(tgt_tok, twords.size()));
   if (res.second)
      twords.push_back(tgt_tok);
   Uint tindex = res.first->second;

   // lookup or add source word and associated target-word list
   res = sword_map.insert(make_pair(src_tok, src_distns.size()));
   if (res.second) {
      src_distns.push_back(empty_distn);
      src_distns_quick.push_back(NULL);
   }
   Uint sindex = res.first->second;

   add(sindex, tindex);
}

void TTable::add(const vector<string>& src_sent, const vector<string>& tgt_sent)
{
   src_inds.clear();
   tgt_inds.clear();

   pair<WordMapIter,bool> res;

   for (Uint i = 0; i < src_sent.size(); ++i) {
      res = sword_map.insert(make_pair(src_sent[i], src_distns.size()));
      if (res.second) {
         src_distns.push_back(empty_distn);
         src_distns_quick.push_back(NULL);
      }
      src_inds.insert(res.first->second);
   }

   for (Uint i = 0; i < tgt_sent.size(); ++i) {
      res = tword_map.insert(make_pair(tgt_sent[i], twords.size()));
      if (res.second)
         twords.push_back(tgt_sent[i]);
      tgt_inds.insert(res.first->second);
   }

   for (Uint i = 0; i < src_inds.contents().size(); ++i)
      for (Uint j = 0; j < tgt_inds.contents().size(); ++j)
         add(src_inds.contents()[i], tgt_inds.contents()[j]);
}

void TTable::getSourceVoc(vector<string>& src_words) const {
   src_words.clear();
   for (WordMapIter p = sword_map.begin(); p != sword_map.end(); ++p)
      src_words.push_back(p->first);
}

int TTable::targetOffset(Uint target_index, const SrcDistn& src_distn) {
   SrcDistnIter sp = lower_bound(src_distn.begin(), src_distn.end(),
                                 make_pair(target_index,0.0),
                                 CompareTIndexes());
   return (sp == src_distn.end() || sp->first != target_index) ?
      -1  : (int) (sp - src_distn.begin());
}

void TTable::getSourceDistnByDecrProb(const string& src_word, SrcDistn& distn) const {
   distn = getSourceDistn(src_word);
   sort(distn.begin(), distn.end(), CompareProbs());
}

void TTable::getSourceDistnByDecrProb(const string& src_word,
                                      vector<string>& tgt_words,
                                      vector<float>& tgt_probs) const {
   SrcDistn distn;
   getSourceDistnByDecrProb(src_word, distn);
   tgt_words.clear(); tgt_probs.clear();
   for (Uint i = 0; i < distn.size(); ++i) {
      tgt_words.push_back(targetWord(distn[i].first));
      tgt_probs.push_back(distn[i].second);
   }
}

double TTable::getBestTrans(const string& src_word, string& best_trans)
{
  TIndexAndProb best = make_pair(numTargetWords(), -1.0);

  SrcDistn& sd = src_distns[sourceIndex(src_word)];

  for (Uint i = 0; i < sd.size(); ++i)
    if (sd[i].second > best.second)
      best = sd[i];

   if (best.first != numTargetWords())
     best_trans = targetWord(best.first);

   return best.second;
}

double TTable::getProb(const string& src_word, const string& tgt_word) const {
   WordMapIter p = tword_map.find(tgt_word);
   if (p == tword_map.end()) {return -1.0;}
   const SrcDistn& distn = getSourceDistn(src_word);
   SrcDistnIter sp = lower_bound(distn.begin(), distn.end(),
                                 make_pair(p->second,0.0), CompareTIndexes());
   return (sp == distn.end() || sp->first != p->second) ? -1.0 : sp->second;
}

void TTable::write(ostream& os) const
{
   streamsize old_precision = os.precision();
   os.precision(9);
   for (WordMapIter p = sword_map.begin(); p != sword_map.end(); ++p) {
      SrcDistnIter pe = src_distns[p->second].end();
      for (SrcDistnIter pt = src_distns[p->second].begin(); pt != pe; ++pt)
         os << p->first << ' ' << twords[pt->first] << ' ' << pt->second << endl;
   }
   os.precision(old_precision);
}

Uint TTable::prune(double thresh)
{
   Uint size = 0;
   for (Uint i = 0; i < numSourceWords(); ++i) {
      double sum = 0.0;
      for (Uint j = 0; j < src_distns[i].size();) {
         if (src_distns[i][j].second >= thresh) {
            sum += src_distns[i][j].second;
            ++j;
         } else {
            src_distns[i].erase(src_distns[i].begin()+j);
         }
      }
      for (Uint j = 0; j < src_distns[i].size(); ++j)
         src_distns[i][j].second /= sum;
      size += src_distns[i].size();
   }
   return size;
}


