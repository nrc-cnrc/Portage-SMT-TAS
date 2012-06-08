/**
 * @author George Foster, with reduced memory usage mods by Darlene Stewart
 * @file phrase_table-cc.h  Implementation of phrase table with joint frequencies.
 *
 *
 * COMMENTS:
 *
 * Represent a phrase table with joint frequencies.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef PHRASE_TABLE_CC_H
#define PHRASE_TABLE_CC_H

#include "word_align_io.h"

namespace Portage {

/*---------------------------------------------------------------------------------------------
  PhraseTableGen implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
void PhraseTableGen<T>::clear()
{
   lang1_voc.clear();
   for ( Uint i = 0; i < phrase_table.size(); ++i )
      delete phrase_table[i];
   phrase_table.clear();
   lang2_voc.clear();
}

template<class T>
bool PhraseTableGen<T>::exists(ToksIter beg1, ToksIter end1, ToksIter beg2, ToksIter end2, T &val)
{
   string phrase1, phrase2;
   compressPhrase(beg1, end1, phrase1, wvoc1);
   compressPhrase(beg2, end2, phrase2, wvoc2);

   const Uint id1 = lang1_voc.index(phrase1.c_str());
   if (id1 == lang1_voc.size())
      return false;

   const Uint id2 = lang2_voc.index(phrase2.c_str());
   if (id2 == lang2_voc.size())
      return false;

   const typename PhraseFreqs::iterator iter = phrase_table[id1]->find(id2);
   if (iter == phrase_table[id1]->end())
      return false;

   val = iter->second;
   return true;
}

template<class T>
void PhraseTableGen<T>::addPhrasePair(ToksIter beg1, ToksIter end1, ToksIter beg2, ToksIter end2, T val, const char* green_alignment)
{
   string phrase1, phrase2;
   compressPhrase(beg1, end1, phrase1, wvoc1);
   compressPhrase(beg2, end2, phrase2, wvoc2);

   Uint id1 = lang1_voc.add(phrase1.c_str());
   num_lang1_phrases = lang1_voc.size();
   Uint id2 = lang2_voc.add(phrase2.c_str(), val);
   if (keep_phrase_table_in_memory) {
      if (id1 == phrase_table.size()) {
         phrase_table.push_back(new PhraseFreqs());

         if ( green_alignment )
            while ( phrase_alignment_table.size() <= id1 )
               phrase_alignment_table.push_back(new PhraseAlignments());
      }
      (*phrase_table[id1])[id2] += val;

      if (green_alignment) {
         Uint alignment_id = alignment_voc.add(green_alignment);
         (*phrase_alignment_table[id1])[id2][alignment_id] += val;

         /*
         Uint key[3] = {id1, id2, alignment_id};
         T* value_p;
         phrase_alignment_table.find_or_insert(key, 3, value_p);
         *value_p += val;
         */
      }
   }
}

template<class T>
void PhraseTableGen<T>::lookupPhrasePair(const string &line, Uint& id1, Uint& id2, T& val, string& alignments)
{
   vector<string> toks;
   typename PhraseTableGen<T>::ToksIter b1, e1, b2, e2, v, a;
   string phrase1, phrase2;

   extractTokens(line, toks, b1, e1, b2, e2, v, a);
   if (swap_on_read) {
      swap(b1, b2);
      swap(e1, e2);
   }
   compressPhrase(b1, e1, phrase1, wvoc1);
   compressPhrase(b2, e2, phrase2, wvoc2);

   val = conv<T>(*v);
   if ( a != toks.end() ) {
      assert((*a)[0] == 'a');
      alignments = a->substr(2);
      if (swap_on_read)
         error(ETFatal, 
               "swap-on-read not compatible with alignment info in jpt");
   } else
      alignments.clear();

   if (phrase_table_read) {
      id1 = lang1_voc.index(phrase1.c_str());
      id2 = lang2_voc.index(phrase2.c_str());
   } else {
      // add to the phrase table if not already fully read in.
      id1 = lang1_voc.add(phrase1.c_str());
      num_lang1_phrases = lang1_voc.size();
      id2 = lang2_voc.add(phrase2.c_str(), val);
      if (keep_phrase_table_in_memory) {
         if (id1 == phrase_table.size())
            phrase_table.push_back(new PhraseFreqs());
         (*phrase_table[id1])[id2] += val;

         // parse and add the alignments too, if present.
         if ( !alignments.empty() ) {
            while ( phrase_alignment_table.size() <= id1 )
               phrase_alignment_table.push_back(new PhraseAlignments());
            parseAndTallyAlignments((*phrase_alignment_table[id1])[id2],
                                    alignment_voc, alignments);
            /*
            parseAndTallyAlignments(phrase_alignment_table.find(id1).find(id2),
                                    alignment_voc, alignments);
            */
         }
      }
   }
}

