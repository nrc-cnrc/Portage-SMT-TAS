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

/*---------------------------------------------------------------------------------------------
  PhraseTableGen implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
void PhraseTableGen<T>::clear()
{
   lang1_voc.clear();
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

   const typename PhraseFreqs::iterator iter = phrase_table[id1].find(id2);
   if (iter == phrase_table[id1].end())
      return false;

   val = iter->second;
   return true;
}

template<class T>
void PhraseTableGen<T>::addPhrasePair(ToksIter beg1, ToksIter end1, ToksIter beg2, ToksIter end2, T val)
{
   string phrase1, phrase2;
   compressPhrase(beg1, end1, phrase1, wvoc1);
   compressPhrase(beg2, end2, phrase2, wvoc2);

   Uint id1 = lang1_voc.add(phrase1.c_str());
   num_lang1_phrases = lang1_voc.size();
   Uint id2 = lang2_voc.add(phrase2.c_str(), val);
   if (keep_phrase_table_in_memory) {
      if (id1 == phrase_table.size()) {
         phrase_table.push_back(PhraseFreqs());
      }
      phrase_table[id1][id2] += val;
   }
}

template<class T>
void PhraseTableGen<T>::lookupPhrasePair(string &line, Uint& id1, Uint& id2, T& val)
{
   vector<string> toks;
   typename PhraseTableGen<T>::ToksIter b1, e1, b2, e2, v;
   string phrase1, phrase2;

   extractTokens(line, toks, b1, e1, b2, e2, v);
   compressPhrase(b1, e1, phrase1, wvoc1);
   compressPhrase(b2, e2, phrase2, wvoc2);

   val = conv<T>(*v);

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
            phrase_table.push_back(PhraseFreqs());
         phrase_table[id1][id2] += val;
      }
   }
}

template<class T>
void PhraseTableGen<T>::lookupPhrasePair(string &line, string& phrase1, Uint& id2, T& val)
{
   vector<string> toks;
   typename PhraseTableGen<T>::ToksIter b1, e1, b2, e2, v;
   string phrase2;

   assert(! keep_phrase_table_in_memory);

   extractTokens(line, toks, b1, e1, b2, e2, v);
   compressPhrase(b1, e1, phrase1, wvoc1);
   compressPhrase(b2, e2, phrase2, wvoc2);

   val = conv<T>(*v);

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
string& PhraseTableGen<T>::getPhrase(Uint lang, const string& coded, string& phrase) {
   phrase = recodePhrase(coded, (lang == 1) ? wvoc1 : wvoc2, sep);
   return phrase;
}

template<class T>
void PhraseTableGen<T>::getPhrase(Uint lang, const string& coded, vector<string>& toks) {
   toks.clear();
   decompressPhrase(coded, toks, (lang == 1) ? wvoc1 : wvoc2);
}

template<class T>
void PhraseTableGen<T>::dump_prob_lang2_given_lang1(ostream& ostr)
{
   assert(keep_phrase_table_in_memory);
   ostr.precision(9); // Enough to keep all the precision of a float
   for (Uint i = 0; i < phrase_table.size(); ++i) {
      double sum = 0.0;
      typename PhraseFreqs::iterator pf;
      for (pf = phrase_table[i].begin(); pf != phrase_table[i].end(); ++pf)
         sum += pf->second;
      for (pf = phrase_table[i].begin(); pf != phrase_table[i].end(); ++pf) {
         double value = pf->second / sum;
         if (value != 0)
            writePhrasePair(ostr, lang2_voc.word(pf->first), lang1_voc.word(i),
        	            value, wvoc2, wvoc1);
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
      for (pf = phrase_table[i].begin(); pf != phrase_table[i].end(); ++pf) {
         double value = pf->second / (double) lang2_voc.freq(pf->first);
         if (value != 0)
            writePhrasePair(ostr, lang1_voc.word(i), lang2_voc.word(pf->first),
			    value, wvoc1, wvoc2);
      }
   }
}

template<class T>
void PhraseTableGen<T>::pruneLang2GivenLang1(Uint nmax, bool per_word)
{
   prune1 = nmax;
   if (!keep_phrase_table_in_memory)
      return;
   for (Uint i = 0; i < phrase_table.size(); ++i) {
      const Uint nwords = max(strlen(lang1_voc.word(i)), 1);   // playing it safe..
      const Uint n = per_word ? nmax * nwords / num_code_bytes : nmax;
      prunePhraseFreqs(phrase_table[i], n);
   }
}

template<class T>
void PhraseTableGen<T>::prunePhraseFreqs(PhraseFreqs &phrase_freqs, Uint n)
{
   if (n >= phrase_freqs.size())
      return;
   partial_sort(phrase_freqs.begin(), phrase_freqs.begin()+n, phrase_freqs.end(),
                ComparePhrasesByJointFreq());
   typename PhraseFreqs::iterator pf;
   for (pf = phrase_freqs.begin()+n; pf < phrase_freqs.end(); ++pf)
      lang2_voc.freq(pf->first) -= pf->second;
   phrase_freqs.resize(n);
}

template<class T>
void PhraseTableGen<T>::dump_freqs_lang1(ostream& ostr)
{
   assert(keep_phrase_table_in_memory);
   vector<string> toks;
   string ph;
   for (Uint i = 0; i < lang1_voc.size(); ++i) {
      double sum = 0.0;
      typename PhraseFreqs::iterator pf;
      for (pf = phrase_table[i].begin(); pf != phrase_table[i].end(); ++pf)
         sum += pf->second;
      toks.clear();
      ph = lang1_voc.word(i);
      decompressPhrase(ph, toks, wvoc1);
      codePhrase(toks.begin(), toks.end(), ph);
      ostr << ph << " " << sum << nf_endl;
   }
   ostr.flush();
}

template<class T>
void PhraseTableGen<T>::dump_freqs_lang2(ostream& ostr)
{
   vector<string> toks;
   string ph;
   for (Uint i = 0; i < lang2_voc.size(); ++i) {
      toks.clear();
      ph = lang2_voc.word(i);
      decompressPhrase(ph, toks, wvoc2);
      codePhrase(toks.begin(), toks.end(), ph);
      ostr << ph << " " << lang2_voc.freq(i) << nf_endl;
   }
   ostr.flush();
}

template<class T>
void PhraseTableGen<T>::dump_joint_freqs(ostream& ostr, T thresh, bool reverse, bool filt)
{
   // If for some odd reason the source corpora have ||| change it for ___|||___
   if (wvoc1.index(psep.c_str()) != wvoc1.size()) {
      wvoc1.remap(psep.c_str(), psep_replacement.c_str());
   }
   if (wvoc2.index(psep.c_str()) != wvoc2.size()) {
      wvoc2.remap(psep.c_str(), psep_replacement.c_str());
   }

   string p1, p2;
   for (iterator it = begin(); it != end(); ++it)
      if (!filt || it.getJointFreq() >= thresh) {
         it.getPhrase(1, p1);
         it.getPhrase(2, p2);
         if (!reverse)
            writePhrasePair(ostr, p1.c_str(), p2.c_str(), it.getJointFreq());
         else
            writePhrasePair(ostr, p2.c_str(), p1.c_str(), it.getJointFreq());
      }
}

template<class T>
void PhraseTableGen<T>::readJointTable(istream& in, bool reduce_memory/*=false*/)
{
   string line;
   Uint id1, id2;
   T val;

   if (phrase_table_read && !keep_phrase_table_in_memory)
      error(ETFatal, "Cannot read multiple phrase table files "
            "if not keeping phrase tables in memory.");
   else if (!phrase_table_read) {
      keep_phrase_table_in_memory = ! reduce_memory;
      jpt_stream = &in;
   }
   if (!keep_phrase_table_in_memory) return;

   phrase_table_read = false;   // reading more.
   while (getline(in, line)) {
      lookupPhrasePair(line, id1, id2, val);
   }
   phrase_table_read = true;
}

