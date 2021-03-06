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



// Base class for Suffix arrays

// (c) 2007-2009 Ulrich Germann. All rights reserved.

#ifndef _ug_sufa_base_h
#define _ug_sufa_base_h

#include <iostream>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/dynamic_bitset.hpp>

#include "tpt_tokenindex.h"
#include "ug_ctrack_base.h"

namespace ugdiss
{
  using namespace std;
  namespace bio=boost::iostreams;

  class Sufa
  {
  public:
    class tree_iterator; // for traversing the tree
    class IndexEntry;    /* stores information about an index entry and
			  * the corpus position it points to
			  */

  protected:
    Ctrack const* corpus;

    /** pointer to the beginning of the index data block with the sorted list
     *  of token positions, depending on the subclass, this index can be
     *  tightly packed (mmSufa) or fixed-width (imSufa)
     */
    char const*     data; 

    /** pointer to the end of the index data block with the sorted list of 
     *  token positions, depending on the subclass, this index can be tightly
     *  packed (mmSufa) or fixed-width (imSufa)
     */
    char const*  endData; 
    
    /** size of the corpus (in number of sentences) of the corpus underlying the 
     *  suffix array. 
     *
     * ATTENTION: This number may differ from corpus->size(), namely when the
     *            suffix array is based on a subset of the sentences of /corpus/.
     */
    size_t corpusSize; 

    /** size of the corpus (in number of tokens) of the corpus underlying the
     * suffix array.  
     *
     * ATTENTION: This number may differ from corpus->numTokens(), namely when
     *            the suffix array is based on a subset of the sentences of 
     *            /corpus/.
     */
    id_type numTokens;  
    id_type indexSize;

    ////////////////////////////////////////////////////////////////
    // private member functions:

    /** @return an index position approximately /fraction/ between 
     *  /startRange/ and /endRange/.
     */ 
    virtual 
    char const* index_jump(char const* startRange, 
                           char const* stopRange,
                           float fraction) const = 0;
    
    /** compare the token represented by /I/ against the reference
     *  sequence [kS,kE).
     * returns -1 if I is strictly less than [kS,kE)
     * returns  0 if I is strictly equal  to [kS,kE)
     * returns  1 if [kS,kE) is a prefix of I, but I continues
     * returns  2 if I is strictly greater than [kS,kE)
     */
    int
    mycmp(IndexEntry const& I, 
          id_type const* kS, 
          id_type const* const kE, 
          size_t  depth) const;

    int
    mycmp(IndexEntry const& A, IndexEntry const& B) const;

    /** return the index position of the first item that 
     *  is equal to or includes [refStart,refEnd) as a prefix
     *  This method does the actual binary search, in O(log(upX-lo)).
     *  Returns NULL if [refStart,refEnd) does not occur in [lo,upX).
     */
    char const* 
    find_start(char const* lo, char const* const upX,
               id_type const* const refStart,
               id_type const* const refEnd,
               size_t d) const;

    /** return the index position of the first item that is greater than
     *  [refStart,refEnd) and does not include it as a prefix.
     *  This method does the actual binary search, in O(log(upX-lo)).
     *  Never returns NULL.
     */
    char const* 
    find_end(char const* lo, char const* const upX,
               id_type const* const refStart,
               id_type const* const refEnd,
               size_t d) const;

    /** return the index position of the first item that is longer than
     *  [refStart,refEnd) and includes it as a prefix
     *  This method does the actual binary search, in O(log(upX-lo)).
     */
    char const* 
    find_longer(char const* lo, char const* const upX,
                id_type const* const refStart,
                id_type const* const refEnd,
                size_t d) const;

    /** Returns the index position in the data block where the first item
     *  starting with token /id/ is located.
     *  Semantically equivalent to find_start(data, endData, &id, (&id)+1, 0)
     *  and lower_bound(&id, (&id)+1), this method is expected to be optimized
     *  by each subclass to take O(1) with a lookup in an additional data
     *  structure, instead of requiring the normal binary search.
     */
    virtual
    char const*
    getLowerBound(id_type id) const = 0;

    /** Returns the index position in the data block just past the last item
     *  starting with token /id/.
     *  Semantically equivalent to find_end(data, endData, &id, (&id)+1, 0) and
     *  upper_bound(&id, (&id)+1), this method is expected to be optimized by
     *  each subclass to take O(1) with a lookup in an additional data
     *  structure, instead of requiring the normal binary search.
     */
    virtual
    char const*
    getUpperBound(id_type id) const = 0;

  public:

    /** @return a pointer to the beginning of the index entry range covering 
     *  [keyStart,keyStop), i.e., the index position of the first item that is
     *  equal to or includes [keyStart,keyEnd) as a prefix.
     *  Returns NULL if [keyStart,keyEnd) does not occur at all.
     */
    char const* 
    lower_bound(vector<id_type>::const_iterator const& keyStart,
                vector<id_type>::const_iterator const& keyStop) const;
    char const* 
    lower_bound(id_type const* keyStart, id_type const* keyStop) const;