template<class T>
void PhraseTableGen<T>::lookupPhrasePair(const string &line, string& phrase1, Uint& id2, T& val, string& alignments)
{
   vector<string> toks;
   typename PhraseTableGen<T>::ToksIter b1, e1, b2, e2, v, a;
   string phrase2;

   assert(! keep_phrase_table_in_memory);

   extractTokens(line, toks, b1, e1, b2, e2, v, a);
   if (swap_on_read) {
      swap(b1, b2);
      swap(e1, e2);
   }
   compressPhrase(b1, e1, phrase1, wvoc1);
   compressPhrase(b2, e2, phrase2, wvoc2);

   val = conv<T>(*v);
   if ( a != toks.end() ) {
      assert((*a)[0] == 'a');
      alignments = a->substr(2);
      if (swap_on_read)
         error(ETFatal, 
               "swap-on-read not compatible with alignment info in jpt");
   } else
      alignments.clear();

   if (phrase_table_read)
      id2 = lang2_voc.index(phrase2.c_str());
   else
      // add to the phrase table if not already fully read in.
      id2 = lang2_voc.add(phrase2.c_str(), val);
}

template<class T>
bool PhraseTableGen<T>::inVoc1(const string& word)
{
   vector<string> toks;
   toks.push_back(word);
   string code;
   compressPhrase(toks.begin(), toks.end(), code, wvoc1);
   return lang1_voc.index(code.c_str()) != lang1_voc.size();
}

template<class T>
bool PhraseTableGen<T>::inVoc2(const string& word)
{
   vector<string> toks;
   toks.push_back(word);
   string code;
   compressPhrase(toks.begin(), toks.end(), code, wvoc2);
   return lang2_voc.index(code.c_str()) != lang2_voc.size();
}

template<class T>
string& PhraseTableGen<T>::getPhrase(Uint lang, Uint id, string& phrase) {
   if (lang == 1) {
      if (id > lang1_voc.size())
         error(ETFatal, "Phrase for index %d in lang 1 not stored in memory.", id);
      phrase = recodePhrase(lang1_voc.word(id), wvoc1, sep);
   } else
      phrase = recodePhrase(lang2_voc.word(id), wvoc2, sep);
   return phrase;
}

template<class T>
void PhraseTableGen<T>::getPhrase(Uint lang, Uint id, vector<string>& toks) {
   toks.clear();
   if (lang == 1) {
      if (id > lang1_voc.size())
         error(ETFatal, "Phrase for index %d in lang 1 not stored in memory.", id);
      decompressPhrase(lang1_voc.word(id), toks, wvoc1);
   } else
      decompressPhrase(lang2_voc.word(id), toks, wvoc2);
}

template<class T>
string& PhraseTableGen<T>::getPhrase(Uint lang, const char* coded, string& phrase) {
   phrase = recodePhrase(coded, (lang == 1) ? wvoc1 : wvoc2, sep);
   return phrase;
}

template<class T>
void PhraseTableGen<T>::getPhrase(Uint lang, const char* coded, vector<string>& toks) {
   toks.clear();
   decompressPhrase(coded, toks, (lang == 1) ? wvoc1 : wvoc2);
}

template<class T>
Uint PhraseTableGen<T>::getPhraseLength(Uint lang, Uint id) {
   if (lang == 1) {
      if (id > lang1_voc.size())
         error(ETFatal, "Phrase for index %d in lang 1 not stored in memory.", id);
      return phraseLength(lang1_voc.word(id));
   } else
      return phraseLength(lang2_voc.word(id));
}

template<class T>
Uint PhraseTableGen<T>::getPhraseLength(Uint lang, const char* coded) {
   return phraseLength(coded);
}

template<class T>
void PhraseTableGen<T>::dump_prob_lang2_given_lang1(ostream& ostr)
{
   assert(keep_phrase_table_in_memory);
   ostr.precision(9); // Enough to keep all the precision of a float
   for (Uint i = 0; i < phrase_table.size(); ++i) {
      double sum = 0.0;
      typename PhraseFreqs::iterator pf;
      for (pf = phrase_table[i]->begin(); pf != phrase_table[i]->end(); ++pf)
         sum += pf->second;
      for (pf = phrase_table[i]->begin(); pf != phrase_table[i]->end(); ++pf) {
         double value = pf->second / sum;
         if (value != 0)
            writePhrasePair(ostr, lang2_voc.word(pf->first), lang1_voc.word(i),
        	            NULL, value, wvoc2, wvoc1);
      }
   }
}

template<class T>
void PhraseTableGen<T>::dump_prob_lang1_given_lang2(ostream& ostr)
{
   assert(keep_phrase_table_in_memory);
   ostr.precision(9); // Enough to keep all the precision of a float
   for (Uint i = 0; i < phrase_table.size(); ++i) {
      typename PhraseFreqs::iterator pf;
      for (pf = phrase_table[i]->begin(); pf != phrase_table[i]->end(); ++pf) {
         double value = pf->second / (double) lang2_voc.freq(pf->first);
         if (value != 0)
            writePhrasePair(ostr, lang1_voc.word(i), lang2_voc.word(pf->first),
			    NULL, value, wvoc1, wvoc2);
      }
   }
}

template<class T>
void PhraseTableGen<T>::pruneLang2GivenLang1(Uint fixed_max, Uint per_word_max)
{
   prune1_fixed = fixed_max;
   prune1_per_word = per_word_max;
   if (!keep_phrase_table_in_memory)
      return;
   for (Uint i = 0; i < phrase_table.size(); ++i) {
      const Uint nwords = phraseLength(lang1_voc.word(i));
      prunePhraseFreqs(*phrase_table[i], nwords);
   }
}

template<class T>
void PhraseTableGen<T>::prunePhraseFreqs(PhraseFreqs &phrase_freqs, Uint lang1_num_words)
{
   const Uint n = prune1_fixed + prune1_per_word * lang1_num_words;
   if (n >= phrase_freqs.size())
      return;

   // EJJ 25 April 2010: we now wish to do a stable n best extraction: keep the
   // n best phrase freqs, but without reordering them.  We do this via a
   // pointer.  Documentation heavy because the technique is not that obvious.

   // phrase freq pointers let us sort on the original criterion, while being
   // able to put them back in their original order.
   typedef pair<Uint,T> PhraseFreqItem;
   vector<PhraseFreqItem*> phrase_freqs_ptr(phrase_freqs.size());
   for (Uint i = 0; i < phrase_freqs_ptr.size(); ++i)
      phrase_freqs_ptr[i] = &(phrase_freqs.at(i));
   const PhraseFreqItem* const phrase_freqs_begin = &(*(phrase_freqs.begin()));
   const PhraseFreqItem* const phrase_freqs_end = &(*(phrase_freqs.end()));

   // sort by reverse frequency - use partial sort to order the top n only.
   partial_sort(phrase_freqs_ptr.begin(), phrase_freqs_ptr.begin()+n,
                phrase_freqs_ptr.end(), ComparePhrasesByJointFreq());
   // sort the n best in memory order, to restore the original ordering.
   sort(phrase_freqs_ptr.begin(), phrase_freqs_ptr.begin()+n);
   // Now put the n best elements at the beginning of phrase_freqs, but do so
   // in such a way as to preserve the order they occurred in, and in such a
   // way as to keep the other ones around in the rest of the structure, for
   // proper discounting.
   for (Uint i = 0; i < n; ++i) {
      assert(phrase_freqs_ptr[i] >= phrase_freqs_begin);
      assert(phrase_freqs_ptr[i] < phrase_freqs_end);
      if ( phrase_freqs_ptr[i] > &(phrase_freqs.at(i)) )
         swap(*(phrase_freqs_ptr[i]), phrase_freqs.at(i));
      else if ( phrase_freqs_ptr[i] == &(phrase_freqs.at(i)) )
         ; // nothing to do since we keep i in its original place
      else
         assert(false && "i should never be greater than ptr[i]");
   }

   typename PhraseFreqs::iterator pf;
   for (pf = phrase_freqs.begin()+n; pf < phrase_freqs.end(); ++pf)
      lang2_voc.freq(pf->first) -= pf->second;
   phrase_freqs.resize(n);
}

template<class T>
void PhraseTableGen<T>::dump_freqs_lang1(ostream& ostr)
{
   assert(keep_phrase_table_in_memory);
   for (Uint i = 0; i < lang1_voc.size(); ++i) {
      double sum = 0.0;
      typename PhraseFreqs::iterator pf;
      for (pf = phrase_table[i]->begin(); pf != phrase_table[i]->end(); ++pf)
         sum += pf->second;
      ostr << recodePhrase(lang1_voc.word(i), wvoc1)
           << " " << sum << nf_endl;
   }
   ostr.flush();
}

template<class T>
void PhraseTableGen<T>::dump_freqs_lang2(ostream& ostr)
{
   for (Uint i = 0; i < lang2_voc.size(); ++i) {
      ostr << recodePhrase(lang2_voc.word(i), wvoc2)
           << " " << lang2_voc.freq(i) << nf_endl;
   }
   ostr.flush();
}

template<class T>
void PhraseTableGen<T>::remap_psep_helper(Voc& voc, const string& token, const string& replacement)
{
   Uint sep_index = voc.index(token.c_str());
   if (sep_index != voc.size()) {
      if (voc.word(sep_index) == token) { // otherwise remap_psep() was already done!
         if (voc.index(replacement.c_str()) != voc.size()) {
            // replacement already exists, replace it too, recursively until there are no collisions.
            remap_psep_helper(voc, replacement, "_" + replacement);
         }
         bool rc = voc.remap(token.c_str(), replacement.c_str());
         assert(rc);
      }
   }
}

template<class T>
void PhraseTableGen<T>::remap_psep()
{
   // If for some odd reason the source corpora have ||| change it for ___|||___
   remap_psep_helper(wvoc1, psep, psep_replacement);
   remap_psep_helper(wvoc2, psep, psep_replacement);
}

template<class T>
void PhraseTableGen<T>::dump_joint_freqs(ostream& ostr,
      T thresh, bool reverse, bool filt,
      Uint display_alignments)
{
   remap_psep();

   string p1, p2, alignments_s;
   const char* alignments_cstr(NULL);
   for (iterator it = begin(); it != end(); ++it) {
      if (!filt || it.getJointFreq() >= thresh) {
         it.getPhrase(1, p1);
         it.getPhrase(2, p2);
         if ( display_alignments ) {
            it.getAlignmentString(alignments_s, reverse, (display_alignments == 1));
            alignments_cstr = alignments_s.c_str();
         }
         if (!reverse)
            writePhrasePair(ostr, p1.c_str(), p2.c_str(), alignments_cstr, it.getJointFreq());
         else
            writePhrasePair(ostr, p2.c_str(), p1.c_str(), alignments_cstr, it.getJointFreq());
      }
   }
}

