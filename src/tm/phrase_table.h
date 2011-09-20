/**
 * @author George Foster, with reduced memory usage mods to by Darlene Stewart
 * @file phrase_table.h  Represent a phrase table with joint frequencies.
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

#ifndef PHRASE_TABLE_H
#define PHRASE_TABLE_H

#include "string_hash.h"
#include <map>
#include <algorithm>
#include <ostream>
#include <cmath>
#include "voc.h"
#include "file_utils.h"
#include "vector_map.h"
#include "portage_defs.h"
#include "ttable.h"
#include "tm_io.h"
#include "ibm.h"
#include "length.h"

namespace Portage {

/**
 * Helper base class to factor out some things from the PhraseTable template
 * class so they can be defined in the .cc file.
 */
struct PhraseTableBase
{
   typedef vector<string>::const_iterator ToksIter;

   static const string sep;     // separate words in a phrase
   static const string psep;    // separate phrases in a pair
   static const string psep_replacement;    // Portage's escaped psep

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
    * Recode a phrase from a packed string representation to a a phrase string.
    */
   static string recodePhrase(const string& coded, Voc& voc,
                              const string& sep = PhraseTableBase::sep);

   /**
    * Determine the length of a coded phrase without decoding it
    */
   static Uint phraseLength(const char* coded);

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
   static void writePhrasePair(ostream& os, const char* p1, const char* p2,
                               T val);

   /**
    * Write a pair of normal phrase strings + associated values on a stream.
    */
   template<class T>
   static void writePhrasePair(ostream& os, const char* p1, const char* p2,
                               vector<T>& vals);

   /**
    * Convert a phrase pair read from a stream into a token sequence.
    * @param line line read from a stream
    * @param[out] toks token sequence
    * @param[out] b1, e1 beginning and end+1 markers for 1st phrase
    * @param[out] b2, e2 beginning and end+1 markers for 2nd phrase
    * @param[out] v position of value
    * @param tolerate_multi_vals allow multiple value fields
    */
   static void extractTokens(const string& line, vector<string>& toks,
                             ToksIter& b1, ToksIter& e1,
                             ToksIter& b2, ToksIter& e2,
                             ToksIter& v, bool tolerate_multi_vals = false);
};

/**
 * Main Phrase table.
 *
 * For compactness, this is stored using a multi-stage strategy. First, phrases
 * in both languages are compressed into packed strings using 3 bytes per word.
 * Next, lang1 and lang2 compressed phrases are mapped to integer indices.
 * Finally, lang1 indices are mapped to lists of (lang2-index,freq) pairs.
 * This is a bit wacky, but it does the job.
 *
 * Storage requirements for the phrase table can be reduced by not keeping
 * it entirely in memory. In this reduced memory state, the lang1 vocabulary
 * and the phrase frequency tables are not kept in memory. In this case,
 * the jpt file is read each time the phrase table is iterated over.
 */
