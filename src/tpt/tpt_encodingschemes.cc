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



// (c) 2007,2008 Ulrich Germann

#include "tpt_encodingschemes.h"
#include <cmath>
#include <functional>
#include <algorithm>
#include "str_utils.h"

namespace ugdiss
{
  using namespace std;

  //  Formulas 1 and 2 are formulas to compute total number of bits
  //  needed with a representation scheme that uses bit blocks of the
  //  sizes stored in /b/ the vector /cnt/ maps from the total number
  //  of bits needed to store a number to the number of numbers in the
  //  underlying distribution requiring that many bits to be
  //  represented. Formula 1 assumes that every block has an additional
  //  bit that indicates whether another block needs to be read.
  //  Formula 2 assumes that the first bit block indicates the total 
  //  number of bits that need to be read.

  Escheme::formula1::formula1() : formula(1) {};
  Escheme::formula2::formula2() : formula(2) {};
  Escheme::formula::formula(uint32_t _id) : id(_id) {};

  uint64_t
  Escheme::
  formula1::
  operator()(vector<uint32_t> const& blk, vector<uint32_t> const& cnt) const
  { // extending number representation by using stop bits to indicate 
    // that a block is the last one to be read
    uint64_t ret=0;
    uint32_t numbits = 0;
    uint32_t k = 2;
    for (uint32_t i = 0; i < blk.size(); i++)
      {
	numbits += blk[i];
	uint32_t extrabits = (i+1==blk.size() ? i : i+1);
	for (; k <= numbits; k++)
	  {
	    ret += cnt[k]*(numbits+extrabits);
#if 0
	    cout << id << ":" << i << ":" <<k 
		 << setw(12) << cnt[k] << " * " 
		 << "(" << setw(2) << numbits << "+" 
		 << extrabits << ") = " 
		 <<  setw(12) << cnt[k]*(numbits+extrabits)
		 << " => " << setw(12) << ret << endl;
#endif
	  }
	//	cout << endl;
      }
    return ret;
  }

  bool
  Escheme::
  operator<(Escheme const& other) const
  {
    if (this->cost == other.cost)
      return this->blockSizes.size() < other.blockSizes.size();
    return this->cost < other.cost;
  }

  ostream&
  operator<<(ostream& os, const Escheme& s)
  {
    os << "blocks: " << Portage::join(s.blockSizes) << " fid: " << s.fid << " cost: " << s.cost;
    return os;
  }

  uint64_t
  Escheme::
  formula2::
  operator()(vector<uint32_t> const& blk, 
	     vector<uint32_t> const& cnt) const
  { // binary encoding of number of blocks to be read at the beginning
    // of each represented number
    uint64_t ret=0;
    uint32_t extrabits = (blk.size() == 1 
			? 0 
			: int(ceil(log2(blk.size()))));
    uint32_t numbits = 0;

    uint32_t k = 2;
    for (uint32_t i = 0; i < blk.size(); i++)
      {
	numbits += blk[i];
	for (; k <= numbits; k++)
	  {
	    ret += cnt[k]*(numbits+extrabits);
#if 0
	    cout << id << ":" << i << ":" <<k 
		 << setw(12) << cnt[k] << " * " 
		 << "(" << setw(2) << numbits << "+" 
		 << extrabits << ") = " 
		 <<  setw(12) << cnt[k]*(numbits+extrabits)
		 << " => " << setw(12) << ret << endl;
#endif
	  }
      }
    // cout << endl;
    return ret;
  }

  Escheme::
  Escheme(vector<uint32_t> const& blocks) 
    : blockSizes(blocks), f(NULL)
  {};

  Escheme::formula const* const f1 = new Escheme::formula1();
  Escheme::formula const* const f2 = new Escheme::formula2();

  Escheme::
  ~Escheme()
  {
    // delete f1;
    // delete f2;
  }
  
  // local helper function, publicly available function
  // is below

  void 
  enumerate_schemes(vector<uint32_t> const& D, 
		    vector<uint32_t>& bins,
		    uint32_t pos, 
		    uint32_t numTokens,
		    vector<Escheme>& schemes)
  {
    // recursive function to be called from optimize(D,max_blocks)
    if (pos+1 == bins.size())
      {
	bins.back() = numTokens;
	Escheme E(bins);
#if 1
	E.fid = 1;
	E.cost = Escheme::formula1()(bins,D);
	schemes.push_back(E);
        //cerr << E << endl;
#endif
	E.fid = 2;
	E.cost = Escheme::formula2()(bins,D);
	schemes.push_back(E);
        //cerr << E << endl;
	return;
      }
    else
      {
	for (uint32_t t = 1; t <= numTokens+1-bins.size()+pos; t++)
	  {
	    bins[pos] = t;
	    enumerate_schemes(D,bins,pos+1,numTokens-t,schemes);
	  }
      }
  }


  // this function enumerates all possible ways to break D.size() bits into
  // up to max_blocks blocks, using two different possible strategies to
  // indicate how many blocks are required: 1) store a flag bit after each
  // block but the last; 2) store the number of blocks before the first block.
  vector<Escheme>
  enumerate_schemes(vector<uint32_t> const& D, uint32_t max_blocks)
  {
    vector<Escheme> schemes;
    vector<uint32_t> bins;
    max_blocks = min(max_blocks,uint32_t(D.size()));
    while (bins.size() < max_blocks)
      {
	bins.push_back(0);
	enumerate_schemes(D,bins,0,D.size()-1,schemes);
      }
    return schemes;
  }

  // Brute force approach to find the best scheme for D:  enumerate all
  // possible ways to break D.size() bits into up to max_blocks blocks; 
  // calculate the cost of each way; sort and pick the best.
  // D[i] is the total number of occurrences of all values requiring at least
  // i bits to be represented: D[i] = sum_{j in 2^(i-1) to 2^i-1} freq(j).
  Escheme
  best_scheme(vector<uint32_t> const& D, uint32_t max_blocks)
  {
    vector<Escheme> S = enumerate_schemes(D,max_blocks);
    std::sort(S.begin(),S.end(),less<Escheme>());
    uint32_t best = 0;
    uint64_t th = 80*1024*1024; 
    // we are willing to spend 10 MB (80 Mbits) of storage for a lower number of
    // blocks for faster access during read time
    cerr << "Most compact block encoding: " << S[0] << endl;
    for (uint32_t i = 1; i < S.size(); i++)
      {    
	if (S[i].cost-S[0].cost >= th) break;
	if (S[i].blockSizes.size() < S[best].blockSizes.size()) {
	  best = i;
          //cerr << "Preferring this encoding: " << S[best]
          //     << " delta: " << (S[best].cost-S[0].cost) << endl;
        }
      }
    if (best != 0)
      cerr << "Preferring this encoding: " << S[best]
           << " delta: " << (S[best].cost-S[0].cost) << endl;
    return S[best];
  }


}