template<class T>
void PhraseTableGen<T>::readJointTable(istream& in, bool reduce_memory, bool swap_on_read)
{
   string line, alignments;
   Uint id1, id2;
   T val;
   this->swap_on_read = swap_on_read;

   if (phrase_table_read && !keep_phrase_table_in_memory)
      error(ETFatal, "Cannot read multiple phrase table files "
            "if not keeping phrase tables in memory.");
   else if (!phrase_table_read) {
      keep_phrase_table_in_memory = ! reduce_memory;
      jpt_stream = &in; // used for unit testing only
   }

   // In "reduce memory" mode (for joint2cond_phrase_tables), we don't
   // actually read the phrase table at this point, we just store the file name
   // (in the other readJointTable() method) and the stream.
   if (!keep_phrase_table_in_memory) return;

   phrase_table_read = false;   // reading more.
   Uint counter(0);
   while (getline(in, line)) {
      lookupPhrasePair(line, id1, id2, val, alignments);
      if ( ++counter % 1000000 == 0 ) cerr << ".";
   }
   cerr << endl;
   phrase_table_read = true;
}

template<class T>
void PhraseTableGen<T>::readJointTable(const string& infile, bool reduce_memory, bool swap_on_read)
{
   jpt_file = infile;
   iSafeMagicStream in(infile);
   readJointTable(in, reduce_memory, swap_on_read);

   // jpt_stream is saved for unit testing, where readJointTable(istream&...)
   // is called directly, but it is not valid when readJointTable(string&...) is
   // called.
   jpt_stream = NULL;
}

template<class T>
typename PhraseTableGen<T>::iterator PhraseTableGen<T>::begin()
{
   return iterator(this);
}

template<class T>
typename PhraseTableGen<T>::iterator PhraseTableGen<T>::end()
{
   return iterator(this, true);
}

/*---------------------------------------------------------------------------------------------
  PhraseTableBase
  -------------------------------------------------------------------------------------------*/

template<class T>
void PhraseTableBase::writePhrasePair(ostream& os, const char* p1, const char* p2,
                                      const char* alignment_info,
                                      T val, Voc& voc1, Voc& voc2)
{
   string s1 = recodePhrase(p1, voc1);
   string s2 = recodePhrase(p2, voc2);
   writePhrasePair(os, s1.c_str(), s2.c_str(), alignment_info, val);
}

template<class T>
void PhraseTableBase::writePhrasePair(ostream& os, const char* p1, const char* p2,
                                      const char* alignment_info,
                                      T val)
{
   os << p1 << " " << psep << " " << p2 << " " << psep << " " << val;
   if ( alignment_info ) {
      if ( alignment_info[0] == '\0' ) {
         static bool warning_printed = false;
         if ( !warning_printed ) {
            error(ETWarn, "Ignoring request to print empty alignment information.");
            warning_printed = true;
         }
      } else {
         os << " a=" << alignment_info;
      }
   }
   os << nf_endl;
}

template<class T>
void PhraseTableBase::writePhrasePair(ostream& os, const char* p1, const char* p2,
                                      const char* alignment_info,
                                      vector<T>& vals, bool write_count, T count, 
                                      vector<T>* avals)
{
   os << p1 << " " << psep << " " << p2 << " " << psep;
   for (Uint i = 0; i < vals.size(); ++i)
      os << " " << vals[i];
   if ( alignment_info ) {
      if ( alignment_info[0] == '\0' ) {
         static bool warning_printed = false;
         if ( !warning_printed ) {
            error(ETWarn, "Ignoring request to print empty alignment information in multi-prob table.");
            warning_printed = true;
         }
      } else {
         os << " a=" << alignment_info;
      }
   }
   if ( write_count )
      os << " c=" << count;
   if (avals && avals->size()) {
      os << ' ' << psep;
      for (Uint i = 0; i < avals->size(); ++i)
         os << ' ' << (*avals)[i];
   }
   os << nf_endl;
}

/*---------------------------------------------------------------------------------------------
  PhraseTableGen::iterator implementation. Assumes throughout that each L1
  phrase maps to at least one corresponding L2 phrase (i.e., no empty rows).
  -------------------------------------------------------------------------------------------*/

template<class T>
PhraseTableGen<T>::iterator::iterator(PhraseTableGen<T>* pt, bool end/*=false*/)
{
   if (pt->keep_phrase_table_in_memory) {
      strategy_type = memoryIterator;
      iterator_strategy = new MemoryIteratorStrategy(pt, end);
   }
   else if(pt->prune1_fixed || pt->prune1_per_word) {
      strategy_type = pruningIterator;
      iterator_strategy = new Pruning2IteratorStrategy(pt, end);
   }
   else {
      strategy_type = fileIterator;
      iterator_strategy = new File2IteratorStrategy(pt, end);
   }
}

