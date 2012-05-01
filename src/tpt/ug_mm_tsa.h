#ifndef _ug_mm_tsa_h
#define _ug_mm_tsa_h

// (c) 2007-2009 Ulrich Germann. All rights reserved.
// Licensed to NRC under special agreement.

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cmath>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>

#include "tpt_tightindex.h"
#include "tpt_tokenindex.h"
#include "tpt_pickler.h"
#include "ug_tsa_base.h"
#include "ug_mm_ttrack.h"

namespace ugdiss
{
  using namespace std;
  namespace bio=boost::iostreams;

  template<typename TOKEN>
  class mmTSA : public TSA<TOKEN>
  {
  public:
    typedef TSA_tree_iterator<mmTSA<TOKEN> > tree_iterator;
    friend class TSA_tree_iterator<mmTSA<TOKEN> >;
  private:
    bio::mapped_file_source file;

    mutable double avg_num_bytes_per_entry;

  public: // temporarily for debugging

    filepos_type const* index; // random access to top-level sufa ranges

  private:

    char const* index_jump(char const* a, char const* z, float ratio) const;
    char const* getLowerBound(id_type t) const;
    char const* getUpperBound(id_type t) const;
		   
  public:
    mmTSA();
    mmTSA(string fname, Ttrack<TOKEN> const* c);
    void open(string fname, Ttrack<TOKEN> const* c);

    count_type
    sntCnt(char const* p, char const * const q) const;

    count_type
    rawCnt(char const* p, char const * const q) const;

    count_type
    approxCnt(char const* p, char const * const q) const;

    void
    getCounts(char const* p, char const * const q, 
              count_type& sids, count_type& raw) const;

    char const* 
    readSid(char const* p, char const* q, id_type& sid) const;

    char const* 
    readOffset(char const* p, char const* q, uint16_t& offset) const;

    char const*
    random_sample(char const* p, char const* q) const;

    void sanityCheck() const;
  }; 

  // ======================================================================

  /** Open the three memory mapped files associated with a token sequence array.
   * @param basename base name for the files
   * @param token_index return TokenIndex for the corpus.
   * @param token_track return mmTtrack (memory mapped corpus track).
   * @param tsa return mmTSA (memory mapped token sequence array).
   *
   * Filenames are of the format basename.tpsa/type, basename/type or
   * basename.type, where type is one of "tdx", "mct", or "msa"/"tsa" as
   * appropriate.
   *
   * The reverse index is also initialized for the token index.
   */
  template <typename TOKEN>
  void
  open_mm_tsa(const string& basename, TokenIndex& token_index,
              mmTtrack<TOKEN>& token_track, mmTSA<TOKEN>& tsa)
  {
    string base;
    if (0 == access((basename + ".tpsa/mct").c_str(), F_OK))
      base = basename + ".tpsa/";
    else if (0 == access((basename + "/mct").c_str(), F_OK))
      base = basename + "/";
    else
      base = basename + ".";

    token_index.open(base+"tdx");
    //token_index.iniReverseIndex();
    token_track.open(base+"mct");
    if (0 == access((base+"msa").c_str(), F_OK))
      tsa.open(base+"msa", &token_track);
    else
      tsa.open(base+"tsa", &token_track);
  }

  // ======================================================================

  /** jump to the point 1/ratio in a tightly packed index
   *  assumes that keys are flagged with '1', values with '0'
   */
  template<typename TOKEN>
  char const* 
  mmTSA<TOKEN>::
  index_jump(char const* a, char const* z, float ratio) const
  {
    assert(ratio >= 0 && ratio < 1);
    char const* m = a+int(ratio*(z-a));
    if (m > a) 
      {
	while (m > a && *m <  0) --m;
	while (m > a && *m >= 0) --m;
	if (*m < 0) ++m;
      }
    assert(*m >= 0);
    return m;
  }

  // ======================================================================

  template<typename TOKEN>
  mmTSA<TOKEN>::
  mmTSA() 
  {
    this->corpus       = NULL;
    this->startArray   = NULL;
    this->endArray     = NULL;
    this->BitSetCachingThreshold = 4096;
    this->avg_num_bytes_per_entry = 0;
  };

  // ======================================================================

  template<typename TOKEN>
  mmTSA<TOKEN>::
  mmTSA(string fname, Ttrack<TOKEN> const* c)
  {
    open(fname,c);
  }

  // ======================================================================

