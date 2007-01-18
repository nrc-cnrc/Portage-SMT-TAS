/**
 * @author George Foster
 * @file phrase_table.h  Represent a phrase table with joint frequencies.
 *
 *
 * COMMENTS:
 *
 * Represent a phrase table with joint frequencies.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef PHRASE_TABLE_H
#define PHRASE_TABLE_H

#include <ext/hash_map>
#include <ext/hash_set>
#include <map>
#include <algorithm>
#include <string>
#include <ostream>
#include <cmath>
#include <voc.h>
#include <file_utils.h>
#include <vector_map.h>
#include <portage_defs.h>
#include "ttable.h"
#include "tm_io.h"
#include "ibm.h"
#include "length.h"

namespace Portage {

using namespace __gnu_cxx;

/**
 * Helper base class to factor out some things from the PhraseTable template
 * class so they can be defined in the .cc file.
 */
struct PhraseTableBase
{
   typedef vector<string>::const_iterator ToksIter;

   static const string sep;     // separate words in a phrase
   static const string psep;    // separate phrases in a pair

   // Constants for coding strategy: number of bytes used to code
   // words within phrases, radix for coding, and highest word count
   static const Uint num_code_bytes = 3;
   static const Uint code_base = 255;
   static const Uint max_code = 16581374; // code_base^num_code_bytes - 1

   /**
    * token sequence -> phrase string
    */
   static void codePhrase(ToksIter beg, ToksIter end, string& coded,
                          const string& sep = PhraseTableBase::sep);

   /**
    * phrase string -> token sequence
    */
   static void decodePhrase(const string& coded, vector<string>& toks,
                            const string& sep = PhraseTableBase::sep);

   /**
    * Encode a token sequence as a packed string, the concatenation of 3-byte
      string encodings of voc indexes of the tokens in the sequence.
    */
   static void compressPhrase(ToksIter beg, ToksIter end, string& coded, Voc& voc);

   /**
    * Decode packed string representation of a token sequence.
    */
   static void decompressPhrase(const string& coded, vector<string>& toks, Voc& voc);

   /**
    * Recode a phrase from a packed string representation to a a phrase string
    */
   static string recodePhrase(const string& coded, Voc& voc,
                              const string& sep = PhraseTableBase::sep);

   /**
    * Write a pair of compressed phrase strings + associated value on a stream.
    */
   template<class T>
   static void writePhrasePair(ostream& os, const char* p1, const char* p2,
                               T val, Voc& voc1, Voc& voc2);

   /**
    * Write a pair of normal phrase strings + associated value on a stream.
    */
   template<class T>
   static void writePhrasePair(ostream& os, const char* p1, const char* p2, T val);

   /**
    * Write a pair of normal phrase strings + associated values on a stream.
    */
   template<class T>
   static void writePhrasePair(ostream& os, const char* p1, const char* p2, vector<T>& vals);

   /**
    * Convert a phrase pair read from a stream into a token sequence.
    * @param line line read from a stream
    * @param toks token sequence
    * @param b1, e1 beginning and end+1 markers for 1st phrase
    * @param b2, e2 beginning and end+1 markers for 2nd phrase
    * @param v position of value
    */
   static void extractTokens(const string& line, vector<string>& toks,
                             ToksIter& b1, ToksIter& e1,
                             ToksIter& b2, ToksIter& e2,
                             ToksIter& v);
};

/**
 * Main Phrase table.
 *
 * For compactness, this is stored using a multi-stage strategy. First, phrases
 * in both languages are compressed into packed strings using 3 bytes per word.
 * Next, lang2 compressed phrases are mapped to integer indexes. Finally, lang1
 * compressed phrases are hashed to lists of (lang2-index,freq) pairs. This is
 * a bit wacky, but it does the job.
 */