template<class T>
PhraseTableGen<T>::iterator::iterator()
   : strategy_type(noStrategy)
{
   // Having a bogus strategy instantiated here keeps the rest of the code
   // simpler.
   iterator_strategy = new MemoryIteratorStrategy();
}

template<class T>
PhraseTableGen<T>::iterator::~iterator()
{
   delete iterator_strategy;
}

// copy constructor
// Note: for the fileIterator and pruningIterator, ownership of the jpt stream
// is transferred, i.e. jpt_stream of the source becomes NULL.
template<class T>
PhraseTableGen<T>::iterator::iterator(const PhraseTableGen<T>::iterator &it) :
   strategy_type(it.strategy_type)
{
   strategy_type = it.strategy_type;
   switch (strategy_type) {
      case memoryIterator:
      default:
         MemoryIteratorStrategy* mis;
         mis = new MemoryIteratorStrategy();
         iterator_strategy = mis;
         *mis = *(it.iterator_strategy);
         break;
      case fileIterator:
         File2IteratorStrategy* fis;
         fis = new File2IteratorStrategy();
         iterator_strategy = fis;
         *fis = *(it.iterator_strategy);
         break;
      case pruningIterator:
         Pruning2IteratorStrategy* pis;
         pis = new Pruning2IteratorStrategy();
         iterator_strategy = pis;
         *pis = *(it.iterator_strategy);
         break;
   }
}

template<class T>
typename PhraseTableGen<T>::iterator& PhraseTableGen<T>::iterator::operator=(const iterator& it)
{
   if (strategy_type != it.strategy_type) {
      delete iterator_strategy;
      strategy_type = it.strategy_type;
      if (strategy_type == fileIterator)
         iterator_strategy = new File2IteratorStrategy();
      else if (strategy_type == pruningIterator)
         iterator_strategy = new Pruning2IteratorStrategy();
      else // memoryIterator || noStrategy
         iterator_strategy = new MemoryIteratorStrategy();
   }
   switch (strategy_type) {
      case memoryIterator:
      default:
         MemoryIteratorStrategy* mis;
         mis = dynamic_cast<MemoryIteratorStrategy*>(iterator_strategy);
         assert(mis);
         *mis = *(it.iterator_strategy);
         break;
      case fileIterator:
         File2IteratorStrategy* fis;
         fis = dynamic_cast<File2IteratorStrategy*>(iterator_strategy);
         assert(fis);
         *fis = *(it.iterator_strategy);
         break;
      case pruningIterator:
         Pruning2IteratorStrategy* pis;
         pis = dynamic_cast<Pruning2IteratorStrategy*>(iterator_strategy);
         assert(pis);
         *pis = *(it.iterator_strategy);
         break;
   }
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator& PhraseTableGen<T>::iterator::operator++()
{
   ++*iterator_strategy;
   return *this;
}

template<class T>
bool PhraseTableGen<T>::iterator::operator==(const iterator& it) const
{
   return *iterator_strategy == *it.iterator_strategy;
}

template<class T>
void PhraseTableGen<T>::iterator::getPhrase(Uint lang, vector<string>& toks) const
{
   iterator_strategy->getPhrase(lang, toks);
}

template<class T>
string& PhraseTableGen<T>::iterator::getPhrase(Uint lang, string& phrase) const
{
   return iterator_strategy->getPhrase(lang, phrase);
}

template<class T>
Uint PhraseTableGen<T>::iterator::getPhraseLength(Uint lang) const
{
   return iterator_strategy->getPhraseLength(lang);
}

template<class T>
Uint PhraseTableGen<T>::iterator::getPhraseIndex(Uint lang) const
{
   return iterator_strategy->getPhraseIndex(lang);
}

template<class T>
const AlignmentFreqs<T>&
PhraseTableGen<T>::iterator::getAlignments() const
{
   return iterator_strategy->getAlignments();
}

template<class T>
void
PhraseTableGen<T>::iterator::getAlignmentString(string& al, bool reverse, bool top_only) const
{
   iterator_strategy->getAlignmentString(al, reverse, top_only);
}

template<class T>
T PhraseTableGen<T>::iterator::getJointFreq() const
{
   return iterator_strategy->getJointFreq();
}

template<class T>
T& PhraseTableGen<T>::iterator::getJointFreqRef()
{
   return iterator_strategy->getJointFreqRef();
}

/*---------------------------------------------------------------------------------------------
  IteratorStrategy implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
typename PhraseTableGen<T>::iterator::IteratorStrategy&
PhraseTableGen<T>::iterator::IteratorStrategy::operator=(const IteratorStrategy& it)
{
   pt = it.pt;
   end = it.end;
   id1 = it.id1;
   id2 = it.id2;
   return *this;
}

template<class T>
bool PhraseTableGen<T>::iterator::IteratorStrategy::operator==(const IteratorStrategy& it) const
{
   // This implementation is carefully made correct for all 5 subclasses.
   if (end || it.end) return end == it.end;
   return id1 == it.id1 && id2 == it.id2;
}

template<class T>
void PhraseTableGen<T>::iterator::IteratorStrategy::getPhrase(Uint lang, vector<string>& toks) const
{
   pt->getPhrase(lang, getPhraseIndex(lang), toks);
}

template<class T>
string& PhraseTableGen<T>::iterator::IteratorStrategy::getPhrase(Uint lang, string& phrase) const
{
   pt->getPhrase(lang, getPhraseIndex(lang), phrase);
   return phrase;
}

template<class T>
Uint PhraseTableGen<T>::iterator::IteratorStrategy::getPhraseLength(Uint lang) const
{
   return pt->getPhraseLength(lang, getPhraseIndex(lang));
}

template<class T>
const AlignmentFreqs<T>&
PhraseTableGen<T>::iterator::IteratorStrategy::getAlignments() const
{
   assert(false && "getAlignments is only implemented for the in-memory iterator strategy; but don't work around this limitation by calling getAlignmentString() and parsing it; instead implement getAlignments() in the iterator strategy where you need it");
   static const AlignmentFreqs<T> empty_alignments;
   return empty_alignments;
}

/*---------------------------------------------------------------------------------------------
  MemoryIteratorStrategy implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
PhraseTableGen<T>::iterator::MemoryIteratorStrategy::MemoryIteratorStrategy(PhraseTableGen<T>* pt_, bool end_/*=false*/) :
   IteratorStrategy(pt_, end_)
{
   if (end) return;
   PhraseTable& phrase_table(pt->phrase_table);
   if (phrase_table.empty()) {
      end = true;
      return;
   }
   row_iter = phrase_table.begin();
   row_iter_end = phrase_table.end();
   elem_iter = (*row_iter)->begin();
   elem_iter_end = (*row_iter)->end();
   id1 = 0;
   id2 = elem_iter->first;

   al_row_iter = pt->phrase_alignment_table.begin();
   al_row_iter_end = pt->phrase_alignment_table.end();
   /*
   if ( al_row_iter != al_row_iter_end ) {
      al_elem_iter = (*al_row_iter).begin();
      al_elem_iter_end = (*al_row_iter).end();
   }
   */
}

