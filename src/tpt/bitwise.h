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


// (c) 2006,2007,2008 Ulrich Germann
#ifndef __BITWISE_HH
#define __BITWISE_HH

#include <string>
#include <vector>
#include <endian.h>
#include <stdint.h>
namespace ugdiss
{
  using namespace std;

  string ruler(vector<int> b);
  string bitwise_str(string const& _x, bool reverse=false);
  string valrep(uint64_t val, vector<int> b);
  string bufrep(uint64_t buf, vector<int> b, int bo, int written);
      
  template<typename T>
  string
  bitwise(T const& _x, size_t markbyte=0)
  {
    string buf;
    T x = _x;
    char* cx = reinterpret_cast<char*>(&x);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    cx += sizeof(T)-1;
#endif
    for (size_t i = 0; i < sizeof(T); i++)
      {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	char c = *(cx-i);
	if (i) buf += (sizeof(T)==markbyte ? '|' : ' ');
#else
	char c = cx[i];
	if (i) buf += (sizeof(T)==i+markbyte ? '|' : ' ');
#endif
	buf += c&128 ? '1' : '0'; 
	buf += c&64  ? '1' : '0'; 
	buf += c&32  ? '1' : '0'; 
	buf += c&16  ? '1' : '0'; 
	buf += c&8   ? '1' : '0'; 
	buf += c&4   ? '1' : '0'; 
	buf += c&2   ? '1' : '0'; 
	buf += c&1   ? '1' : '0'; 
      }
    return buf;
  }

  string bitwise(char const* _x);

  std::string bitpattern(char const& c);
  std::string bitpattern(unsigned char const& c);
  std::string bitpattern(uint32_t const& x);
  std::string bitpattern(uint64_t const& x);
  std::string bitpattern(string const& x);
  std::string bitpattern(string const& x, size_t offset);
  std::string bitpattern(char const& c, int offset);
  //std::string bitpattern(uint32_t const& x, size_t bytes, size_t bits);


}
#endif
