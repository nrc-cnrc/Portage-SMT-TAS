/**
 * @author George Foster
 * @file ttable.cc  Implementation of TTable.
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

#include "ttable.h"
#include "str_utils.h"
#include "file_utils.h"
#include "binio.h"
#include <cmath>
#include <algorithm>

using namespace Portage;

const string TTable::sep_str = "|||";
static const string TTable_Bin_Format_Header = 
   "NRC PORTAGE TTable binary file format v1.0";

const TTable::SrcDistn TTable::empty_distn;

void TTable::init()
{
   case_null = "NULL";
   speed = 2;
   tword_casemap = NULL;
   sword_casemap = NULL;
   filter_singletons = 0;
}

TTable::TTable(const string& filename, const Voc* src_voc)
{
   init();
   string line;
   if (filename != "-") {
      iSafeMagicStream is(filename, true);
      getline(is, line);
   }
   if ( line == TTable_Bin_Format_Header )
      read_bin(filename, src_voc);
   else
      read(filename, src_voc);
}

TTable::TTable()
{
   init();
}

TTable::~TTable()
{
}

void TTable::output_in_plain_text(ostream& os, const string& filename,
                                  bool verbose, bool sorted)
{
   string line;
   iSafeMagicStream is(filename, true);
   getline(is, line);
   if ( line == TTable_Bin_Format_Header ) {
      // Binary TTables have to be loaded and interpreted correctly
      if ( verbose ) cerr << "Loading binary TTable file." << endl;
      TTable ttable(filename);
      if ( verbose ) cerr << "Done loading binary TTable file." << endl;
      ttable.write(os, sorted);
   } else if ( sorted ) {
      // To sort a text TTable, we have to load it in memory
      if ( verbose ) cerr << "Loading text TTable file so we can sort it." << endl;
      TTable ttable(filename);
      if ( verbose ) cerr << "Done loading text TTable file." << endl;
      ttable.write(os, sorted);
   } else {
      // Plain text TTables can just be output without any processing.
      if ( verbose ) cerr << "TTable model file is not binary, using simple zcat." << endl;
      os << line << endl;
      os << is.rdbuf();
   }
}

void TTable::read(const string& filename, const Voc* src_voc)
{
   assert(!counting_src_voc);
   assert(!counting_tgt_voc);

   time_t start_time(time(NULL));
   iSafeMagicStream ifs(filename);

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

      float pr(0);
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

   cerr << "Read TTable in " << (time(NULL) - start_time) << " seconds" << endl;
} // TTable::read

void TTable::displayStructureSizes() const {
   //time_t start_time(time(NULL));

   Uint64 src_distns_size = src_distns.size() * sizeof(SrcDistn);
   Uint64 src_distns_capacity = src_distns.capacity() * sizeof(SrcDistn);
   for (Uint i = 0; i < src_distns.size(); ++i) {
      if (!src_distns_quick[i])
         src_distns_size += src_distns[i].size() * sizeof(TIndexAndProb);
      src_distns_capacity += src_distns[i].capacity() * sizeof(TIndexAndProb);
   }
   Uint64 src_distns_quick_size = src_distns_quick.size() * sizeof(void*);
   Uint64 src_distns_quick_capacity = src_distns_quick.capacity() * sizeof(void*);
   assert(boost::dynamic_bitset<>::bits_per_block % 8 == 0);
   const Uint bytes_per_block = boost::dynamic_bitset<>::bits_per_block/8;
   for (Uint i = 0; i < src_distns_quick.size(); ++i)
      if (src_distns_quick[i]) {
         src_distns_quick_size += sizeof(*src_distns_quick[i]);
         src_distns_quick_capacity += sizeof(*src_distns_quick[i]);
         if ( 0 ) {
            // this variant is expensive, so we only enable it for debugging
            boost::dynamic_bitset<>::size_type
               largest(0),
               next(src_distns_quick[i]->find_first());
            while (next != boost::dynamic_bitset<>::npos) {
               largest = next;
               next = src_distns_quick[i]->find_next(next);
            }
            src_distns_quick_size += (largest+1) / 8;
         } else {
            src_distns_quick_size +=
               src_distns_quick[i]->size() / 8;
         }
         src_distns_quick_capacity +=
            src_distns_quick[i]->num_blocks() * bytes_per_block;
            //src_distns_quick[i]->size() * 32; // gross estimate for hash_set
      }

   //cerr << "Calculated sizes in " << (time(NULL) - start_time) << " seconds." << endl;
   cerr << "TTable size     in bytes: " << src_distns_size << " regular + "
        << src_distns_quick_size << " quick." << endl;
   cerr << "TTable capacity in bytes: " << src_distns_capacity << " regular + "
        << src_distns_quick_capacity << " quick." << endl;
   cerr << "TTable overhead in bytes: " << (src_distns_capacity-src_distns_size) << " regular + "
        << (src_distns_quick_capacity-src_distns_quick_size) << " quick." << endl;

}

void TTable::makeDistnsUniform()
{
   //displayStructureSizes();

   // Compact non-quick structures to minimize memory overhead, now that all
   // values have been inserted.
   for (Uint i = 0; i < src_distns.size(); ++i) {
      SrcDistn& distn = src_distns[i];
      if (distn.capacity() > 0 && float(distn.size()) / distn.capacity() < .9) {
         SrcDistn compact_copy(distn);
         distn.swap(compact_copy);
         assert(distn.capacity() == distn.size());
      }
   }

   //displayStructureSizes();

   // Copy contents of src_distns_quick, if any, to src_distns, deallocating as
   // we go.
   for (Uint i = 0; i < src_distns_quick.size(); ++i) {
      if (src_distns_quick[i]) {
         SrcDistn& distn = src_distns[i];
         distn.clear();         // anything already there got copied to quick

         /* using vector<bool>
         for (Uint j = 0; j < src_distns_quick[i]->size(); ++j)
            if ((*src_distns_quick[i])[j])
               distn.push_back(make_pair(j,0.0));
         */

         /* using boost::dynamic_bitset. -- slightly better than vector<bool> */
         distn.reserve(src_distns_quick[i]->count());
         for ( boost::dynamic_bitset<>::size_type j = src_distns_quick[i]->find_first();
               j != boost::dynamic_bitset<>::npos;
               j = src_distns_quick[i]->find_next(j) )
            distn.push_back(make_pair(Uint(j),0.0));

         delete src_distns_quick[i];
         src_distns_quick[i] = NULL;
      }
   }

   assert(src_distns_quick.size() == src_distns.size());
   //src_distns_quick.resize(src_distns.size(), NULL); // for future add()'s

   //displayStructureSizes();

   // sort and do normalization on regular src_distns tables

   for (Uint i = 0; i < src_distns.size(); ++i) {
      // Only sort if the src_distns is not already sorted - it should always
      // already be sorted, in fact.
      if ( adjacent_find(src_distns[i].rbegin(), src_distns[i].rend(), CompareTIndexes())
           != src_distns[i].rend() ) {
         cerr << "S";
         sort(src_distns[i].begin(), src_distns[i].end(), CompareTIndexes());
      }
      // TODO: should probably use iterator here
      float uniform_prob = 1.0 / (float) src_distns[i].size();
      for (Uint j = 0; j < src_distns[i].size(); ++j)
         src_distns[i][j].second = uniform_prob;
   }
}