template<class T>
typename PhraseTableGen<T>::iterator::MemoryIteratorStrategy&
PhraseTableGen<T>::iterator::MemoryIteratorStrategy::operator=(const IteratorStrategy& it)
{
   const MemoryIteratorStrategy* m_it(dynamic_cast<const MemoryIteratorStrategy*>(&it));
   assert(m_it != 0);
   IteratorStrategy::operator=(it);
   row_iter = m_it->row_iter;
   row_iter_end = m_it->row_iter_end;
   elem_iter = m_it->elem_iter;
   elem_iter_end = m_it->elem_iter_end;
   al_row_iter = m_it->al_row_iter;
   al_row_iter_end = m_it->al_row_iter_end;
   /*
   al_elem_iter = m_it->al_elem_iter;
   al_elem_iter_end = m_it->al_elem_iter_end;
   */
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator::MemoryIteratorStrategy&
PhraseTableGen<T>::iterator::MemoryIteratorStrategy::operator++()
{
   if (end) return *this;

   if (++elem_iter == elem_iter_end) {
      ++id1;
      if (++row_iter == row_iter_end) {
         end = true;
         return *this;
      }
      elem_iter = (*row_iter)->begin();
      elem_iter_end = (*row_iter)->end();
      if ( al_row_iter != al_row_iter_end ) ++al_row_iter;
   }
   id2 = elem_iter->first;
   return *this;
}

template<class T>
const AlignmentFreqs<T>&
PhraseTableGen<T>::iterator::MemoryIteratorStrategy::getAlignments() const
{
   static const AlignmentFreqs<T> empty_alignments;
   /*
   if ( al_row_iter != al_row_iter_end && al_elem_iter != al_elem_iter_end )
      return al_elem_iter->second;
   else
      return empty_alignments;
   */

   if ( al_row_iter != al_row_iter_end ) {
      typename PhraseAlignments::iterator al_elem_iter = (*al_row_iter)->find(id2);
      if ( al_elem_iter != (*al_row_iter)->end() )
         return al_elem_iter->second;
   }
   return empty_alignments;
}

template<class T>
void
PhraseTableGen<T>::iterator::MemoryIteratorStrategy::getAlignmentString(string& al, bool reverse, bool top_only) const
{
   const AlignmentFreqs<T>& alignments(getAlignments());
   displayAlignments(al, alignments, pt->alignment_voc,
                     getPhraseLength(1), getPhraseLength(2), reverse, top_only);
}

template<class T>
T PhraseTableGen<T>::iterator::MemoryIteratorStrategy::getJointFreq() const
{
   return elem_iter->second;
};

template<class T>
T& PhraseTableGen<T>::iterator::MemoryIteratorStrategy::getJointFreqRef()
{
   return elem_iter->second;
};

/*---------------------------------------------------------------------------------------------
  FileIteratorStrategy implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
PhraseTableGen<T>::iterator::FileIteratorStrategy::FileIteratorStrategy(PhraseTableGen<T>* pt_, bool end_/*=false*/) :
   IteratorStrategy(pt_, end_), jpt_stream(NULL)
{
   if (end) return;
   jpt_stream.reset();
   if (pt->jpt_file.length() > 0) {
      auto_ptr<iSafeMagicStream> new_stream(new iSafeMagicStream(pt->jpt_file));
      jpt_stream = new_stream;
   } else if (pt->jpt_stream) {
      // This path is used only for unit testing
      auto_ptr<istream> new_stream(new istream(pt->jpt_stream->rdbuf()));
      jpt_stream = new_stream;
      jpt_stream->clear();
      jpt_stream->seekg(0);
   } else
      error(ETFatal, "No input stream set.");
   operator++();
}

