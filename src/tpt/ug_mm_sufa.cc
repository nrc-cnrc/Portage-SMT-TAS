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



// (c) 2007-2009 Ulrich Germann. All rights reserved.

#include "ug_mm_sufa.h"
#include "ug_mm_ctrack.h"

#include "tpt_tokenindex.h"
#include "tpt_pickler.h"
#include "tpt_tightindex.h"
#include "tpt_utils.h"

#ifdef CYGWIN
typedef unsigned long ulong;
#endif

namespace ugdiss
{
  using namespace std;
  typedef id_type const* idptr;

  /** jump to the point 1/ratio in a tightly packed index
   *  assumes that keys are flagged with '1', values with '0'
   */
  char const* 
  mmSufa::
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


  mmSufa::
  mmSufa() 
  {
    corpus = NULL;
    data   = NULL;
  };

  mmSufa::
  mmSufa(string fname, Ctrack const* c)
  {
    open(fname,c);
  }

  void
  mmSufa::
  open(string fname, Ctrack const* c)
  {
    assert(c);
    corpus = c;
    open_mapped_file_source(file, fname);
    char const* p = file.data();
    uint64_t fSize = getFileSize(fname);
    if (fSize < sizeof(filepos_type)+sizeof(id_type))
       cerr << efatal << "Bad memory mapped suffix array file '" << fname << "'."
            << exit_1;
    filepos_type idxOffset;
    p = numread(p,idxOffset);
    p = numread(p,indexSize);
    data = p;
    index = reinterpret_cast<filepos_type const*>(file.data()+idxOffset);
    endData = reinterpret_cast<char const*>(index);
    // Include the end index for the last unique token id when computing file size.
    if (fSize != idxOffset + (indexSize+1)*sizeof(filepos_type))
       cerr << efatal << "Bad memory mapped suffix array file '" << fname << "'."
            << exit_1;
    corpusSize = c->size();
    numTokens  = c->numTokens();
  }

  char const*
  mmSufa::
  getLowerBound(id_type id) const
  {
    if (id >= indexSize)
      cerr << efatal << "Encountered bad token id: " << id
           << ". # ids in suffix array =" << indexSize << "." << exit_1;
    return data+index[id];
  }

  char const*
  mmSufa::
  getUpperBound(id_type id) const
  {
    if (id >= indexSize)
      cerr << efatal << "Encountered bad token id: " << id
           << ". # ids in suffix array =" << indexSize << "." << exit_1;
    if (index[id] == index[id+1])
      return NULL;
    else
      return data+index[id+1];
  }

  char const*
  mmSufa::
  readSid(char const* p, char const* q, id_type& sid) const
  {
    return tightread(p,q,sid);
  }

  inline
  char const*
  mmSufa::
  readOffset(char const* p, char const* q, uint16_t& offset) const
  {
    return tightread(p,q,offset);
  }

  count_type
  mmSufa::
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
  
  void 
  mmSufa::
  getCounts(char const* p, char const* const q, 
	    count_type& sids, count_type& raw) const
  {
    raw = 0;
    id_type sid; uint16_t off;
    boost::dynamic_bitset<uint64_t> check(corpus->size());
    while (p < q)
      {
	p = tightread(p,q,sid);
	p = tightread(p,q,off);
	check.set(sid);
	raw++;
      }
    sids = check.count();
  }

  mmSufa::
  tree_iterator::
  tree_iterator(mmSufa const* s)
    : Sufa::tree_iterator::tree_iterator(reinterpret_cast<Sufa const*>(s))
  {};

//   /** returns -1 if [sA,eA) is less than or equal to [sB,eB)
//    *  returns  0 if [sA,eA) is equal or greater than [sB,eB)
//    *                AND [sB,eB) is a prefix of [sA,eA)
//    *  returns  1 otherwise.
//    * This is an auxiliary function for Sufa::tree_iterator::down().
//    */
//   inline
//   int
//   longerWithSamePrefix(id_type const* sA, id_type const* const eA,
//                        id_type const* sB, id_type const* const eB)
//   {
//     for (;sA < eA && sB < eB; ++sA, ++sB)
//       {
//         if (*sA < *sB) return -1;
//         if (*sA > *sB) return  1;
//       }
//     // cout << eA - sA << " " << eB-sB << endl;
//     if (sB == eB) return (sA==eA ? -1 : 0);
//   }


//         // this means we need to find a longer suffix with 
//         // the same prefix
//         char const* up = upper.back();
//         assert(lo < up);
//         IndexEntry I2(root,index_jump(lo,up,10));
//         idptr sCand = S.corpus->sntStart(I2.sid)+I2.offset;
//         idptr eCand = S.corpus->sntEnd(I2.sid);
//         int x = longerWithSamePrefix(sCand,eCand,sRef,eRef);
//         while (lo < up)
//           {
// #if 0
//             cout << "L: [" << ulong(lo) << "] " << S.suffixAt(lo) << endl;
//             cout << "M: [" << ulong(I2.pos) << "] " 
//                  << S.suffixAt(I2.pos) << endl;
//             cout << "U: [" << ulong(up) << "] " << S.suffixAt(up) << endl;
//             cout << x << " " << up-lo << "\n" << endl;
// #endif
//             if (x == -1) 
//               lo = I2.next;
//             else 
//               up = I2.pos;
//             S.readEntry(index_jump(lo,up,10),I2);
//             sCand = S.corpus->sntStart(I2.sid)+I2.offset;
//             eCand = S.corpus->sntEnd(I2.sid);
//             x = longerWithSamePrefix(sCand,eCand,sRef,eRef);
//           }
//         assert(lo == up);
//         if (lo == S.dataEnd() || x != 0)
//           {
//             // cout << "DOWN FAILED!" << endl;
//             return false;
//           }
//       }

//     lower.push_back(lo);
//     S.readEntry(lo,I);
//     idptr r = S.corpus->sntStart(I.sid)+I.offset;


//     char const* const up = S.data+S.index[*r+1];
//     char const* u = S.find_end(lo,up,r,r+lower.size(),0);
// #if 1
//     if (!u)
//       {
//         debug = true;
//         u = S.find_end(lo,up,r,r+lower.size(),0);
//         cout << "NEW: ";
//         copy(r,r+lower.size(),ostream_iterator<int>(cout," ")); 
//         cout << endl;
//       }
// #endif
//     assert(u);
//     upper.push_back(u);
//     return true;
//   }


//   /** @return -1 if the string at [sStart,sEnd) precedes [kStart,kEnd)
//    *           0 if they are equal
//    *           1 if the string at [kStart,kEnd) precedes [eStart,eEnd)
//    * /depth/ indicates the length of the known common prefix
//    */
//   int
//   mycmp(id_type const* const sStart,
//         id_type const* const sEnd,
//         id_type const* const kStart,
//         id_type const* const kEnd,
//         size_t  depth)
//   {

//     // extern TokenIndex const* vocab;

//     id_type const* s = sStart+depth;
//     id_type const* k = kStart+depth;

// #if 0
//     cout << "COMPARE:\n";
//     id_type const* s2 = sStart+min((kEnd-kStart),(sEnd-sStart))-depth;
//     for (id_type const* x = s; x < s2; x++)
//       cout << *x << " ";
//     cout << endl;
//     for (id_type const* x = k; x < kEnd; x++)
//       cout << *x << " ";
//     cout << endl;
// #endif

//     for (;s < sEnd && k < kEnd; ++s,++k)
//       if (*s != *k) 
// 	return *s < *k ? -1 : 1;
//     return k != kEnd ? -1 : 0;
//   }


//   class SuffixLess // functor for less-than comparisons of tokens
//   {
//     Ctrack const* c;
//   public:
//     SuffixLess(Ctrack const* _c) : c(_c) {};
//     bool operator()(Sufa::IndexEntry const& A, Sufa::IndexEntry const& B) const
//     {
//       id_type const* aS = c->sntStart(A.sid)+A.offset;
//       id_type const* aE = c->sntEnd(A.sid);
//       id_type const* bS = c->sntStart(B.sid)+B.offset;
//       id_type const* bE = c->sntEnd(B.sid);
//       for (;aS < aE && bS < bE; aS++, bS++)
// 	if (*aS!=*bS) return *aS < *bS;
//       if (aS == aE && bS == bE) return false;
//       if (aS == aE) return true;
//       assert(bS == bE);
//       return false;
//     }
//   };

  class SuffixLEQ // functor for <= comparisons of tokens
  {
    Ctrack const* c;
  public:
    SuffixLEQ(Ctrack const* _c) : c(_c) {};
    bool operator()(Sufa::IndexEntry const& A, Sufa::IndexEntry const& B) const
    {
      id_type const* aS = c->sntStart(A.sid)+A.offset;
      id_type const* aE = c->sntEnd(A.sid);
      id_type const* bS = c->sntStart(B.sid)+B.offset;
      id_type const* bE = c->sntEnd(B.sid);
      for (;aS < aE && bS < bE; aS++, bS++)
	if (*aS!=*bS) return *aS < *bS;
      if (aS == aE && bS == bE) return true;
      if (aS == aE) return true;
      assert(bS == bE);
      return false;
    }
  };

  /** a sanity check for the integrity of the underlying data structure
   *  (and some of the member functions implemented by this class ...)
   */
  void
  mmSufa::
  sanityCheck() const
  {
    SuffixLEQ LEQ(corpus);
    IndexEntry I2;
    cerr << "Verifying proper sorting ... ";
    for (IndexEntry I1(this,data); I1.next != endData; I1 = I2)
      {
	readEntry(I1.next,I2);
	if (!LEQ(I1,I2))
	  cerr << efatal << " TEST FAILED!" << exit_1;
      }
    cerr << "OK\n";
    cerr << "Verifying index integrity ...\n" << endl;
    for (size_t i = 0; i+1 < indexSize; ++i)
      {
	if (index[i] < index[i+1]) 
	  {
	    IndexEntry I(this,data+index[i]);
	    id_type const* n = corpus->sntStart(I.sid)+I.offset;
	    assert(*n == i);
	    idptr sK = corpus->sntStart(I.sid)+I.offset;
	    idptr eK = corpus->sntEnd(I.sid);

 	    char const* x = find_start(I.pos,data+index[i+1],sK,eK,0);
            if (x != I.pos) 
                cerr << efatal << "[" << i << "] find_start "
                     << "expected: " << ulong(I.pos) << " got: " << ulong(x)
                     << exit_1;

 	    x = find_end(I.pos,endData,n,n+1,0);
	    size_t k = i+1;
	    while (index[k] == index[i]) ++k;
	    if (x != data+index[k])
	      cerr << efatal << "[" << i << "] find_end "
	           << "expected: " << ulong(data+index[k]) << " got: " << ulong(x)
		   << exit_1;
	  }
      }
    cerr << "OK\n";
  }

  void
  open_memory_mapped_suffix_array(const string& basename,
                                  TokenIndex& token_index,
                                  mmCtrack& corpus_track,
                                  mmSufa& suffix_array)
  {
     string base;
     if (!access((basename + ".tpsa/msa").c_str(), F_OK))
       base = basename + ".tpsa/";
     else if (!access((basename + "/msa").c_str(), F_OK))
       base = basename + "/";
     else
       base = basename + ".";

     token_index.open(base+"tdx");
     token_index.iniReverseIndex();
     corpus_track.open(base+"mct");
     suffix_array.open(base+"msa", &corpus_track);
  }

}