template<class T> class PhraseTableGen : public PhraseTableBase
{
   typedef vector_map<Uint,T> PhraseFreqs; // l2-index -> frequency
   typedef vector<PhraseFreqs*> PhraseTable;

   Voc lang1_voc;		// l1compressedphrase <-> l1index
   PhraseTable phrase_table;    // l1index -> (l2index,freq), (l2index,freq), ...
   _CountingVoc<T> lang2_voc;   // l2compressedphrase <-> (index, freq)

   Voc wvoc1;                   // l1 _words_ <-> indexes, used to compress/decompress phrases
   Voc wvoc2;                   // l2 ""

   Uint num_lang1_phrases;      // number of language 1 phrases.

   bool keep_phrase_table_in_memory; // set if phrase table is kept fully in memory
   // pruning: no pruning if prune1_fixed == prune1_per_word == 0
   Uint prune1_fixed;           // fixed part of pruning factor;
   Uint prune1_per_word;        // variable part of pruning factor; 

   string jpt_file;             // file name of jpt file
   istream* jpt_stream;         // stream used for reading a jpt file - for unit testing only
   bool phrase_table_read;      // set upon completion of first jpt file reading

private:
   /// noncopyable - disable copy constructor
   PhraseTableGen(const PhraseTableGen&);
   /// noncopyable - disable assignment opeartor
   PhraseTableGen& operator=(const PhraseTableGen&);

   void remap_psep_helper(Voc& voc, const string& sep, const string& replacement);

public:
   /**
    *  iterator over contents (phrase pairs and joint frequencies).
    *
    *  This iterator supports iteration over the phrase table kept entirely
    *  in memory or over the jpt file (with or without pruning) to reduce the
    *  memory needed to hold phrase table information.
    *
    *  When iterating over a jpt file, the increment operator (++) cannot be
    *  applied to iterator object that has been copied using either the
    *  assignment operator (=) of a copy constructor because ownership of the
    *  stream is transferred upon copying.
    */
   class iterator {
      enum {noStrategy, memoryIterator, fileIterator, pruningIterator} strategy_type;
      class IteratorStrategy;
      IteratorStrategy* iterator_strategy;

      // Private constructor to permit creation only through
      // PhraseTableGen<T>::begin() and PhraseTableGen<T>::end().
      iterator(PhraseTableGen<T>* pt, bool end=false);
      friend iterator PhraseTableGen<T>::begin();
      friend iterator PhraseTableGen<T>::end();
   public:
      iterator();
      ~iterator();
      iterator(const iterator &it);	// copy constructor
      iterator& operator=(const iterator& it); // assignment
      iterator& operator++();   // increment
      bool operator==(const iterator& it) const;
      bool operator!=(const iterator& it) const { return ! operator==(it); }
      string& getPhrase(Uint lang, string& phrase) const; // lang is 1 or 2
      void getPhrase(Uint lang, vector<string>& toks) const; // lang is 1 or 2
      Uint getPhraseLength(Uint lang) const; // lang is 1 or 2
      Uint getPhraseIndex(Uint lang) const; // unique contiguous index for L1 or L2 phrase
      T getJointFreq() const;
      T& getJointFreqRef();

   private:
      // Base class for iterator strategies.
      class IteratorStrategy
      {
      private:
         // copying is only supported via assignment, not copy construction.
         IteratorStrategy(const IteratorStrategy&);
      protected:
         PhraseTableGen<T>* pt;
         bool end;
         Uint id1, id2;
         IteratorStrategy() : pt(NULL), end(true) {};
         IteratorStrategy(PhraseTableGen<T>* pt_, bool end_=false) : pt(pt_), end(end_)
            { assert(pt != NULL); };
      public:
         virtual ~IteratorStrategy() {};
         virtual IteratorStrategy& operator=(const IteratorStrategy& it);
         virtual IteratorStrategy& operator++() = 0;    // increment
         bool operator==(const IteratorStrategy& it) const;
         bool operator!=(const IteratorStrategy& it) const { return ! operator==(it); }
         virtual void getPhrase(Uint lang, vector<string>& toks) const; // lang is 1 or 2
         virtual string& getPhrase(Uint lang, string& phrase) const; // lang is 1 or 2
         virtual Uint getPhraseLength(Uint lang) const; // lang is 1 or 2
         Uint getPhraseIndex(Uint lang) const { return lang == 1 ? id1 : id2; }
         virtual T getJointFreq() const = 0;
         virtual T& getJointFreqRef() = 0;
      }; // class IteratorStrategy

      // Strategy for keeping the phrase table entirely in memory.
      class MemoryIteratorStrategy : public IteratorStrategy
      {
         typename PhraseTable::iterator row_iter;
         typename PhraseTable::iterator row_iter_end;
         typename PhraseFreqs::iterator elem_iter;
         typename PhraseFreqs::iterator elem_iter_end;
      protected:
         using IteratorStrategy::end;
         using IteratorStrategy::pt;
         using IteratorStrategy::id1;
         using IteratorStrategy::id2;
      public:
         MemoryIteratorStrategy() : IteratorStrategy() {};
         MemoryIteratorStrategy(PhraseTableGen<T>* pt_, bool end_=false);
         virtual MemoryIteratorStrategy& operator=(const IteratorStrategy& it);
         virtual MemoryIteratorStrategy& operator++();  // increment
         virtual T getJointFreq() const;
         virtual T& getJointFreqRef();
      }; // class MemoryIteratorStrategy

      // Strategy for not keeping the phrase frequency tables in memory
      // (without pruning).
      class FileIteratorStrategy : public IteratorStrategy
      {
      protected:
         using IteratorStrategy::end;
         using IteratorStrategy::pt;
         using IteratorStrategy::id1;
         using IteratorStrategy::id2;
         auto_ptr<istream> jpt_stream;
         T val;
      public:
         FileIteratorStrategy() : IteratorStrategy(), jpt_stream(NULL) {};
         FileIteratorStrategy(PhraseTableGen<T>* pt_, bool end_=false);
         virtual FileIteratorStrategy& operator=(const IteratorStrategy& it);
         virtual FileIteratorStrategy& operator++(); // increment
         virtual T getJointFreq() const;
         virtual T& getJointFreqRef();
      }; // class FileIteratorStrategy

      // Strategy for not keeping neither the lang1 vocabulary nor the phrase
      // frequency tables in memory (without pruning).
      class File2IteratorStrategy : public FileIteratorStrategy
      {
      protected:
         using IteratorStrategy::end;
         using IteratorStrategy::pt;
         using IteratorStrategy::id1;
         using IteratorStrategy::id2;
         using FileIteratorStrategy::jpt_stream;
         using FileIteratorStrategy::val;
         string phrase1;
      public:
         File2IteratorStrategy() : FileIteratorStrategy() {};
         File2IteratorStrategy(PhraseTableGen<T>* pt_, bool end_=false);
         virtual File2IteratorStrategy& operator=(const IteratorStrategy& it);
         virtual File2IteratorStrategy& operator++(); // increment
         virtual void getPhrase(Uint lang, vector<string>& toks) const; // lang is 1 or 2
         virtual string& getPhrase(Uint lang, string& phrase) const; // lang is 1 or 2
         virtual Uint getPhraseLength(Uint lang) const; // lang is 1 or 2
      }; // class File2IteratorStrategy

      // Strategy for not keeping the phrase frequency tables in memory
      // (with pruning).
      class PruningIteratorStrategy : public FileIteratorStrategy
      {
         using IteratorStrategy::end;
         using IteratorStrategy::pt;
         using IteratorStrategy::id1;
         using IteratorStrategy::id2;
         using FileIteratorStrategy::jpt_stream;
         using FileIteratorStrategy::val;

         PhraseFreqs phrase_freqs;
         Uint pf_index;
         Uint next_id1, next_id2;
         T next_val;
      public:
         PruningIteratorStrategy() : FileIteratorStrategy() {};
         PruningIteratorStrategy(PhraseTableGen<T>* pt_, bool end_=false);
         virtual PruningIteratorStrategy& operator=(const IteratorStrategy& it);
         virtual PruningIteratorStrategy& operator++(); // increment
      }; // class PruningIteratorStrategy

      // Strategy for not keeping neither the lang1 vocabulary nor the phrase
      // frequency tables in memory (with pruning).
      class Pruning2IteratorStrategy : public File2IteratorStrategy
      {
      private:
         using IteratorStrategy::end;
         using IteratorStrategy::pt;
         using IteratorStrategy::id1;
         using IteratorStrategy::id2;
         using FileIteratorStrategy::jpt_stream;
         using FileIteratorStrategy::val;
         using File2IteratorStrategy::phrase1;

         PhraseFreqs phrase_freqs;
         Uint pf_index;
         Uint next_id1, next_id2;
         T next_val;
         string next_phrase1;
      public:
         Pruning2IteratorStrategy() : File2IteratorStrategy() {};
         Pruning2IteratorStrategy(PhraseTableGen<T>* pt_, bool end_=false);
         virtual Pruning2IteratorStrategy& operator=(const IteratorStrategy& it);
         virtual Pruning2IteratorStrategy& operator++(); // increment
      }; // class Pruning2IteratorStrategy

   }; // class iterator

   /**
    * For sorting phrases by decreasing frequency
    */
   struct ComparePhrasesByJointFreq
   {
      bool operator()(const pair<Uint,T>* p1, const pair<Uint,T>* p2) {
         if (p1->second > p2->second)
            return true;
         else if (p1->second == p2->second)
            return p1->first > p2->first;
         else
            return false;
      }
   };

   PhraseTableGen() : num_lang1_phrases(0), keep_phrase_table_in_memory(true),
                      prune1_fixed(0), prune1_per_word(0),
                      jpt_stream(NULL), phrase_table_read(false) {}

   ~PhraseTableGen() {}

   /**
    * Read contents as a joint table, in the format produced by dump_joint_freqs.
    */
   void readJointTable(istream& in, bool reduce_memory=false);
   void readJointTable(const string& infile, bool reduce_memory=false);

   /**
    * Remap psep so it doesn't get output as a token in the resulting phrase table.
    */
   void remap_psep();

   /**
    * Write contents to stream, filtering any pairs with freq < thresh (unless
    * the filt flag is false). Order is lang1,lang2; or lang2,lang1 if reverse
    * is true.
    * @param ostr output stream
    * @param thresh threshold under which to discard phrase pairs, unless filt=false
    * @param reserve if true, language order is lang2,lang1.
    * @param filt disable filtering - mostly useful when T=float, since counts
    *             could theoretically be negative.
    */
   void dump_joint_freqs(ostream& ostr, T thresh = 0, bool reverse = false,
                         bool filt=true);

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
    * Discard all but the best (most frequent) translations for each lang1
    * phrase.  This may remove some lang2 phrases from the table.
    * The number of translations kept is n = fixed_max + per_word_max * the
    * number of words in the lang1 phrase.
    * @param fixed_max     "b" - keep fixed_max lang2 phrases, plus
    * @param per_word_max  "a"x - also keep per_word_max * len(lang1 phrase)
    *                      lang2 phrases
    */
   void pruneLang2GivenLang1(Uint fixed_max, Uint per_word_max);
private:
   /**
    * Prune a single phrase_freqs table, using the pruning parameters in *this.
    * @param lang1_num_words  number of words in the lang1 phrase for which
    *                         phrase_freqs is the list of target phrases.
    */
   void prunePhraseFreqs(PhraseFreqs& phrase_freqs, Uint lang1_num_words);
public:

   /**
    * Completely clear contents.
    */
   void clear();

   /**
    * Add a pair of phrases (represented as ptrs into token sequences) with a given count.
    * If pair exists already, increment its count by val.
    */
   void addPhrasePair(ToksIter beg1, ToksIter end1,
                      ToksIter beg2, ToksIter end2,
                      T val=1);

private:
   /**
    * Extract a pair of phrases with a given count from an input line and
    * and look them up in the phrase table, adding them if the phrase table
    * has not already been constructed. During phrase table construction,
    * if pair exists already, increment its count by val.
    * @param line input line to parse from the joint count phrase table
    * @param[out] id1 lang-1 phrase, represented by its id in lang1_voc
    * @param[out] id2 lang-2 phrase, represented by its id in lang2_voc
    * @param[out] val the frequency of this phrase pair.
    */
   void lookupPhrasePair(const string &line, Uint& id1, Uint& id2, T& val);
   /**
    * Same as the other lookupPhrasePair() method, except for how the lang-1
    * phrase is returned.
    * @param[out] phrase1 lang-1 phrase, represented in compressed form
    */
   void lookupPhrasePair(const string &line, string& phrase1, Uint& id2, T& val);

public:
   /**
    * If given phrase pair exists in the table, return true and set val to
    * its frequency within the table.
    */
   bool exists(ToksIter beg1, ToksIter end1, ToksIter beg2, ToksIter end2, T &val);

   /**
    * Number of phrases in each language.
    */
   Uint numLang1Phrases() const {return num_lang1_phrases;}
   Uint numLang2Phrases() const {return lang2_voc.size();}

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

   /**
    * Fetch a phrase associated with a phrase index in a given language.
    */
   string& getPhrase(Uint lang, Uint id, string &phrase);
   void  getPhrase(Uint lang, Uint id, vector<string>& toks);
private:
   string& getPhrase(Uint lang, const string &coded, string &phrase);
   void  getPhrase(Uint lang, const string &coded, vector<string>& toks);

   Uint getPhraseLength(Uint lang, Uint id);
   Uint getPhraseLength(Uint lang, const char* coded);
public:

}; // class PhraseTableGen

// typedef-like definition for the commonly needed PhraseTableGen<Uint> case.
class PhraseTableUint : public PhraseTableGen<Uint> {};

} // namespace Portage

#include "phrase_table-cc.h"

#endif