    /** @return a pointer to the end point of the index entry range covering 
     *  [keyStart,keyStop), i.e., the index position of the first item that is
     *  greater than [keyStart,keyEnd) and does not include it as a prefix.
     *  Never returns NULL.
     */
    char const* 
    upper_bound(vector<id_type>::const_iterator const& keyStart,
                vector<id_type>::const_iterator const& keyStop) const;
    char const* 
    upper_bound(id_type const* keyStart, id_type const* keyStop) const;


    /** sample one element randomly in [p,q) */
    virtual
    char const*
    random_sample(char const* p, char const* q) const = 0;

    /** dump all suffixes in order to /out/ */
    void dump(ostream& out, TokenIndex const& T) const;
    
    /** fill the dynamic bit set with true for all sentences that contain 
     *  /phrase/.
     *  @return the raw number of occurrences.
     */
    count_type
    fillBitSet(vector<id_type> const& phrase,
               boost::dynamic_bitset<uint64_t>& dest) const;

    /** read the sentence ID into /sid/ 
     *  @return position of associated offset. 
     *  The function provides an abstraction that uses the right interpretation of 
     *  the position based on the subclass (memory-mapped or in-memory).
     */
    virtual
    char const* 
    readSid(char const* p, char const* q, id_type& sid) const = 0;

    /** read the offset part of the index entry into /offset/ 
     *  @return position of the next entry in the index. 
     *  The function provides an abstraction that uses the right interpretation of 
     *  the position based on the subclass (memory-mapped or in-memory).
     */
    virtual
    char const* 
    readOffset(char const* p, char const* q, uint16_t& offset) const = 0;

    /** @return sentence count 
     */
    count_type
    sntCnt(char const* p, char const* const q) const; 
    
    /** @return raw occurrence count
     *  depending on the subclass, this is constant time (imSufa) or linear in
     *  in the number of occurrences (mmSufa).
     */
    virtual
    count_type
    rawCnt(char const* p, char const* const q) const = 0; 

    /** get both sentence and word counts. Avoids having to go over the byte
     *  range representing the range of suffixes in question twice when dealing
     *  with memory-mapped suffix arrays.
     */ 
    virtual
    void 
    getCounts(char const* p, char const* const q, 
	      count_type& sids, count_type& raw) const = 0; 

    string 
    suffixAt(char const* p, TokenIndex const* V=NULL, size_t maxlen=0) 
      const;

    string 
    suffixAt(IndexEntry const& I, TokenIndex const* V=NULL, size_t maxlen=0) 
      const;

    void readEntry(char const* p, IndexEntry& I) const;

    /** return pointer to the beginning of the data block */
    char const* dataBegin() const;
    /** return pointer to the end of the data block */
    char const* dataEnd() const;
    
    /** Return an ID that represents a given phrase; 
        This should NEVER be 0!
        Structure of a phrase ID: 
        leftmost 32 bits: sentence ID in the corpus
            next 16 bits: offset from the start of the sentence
            next 16 bits: length of the phrase
     */
    uint64_t 
    getPhraseID(vector<id_type>::const_iterator const& pstart,
                vector<id_type>::const_iterator const& pstop) const;
    
    /** Return the phrase represented by phrase ID pid_ */
    string
    getPhrase(uint64_t pid, TokenIndex const& V) const;

    size_t 
    getCorpusSize() const;

    Ctrack const*
    getCorpus() const;

    /** Destructor should be virtual for abstract base class */
    virtual ~Sufa() {}
  };

  class
  Sufa::
  tree_iterator
  {
  protected:
    Sufa const* root;
    vector<char const*> lower;
    vector<char const*> upper;

    // for debugging ...
    void showBounds(ostream& out) const;

  public:
    tree_iterator(Sufa const* s);
    tree_iterator(Sufa const* s, id_type id);
    tree_iterator(Sufa const* s, id_type const* kstart, id_type const* kend);

    char const* lower_bound(int p) const;
    char const* upper_bound(int p) const;

    size_t size() const;
    id_type wid(int p) const;
    id_type const* crpPos(int p) const;

    bool extend(id_type id);
    bool down();
    bool over();
    bool up();

    string str(TokenIndex const* V=NULL) const;
    string str(Vocab const& V) const;
  };

  class
  Sufa::
  IndexEntry
  {
  public:
    char const* pos;
    char const* next;
    id_type sid;
    ushort  offset;
    IndexEntry();
    IndexEntry(Sufa const* S, char const* p);
    id_type const* suffixStart(Sufa const* S) const;
    id_type const* suffixEnd(Sufa const* S) const;
  };

}

#endif