void TTable::setCountingVocs(CountingVoc* _counting_src_voc,
                             CountingVoc* _counting_tgt_voc)
{
   counting_src_voc.reset(_counting_src_voc);
   counting_tgt_voc.reset(_counting_tgt_voc);
   assert(counting_src_voc);
   assert(counting_tgt_voc);
   assert(twords.empty());
   assert(tword_map.empty());
   assert(sword_map.empty());
   assert(src_distns.empty());
   assert(src_distns_quick.empty());
   // copy the source vocabulary
   src_distns.resize(counting_src_voc->size(), empty_distn);
   src_distns_quick.resize(counting_src_voc->size(), NULL);
   pair<WordMapIter,bool> res;
   for (Uint i = 0; i < counting_src_voc->size(); ++i) {
      res = sword_map.insert(make_pair(string(counting_src_voc->word(i)), i));
      assert(res.second);
   }
   // copy the target vocabulary
   twords.reserve(counting_tgt_voc->size());
   for (Uint i = 0; i < counting_tgt_voc->size(); ++i) {
      string tgt_tok = counting_tgt_voc->word(i);
      res = tword_map.insert(make_pair(tgt_tok, i));
      assert(res.second);
      twords.push_back(tgt_tok);
   }
   
} // TTable::setCountingVocs