  template<typename TOKEN>
  void
  mmTSA<TOKEN>::
  open(string fname, Ttrack<TOKEN> const* c)
  {
    avg_num_bytes_per_entry = 0;
    if (access(fname.c_str(),F_OK))
      {
        ostringstream msg;
        msg << "mmTSA<>::open: File '" << fname << "' does not exist.";
        throw std::runtime_error(msg.str().c_str());
      }
    assert(c);
    this->corpus = c;
    file.open(fname);
    char const* p = file.data();
    uint64_t fSize = file.size();
    if (fSize < sizeof(filepos_type)+sizeof(id_type))
      cerr << efatal << "Bad token sequence array file '" << fname << "'."
           << exit_1;
    filepos_type idxOffset;
    p = numread(p,idxOffset);
    p = numread(p,this->indexSize);
    
    // cerr << fname << ": " << idxOffset << " " << this->indexSize << endl;
    
    this->startArray = p;
    this->index      = reinterpret_cast<filepos_type const*>(file.data()+idxOffset);
    this->endArray   = reinterpret_cast<char const*>(index);
    this->corpusSize = c->size();
    this->numTokens  = c->numTokens();

    // Accept msa file generated with buggy and fixed version of mmtsa.build,
    // as well as mmsufa.build: indexSize was previously erroneously written
    // including the end index.  Now it correctly never includes the end index.
    if (fSize != idxOffset + (this->indexSize+1)*sizeof(filepos_type) &&
        fSize != idxOffset + (this->indexSize)*sizeof(filepos_type))
       cerr << efatal << "Bad memory mapped suffix array file '" << fname << "'."
            << exit_1;
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  mmTSA<TOKEN>::
  getLowerBound(id_type id) const
  {
    if (id >= this->indexSize) 
      return NULL;
    return this->startArray + this->index[id];
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  mmTSA<TOKEN>::
  getUpperBound(id_type id) const
  {
    if (id >= this->indexSize) 
      return NULL;
    // if (index[id] == index[id+1])
    // return NULL;
    else
      return this->startArray + this->index[id+1];
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  mmTSA<TOKEN>::
  readSid(char const* p, char const* q, id_type& sid) const
  {
    return tightread(p,q,sid);
  }

  // ======================================================================

  template<typename TOKEN>
  inline
  char const*
  mmTSA<TOKEN>::
  readOffset(char const* p, char const* q, uint16_t& offset) const
  {
    return tightread(p,q,offset);
  }

  // ======================================================================

  template<typename TOKEN>
  char const*
  mmTSA<TOKEN>::
  random_sample(char const* p, char const* q) const
  {
    size_t distance = q-p;
    int attempts = 0;
    while (true) {
      uint64_t random;
      if (distance > 0x00ffffff)
        // rand returns a 31 bit random number - use it twice to get enough
        random = (rand() + (rand() << 31));
      else
        random = rand();

      char const* sampled_m = p + (random % distance);
      char const* m = sampled_m;
      
      if (m > p)
      {
        while (m > p && *m <  0) --m;
        while (m > p && *m >= 0) --m;
        if (*m < 0) ++m;
      }
      assert(*m >= 0);

      /* this idea fails miserably at generating an unbiased sample, and is
       * more expensive than resampling.
      int walk = rand() % 100;
      for (int i = 0; i < walk; ++i) {
        IndexEntry I;
        readEntry(m, I);
        m = I.next;
        if (m == q) m = p;
      }
      return m;
      */

      // To make sure the sampling uses a uniform distribution, we give each
      // index position two changes to be picked: the random number has to have
      // landed on its first or second byte; otherwise we try again.  Two bytes
      // is the size of the most tightly packed entries, so we know every entry
      // has at least two bytes.  A significant bias in favour of index
      // positions with larger encodings would be introduced if we did not do
      // this.
      if (sampled_m == m || sampled_m == m+1)
        return m;
      else
      {
        ++attempts;
        // We limit ourselves to 100 attempts, to remove the theoretical
        // infinite loop.  We don't really need to do this: the binomial
        // distribution says on average we'll try 2-3 times per sample
        // requested.  But just in case...  The bias introduced here is trivial
        // enough to ignore; a lower limit, however, is not OK: at 10 it
        // already introduces a significant bias.
        if (attempts >= 100) return m;
      }
    }
  }

  // ======================================================================

  template<typename TOKEN>
  count_type
  mmTSA<TOKEN>::
  rawCnt(char const* p, char const* const q) const
  {
    id_type sid; uint16_t off;
    size_t ret=0;
    while (p < q)
      {
	p = tightread(p,q,sid);
	p = tightread(p,q,off);
	ret++;
      }
    return ret;
  }
  
  // ======================================================================

  template<typename TOKEN>
  count_type
  mmTSA<TOKEN>::
  approxCnt(char const* p, char const* const q) const
  {
    if (avg_num_bytes_per_entry == 0)
    {
      // the first time we use approxCnt, estimate the average number of bytes
      // per encoded index entry.
      ostringstream oss;
      size_t k = binwrite(oss, this->corpus->size());
      double avg_num_bytes = k - ((pow(double(128.0),double(k-1))-1) / double(this->corpus->size()));
      avg_num_bytes_per_entry =
        avg_num_bytes   // avg num bytes to encode sentence ID
        + 1;            // avg num bytes to encode offset
      assert(avg_num_bytes_per_entry >= 2.0);
    }

    if (p >= q) return 0;
    uint64_t distance = q - p;
    return distance / avg_num_bytes_per_entry + 1;
  }
  
  // ======================================================================

  template<typename TOKEN>
  void 
  mmTSA<TOKEN>::
  getCounts(char const* p, char const* const q, 
	    count_type& sids, count_type& raw) const
  {
    raw = 0;
    id_type sid; uint16_t off;
    boost::dynamic_bitset<uint64_t> check(this->corpus->size());
    while (p < q)
      {
	p = tightread(p,q,sid);
	p = tightread(p,q,off);
	check.set(sid);
	raw++;
      }
    sids = check.count();
  }

  // ======================================================================
#if 0
  template<typename TOKEN>
  class
  mmTSA<TOKEN>::
  tree_iterator : public TSA<TOKEN>::tree_iterator
  {
  public:
    tree_iterator(mmTSA<TOKEN> const* s);
    tree_iterator(mmTSA<TOKEN> const* s, TOKEN const& t);
    tree_iterator(mmTSA<TOKEN> const* s, TOKEN const* kstart, TOKEN const* kend);
    bool down() { return TSA<TOKEN>::tree_iterator::down(); }
    bool over() { return TSA<TOKEN>::tree_iterator::over(); }
  };

  // ======================================================================

  template<typename TOKEN>
  mmTSA<TOKEN>::
  tree_iterator::
  tree_iterator(mmTSA<TOKEN> const* s)
    : TSA<TOKEN>::tree_iterator::tree_iterator(s)
  {};

  // ======================================================================
#endif

} // end of namespace ugdiss

// #include "ug_mm_tsa_extra.h"
#endif
