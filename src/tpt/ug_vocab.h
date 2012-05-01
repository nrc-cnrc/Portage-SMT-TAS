// This file is derivative work from Ulrich Germann's Tightly Packed Tries
// package (TPTs and related software).
//
// Original Copyright:
// Copyright 2005-2009 Ulrich Germann; all rights reserved.
// Under licence to NRC.
//
// Copyright for modifications:
// Technologies langagieres interactives / Interactive Language Technologies
// Inst. de technologie de l'information / Institute for Information Technology
// Conseil national de recherches Canada / National Research Council Canada
// Copyright 2008-2010, Sa Majeste la Reine du Chef du Canada /
// Copyright 2008-2010, Her Majesty in Right of Canada



/**
 * @author Ulrich Germann
 * @file ug_vocab.h Vocabulary with word count capabilities.
 *
 * Vocab is mutable (can add items on the fly).
 *
 *  (c) 2006,2007,2008 Ulrich Germann.
 */
#ifndef UG_VOCAB_H
#define UG_VOCAB_H

#include <map>
#include <string>
#include <vector>
#include <iostream>

#include "tpt_typedefs.h"

namespace ugdiss
{

/** Vocab is a class to manage vocabularies. It maintains
 * - mappings from strings to token IDs and vice versa
 * - word counts
 *
 * Features:
 * - new vocabulary items are automatically assigned a new token ID
 * - allows re-assignment of token IDs based on decreasing order of 
 *   frequency (default) or ascending alphabetical order
 * - compact binary storage and loading via pickle() and unpickle()
 */
class Vocab
{
public:
  class Word;
  typedef pair<id_type, count_type> IdCntPair;
  typedef map<string, IdCntPair>::iterator iterator;
  typedef map<string, IdCntPair>::const_iterator const_iterator;
private:
  void init();
  typedef map<string, IdCntPair>::value_type EntryType;
  map<string, IdCntPair>  sIdx; /* for access by string */
  vector<Vocab::iterator> nIdx; /* for access by ID */
  count_type mxStrLen;
  class cmpWrdsByCnt;   /* for sorting */
  class cmpWrdsByAlpha; /* for sorting */
  enum { VOCAB_STARTID = 0 }; /* 0 is reserved for NULL, 1 for 'UNK' */
public:
  enum FILEMODE { BIN, TXT, BZGIZA };
  count_type totalCount;
  count_type refCount; // # of pointers pointing to me
  Vocab();
  Vocab(string filename);
  Vocab(string filename,FILEMODE mode);
  /** open an existing Vocab file */
  void open(string filename);
  void open(string filename,FILEMODE mode);
  /** Access a vocabulary item.
   *  If we want const versions of these, we'll have to have two variants of the 
   *  Word class, one that allows the cnt member to be changed and one with a const
   *  cnt member.
   */
  Word operator[](string const& str);
  Word operator[](id_type id) const;
  Word get(string const& str);



  Vocab& operator=(Vocab const& other);
  double p(string wrd);
  double p(id_type id); 
  size_t size() { return nIdx.size(); }
  iterator begin();
  iterator end();
  /**
   * Write vocabulary to binary stream
   */
  filepos_type pickle(ostream& out);
  filepos_type pickle(string ofile);
  
  /**
   * Load from binary stream
   */
  void unpickle(istream& in);

  /** 
   * Dump in readable form to output stream out in lexical order.
   * WriteSpecials determines whether (true) or not (false) NULL and UNK
   * are included in the output.
   */
  void dump(ostream& out,bool writeSpecials=false,
	    bool withP=false, bool withCumP=false, bool noColumns=false);
  void clear();

  /** Reassigns IDs so that frequent words have low IDs.*/
  void optimizeIDs(); 
  /** Reassigns IDs to reflect ascending alphabetical order.*/
  void sortAlphabetically();
  /** Check if the word exits without adding it if not.*/
  bool exists(string const& wrd);

  string toString(vector<id_type> const& v, size_t start=0, size_t stop=0) const;
  string toString(vector<id_type>::const_iterator a,
                  vector<id_type>::const_iterator const& z) const;

  void toIdSeq(vector<id_type>& idSeq, string const& foo, bool createIfNecessary=false);

  /** Write Vocab in TokenIndex format */
  void toTokenIndex(ostream& out);

};

/** for sorting the words by count (in reverse order)*/
class 
Vocab::cmpWrdsByCnt
{
public:
  bool operator()(const Vocab::iterator& a, const Vocab::iterator& b)
  {
    return a->second.second > b->second.second;
  }
};

class 
Vocab::cmpWrdsByAlpha
{
public:
  bool operator()(const Vocab::iterator& a, const Vocab::iterator& b)
  {
    return a->first < b->first;
  }
};

/** The class Vocab::Word is a bundle of references into the central data
 *  structure of a Vocab object. It is the return value for the
 *  standard lookup by string or token ID via the [] operator.
 */
class Vocab::Word
{
public: 
  id_type const  id;
  string  const& str;
  count_type& cnt;
  Word(id_type xID, string const& xS, count_type& xcnt)
    : id(xID),str(xS),cnt(xcnt) {};
};

ostream& operator<<(ostream& out, Vocab& V);
istream& operator>>(istream& in,  Vocab& V);

} // end namespace ugdiss
#endif