void TTable::add(Uint sindex, Uint tindex)
{
   if ( filter_singletons ) {
      assert(counting_src_voc);
      assert(counting_tgt_voc);
      assert(sindex < counting_src_voc->size());
      assert(tindex < counting_tgt_voc->size());

      // Do not register singletons against high-frequency words, since
      // these will be filtered out rapidly in IBM1 iterations and require
      // a lot of memory.
      if ( counting_tgt_voc->freq(tindex) == 1 &&
           counting_src_voc->freq(sindex) >= filter_singletons )
         return;

      // We do it only for target-language singletons, since that's where the
      // performance problem we're trying to solve exists, and, in the opposite
      // directory, it makes sense to keep high freq:
      // P(HIGH_FREQ_TGT|SINGLETON_SRC) is a fairly high number, which should be
      // kept, while P(SINGLETON_TGT|HIGH_FREQ_SRC) is expected to be a small
      // number that might get pruned away.
      /*
      if ( counting_src_voc->freq(sindex) == 1 &&
           counting_tgt_voc->freq(tindex) >= SingletonThreshold )
         return;
      */
   }

   // check for current src/tgt pair in quick table first then in slow table
   // (quick table entry will only be non-null if speed == 2)

   // EJJ 22Sept2010 We use a small growth factor because this works just as
   // well as the typical factor of 2, but wastes a lot less space.  In the
   // worst case, with 1.1, we have a 9% space overhead in pre-allocation,
   // whereas that overhead can be 50% if the ratio is 2.  However, the speed
   // is about the same either way: the growth is still exponential, so
   // allocation still takes a total of O(log(N)).
   // This theory is confirmed empirically: there was no measurable time
   // difference between factors of 1.1, 1.2 and 2, but memory use was reduced
   // with each reduction, which implies an actual saving in resources since we
   // require less memory and are less likely to thrash if working close to
   // available RAM.
   static const float bitset_growth_factor = 1.1;

   if (src_distns_quick[sindex]) {
      /* using boost::dynamic_bitset<> or vector<bool> */
      if (src_distns_quick[sindex]->size() <= tindex)
         src_distns_quick[sindex]->resize(bitset_growth_factor * tindex);
      (*src_distns_quick[sindex])[tindex] = true;
   } else {
      SrcDistn& distn = src_distns[sindex];
      SrcDistn::iterator sp = lower_bound(distn.begin(), distn.end(),
                                          make_pair(tindex,0.0), CompareTIndexes());
      if (sp == distn.end() || sp->first != tindex)
         distn.insert(sp, make_pair(tindex,(float)0.0));

      if (speed == 2 && distn.size() > 2000) { // convert to quick table if warranted
         /* using dynamic_bitset or vector<bool> */
         // EJJ 22Sept2010 - allocate conservatively -- see explanation above.
         //const Uint init_size = max(50000,tword_map.size()+1000);
         const Uint init_size = distn.back().first * bitset_growth_factor;
         src_distns_quick[sindex] = new SrcDistnQuick(init_size);

         for (sp = distn.begin(); sp != distn.end(); ++sp)
            (*src_distns_quick[sindex])[sp->first] = true;

         // EJJ Nov 2009: release the memory of the regular set -- we'll need
         // it again later, but not at the same size, and it's expensive to
         // keep both the quick and regular structures around at the same time,
         // especially for very large corpora.
         SrcDistn empty_src_distn;
         distn.swap(empty_src_distn);
         assert(distn.capacity() == 0);
      }
   }
}