template<class T> class PhraseTableGen : public PhraseTableBase
{
   typedef vector_map<Uint,T> PhraseFreqs; // l2-index -> frequency
   typedef hash_map<string,PhraseFreqs,StringHash> PhraseTable;

   PhraseTable phrase_table;    // l1compressedphrase -> (l2index,freq), (l2index,freq), ...
   _CountingVoc<T> lang2_voc;   // l2compressedphrase <-> (index, freq)

   Voc wvoc1;                   // l1 _words_ <-> indexes, used to compress/decompress phrases
   Voc wvoc2;                   // l2 ""

public:

   /**
    *  (crude) iterator over contents (phrase pairs and joint frequencies)
    */
   class iterator {

      PhraseTableGen<T>* pt;
      Uint row_count;
      typename PhraseTable::iterator row_iter;
      typename PhraseTable::iterator row_iter_end;
      typename PhraseFreqs::iterator elem_iter;
      typename PhraseFreqs::iterator elem_iter_end;

   public:

      iterator() {};

      iterator(PhraseTableGen<T>* pt,
               typename PhraseTable::iterator row_iter,
               typename PhraseTable::iterator row_iter_end,
               typename PhraseFreqs::iterator elem_iter,
               typename PhraseFreqs::iterator elem_iter_end) :
         pt(pt), row_count(0),
         row_iter(row_iter), row_iter_end(row_iter_end),
         elem_iter(elem_iter), elem_iter_end(elem_iter_end) {}

      void incr();              // increment
      bool equal(const iterator& it) const; // true if equal
      iterator& operator=(const iterator& it); // assignment
      string& getPhrase(Uint lang, string& phrase) const; // lang is 1 or 2
      void getPhrase(Uint lang, vector<string>& toks) const; // lang is 1 or 2
      Uint getPhraseIndex(Uint lang) const; // unique contiguous index for L1 or L2 phrase
      T getJointFreq() const;
   };

   PhraseTableGen() {}
   ~PhraseTableGen() {}

   /**
    * Read contents as a joint table, in the format produced by dump_joint_freqs
    */
   void readJointTable(istream& in);
   void readJointTable(const string& infile);

   /**
    * Write contents to stream, filtering any pairs with freq < thresh.
    * Order is lang1,lang2; or lang2,lang1 if reverse is true.
    */
   void dump_joint_freqs(ostream& ostr, T thresh = 0, bool reverse = false);

   /**
    * Write relative-frequency conditional distributions to stream, in TMText
    * format.
    */
   void dump_prob_lang2_given_lang1(ostream& ostr);
   void dump_prob_lang1_given_lang2(ostream& ostr);

   /**
    * Write marginal frequencies to stream.
    */
   void dump_freqs_lang1(ostream& ostr);
   void dump_freqs_lang2(ostream& ostr);

   /**
    * Completely clear contents.
    */
   void clear();

   /**
    * Add a pair of phrases (reprensented as ptrs into token sequences) with a given count.
    * If pair exists already, increment its count by val.
    */
   void addPhrasePair(ToksIter beg1, ToksIter end1, ToksIter beg2, ToksIter end2, T val=1);

   /**
    * If given phrase pair exists in the table, return true and set val to
    * its frequency within the table.
    */
   bool exists(ToksIter beg1, ToksIter end1, ToksIter beg2, ToksIter end2, T &val);

   /**
    * Number of phrases in each language.
    */
   Uint numLang1Phrases() {return phrase_table.size();}
   Uint numLang2Phrases() {return lang2_voc.size();}

   /**
    * Sequential access to table, via iterator class above.
    */
   iterator begin();
   iterator end();

   /**
    * Test if a given word exists as a phrase on its own.
    */
   bool inVoc1(const string& word);
   bool inVoc2(const string& word);

};

/*---------------------------------------------------------------------------------------------
  PhraseTableGen implementation
  -------------------------------------------------------------------------------------------*/

template<class T>
void PhraseTableGen<T>::clear()
{
   phrase_table.clear();
   lang2_voc.clear();
}

template<class T>
bool PhraseTableGen<T>::exists(ToksIter beg1, ToksIter end1, ToksIter beg2, ToksIter end2, T &val)
{
   string phrase1, phrase2;
   compressPhrase(beg1, end1, phrase1, wvoc1);
   compressPhrase(beg2, end2, phrase2, wvoc2);

   Uint id2 = lang2_voc.index(phrase2.c_str());
   if (id2 == lang2_voc.size())
      return false;

   typename PhraseTable::iterator iter1 = phrase_table.find(phrase1);
   if (iter1 == phrase_table.end())
      return false;

   typename PhraseFreqs::iterator iter2 = iter1->second.find(id2);
   if (iter2 == iter1->second.end())
      return false;

   val = iter2->second;
   return true;
}

