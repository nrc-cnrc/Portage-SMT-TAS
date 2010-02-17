// (c) 2007-2009 Ulrich Germann. All rights reserved.
// Licensed to NRC under special agreement.

#include "ug_sufa_base.h"
#include "ug_vocab.h"
#include "tpt_typedefs.h"

#include <sstream>

namespace ugdiss
{
  using namespace std;

  typedef id_type const* idptr;

  bool debug = false;
  void 
  Sufa::
  dump(ostream& out, TokenIndex const& T) const
  {
    if (!corpus or !data)
      cerr << efatal << "Empty suffix array or corpus. Suffix array or "
           << "corpus track file may not opened." << exit_1;
    char const* p = data;
    size_t token_count = 0;
    while (p < endData)
      {
        ++token_count;
	id_type sid; uint16_t off;
	p = readSid(p,endData,sid);
	p = readOffset(p,endData,off);
	id_type const* i = corpus->sntStart(sid)+off;
	id_type const* const stop = corpus->sntEnd(sid);
	if (i < stop)
	  out << T[*i];
	while (++i < stop)
	  out << " " << T[*i];
	out << endl;
      }
    if (token_count != numTokens)
      cerr << efatal << "Bad suffix array. Token count mismatch: read "
           << token_count << " but expected " << numTokens << exit_1;
  }

  /** fill the dynamic bitset with information as to which sentences
   *  the phrase occurs in
   * @return total number of occurrences of the phrase in the corpus
   */
  count_type 
  Sufa::
  fillBitSet(vector<id_type> const& key,
             boost::dynamic_bitset<uint64_t>& bitset) const
  {
    char const* lo = lower_bound(key.begin(),key.end());
    char const* up = upper_bound(key.begin(),key.end());
    bitset.resize(corpus->size());
    bitset.reset();
    char const* p = lo;
    id_type sid;
    ushort  off;
    count_type wcount=0;
    while (p < up)
      {
        p = readSid(p,up,sid);
        corpus->check_sid(sid);
        p = readOffset(p,up,off);
        bitset.set(sid);
        wcount++;
      }
    return wcount;
  }

  count_type
  Sufa::
  sntCnt(char const* p, char const* const q) const
  {
    id_type sid; uint16_t off;
    boost::dynamic_bitset<uint64_t> check(corpus->size());
    while (p < q)
      {
	p = readSid(p,q,sid);
        corpus->check_sid(sid);
	p = readOffset(p,q,off);
	check.set(sid);
      }
    return check.count();
  }

  string 
  Sufa::
  suffixAt(char const* p, TokenIndex const* V, size_t maxlen) const
  {
    if (!p) return "";
    if (p < data || p >= endData)
      cerr << efatal << "Encountered bad suffix array pointer." << exit_1;
    return suffixAt(IndexEntry(this,p),V,maxlen);
  }

  string 
  Sufa::
  suffixAt(IndexEntry const& I, TokenIndex const* V,size_t maxlen) const
  {
    if (!I.pos) return "";
    id_type const* a = corpus->sntStart(I.sid)+I.offset;
    id_type const* z = corpus->sntEnd(I.sid);
    if (maxlen) z = a+min(int(z-a),int(maxlen));
    if (a>=z) return "";
    ostringstream buf;
    if (V) 
      {
	buf << (*V)[*a]; 
	while (++a < z)
	  buf << " " << (*V)[*a];
      }
    else
      { 
	buf << *a;
	while (++a < z) 
	  buf << " " << *a;
      }
    return buf.str();
  }

  void
  Sufa::
  readEntry(char const* p, IndexEntry& I) const
  {
    I.pos  = p;
    p      = readSid(p,endData,I.sid);
    I.next = readOffset(p,endData,I.offset);

  };

  Sufa::
  IndexEntry::
  IndexEntry()
    : pos(NULL), next(NULL), sid(0), offset(0)
  {};

  Sufa::
  IndexEntry::
  IndexEntry(Sufa const* S, char const* p)
  {
    S->readEntry(p,*this);
  }

  id_type const*
  Sufa::
  IndexEntry::
  suffixStart(Sufa const* S) const
  {
    if (!pos) return NULL;
    return S->corpus->sntStart(sid)+offset;
  }

  id_type const*
  Sufa::
  IndexEntry::
  suffixEnd(Sufa const* S) const
  {
    if (!pos) return NULL;
    return S->corpus->sntEnd(sid);
  }

  char const*
  Sufa::dataEnd() const
  {
    return endData;
  }

  /** @return -1 if the string at p precedes [kS,kE)
   *           0 if they are equal
   *           1 if [kS,kE) is the prefix of the string at p,
   *             but p is longer
   *           2 if [kStart,kEnd) precedes the string at p
   * /depth/ indicates the length of the known common prefix
   */
  int
  Sufa::
  mycmp(IndexEntry const& I, 
	id_type const* kS, 
        id_type const* const kE,
        size_t  depth) const
  {
    // /depth/ is for the case that we have a guarantee that the first
    // /depth/ tokens are irrelevant (e.g., because they are known to be 
    // identical. In this case we consider the suffix I only from position
    // /depth/ on. The reference range of integer pointers is assumed to 
    // point only to the relevant parts of the key.
    // THIS HAS NOT BEEN TESTED YET!
    idptr xS = corpus->sntStart(I.sid)+I.offset+depth;
    idptr xE = corpus->sntEnd(I.sid);
    for (; kS < kE && xS < xE; ++kS, ++xS)
      {
	if (*xS < *kS) return -1;
	if (*xS > *kS) return  2;
      }
    // cout << xE-xS << " " << kE-kS << endl;
    if (xS == xE && kS == kE) return 0;
    if (xS < xE) return 1;
    return -1;
  }

  /** compare the suffixes at two index positions */
  int
  Sufa::
  mycmp(IndexEntry const& A, IndexEntry const& B) const
  {
    idptr kS = corpus->sntStart(B.sid)+B.offset;
    idptr kE = corpus->sntEnd(B.sid);
    return mycmp(A,kS,kE,0);
  }

  /** return the lower bound (first matching entry)
   *  of the token range matching [startKey,endKey)
   */
  char const* 
  Sufa::
  find_start(char const* lo, char const* const upX,
	     id_type const* const refStart,
	     id_type const* const refEnd,
	     size_t d) const
  {
    char const* up = upX;
    if (lo >= up) return NULL;
    int x;
    IndexEntry I;
    while (lo < up)
      {
        readEntry(index_jump(lo,up,.5),I);
        x = mycmp(I,refStart,refEnd,d);
#if 0
        debug=true;
        if (debug) 
          {
            cout << "R: " << string(int(log10(size_t(lo)))+4,' ');
            copy(refStart,refEnd,ostream_iterator<int>(cout," "));
            cout << endl;
            cout << "L: [" << ulong(lo) << "] " << suffixAt(lo) << endl;
            cout << "M: [" << ulong(I.pos) << "] " << suffixAt(I.pos) << endl;
            cout << "U: [" << ulong(up) << "] " << suffixAt(up) << endl;
            cout << x << " " << up-lo << "\n" << endl;
          }
        debug=false;
#endif
        if (x >= 0)
          up = I.pos;
        else
          lo = I.next;
      }
    if (lo > up)
      cerr << efatal << "Binary search of suffix array failed." << endl;
    if (lo < upX)
      {
        readEntry(lo,I);
        x = mycmp(I,refStart,refEnd,d);
      }
    if (x >= 0) 
      return lo;
    return NULL;
  }

  /** return the upper bound (first entry beyond)
   *  of the token range matching [startKey,endKey)
   */
  char const* 
  Sufa::
  find_end(char const* lo, char const* const upX,
           id_type const* const refStart,
           id_type const* const refEnd,
           size_t d) const
           
  {
    char const* up = upX;
    if (lo >= up) return NULL;
    int x;
    IndexEntry I;
    float ratio = .1;
    while (lo < up)
      {
        readEntry(index_jump(lo,up,.1),I);
        x = mycmp(I,refStart,refEnd,d);
#if 0
        debug=true;
        if (debug)
          {
            cout << "R: " << string(int(log10(ulong(lo)))+4,' ');
            copy(refStart,refEnd,ostream_iterator<int>(cout," "));
            cout << endl;
            cout << "L: [" << ulong(lo) << "] " << suffixAt(lo) << endl;
            cout << "M: [" << ulong(I.pos) << "] " << suffixAt(I.pos) << endl;
            cout << "U: [" << ulong(up) << "] " << suffixAt(up) << endl;
            cout << x << " " << up-lo << "\n" << endl;
          }
        debug=false;
#endif
        if (x == 2)
          up = I.pos;
        else
          lo = I.next;
        ratio = .5;
      }
    if (lo > up)
      cerr << efatal << "Binary search of suffix array failed." << endl;
    if (lo < upX)
      {
        readEntry(lo,I);
        x = mycmp(I,refStart,refEnd,d);
      }
    if (x == 2) 
      return up;
    return upX;
  }

  /** return the first entry that has the prefix [refStart,refEnd)
   *  but continues on
   */
  char const* 
  Sufa::
  find_longer(char const* lo, char const* const upX,
              id_type const* const refStart,
              id_type const* const refEnd,
              size_t d) const
  {
    char const* up = upX;
    if (lo >= up) return NULL;
    int x;
    IndexEntry I;
    while (lo < up)
      {
        readEntry(index_jump(lo,up,.5),I);
        x = mycmp(I,refStart,refEnd,d);
#if 0
        if (debug)
          {
            cout << "R: " << string(17,' ');
            copy(refStart,refEnd,ostream_iterator<int>(cout," "));
            cout << endl;
            cout << "L: [" << ulong(lo) << "] " << suffixAt(lo) << endl;
            cout << "M: [" << ulong(I.pos) << "] " << suffixAt(I.pos) << endl;
            cout << "U: [" << ulong(up) << "] " << suffixAt(up) << endl;
            cout << x << " " << up-lo << "\n" << endl;
          }
#endif
        if (x > 0)
          up = I.pos;
        else
          lo = I.next;
      }
    if (lo > up)
      cerr << efatal << "Binary search of suffix array failed." << endl;
    if (lo < upX)
      {
        readEntry(index_jump(lo,up,.5),I);
        x = mycmp(I,refStart,refEnd,d);
      }
    if (x == 1) 
      return up;
    return NULL;
  }

  /** returns the start position in the byte array representing
   *  the tightly packed sorted list of corpus positions for the
   *  given search phrase (i.e., points just beyond the range)
   */
  char const* 
  Sufa::
  lower_bound(vector<id_type>::const_iterator const& keyStart,
              vector<id_type>::const_iterator const& keyStop) const
  {
     return lower_bound(&(*keyStart), &(*keyStop));
  }

  /** returns the start position in the byte array representing
   *  the tightly packed sorted list of corpus positions for the
   *  given search phrase
   */
  char const* 
  Sufa::
  lower_bound(id_type const* const keyStart,
              id_type const* const keyStop) const
  {
    if (keyStart == keyStop) return data;
    char const* const lower = getLowerBound(*keyStart);
    char const* const upper = getUpperBound(*keyStart);
    return find_start(lower,upper,keyStart,keyStop,0);
  }

  /** returns the upper bound in the byte array representing
   *  the tightly packed sorted list of corpus positions for the
   *  given search phrase (i.e., points just beyond the range)
   */
  char const* 
  Sufa::
  upper_bound(vector<id_type>::const_iterator const& keyStart,
              vector<id_type>::const_iterator const& keyStop) const
  {
     return upper_bound(&(*keyStart), &(*keyStop));
  }

  /** returns the upper bound in the byte array representing
   *  the tightly packed sorted list of corpus positions for the
   *  given search phrase (i.e., points just beyond the range)
   */
  char const* 
  Sufa::
  upper_bound(id_type const* keyStart, id_type const* keyStop) const
  {
    if (keyStart == keyStop) return reinterpret_cast<char const*>(index);
    char const* const lower = getLowerBound(*keyStart);
    char const* const upper = getUpperBound(*keyStart);
    return find_end(lower,upper,keyStart,keyStop,0);
  }

  uint64_t
  Sufa::
  getPhraseID(vector<id_type>::const_iterator const& pstart,
              vector<id_type>::const_iterator const& pstop) const
  {
    char const* p = lower_bound(pstart,pstop);
    if (!p) return 0; // not found!
    IndexEntry I;
    readEntry(p,I);
    uint64_t ret = I.sid;
    ret <<= 16;
    ret += I.offset;
    ret <<= 16;
    ret += pstop-pstart;
    return ret;
  }

  string
  Sufa::
  getPhrase(uint64_t pid, TokenIndex const& V) const
  {
    size_t   plen = pid % 65536;
    size_t offset = (pid >> 16) % 65536;
    id_type const* w = corpus->sntStart(pid >> 32)+offset;
    id_type const* stop = w+plen;
    ostringstream buf;
    buf << V[*w];
    for(++w; w < stop; ++w)
      buf << " " << V[*w];
    return buf.str();
  }

  size_t
  Sufa::
  getCorpusSize() const
  {
    return corpusSize;
  }

  Ctrack const*
  Sufa::
  getCorpus() const
  {
    return corpus;
  }



  Sufa::
  tree_iterator::
  tree_iterator(Sufa const* s)
    : root(s)
  {};

  Sufa::
  tree_iterator::
  tree_iterator(Sufa const* s, id_type id)
    : root(s)
  {
    char const* up = root->getUpperBound(id);
    if (!up) return;
    lower.push_back(root->getLowerBound(id));
    upper.push_back(up);
  };

  Sufa::
  tree_iterator::
  tree_iterator(Sufa const* s, id_type const* kstart, id_type const* kend)
    : root(s)
  {
    assert(kstart < kend);
    while (kstart < kend)
      if (!extend(*kstart++))
        break;
    if (kstart < kend)
      {
        lower.clear();
        upper.clear();
      }
  };

  bool
  Sufa::
  tree_iterator::
  extend(id_type id)
  {
    if (lower.size())
      {
        char const* lo = lower.back();
        char const* hi = upper.back();
        lo = root->find_start(lo, hi, &id, &id+1, lower.size());
        if (!lo) return false;
        lower.push_back(lo);
        hi = root->find_end(lo, hi, &id, &id+1, lower.size());
      }
    else
      {
        char const* up = root->getUpperBound(id);
        if (!up) return false;
        lower.push_back(root->getLowerBound(id));
        upper.push_back(up);
      }
    return true;
  };

  size_t
  Sufa::
  tree_iterator::
  size() const
  {
    return lower.size();
  }

  char const*
  Sufa::
  tree_iterator::
  lower_bound(int p) const
  {
    if (p < 0)
      {
        assert(int(lower.size()) > -p);
        return *(lower.rbegin()-p);
      }
    else
      {
        assert(p < int(lower.size()));
        return lower[p];
      }
  }

  char const*
  Sufa::
  tree_iterator::
  upper_bound(int p) const
  {
    if (p < 0)
      {
        assert(int(upper.size()) > -p);
        return *(upper.rbegin()-p);
      }
    else
      {
        assert(p < int(upper.size()));
        return upper[p];
      }
  }

  /* @return a pointer to the position in the corpus
   * where this->wid(p) is read from
   */
  id_type const*
  Sufa::
  tree_iterator::
  crpPos(int p) const
  {
    char const* x = lower.back();
    char const* z = upper.back();
    id_type sid; ushort off;
    x = root->readSid(x,z,sid);
    x = root->readOffset(x,z,off);
    if (p>=0)
      return root->corpus->sntStart(sid)+off+p;
    else
      return root->corpus->sntStart(sid)+off+lower.size()-1+p;
  }

  id_type
  Sufa::
  tree_iterator::
  wid(int p) const
  {
    return *crpPos(p);
  }

  bool
  Sufa::
  tree_iterator::
  up()
  {
    if (lower.size())
      {
        lower.pop_back();
        upper.pop_back();
        return true;
      }
    else
      return false;
  }

  bool
  Sufa::
  tree_iterator::
  over()
  {
    if (lower.size() == 0) 
      return false;
    else if (lower.size() == 1)
      {
        id_type id = this->wid(0);
        for (++id; id < root->indexSize; ++id)
          {
            char const* lo = root->getLowerBound(id);
            char const* hi = root->getLowerBound(id+1);
            if (lo == hi) continue;
            lower[0] = lo;
            upper[0] = hi;
            assert(lower[0]);
            assert(upper[0]);
#if 0
            cout << "L: " << root->suffixAt(lower[0]) << endl;
            cout << "U: " << root->suffixAt(upper[0]) << endl;
#endif
            break;
          }
        return id < root->indexSize;
      }
    else
      {
        if (upper.back() == root->endData)
          return false;
        idptr a = IndexEntry(root,lower.back()).suffixStart(root);
        idptr z = a + lower.size()-1;
        IndexEntry U(root,upper.back());
        int x = root->mycmp(U,a,z,0);
#if 0
        cout << "[1] L: " << root->suffixAt(lower.back(),NULL,5) << endl;
        cout << "[1] U: " << root->suffixAt(upper.back(),NULL,5) << endl;
        copy(a,z,ostream_iterator<int>(cerr, " ")); cerr << endl;
        cerr << root->suffixAt(upper.back()) << endl << x << endl;
#endif
        if (x != 1)
          return false;
        
        lower.back() = upper.back();
        idptr foo = IndexEntry(root,lower.back()).suffixStart(root);
        // here is the point where we could take advantage of the
        // 'd' parameter in find_end() ...
        // We'll skip that for the time being, to play it safe
        upper.back() = root->upper_bound(foo,foo+lower.size());
#if 0
        cout << "[2] L: " << root->suffixAt(lower.back(),NULL,5) << endl;
        cout << "[2] U: " << root->suffixAt(upper.back(),NULL,5) << endl;
#endif
        return true;
      }
  }

  bool
  Sufa::
  tree_iterator::
  down()
  {
#define LOCALDEBUG 0
    if (lower.size() == 0)
      {
#if 0
        for (id_type i = 0; i < root->indexSize; ++i)
          cout << i << ": " << root->suffixAt(root->getLowerBound(i)) << endl;
#endif
        char const* lo = root->getLowerBound(0);
        if (lo == root->endData) return false;
        IndexEntry L(root,lo);
        char const* hi = root->getUpperBound(*(L.suffixStart(root)));
        assert (lo < hi);
        lower.push_back(lo);
        upper.push_back(hi);
#if LOCALDEBUG
        cout << "[DOWW] L: " << root->suffixAt(lower.back()) << endl;
        cout << "[DOWN] U: " << root->suffixAt(upper.back()) << endl;
#endif
        return lower.size();
      }

    //---------------------------------------------------------

    IndexEntry I(root,lower.back());
    idptr sRef = root->corpus->sntStart(I.sid)+I.offset;
    idptr eRef = root->corpus->sntEnd(I.sid);
    assert(eRef-sRef >= int(lower.size()));
    char const* lo=lower.back();
#if 0
    cerr << root->suffixAt(lo) << endl;
    copy(sRef,eRef,ostream_iterator<int>(cerr," "));
    cerr << endl;
#endif
    
    if (eRef == sRef+lower.size())
      {
        char const* up = upper.back();
        lo = root->find_longer(lo,up,sRef,eRef,0);
        if (!lo) return false;
        root->readEntry(lo,I);
        sRef = root->corpus->sntStart(I.sid)+I.offset;
        eRef = root->corpus->sntEnd(I.sid);
        assert(eRef-sRef >= int(lower.size()));
      }

    lower.push_back(lo);
    char const* up = root->getUpperBound(*sRef);
#if LOCALDEBUG
    cerr << "lo: " << root->suffixAt(lo) << endl;
    cerr << "up: " << root->suffixAt(up) << endl;
    debug = true;
#endif
    char const* u = root->find_end(lo,up,sRef,sRef+lower.size(),0);
#if LOCALDEBUG
    debug = false;
#endif
    assert(u);
    upper.push_back(u);

#if LOCALDEBUG
    cout << "[DOWW] L: " << root->suffixAt(lo) << endl;
    cout << "[DOWN] U: " << root->suffixAt(u) << endl;
#endif
#undef LOCALDEBUG

    return true;
  }

  string
  Sufa::
  tree_iterator::
  str(Vocab const& V) const
  {
    if (lower.size() == 0) return "";
    ostringstream buf;
    id_type const* w = crpPos(0);
    buf << V[*w].str;
    for (size_t i = 1; i < size(); ++i)
      buf << " " << V[*++w].str;
    return buf.str();
  }

  string
  Sufa::
  tree_iterator::
  str(TokenIndex const* V) const
  {
    if (lower.size() == 0) return "";
    ostringstream buf;
    id_type const* w = crpPos(0);
    if (V)
      {
        buf << (*V)[*w];
        for (size_t i = 1; i < size(); ++i)
          buf << " " << (*V)[*++w];
      }
    else
      {
        buf << *w;
        for (size_t i = 1; i < size(); ++i)
          buf << " " << ++w;
      }
    return buf.str();
  }

  void
  Sufa::
  tree_iterator::
  showBounds(ostream& out) const
  {
    for (size_t i = 0; i < lower.size(); i++)
      cout << "[" << i   << "] " << root->suffixAt(lower[i])
           << endl;
    cout << "---" << endl;
    for (size_t i = upper.size(); i > 0; i--)
      cout << "[" << i-1 << "] " << root->suffixAt(upper[i-1]) << endl;
  }

}
