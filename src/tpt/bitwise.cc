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
#include<vector>
#include<string>
#include <algorithm>
#include "bitwise.h"
#include <sstream>

using namespace std;
namespace ugdiss
{

  std::string bitpattern(char const& c)
  {
    std::ostringstream out;
    out << ((c&128)  ? 1 : 0);
    out << ((c&64)   ? 1 : 0);
    out << ((c&32)   ? 1 : 0);
    out << ((c&16)   ? 1 : 0);
    out << ((c&8)    ? 1 : 0);
    out << ((c&4)    ? 1 : 0);
    out << ((c&2)    ? 1 : 0);
    out << ((c&1)    ? 1 : 0);
    return out.str();
  }

  std::string bitpattern(char const& c, int offset)
  {
    std::ostringstream out;
    if (!offset) return bitpattern(c);
    out << (offset-- > 0? ((c&128)  ? '1' : '0') : '.');
    out << (offset-- > 0? ((c& 64)  ? '1' : '0') : '.');
    out << (offset-- > 0? ((c& 32)  ? '1' : '0') : '.');
    out << (offset-- > 0? ((c& 16)  ? '1' : '0') : '.');
    out << (offset-- > 0? ((c&  8)  ? '1' : '0') : '.');
    out << (offset-- > 0? ((c&  4)  ? '1' : '0') : '.');
    out << (offset-- > 0? ((c&  2)  ? '1' : '0') : '.');
    out << (offset-- > 0? ((c&  1)  ? '1' : '0') : '.');
    return out.str();
  }

  std::string bitpattern(unsigned char const& c)
  {
    std::ostringstream out;
    out << ((c&128)  ? 1 : 0);
    out << ((c&64)   ? 1 : 0);
    out << ((c&32)   ? 1 : 0);
    out << ((c&16)   ? 1 : 0);
    out << ((c&8)    ? 1 : 0);
    out << ((c&4)    ? 1 : 0);
    out << ((c&2)    ? 1 : 0);
    out << ((c&1)    ? 1 : 0);
    return out.str();
  }

  std::string bitpattern(uint32_t const& x)
  {
    unsigned char const* p = reinterpret_cast<unsigned char const*>(&x+1);
    unsigned char const* stop = reinterpret_cast<unsigned char const*>(&x);
    string ret;
    while (--p >= stop)
      ret += bitpattern(*p)+" ";
    return ret;
  }

  std::string bitpattern(string const& x)
  {
    string ret;
    for (size_t i = 0; i < x.size(); i++)
      ret += bitpattern(x[i])+" ";
    return ret;
  }

  std::string bitpattern(string const& x,size_t offset)
  {
    string ret;
    for (size_t i = 0; i+1 < x.size(); i++)
      ret += bitpattern(x[i])+" ";
    if (x.size())
      ret += bitpattern(x[x.size()-1],offset)+" ";
    return ret;
  }

  string ruler(vector<int> b)
  {
    string ret; int x=0;
    for (size_t i = 0; i < b.size(); i++)
      {
	for (int k = 0; k < b[i]; k++)
	  {
	    ret += ' ';
	    if (++x%8==0)
	      ret += ' ';
	  }
	ret += '|';
	if (++x%8==0)
	  ret += ' ';
      }
    ret += string(71-ret.size(),' ');
    reverse(ret.begin(),ret.end());
    return ret;
  }

  string valrep(uint64_t val, vector<int> b)
  {
    string ret;
    size_t bits_written = 0;
    for (size_t i = 0; i < b.size(); i++)
      {
	for (int k = 0; k < b[i]; k++)
	{
	  ret += val%2 ? '1' : '0';
	  val >>=1;
	  if (++bits_written%8==0 && k+1 < b[i])
	    ret += ' ';
	}
	if (i+1 < b.size()) 
	  {
	    ret += '|';
	    bits_written++;
	  }
	if (bits_written%8==0) ret += ' ';
      }
    reverse(ret.begin(),ret.end());
    return string(71+b.size()-ret.size(), ' ')+ret;
  }
  
  string bufrep(uint64_t val, vector<int> b, int bo, int bytes)
  {
    string ret = bo ? string(bo,'-') : "";
    val >>=bo;
    int bits_written = bo;
    for (size_t i = 0; i < b.size(); i++)
      {
	for (int k = 0; k < b[i]; k++)
	{
	  ret += val%2 ? '1' : '0';
	  val >>=1;
	  if (++bits_written%8==0 && k+1 < b[i])
	    ret += ' ';
	}
	ret += val%2 ? '*' : '|';
	bits_written++;
	val >>=1;
	if (bits_written%8==0) 
	  ret += (bits_written/8==bytes ? ':' : ' ');
      }
    reverse(ret.begin(),ret.end());
    return string(71+b.size()-ret.size(), ' ')+ret;
  }


  string
  bitwise_str(string const& _x, bool reverse)
  {
    if (_x.size() == 0) return "";
    size_t i = reverse ? _x.size()-1 : 0;
    string buf = bitwise(_x[i]);
    for (size_t i = 1; i < _x.size(); i++)
      { 
	buf += ' '; 
	buf += bitwise(_x.at(reverse ? _x.size()-1-i : i));
      }
    return buf;
  }
}