void TTable::add(const string& src_tok, const string& tgt_tok)
{
   pair<WordMapIter,bool> res;

   // lookup or add target word
   res = tword_map.insert(make_pair(tgt_tok, twords.size()));
   if (res.second) {
      twords.push_back(tgt_tok);
      if ( counting_tgt_voc ) counting_tgt_voc->add(tgt_tok.c_str(), 10);
   }
   Uint tindex = res.first->second;

   // lookup or add source word and associated target-word list
   res = sword_map.insert(make_pair(src_tok, src_distns.size()));
   if (res.second) {
      src_distns.push_back(empty_distn);
      src_distns_quick.push_back(NULL);
      if ( counting_src_voc ) counting_src_voc->add(src_tok.c_str(), 10);
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
         if ( counting_src_voc ) counting_src_voc->add(src_sent[i].c_str(), 10);
      }
      src_inds.insert(res.first->second);
   }

   for (Uint i = 0; i < tgt_sent.size(); ++i) {
      res = tword_map.insert(make_pair(tgt_sent[i], twords.size()));
      if (res.second) {
         twords.push_back(tgt_sent[i]);
         if ( counting_tgt_voc ) counting_tgt_voc->add(tgt_sent[i].c_str(), 10);
      }
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

int TTable::targetOffset(Uint target_index, const SrcDistn& src_distn) const {
   SrcDistnIter sp = lower_bound(src_distn.begin(), src_distn.end(),
                                 make_pair(target_index,0.0),
                                 CompareTIndexes());
   return (sp == src_distn.end() || sp->first != target_index) ?
      -1  : (int) (sp - src_distn.begin());
}

// Hash function to resolve ties
Uint TTable::mHash(const string& s)
{
   Uint hash = 0;
   const char* sz = s.c_str();
   for ( ; *sz; ++sz)
      hash = *sz + 5*hash;
   return ((hash>>16) + (hash<<16));
}

bool TTable::CompareProbs::operator()(const TIndexAndProb& x, const TIndexAndProb& y) const
{
   //OLD implementation, without tie-breaking: return x.second > y.second;
   // First comparison criterion: the prob score itself
   if ( x.second > y.second ) return true;
   else if ( x.second < y.second ) return false;
   else {
      // EJJ July 2010: add tie breaking that is stable on 32 and 64 bits, does
      // not rely on the hash function used for the hash table itself, yet is
      // arbitrary and thus won't introduce a bias, like in
      // PhraseTable::PhraseScorePairLessThan::operator()
      const Uint x_hash = mHash(parent->twords[x.first]);
      const Uint y_hash = mHash(parent->twords[y.first]);
      if ( x_hash > y_hash ) return true;
      else if ( x_hash < y_hash ) return false;
      else return parent->twords[x.first] > parent->twords[y.first];
   }
}

void TTable::getSourceDistnByDecrProb(const string& src_word, SrcDistn& distn) const {
   distn = getSourceDistn(src_word);
   sort(distn.begin(), distn.end(), CompareProbs(this));
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

double TTable::getProb(const string& src_word, const string& tgt_word, double smooth) const {
   WordMapIter p = tword_map.find(mapTgtCase(tgt_word));
   if (p == tword_map.end()) {return smooth;}
   const SrcDistn& distn = getSourceDistn(src_word);
   SrcDistnIter sp = lower_bound(distn.begin(), distn.end(),
                                 make_pair(p->second,0.0), CompareTIndexes());
   return (sp == distn.end() || sp->first != p->second) ? smooth : sp->second;
}

struct TIndexAndProbWordLessThan {
   const vector<string>& twords;
   TIndexAndProbWordLessThan(const vector<string>& twords) : twords(twords) {}
   bool operator()(TTable::TIndexAndProb i, TTable::TIndexAndProb j) {
      assert(i.first < twords.size());
      assert(j.first < twords.size());
      return twords[i.first] < twords[j.first];
   }
};

void TTable::write(ostream& os, bool sorted) const
{
   time_t start_time(time(NULL));
   streamsize old_precision = os.precision();
   os.precision(9);
   if ( !sorted ) {
      for ( WordMapIter p(sword_map.begin()), p_end(sword_map.end());
            p != p_end; ++p ) {
         for ( SrcDistnIter pt(src_distns[p->second].begin()),
                            pe(src_distns[p->second].end());
               pt != pe; ++pt )
            os << p->first << ' '
               << twords[pt->first] << ' '
               << pt->second << nf_endl;
      }
   } else {
      // sorting requested.  Sort source words asciibetically for the outer
      // loop, then target words asciibetically for the inner loop.
      assert(sword_map.size() == src_distns.size());
      vector<string> swords_sorted;
      swords_sorted.reserve(sword_map.size());
      for ( WordMapIter p(sword_map.begin()), p_end(sword_map.end());
            p != p_end; ++p )
         swords_sorted.push_back(p->first);
      std::sort(swords_sorted.begin(), swords_sorted.end());
      Uint i(0);
      cerr << "Writing sorted TTable" << endl;
      for ( vector<string>::const_iterator s_it(swords_sorted.begin()),
                                           s_end(swords_sorted.end());
            s_it != s_end; ++s_it ) {
         if (++i % 100000 == 1)
            cerr << (time(NULL) - start_time) << "s..." << endl;
         WordMapIter res = sword_map.find(*s_it);
         assert(res != sword_map.end());
         Uint sword_index = res->second;
         assert(sword_index < src_distns.size());
         SrcDistn distn_ordered = src_distns[sword_index];
         std::sort(distn_ordered.begin(), distn_ordered.end(),
                   TIndexAndProbWordLessThan(twords));
         for ( SrcDistnIter pt(distn_ordered.begin()),
                            pe(distn_ordered.end());
               pt != pe; ++pt)
            os << *s_it << ' '
               << twords[pt->first] << ' '
               << pt->second << nf_endl;
      }
   }
   os.flush();
   os.precision(old_precision);
   cerr << "Wrote TTable in " << (time(NULL) - start_time) << " seconds" << endl;
}

void TTable::write_bin(ostream& os) const
{
   time_t start_time(time(NULL));
   // src_distns_quick must be empty at this stage.
   for (Uint i(0); i < src_distns_quick.size(); ++i)
      if (src_distns_quick[i])
         error(ETFatal,
            "Called TTable::write_bin with src_distns_quick still in place.");

   // Header
   os << TTable_Bin_Format_Header << endl;

   // Source language vocabulary
   os << sword_map.size() << " Source language vocabulary" << endl;
   for (WordMapIter p = sword_map.begin(); p != sword_map.end(); ++p) {
      os << p->second << ":" << p->first << nf_endl;
      assert(p->second < sword_map.size());
   }
   os.flush();

   // Target language vocabulary
   os << twords.size() << " Target language vocabulary" << endl;
   for (Uint i(0); i < twords.size(); ++i)
      os << twords[i] << nf_endl;
   os.flush();

   // src_distns
   using namespace BinIO;
   if(sword_map.size() != src_distns.size())
      cerr << "twords.size() = " << twords.size()
           << " src_distns.size() = " << src_distns.size() << endl;
   assert(sword_map.size() == src_distns.size());
   os << "Source distributions" << endl;
   assert(sizeof(TIndexAndProb) == 8);
   for (Uint i(0); i < src_distns.size(); ++i) {
      os << " " << i << ":";
      writebin(os, src_distns[i]);
   }

   // Footer
   os << " End of " << TTable_Bin_Format_Header << endl;

   cerr << "Wrote bin TTable in " << (time(NULL) - start_time) << " seconds" << endl;
} // TTable::write_bin()

void TTable::read_bin(const string& filename, const Voc* src_voc)
{
   assert(!counting_src_voc);
   assert(!counting_tgt_voc);

   time_t start_time(time(NULL));
   iSafeMagicStream is(filename);
   const char* fname(filename.c_str());

   cerr << "Reading " << filename << endl;

   // Header
   string line;
   if ( !getline(is, line) )
      error(ETFatal, "Unexpected end of file at the beginning of %s", fname);
   if ( line != TTable_Bin_Format_Header )
      error(ETFatal, "File %s does not look like a binary TTable", fname);

   // Source language vocabulary
   Uint svoc_size(0);
   is >> svoc_size;
   if ( !getline(is, line) )
      error(ETFatal, "Unexpected end of file in %s before source voc", fname);
   if ( line != " Source language vocabulary" )
      error(ETFatal, "Bad source voc header in %s", fname);
   Uint svoc_lineno(0);
   Uint sindex;
   string sword;
   vector<string> swords; // to map source word to index for filtering src_distn
   if (src_voc)
      swords.resize(svoc_size);
   char c;
   while (svoc_lineno < svoc_size) {
      if ( is.eof() )
         error(ETFatal, "Unexpected end of file %s svoc line %d",
               fname, svoc_lineno);
      ++svoc_lineno;
      is >> sindex >> c;
      if ( sindex >= svoc_size )
         error(ETFatal, "SVoc index %d exceeds SVoc size %d in %s svoc line %d",
               sindex, svoc_size, fname, svoc_lineno);
      if ( c != ':' )
         error(ETFatal, "Invalid format in %s svoc line %d",
               fname, svoc_lineno);
      if ( !getline(is, sword) )
         error(ETFatal, "Unexpected end of file in %s svoc line %d",
               fname, svoc_lineno);
      pair<WordMapIter,bool> res =
         sword_map.insert(make_pair(sword, sindex));
      if (src_voc)
         swords[sindex] = sword;
      if ( !res.second )
         error(ETFatal, "Duplicate source word %s in %s svoc line %d",
               sword.c_str(), fname, svoc_lineno);
   }

   // Target language vocabulary
   Uint tvoc_size(0);
   is >> tvoc_size;
   if ( !getline(is, line) )
      error(ETFatal, "Unexpected end of file in %s before target voc", fname);
   if ( line != " Target language vocabulary" )
      error(ETFatal, "Bad target voc header in %s", fname);
   twords.resize(tvoc_size);
   Uint tindex(0);
   string tword;
   while (tindex < tvoc_size && getline(is, tword)) {
      twords[tindex] = tword;
      pair<WordMapIter,bool> res =
         tword_map.insert(make_pair(tword, tindex));
      if ( !res.second )
         error(ETFatal, "Duplicate target word %s in %s tvoc line %d",
               tword.c_str(), fname, tindex+1);
      ++tindex;
   }
   if ( tindex < tvoc_size )
      error(ETFatal, "Unexpected end of file in %s tvoc line %d",
            fname, tindex);

   // src_distns
   using namespace BinIO;
   src_distns.resize(svoc_size);
   if ( !getline(is, line) )
      error(ETFatal, "Unexpected end of file in %s before src distns", fname);
   if ( line != "Source distributions" )
      error(ETFatal, "Bad source distns header in %s", fname);
   assert(sizeof(TIndexAndProb) == 8);
   Uint i_read(-1); // -1 is intentionally bad init value for error detection
   for (Uint i(0); i < src_distns.size(); ++i) {
      if ( is.eof() )
         error(ETFatal, "Unexpected end of file in %s src_distn %d", fname, i);
      is >> i_read >> c;
      if ( i != i_read )
         error(ETFatal, "Bad i (%d) in file %s src_distns %d",
               i_read, fname, i);
      if ( c != ':' )
         error(ETFatal, "Invalid format in file %s src_distns %d", fname, i);
      readbin(is, src_distns[i]);
      // If src_voc is specified, we don't need src_distn for source words not in it.
      if (src_voc && src_voc->index(swords[i].c_str()) == src_voc->size()) {
         SrcDistn empty;
         src_distns[i].swap(empty);
      }
   }

   // Footer
   if ( !getline(is, line) )
      error(ETFatal, "Unexpected end of file in %s after src distns", fname);
   if ( line != " End of " + TTable_Bin_Format_Header )
      error(ETFatal, "Bad footer in %s", fname);
   
   cerr << "Read bin TTable in " << (time(NULL) - start_time) << " seconds" << endl;
} // TTable::read_bin()

namespace Portage{
static bool operator==(
   const unordered_map<string,Uint>& map1,
   const unordered_map<string,Uint>& map2
) {
   typedef unordered_map<string,Uint>::const_iterator iterT;
   for ( iterT it(map1.begin()), end(map1.end()); it != end; ++it )
      if ( map2.find(it->first) == map2.end() )
         return false;
   for ( iterT it(map2.begin()), end(map2.end()); it != end; ++it )
      if ( map1.find(it->first) == map1.end() )
         return false;
   return true;
}
}

void TTable::test_read_write_bin(const string& filename)
{
   {
      oSafeMagicStream os(filename);
      write_bin(os);
   }
   TTable tt_copy;
   tt_copy.read_bin(filename);
   if ( !(tword_map == tt_copy.tword_map) )
      error(ETWarn, "tword_map not identical in %s", filename.c_str());
   if ( !(sword_map == tt_copy.sword_map) )
      error(ETWarn, "sword_map not identical in %s", filename.c_str());
   if ( twords != tt_copy.twords )
      error(ETWarn, "twords not identical in %s", filename.c_str());
   if ( src_distns != tt_copy.src_distns )
      error(ETWarn, "src_distns not identical in %s", filename.c_str());
} // TTable::test_read_write_bin()

Uint TTable::prune(double thresh, double null_thresh, const string& null_word)
{
   Uint null_index = sourceIndex(null_word);
   Uint size = 0;
   for (Uint i = 0; i < numSourceWords(); ++i) {
      const double this_thresh(i == null_index ? null_thresh : thresh);
      double sum = 0.0;
      Uint j(0), keep_j(0); // Need these after the loop.
      for (Uint end(src_distns[i].size()); j < end; ) {
         if (src_distns[i][j].second >= this_thresh) {
            sum += src_distns[i][j].second;
            if ( j != keep_j ) {
               assert(j > keep_j);
               src_distns[i][keep_j] = src_distns[i][j];
            }
            ++j;
            ++keep_j;
         } else {
            //src_distns[i].erase(src_distns[i].begin()+j);
            ++j;
         }
      }
      if ( j > keep_j ) {
         src_distns[i].erase(src_distns[i].begin()+keep_j, src_distns[i].end());
         assert(src_distns[i].size() == keep_j);
      }
      for (Uint j = 0; j < src_distns[i].size(); ++j)
         src_distns[i][j].second /= sum;
      size += src_distns[i].size();
   }
   return size;
}