template<class T>
typename PhraseTableGen<T>::iterator::FileIteratorStrategy&
PhraseTableGen<T>::iterator::FileIteratorStrategy::operator=(const IteratorStrategy& it)
{
   const FileIteratorStrategy* f_it(dynamic_cast<const FileIteratorStrategy*>(&it));
   assert(f_it != 0);
   IteratorStrategy::operator=(it);
   val = f_it->val;
   alignments = f_it->alignments;
   // Note: ownership of the jpt stream is transferred, i.e. jpt_stream of the
   // source becomes NULL.
   jpt_stream = (const_cast<FileIteratorStrategy *>(f_it))->jpt_stream;
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator::FileIteratorStrategy&
PhraseTableGen<T>::iterator::FileIteratorStrategy::operator++()
{
   if (end) return *this;
   assert(jpt_stream.get() != NULL);
   string line;
   if (! getline(*jpt_stream, line)) {
      end = true;
      pt->phrase_table_read = true;
      return *this;
   }
   pt->lookupPhrasePair(line, id1, id2, val, alignments);
   return *this;
}

template<class T>
void
PhraseTableGen<T>::iterator::FileIteratorStrategy::getAlignmentString(string& al, bool reverse, bool top_only) const
{
   if ( top_only || reverse ) {
      AlignmentFreqs<T> alignment_freqs;
      parseAndTallyAlignments(alignment_freqs, pt->alignment_voc, alignments);
      displayAlignments(al, alignment_freqs, pt->alignment_voc,
                        getPhraseLength(1), getPhraseLength(2), reverse, top_only);
   } else {
      // In the default case, we don't actually have to parse the alignment
      // string, we just copy it through.
      al = alignments;
   }
};

template<class T>
T PhraseTableGen<T>::iterator::FileIteratorStrategy::getJointFreq() const
{
   return val;
};

template<class T>
T& PhraseTableGen<T>::iterator::FileIteratorStrategy::getJointFreqRef()
{
   return val;
};

/*---------------------------------------------------------------------------------------------
  File2IteratorStrategy implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
PhraseTableGen<T>::iterator::File2IteratorStrategy::File2IteratorStrategy(PhraseTableGen<T>* pt_, bool end_/*=false*/) :
   FileIteratorStrategy(pt_, end_)
{
   if (end) return;
   vector<string> toks;
   pt->getPhrase(1, id1, toks);
   pt->compressPhrase(toks.begin(), toks.end(), phrase1, pt->wvoc1);
}

template<class T>
typename PhraseTableGen<T>::iterator::File2IteratorStrategy&
PhraseTableGen<T>::iterator::File2IteratorStrategy::operator=(const IteratorStrategy& it)
{
   const File2IteratorStrategy* f_it(dynamic_cast<const File2IteratorStrategy*>(&it));
   assert(f_it != 0);
   FileIteratorStrategy::operator=(it);
   phrase1 = f_it->phrase1;
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator::File2IteratorStrategy&
PhraseTableGen<T>::iterator::File2IteratorStrategy::operator++()
{
   if (end) return *this;
   assert(jpt_stream.get() != NULL);
   string line;
   if (! getline(*jpt_stream, line)) {
      end = true;
      pt->phrase_table_read = true;
      return *this;
   }
   string next_ph1;
   pt->lookupPhrasePair(line, next_ph1, id2, val, alignments);
   if (next_ph1 != phrase1) {
      phrase1 = next_ph1;
      ++id1;
      if (! pt->phrase_table_read)
         ++pt->num_lang1_phrases;
   }
   return *this;
}

template<class T>
void PhraseTableGen<T>::iterator::File2IteratorStrategy::getPhrase(Uint lang, vector<string>& toks) const
{
   if (lang == 1)
      pt->getPhrase(lang, phrase1.c_str(), toks);
   else
      pt->getPhrase(lang, getPhraseIndex(lang), toks);
}

template<class T>
string& PhraseTableGen<T>::iterator::File2IteratorStrategy::getPhrase(Uint lang, string& phrase) const
{
   if (lang == 1)
      pt->getPhrase(lang, phrase1.c_str(), phrase);
   else
      pt->getPhrase(lang, getPhraseIndex(lang), phrase);
   return phrase;
}

template<class T>
Uint PhraseTableGen<T>::iterator::File2IteratorStrategy::getPhraseLength(Uint lang) const
{
   if (lang == 1)
      return pt->getPhraseLength(lang, phrase1.c_str());
   else
      return pt->getPhraseLength(lang, getPhraseIndex(lang));
}

/*---------------------------------------------------------------------------------------------
  PruningIteratorStrategy implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
PhraseTableGen<T>::iterator::PruningIteratorStrategy::PruningIteratorStrategy(PhraseTableGen<T>* pt_, bool end_/*=false*/) :
   FileIteratorStrategy(pt_, end_)
{
   if (end) return;
   pf_index = phrase_freqs.size();
   next_id1 = id1;
   next_id2 = id2;
   next_val = val;
   next_alignment_string = alignments;
   operator++();
}