template<class T>
void PhraseTableGen<T>::addPhrasePair(ToksIter beg1, ToksIter end1, ToksIter beg2, ToksIter end2, T val)
{
   string phrase1, phrase2;
   compressPhrase(beg1, end1, phrase1, wvoc1);
   compressPhrase(beg2, end2, phrase2, wvoc2);

   Uint id2 = lang2_voc.add(phrase2.c_str(), val);
   phrase_table[phrase1][id2] += val;
}

template<class T>
bool PhraseTableGen<T>::inVoc1(const string& word)
{
   vector<string> toks;
   toks.push_back(word);
   string code;
   compressPhrase(toks.begin(), toks.end(), code, wvoc1);
   return phrase_table.find(code) != phrase_table.end();
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
void PhraseTableGen<T>::dump_prob_lang2_given_lang1(ostream& ostr)
{
   ostr.precision(9); // Enough to keep all the precision of a float
   typename PhraseTable::iterator p;
   for (p = phrase_table.begin(); p != phrase_table.end(); ++p) {
      double sum = 0.0;
      typename PhraseFreqs::iterator pf;
      for (pf = p->second.begin(); pf != p->second.end(); ++pf)
         sum += pf->second;
      for (pf = p->second.begin(); pf != p->second.end(); ++pf) {
         double value = pf->second / sum;
         if (value != 0)
            writePhrasePair(ostr, lang2_voc.word(pf->first), p->first.c_str(), value, wvoc2, wvoc1);
      }
   }
}


template<class T>
void PhraseTableGen<T>::dump_prob_lang1_given_lang2(ostream& ostr)
{
   ostr.precision(9); // Enough to keep all the precision of a float
   typename PhraseTable::iterator p;
   for (p = phrase_table.begin(); p != phrase_table.end(); ++p) {
      typename PhraseFreqs::iterator pf;
      for (pf = p->second.begin(); pf != p->second.end(); ++pf) {
         double value = pf->second / (double) lang2_voc.freq(pf->first);
         if (value != 0)
            writePhrasePair(ostr, p->first.c_str(), lang2_voc.word(pf->first), value, wvoc1, wvoc2);
      }
   }
}

template<class T>
void PhraseTableGen<T>::dump_freqs_lang1(ostream& ostr) {
   vector<string> toks;
   string ph;
   typename PhraseTable::iterator p;
   for (p = phrase_table.begin(); p != phrase_table.end(); ++p) {
      double sum = 0.0;
      toks.clear();
      typename PhraseFreqs::iterator pf;
      for (pf = p->second.begin(); pf != p->second.end(); ++pf)
         sum += pf->second;
      decompressPhrase(p->first, toks, wvoc1);
      codePhrase(toks.begin(), toks.end(), ph);
      ostr << ph << " " << sum << endl;
   }
}

template<class T>
void PhraseTableGen<T>::dump_freqs_lang2(ostream& ostr) {
   vector<string> toks;
   string ph;
   for (Uint i = 0; i < lang2_voc.size(); ++i) {
      toks.clear();
      ph = lang2_voc.word(i);
      decompressPhrase(ph, toks, wvoc2);
      codePhrase(toks.begin(), toks.end(), ph);
      ostr << ph << " " << lang2_voc.freq(i) << endl;
   }
}

template<class T>
void PhraseTableGen<T>::dump_joint_freqs(ostream& ostr, T thresh, bool reverse)
{
   string p1, p2;
   for (iterator it = begin(); !it.equal(end()); it.incr())
      if (it.getJointFreq() >= thresh) {
         it.getPhrase(1, p1);
         it.getPhrase(2, p2);
         if (!reverse)
            writePhrasePair(ostr, p1.c_str(), p2.c_str(), it.getJointFreq());
         else
            writePhrasePair(ostr, p2.c_str(), p1.c_str(), it.getJointFreq());

      }
}

template<class T>
void PhraseTableGen<T>::readJointTable(istream& in)
{
   string line;
   vector<string> toks;
   ToksIter b1, e1, b2, e2, v;

   while (getline(in, line)) {
      extractTokens(line, toks, b1, e1, b2, e2, v);
      addPhrasePair(b1, e1, b2, e2, conv<T>(*v));
   }
}

template<class T>
void PhraseTableGen<T>::readJointTable(const string& infile)
{
   IMagicStream in(infile);
   readJointTable(in);
}

template<class T>
typename PhraseTableGen<T>::iterator PhraseTableGen<T>::begin()
{
   typename PhraseFreqs::iterator null_it(0);
   return iterator(this, phrase_table.begin(), phrase_table.end(),
                   phrase_table.size() ? phrase_table.begin()->second.begin() : null_it,
                   phrase_table.size() ? phrase_table.begin()->second.end() : null_it);
}

template<class T>
typename PhraseTableGen<T>::iterator PhraseTableGen<T>::end()
{
   typename PhraseFreqs::iterator null_it(0);
   return iterator(this, phrase_table.end(), phrase_table.end(), null_it, null_it);
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


   os << s1 << " " << psep << " " << s2 << " " << psep << " " << val << endl;
}

template<class T>
void PhraseTableBase::writePhrasePair(ostream& os, const char* p1, const char* p2, T val)
{
   os << p1 << " " << psep << " " << p2 << " " << psep << " " << val << endl;
}

template<class T>
void PhraseTableBase::writePhrasePair(ostream& os, const char* p1, const char* p2, vector<T>& vals)
{
   os << p1 << " " << psep << " " << p2 << " " << psep;
   for (Uint i = 0; i < vals.size(); ++i)
      os << " " << vals[i];
   os << endl;
}


/*---------------------------------------------------------------------------------------------
  PhraseTableGen::iterator implementation. Assumes throughout that each L1
  phrase in PhraseTableGen's hash table maps to at least one corresponding L2
  phrase (ie, no empty rows).
  -------------------------------------------------------------------------------------------*/

template<class T>
void PhraseTableGen<T>::iterator::incr()
{
   if (row_iter != row_iter_end)
      if (++elem_iter == elem_iter_end) {
         ++row_count;
         if (++row_iter != row_iter_end) {
            elem_iter = row_iter->second.begin();
            elem_iter_end = row_iter->second.end();
         }
      }
}

template<class T>
bool PhraseTableGen<T>::iterator::equal(const iterator& it) const
{
   return row_iter == it.row_iter && (row_iter == row_iter_end || elem_iter == it.elem_iter);
}

template<class T>
typename PhraseTableGen<T>::iterator& PhraseTableGen<T>::iterator::operator=(const iterator& it)
{
   row_iter = it.row_iter;
   row_iter_end = it.row_iter_end;
   elem_iter = it.elem_iter;
   elem_iter_end = it.elem_iter_end;
   return *this;
}

template<class T>
void PhraseTableGen<T>::iterator::getPhrase(Uint lang, vector<string>& toks) const
{
   if (lang == 1) {
      const string& compressed_phrase = row_iter->first;
      decompressPhrase(compressed_phrase, toks, pt->wvoc1);
   } else {
      string compressed_phrase = pt->lang2_voc.word(elem_iter->first);
      decompressPhrase(compressed_phrase, toks, pt->wvoc2);
   }
}

template<class T>
string& PhraseTableGen<T>::iterator::getPhrase(Uint lang, string& phrase) const
{
   vector<string> toks;
   getPhrase(lang, toks);
   phrase.clear();
   join(toks.begin(), toks.end(), phrase, PhraseTableBase::sep);
   return phrase;
}

template<class T>
Uint PhraseTableGen<T>::iterator::getPhraseIndex(Uint lang) const
{
   return lang == 1 ? row_count : elem_iter->first;
}

template<class T>
T PhraseTableGen<T>::iterator::getJointFreq() const
{
   return elem_iter->second;
}

} // namespace Portage

#endif
// vim:sw=3:
