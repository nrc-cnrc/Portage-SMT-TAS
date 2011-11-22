#ifndef __ug_tsa_tree_iterator_h
#define __ug_tsa_tree_iterator_h

#include "ug_tsa_array_entry.h"

// (c) 2007 - 2010 Ulrich Germann. All rights reserved.
// Licensed to NRC under special agreement.

namespace ugdiss
{

#ifndef _DISPLAY_CHAIN
#define _DISPLAY_CHAIN
  // for debugging only
  template<typename T>
  void display(T const* x, string label)
  {
    cout << label << ":"; for (;x;x=next(x)) cout << " " << x->lemma; cout << endl; 
  }
#endif

  template<typename T> class TSA;

  // CLASS DEFINITION
  // The TSA_tree_iterator allows traversal of a Token Sequence Array as if it
  // was a trie.
  // down(): go to first child
  // over(): go to next sibling 
  // up():   go to parent
  // extend(id): go to a specific child node
  // all four functions return true if successful, false otherwise
  // lower_bound() and upper_bound() give the range of entries in the array
  // covered by the "virtual trie node", if p==-1 or p==lower.size()-1, the
  // parent node if p==-2 or p==lower.size()-2, etc.
  template<typename TSA_TYPE>
  class
  TSA_tree_iterator
  {
  protected:
    vector<char const*> lower;
    vector<char const*> upper;

    // for debugging ...
    void showBounds(ostream& out) const;
  public:
    typedef typename TSA_TYPE::Token Token;

    TSA_TYPE const* root; // TO BE DONE: make the pointer private and add a const function to return the pointer

    TSA_tree_iterator(TSA_tree_iterator const& other);
    TSA_tree_iterator(TSA_TYPE const* s);
    TSA_tree_iterator(TSA_TYPE const* s, Token const& t);
    TSA_tree_iterator(TSA_TYPE const* s, Token const* kstart, Token const* kend);

    char const* lower_bound(int p) const;
    char const* upper_bound(int p) const;

    size_t size() const;
    Token const& wid(int p) const;
    virtual Token const* getToken(int p) const;
    id_type getSid() const;
    ushort getOffset(int p) const;
    size_t sntCnt(int p=-1) const;
    size_t rawCnt(int p=-1) const;

    virtual bool extend(Token const& id);
    virtual bool down();
    virtual bool over();
    virtual bool up();

    string str(TokenIndex const* V=NULL) const;
    string str(Vocab const& V) const;

    // fillBitSet: deprecated; use markSentences() instead
    count_type fillBitSet(boost::dynamic_bitset<uint64_t>& bitset) const;

    count_type markSentences(boost::dynamic_bitset<uint64_t>& bitset) const;
    count_type markOccurrences(boost::dynamic_bitset<uint64_t>& bitset) const;
    count_type markOccurrences(vector<ushort>& dest) const;

    uint64_t getSequenceId() const;

  };

  //---------------------------------------------------------------------------
  // DOWN
  //---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  bool
  TSA_tree_iterator<TSA_TYPE>::
  down()
  {
    if (lower.size() == 0)
      {
	char const* lo = root->arrayStart();
        assert(lo < root->arrayEnd());
	if (lo == root->arrayEnd()) return false; // array is empty, can't go down
        tsa::ArrayEntry A(root,lo);
	Token const* t = root->corpus->getToken(A); 
        assert(t);
        char const* hi = root->getUpperBound(t->id());
        assert (lo < hi);
        lower.push_back(lo);
        Token const* foo = this->getToken(0); 
        upper.push_back(root->upper_bound(foo,lower.size()));
        return lower.size();
      }
    else
      {
        char const* lo = lower.back();
        tsa::ArrayEntry A(root,lo);
        typename TSA_TYPE::Token const* a = root->corpus->getToken(A); assert(a);
        typename TSA_TYPE::Token const* z = next(a);
        for (size_t i = 1; i < size(); ++i) z = next(z);
        if (z < root->corpus->sntStart(A.sid) || z >= root->corpus->sntEnd(A.sid))
          { 
            char const* up = upper.back();
            lo = root->find_longer(lo,up,a,lower.size(),0);
            if (!lo) return false;
            root->readEntry(lo,A);
            a = root->corpus->getToken(A); assert(a);
            z = next(a);
            assert(z >= root->corpus->sntStart(A.sid) && z < root->corpus->sntEnd(A.sid));
          }
        lower.push_back(lo);
        char const* up = root->getUpperBound(a->id());
        char const* u  = root->find_end(lo,up,a,lower.size(),0);
        assert(u);
        upper.push_back(u);
        return true;
      }
  }

  // ---------------------------------------------------------------------------
  // OVER
  //---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  bool
  TSA_tree_iterator<TSA_TYPE>::
  over()
  {
    if (lower.size() == 0) 
      return false;
    if (lower.size() == 1)
      {
        Token const* t = this->getToken(0);
	id_type wid = t->id();
        char const* hi = root->getUpperBound(wid);
        if (upper[0] < hi)
          {
            lower[0] = upper[0];
            Token const* foo = this->getToken(0); 
            upper.back() = root->upper_bound(foo,lower.size());
          }
        else
          {
            for (++wid; wid < root->indexSize; ++wid)
              {
                char const* lo = root->getLowerBound(wid);
                if (lo == root->endArray) return false;
                char const* hi = root->getUpperBound(wid);
                //if (!hi) { cout << "BOOM!" << endl; return false; }
                if (!hi) return false; 
                if (lo == hi) continue;
                assert(lo);
                lower[0] = lo;
                Token const* foo = this->getToken(0); 
                upper.back() = root->upper_bound(foo,lower.size());
                // upper[0] = hi;
                break;
              }
          }
        return wid < root->indexSize;
      }
    else
      {
        if (upper.back() == root->arrayEnd())
          return false;
        tsa::ArrayEntry L(root,lower.back());
        tsa::ArrayEntry U(root,upper.back());

        // display(root->corpus->getToken(L),"L1");
        // display(root->corpus->getToken(U),"U1");

	int x = root->corpus->cmp(U,L,lower.size()-1);
	// cerr << "x=" << x << endl;
        if (x != 1)
          return false;
        lower.back() = upper.back();

        // display(root->corpus->getToken(U),"L2");

        Token const* foo = this->getToken(0); 
        // display(foo,"F!");
        upper.back() = root->upper_bound(foo,lower.size());
        return true;
      }
  }

  // ---------------------------------------------------------------------------
  // UP
  //---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  bool
  TSA_tree_iterator<TSA_TYPE>::
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


  // ---------------------------------------------------------------------------
  // CONSTRUCTORS
  //----------------------------------------------------------------------------

  template<typename TSA_TYPE>
  TSA_tree_iterator<TSA_TYPE>::
  TSA_tree_iterator(TSA_TYPE const* s)
    : root(s) 
  {};

  // ---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  TSA_tree_iterator<TSA_TYPE>::
  TSA_tree_iterator(TSA_tree_iterator<TSA_TYPE> const& other)
    : root(other.root)
  {
    lower = other.lower;
    upper = other.upper;
  };

  // ---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  TSA_tree_iterator<TSA_TYPE>::
  TSA_tree_iterator(TSA_TYPE const* s, Token const& t)
    : root(s) 
  {
    char const* up = root->getUpperBound(t);
    if (!up) return;
    lower.push_back(root->getLowerBound(t));
    upper.push_back(up);
  };

  // ---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  TSA_tree_iterator<TSA_TYPE>::
  TSA_tree_iterator(TSA_TYPE const* s, 
                    Token    const* kstart, 
                    Token    const* kend)
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

  // ---------------------------------------------------------------------------
  // EXTEND
  // ---------------------------------------------------------------------------
  
  template<typename TSA_TYPE>
  bool
  TSA_tree_iterator<TSA_TYPE>::
  extend(Token const& t)
  {
    if (lower.size())
      {
        char const* lo = lower.back();
        char const* hi = upper.back();
        lo = root->find_start(lo, hi, &t, 1, lower.size());
        if (!lo) return false;
        lower.push_back(lo);
        hi = root->find_end(lo, hi, getToken(-1), 1, lower.size()-1);
        upper.push_back(hi);
      }
    else
      {
        char const* lo = root->getLowerBound(t.id());
        char const* hi = root->getUpperBound(t.id());
        if (lo==hi) return false;
        lo = root->find_start(lo, hi, &t, 1, lower.size());
        lower.push_back(lo);
#if 0
        tsa::ArrayEntry I;
        root->readEntry(lo,I);
        cout << I.sid << " " << I.offset << endl;
        cout << root->corpus->sntLen(I.sid) << endl;
#endif
        hi = root->find_end(lo, hi, getToken(0), 1, 0);
        upper.push_back(hi);
      }
    return true;
  };

  // ---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  size_t
  TSA_tree_iterator<TSA_TYPE>::
  size() const 
  { 
    return lower.size(); 
  }

  // ---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  id_type
  TSA_tree_iterator<TSA_TYPE>::
  getSid() const 
  { 
    char const* p = (lower.size() ? lower.back() : root->startArray);
    char const* q = (upper.size() ? upper.back() : root->endArray);
    id_type sid;
    root->readSid(p,q,sid);
    return sid;
  }

  // ---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  char const*
  TSA_tree_iterator<TSA_TYPE>::
  lower_bound(int p) const
  {
    if (p < 0) p += lower.size();
    assert(p >= 0 && p < int(lower.size()));
    return lower[p];
  }

  // ---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  char const*
  TSA_tree_iterator<TSA_TYPE>::
  upper_bound(int p) const
  {
    if (p < 0) p += upper.size();
    assert(p >= 0 && p < int(upper.size()));
    return upper[p];
  }

  // ---------------------------------------------------------------------------

  /* @return a pointer to the position in the corpus
   * where this->wid(p) is read from
   */
  template<typename TSA_TYPE>
  typename TSA_TYPE::Token const*
  TSA_tree_iterator<TSA_TYPE>::
  getToken(int p) const
  {
    if (lower.size()==0) return NULL;
    tsa::ArrayEntry A(root,lower.back());
    Token const* t   = root->corpus->getToken(A); assert(t);
    Token const* bos = root->corpus->sntStart(A.sid);
    Token const* eos = root->corpus->sntEnd(A.sid);
    if (p < 0) p += lower.size();
    // cerr << p << ". " << t->id() << endl;
    while (p-- > 0)
      {
        t = next(t);
	// if (t) cerr << p << ". " << t->id() << endl;
        assert(t >= bos && t < eos);
      }
    return t;
  }

  // ---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  size_t
  TSA_tree_iterator<TSA_TYPE>::
  sntCnt(int p) const
  {
    if (p < 0)
      p = lower.size()+p;
    assert(p>=0);
    if (lower.size() == 0) return root->getCorpusSize();
    return reinterpret_cast<TSA<Token> const* const>(root)->sntCnt(lower[p],upper[p]);
  }

  // ---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  size_t
  TSA_tree_iterator<TSA_TYPE>::
  rawCnt(int p) const
  {
    if (p < 0)
      p = lower.size()+p;
    assert(p>=0);
    if (lower.size() == 0) return root->getCorpusSize();
    return root->rawCnt(lower[p],upper[p]);
  }

  //---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  count_type 
  TSA_tree_iterator<TSA_TYPE>::
  fillBitSet(boost::dynamic_bitset<uint64_t>& bitset) const
  {
    return markSentences(bitset);
  }

  //---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  count_type 
  TSA_tree_iterator<TSA_TYPE>::
  markSentences(boost::dynamic_bitset<uint64_t>& bitset) const
  {
    assert(root && root->corpus);
    bitset.resize(root->corpus->size());
    bitset.reset();
    if (lower.size()==0) return 0;
    char const* lo = lower.back();
    char const* up = upper.back();
    char const* p = lo;
    id_type sid;
    ushort  off;
    count_type wcount=0;
    while (p < up)
      {
        p = root->readSid(p,up,sid);
        p = root->readOffset(p,up,off);
        bitset.set(sid);
        wcount++;
      }
    return wcount;
  }

  //---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  count_type 
  TSA_tree_iterator<TSA_TYPE>::
  markOccurrences(boost::dynamic_bitset<uint64_t>& bitset) const
  {
    assert(root && root->corpus);
    if (bitset.size() != root->corpus->numTokens())
      bitset.resize(root->corpus->numTokens());
    bitset.reset();
    if (lower.size()==0) return 0;
    char const* lo = lower.back();
    char const* up = upper.back();
    char const* p = lo;
    id_type sid;
    ushort  off;
    count_type wcount=0;
    typename TSA_TYPE::Token const* crpStart = root->corpus->sntStart(0);
    while (p < up)
      {
        p = root->readSid(p,up,sid);
        p = root->readOffset(p,up,off);
        typename TSA_TYPE::Token const* t = root->corpus->sntStart(sid)+off;
        for (size_t i = 0; i < lower.size(); ++i, t = t->next())
          bitset.set(t-crpStart);
        wcount++;
      }
    return wcount;
  }
  //---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  count_type 
  TSA_tree_iterator<TSA_TYPE>::
  markOccurrences(vector<ushort>& dest) const
  {
    assert(root && root->corpus);
    assert(dest.size() == root->corpus->numTokens());
    if (lower.size()==0) return 0;
    char const* lo = lower.back();
    char const* up = upper.back();
    char const* p = lo;
    id_type sid;
    ushort  off;
    count_type wcount=0;
    typename TSA_TYPE::Token const* crpStart = root->corpus->sntStart(0);
    while (p < up)
      {
        p = root->readSid(p,up,sid);
        p = root->readOffset(p,up,off);
        typename TSA_TYPE::Token const* t = root->corpus->sntStart(sid)+off;
        for (size_t i = 1; i < lower.size(); ++i, t = t->next());
        dest[t-crpStart]++;
        wcount++;
      }
    return wcount;
  }
  //---------------------------------------------------------------------------

  template<typename TSA_TYPE>
  uint64_t
  TSA_tree_iterator<TSA_TYPE>::
  getSequenceId() const
  {
    if (this->size() == 0) return 0;
    char const* p = this->lower_bound(-1);
    typename TSA_TYPE::ArrayEntry I;
    root->readEntry(p,I);
    return (I.sid<<32)+(I.offset<<16)+this->size();
  }


}
#endif