template<class T>
typename PhraseTableGen<T>::iterator::PruningIteratorStrategy&
PhraseTableGen<T>::iterator::PruningIteratorStrategy::operator=(const IteratorStrategy& it)
{
   const PruningIteratorStrategy* p_it(dynamic_cast<const PruningIteratorStrategy*>(&it));
   assert(p_it != 0);
   FileIteratorStrategy::operator=(it);
   next_id1 = p_it->next_id1;
   next_id2 = p_it->next_id2;
   next_val = p_it->next_val;
   next_alignment_string = p_it->next_alignment_string;
   phrase_freqs = p_it->phrase_freqs;
   alignment_strings = p_it->alignment_strings;
   pf_index = p_it->pf_index;
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator::PruningIteratorStrategy&
PhraseTableGen<T>::iterator::PruningIteratorStrategy::operator++()
{
   if (end) return *this;

   if (++pf_index >= phrase_freqs.size()) {
      assert(jpt_stream.get() != NULL);
      string line;
      if (jpt_stream->eof()) {
         end = true;
         pt->phrase_table_read = true;
         return *this;
      }
      id1 = next_id1;
      phrase_freqs.clear();
      phrase_freqs[next_id2] += next_val;
      alignment_strings.clear();
      if ( ! next_alignment_string.empty() )
         alignment_strings[next_id2] = next_alignment_string;
      while (getline(*jpt_stream, line)) {
         pt->lookupPhrasePair(line, next_id1, next_id2, next_val, next_alignment_string);
         if (next_id1 == id1) {
            phrase_freqs[next_id2] += next_val;
            if ( ! next_alignment_string.empty() )
               alignment_strings[next_id2] = next_alignment_string;
         }
         else break;
      }
      pt->prunePhraseFreqs(id1, phrase_freqs, getPhraseLength(1));
      pf_index = 0;
   }

   id2 = phrase_freqs.at(pf_index).first;
   val = phrase_freqs.at(pf_index).second;
   alignments = alignment_strings[id2];
   return *this;
}

/*---------------------------------------------------------------------------------------------
  Pruning2IteratorStrategy implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
PhraseTableGen<T>::iterator::Pruning2IteratorStrategy::Pruning2IteratorStrategy(PhraseTableGen<T>* pt_, bool end_/*=false*/) :
   File2IteratorStrategy(pt_, end_)
{
   if (end) return;
   pf_index = phrase_freqs.size();
   next_id1 = id1;
   next_id2 = id2;
   next_val = val;
   next_phrase1 = phrase1;
   next_alignment_string = alignments;
   operator++();
}

template<class T>
typename PhraseTableGen<T>::iterator::Pruning2IteratorStrategy&
PhraseTableGen<T>::iterator::Pruning2IteratorStrategy::operator=(const IteratorStrategy& it)
{
   const Pruning2IteratorStrategy* p_it(dynamic_cast<const Pruning2IteratorStrategy*>(&it));
   assert(p_it != 0);
   File2IteratorStrategy::operator=(it);
   next_id1 = p_it->next_id1;
   next_id2 = p_it->next_id2;
   next_val = p_it->next_val;
   next_alignment_string = p_it->next_alignment_string;
   next_phrase1 = p_it->next_phrase1;
   phrase_freqs = p_it->phrase_freqs;
   alignment_strings = p_it->alignment_strings;
   pf_index = p_it->pf_index;
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator::Pruning2IteratorStrategy&
PhraseTableGen<T>::iterator::Pruning2IteratorStrategy::operator++()
{
   if (end) return *this;

   if (++pf_index >= phrase_freqs.size()) {
      assert(jpt_stream.get() != NULL);
      string line;
      if (jpt_stream->eof()) {
         end = true;
         pt->phrase_table_read = true;
         return *this;
      }
      id1 = next_id1;
      phrase1 = next_phrase1;
      phrase_freqs.clear();
      phrase_freqs[next_id2] += next_val;
      alignment_strings.clear();
      if ( ! next_alignment_string.empty() )
         alignment_strings[next_id2] = next_alignment_string;
      while (getline(*jpt_stream, line)) {
         pt->lookupPhrasePair(line, next_phrase1, next_id2, next_val, next_alignment_string);
         if (next_phrase1 == phrase1) {
            phrase_freqs[next_id2] += next_val;
            if ( ! next_alignment_string.empty() )
               alignment_strings[next_id2] = next_alignment_string;
         } else {
            ++next_id1;
            if (! pt->phrase_table_read)
               ++pt->num_lang1_phrases;
            break;
         }
      }
      pt->prunePhraseFreqs(phrase_freqs, getPhraseLength(1));
      pf_index = 0;
   }

   id2 = phrase_freqs.at(pf_index).first;
   val = phrase_freqs.at(pf_index).second;
   alignments = alignment_strings[id2];
   return *this;
}

} // Portage

#endif // PHRASE_TABLE_CC_H
