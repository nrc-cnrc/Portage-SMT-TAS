/**
 * @author George Foster
 * @file dmstruct.h  Represent distortion model counts
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef DMSTRUCT_H
#define DMSTRUCT_H

#include <iostream>
#include <vector>
#include <str_utils.h>
#include <cstdlib>
#include <string>

namespace Portage {

// Represent distortion counts for a phrase pair

struct DistortionCount {

   static Uint size() {return 6;}   // number of counts

   Uint prevmono;
   Uint prevswap;
   Uint prevdisc;
   Uint nextmono;
   Uint nextswap;
   Uint nextdisc;

   // Convenience mapping from [0..size()-1] -> count param

   Uint& val(Uint i) {
      switch (i) {
      case 0: return prevmono;
      case 1: return prevswap;
      case 2: return prevdisc;
      case 3: return nextmono;
      case 4: return nextswap;
      case 5: return nextdisc;
      default: assert(false); exit(EXIT_FAILURE);
      }
   }

   Uint val(Uint i) const {
      switch (i) {
      case 0: return prevmono;
      case 1: return prevswap;
      case 2: return prevdisc;
      case 3: return nextmono;
      case 4: return nextswap;
      case 5: return nextdisc;
      default: assert(false); exit(EXIT_FAILURE);
      }
   }

   // Global phrase frequency is sum of prev counts (= sum of next counts),
   // since each phrase occurrence counts exactly one of these possibilities.

   Uint freq() const {return prevmono + prevswap + prevdisc;}


   void clear() { for (Uint i = 0; i < size(); ++i) val(i) = 0; }

   DistortionCount() {clear();}
   DistortionCount(int x) {clear();}
   DistortionCount( Uint prevmono, Uint prevswap, Uint prevdisc, Uint nextmono, Uint nextswap, Uint nextdisc)
      : prevmono(prevmono)
      , prevswap(prevswap)
      , prevdisc(prevdisc)
      , nextmono(nextmono)
      , nextswap(nextswap)
      , nextdisc(nextdisc)
   {}

   DistortionCount& operator+=(const DistortionCount& dc) {
      for (Uint i = 0; i < size(); ++i) val(i) += dc.val(i);
      return *this;
   }

   DistortionCount& operator-=(const DistortionCount& dc) {
      for (Uint i = 0; i < size(); ++i) val(i) -= dc.val(i);
      return *this;
   }

   bool operator>(const DistortionCount& dc) const {return freq() > dc.freq();}
   bool operator>=(const DistortionCount& dc) const {return freq() >= dc.freq();}

   // NB: used during sorting for phrasetable pruning. Not applicable in all
   // contexts!
   bool operator==(const DistortionCount& dc) const {return freq() == dc.freq();}
   bool operator!=(const DistortionCount& dc) const {return freq() == dc.freq();}

   // Read from a token sequence
   void read(vector<string>::const_iterator b, vector<string>::const_iterator e) {
      if (size_t(e - b) != size())
         error(ETFatal, "distortion count format error - expecting %d values", size());
      for (Uint i = 0; i < size(); ++i)
         val(i) = conv<Uint>(*b++);
   }
 
   // Dump counts for a single phrase-pair occurrence
   void dumpSingleton(ostream& os) {
      os << "prev=" << (prevmono ? "mono" : prevswap ? "swap" : prevdisc ? "disc" : "ERROR") 
         << ", next=" << (nextmono ? "mono" : nextswap ? "swap" : nextdisc ? "disc" : "ERROR");
   }
};

// write contents of dc on a stream
std::ostream& operator<<(std::ostream& os, const DistortionCount& dc);

inline bool conv(const string& s, DistortionCount& dc)
{
   vector<string> toks;
   if (split(s, toks) != dc.size())
      return false;
   dc.read(toks.begin(), toks.end());
   return true;
};

}

#endif