template<class T>
void PhraseTableGen<T>::readJointTable(const string& infile, bool reduce_memory/*=false*/)
{
   jpt_file = infile;
   iSafeMagicStream in(infile);
   readJointTable(in, reduce_memory);
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
void PhraseTableBase::writePhrasePair(ostream& os, const char* p1, const char* p2, T val,
                                      Voc& voc1, Voc& voc2)
{
   string s1(p1), s2(p2);
   vector<string> toks;

   decompressPhrase(s1, toks, voc1);
   s1.clear();
   join(toks.begin(), toks.end(), s1, sep);
   toks.clear();

   decompressPhrase(s2, toks, voc2);
   s2.clear();
   join(toks.begin(), toks.end(), s2, sep);

   os << s1 << " " << psep << " " << s2 << " " << psep << " " << val << nf_endl;
}

template<class T>
void PhraseTableBase::writePhrasePair(ostream& os, const char* p1, const char* p2, T val)
{
   os << p1 << " " << psep << " " << p2 << " " << psep << " " << val << nf_endl;
}

template<class T>
void PhraseTableBase::writePhrasePair(ostream& os, const char* p1, const char* p2, vector<T>& vals)
{
   os << p1 << " " << psep << " " << p2 << " " << psep;
   for (Uint i = 0; i < vals.size(); ++i)
      os << " " << vals[i];
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
   else if(pt->prune1) {
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
Uint PhraseTableGen<T>::iterator::getPhraseIndex(Uint lang) const
{
   return iterator_strategy->getPhraseIndex(lang);
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

/*---------------------------------------------------------------------------------------------
  MemoryIteratorStrategy implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
PhraseTableGen<T>::iterator::MemoryIteratorStrategy::MemoryIteratorStrategy(PhraseTableGen<T>* pt_, bool end_/*=false*/) :
   IteratorStrategy(pt_, end_)
{
   if (IteratorStrategy::end) return;
   PhraseTable& phrase_table(IteratorStrategy::pt->phrase_table);
   if (! phrase_table.size()) {
      IteratorStrategy::end = true;
      return;
   }
   row_iter = phrase_table.begin();
   row_iter_end = phrase_table.end();
   elem_iter = (*row_iter).begin();
   elem_iter_end = (*row_iter).end();
   IteratorStrategy::id1 = 0;
   IteratorStrategy::id2 = elem_iter->first;
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
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator::MemoryIteratorStrategy&
PhraseTableGen<T>::iterator::MemoryIteratorStrategy::operator++()
{
   if (IteratorStrategy::end) return *this;

   if (++elem_iter == elem_iter_end) {
      ++IteratorStrategy::id1;
      if (++row_iter == row_iter_end) {
         IteratorStrategy::end = true;
         return *this;
      }
      elem_iter = (*row_iter).begin();
      elem_iter_end = (*row_iter).end();
   }
   IteratorStrategy::id2 = elem_iter->first;
   return *this;
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
   if (IteratorStrategy::end) return;
   jpt_stream.reset();
   if (IteratorStrategy::pt->jpt_file.length() > 0) {
      auto_ptr<iSafeMagicStream> new_stream(new iSafeMagicStream(IteratorStrategy::pt->jpt_file));
      jpt_stream = new_stream;
   } else if (IteratorStrategy::pt->jpt_stream) {
      auto_ptr<istream> new_stream(new istream(IteratorStrategy::pt->jpt_stream->rdbuf()));
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
   // Note: ownership of the jpt stream is transferred, i.e. jpt_stream of the
   // source becomes NULL.
   jpt_stream = (const_cast<FileIteratorStrategy *>(f_it))->jpt_stream;
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator::FileIteratorStrategy&
PhraseTableGen<T>::iterator::FileIteratorStrategy::operator++()
{
   if (IteratorStrategy::end) return *this;
   assert(jpt_stream.get() != NULL);
   string line;
   if (! getline(*jpt_stream, line)) {
      IteratorStrategy::end = true;
      IteratorStrategy::pt->phrase_table_read = true;
      return *this;
   }
   IteratorStrategy::pt->lookupPhrasePair(line, IteratorStrategy::id1,
                                          IteratorStrategy::id2, val);
   return *this;
}

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
   if (IteratorStrategy::end) return;
   vector<string> toks;
   IteratorStrategy::pt->getPhrase(1, IteratorStrategy::id1, toks);
   IteratorStrategy::pt->compressPhrase(toks.begin(), toks.end(), phrase1,
                                        IteratorStrategy::pt->wvoc1);
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
   if (IteratorStrategy::end) return *this;
   assert(FileIteratorStrategy::jpt_stream.get() != NULL);
   string line;
   if (! getline(*FileIteratorStrategy::jpt_stream, line)) {
      IteratorStrategy::end = true;
      IteratorStrategy::pt->phrase_table_read = true;
      return *this;
   }
   string next_ph1;
   IteratorStrategy::pt->lookupPhrasePair(line, next_ph1, IteratorStrategy::id2,
                                          FileIteratorStrategy::val);
   if (next_ph1 != phrase1) {
      phrase1 = next_ph1;
      ++IteratorStrategy::id1;
      if (! IteratorStrategy::pt->phrase_table_read)
         ++IteratorStrategy::pt->num_lang1_phrases;
   }
   return *this;
}

template<class T>
void PhraseTableGen<T>::iterator::File2IteratorStrategy::getPhrase(Uint lang, vector<string>& toks) const
{
   if (lang == 1)
      IteratorStrategy::pt->getPhrase(lang, phrase1, toks);
   else
      IteratorStrategy::pt->getPhrase(lang, getPhraseIndex(lang), toks);
}

template<class T>
string& PhraseTableGen<T>::iterator::File2IteratorStrategy::getPhrase(Uint lang, string& phrase) const
{
   if (lang == 1)
      IteratorStrategy::pt->getPhrase(lang, phrase1, phrase);
   else
      IteratorStrategy::pt->getPhrase(lang, getPhraseIndex(lang), phrase);
   return phrase;
}

/*---------------------------------------------------------------------------------------------
  PruningIteratorStrategy implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
PhraseTableGen<T>::iterator::PruningIteratorStrategy::PruningIteratorStrategy(PhraseTableGen<T>* pt_, bool end_/*=false*/) :
   FileIteratorStrategy(pt_, end_)
{
   if (IteratorStrategy::end) return;
   pf_index = phrase_freqs.size();
   next_id1 = IteratorStrategy::id1;
   next_id2 = IteratorStrategy::id2;
   next_val = FileIteratorStrategy::val;
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
   phrase_freqs = p_it->phrase_freqs;
   pf_index = p_it->pf_index;
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator::PruningIteratorStrategy&
PhraseTableGen<T>::iterator::PruningIteratorStrategy::operator++()
{
   if (FileIteratorStrategy::end) return *this;

   if (++pf_index >= phrase_freqs.size()) {
      assert(FileIteratorStrategy::jpt_stream.get() != NULL);
      string line;
      if (FileIteratorStrategy::jpt_stream->eof()) {
         IteratorStrategy::end = true;
         IteratorStrategy::pt->phrase_table_read = true;
         return *this;
      }
      IteratorStrategy::id1 = next_id1;
      phrase_freqs.clear();
      phrase_freqs[next_id2] += next_val;
      while (getline(*FileIteratorStrategy::jpt_stream, line)) {
         IteratorStrategy::pt->lookupPhrasePair(line, next_id1, next_id2, next_val);
         if (next_id1 == IteratorStrategy::id1)
            phrase_freqs[next_id2] += next_val;
         else break;
      }
      IteratorStrategy::pt->prunePhraseFreqs(IteratorStrategy::id1, phrase_freqs,
                                             IteratorStrategy::pt->prune1);
      pf_index = 0;
   }

   IteratorStrategy::id2 = phrase_freqs.at(pf_index).first;
   FileIteratorStrategy::val = phrase_freqs.at(pf_index).second;
   return *this;
}

/*---------------------------------------------------------------------------------------------
  Pruning2IteratorStrategy implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
PhraseTableGen<T>::iterator::Pruning2IteratorStrategy::Pruning2IteratorStrategy(PhraseTableGen<T>* pt_, bool end_/*=false*/) :
   File2IteratorStrategy(pt_, end_)
{
   if (IteratorStrategy::end) return;
   pf_index = phrase_freqs.size();
   next_id1 = IteratorStrategy::id1;
   next_id2 = IteratorStrategy::id2;
   next_val = FileIteratorStrategy::val;
   next_phrase1 = File2IteratorStrategy::phrase1;
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
   next_phrase1 = p_it->next_phrase1;
   phrase_freqs = p_it->phrase_freqs;
   pf_index = p_it->pf_index;
   return *this;
}

template<class T>
typename PhraseTableGen<T>::iterator::Pruning2IteratorStrategy&
PhraseTableGen<T>::iterator::Pruning2IteratorStrategy::operator++()
{
   if (IteratorStrategy::end) return *this;

   if (++pf_index >= phrase_freqs.size()) {
      assert(File2IteratorStrategy::jpt_stream.get() != NULL);
      string line;
      if (File2IteratorStrategy::jpt_stream->eof()) {
         IteratorStrategy::end = true;
         IteratorStrategy::pt->phrase_table_read = true;
         return *this;
      }
      IteratorStrategy::id1 = next_id1;
      File2IteratorStrategy::phrase1 = next_phrase1;
      phrase_freqs.clear();
      phrase_freqs[next_id2] += next_val;
      while (getline(*File2IteratorStrategy::jpt_stream, line)) {
         IteratorStrategy::pt->lookupPhrasePair(line, next_phrase1, next_id2, next_val);
         if (next_phrase1 == File2IteratorStrategy::phrase1)
            phrase_freqs[next_id2] += next_val;
         else {
            ++next_id1;
            if (! IteratorStrategy::pt->phrase_table_read)
               ++IteratorStrategy::pt->num_lang1_phrases;
            break;
         }
      }
      IteratorStrategy::pt->prunePhraseFreqs(phrase_freqs, IteratorStrategy::pt->prune1);
      pf_index = 0;
   }

   IteratorStrategy::id2 = phrase_freqs.at(pf_index).first;
   FileIteratorStrategy::val = phrase_freqs.at(pf_index).second;
   return *this;
}


#endif // PHRASE_TABLE_CC_H
