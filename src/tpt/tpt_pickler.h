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

#ifndef __Pickler
#define __Pickler

#include<iostream>
#include<string>
#include<vector>
#include<map>
#include "tpt_typedefs.h"
#include "num_read_write.h"

// using namespace std;

namespace ugdiss
{
  /** 
   * The following functions write and read data in a compact binary 
   * representation. Write and read errors can be checked directly
   * on the ostream object after the function call, so no return value is
   * necessary.*/
  void binwrite(std::ostream& out, char               data); 
  void binwrite(std::ostream& out, unsigned char      data); 
  void binwrite(std::ostream& out, unsigned short     data);
  void binwrite(std::ostream& out, unsigned int       data);
  void binwrite(std::ostream& out, unsigned long      data);
  void binwrite(std::ostream& out, size_t             data);
  void binwrite(std::ostream& out, unsigned long long data);
  void binwrite(std::ostream& out, std::string const& data);
  void binwrite(std::ostream& out, float              data); 

  void binread(std::istream& out, char               &data); 
  void binread(std::istream& out, unsigned char      &data); 
  void binread(std::istream& out, unsigned short     &data);
  void binread(std::istream& out, unsigned int       &data);
  void binread(std::istream& out, unsigned long      &data);
  void binread(std::istream& out, size_t             &data);
  void binread(std::istream& out, unsigned long long &data);
  void binread(std::istream& out, std::string const  &data);
  void binread(std::istream& out, float              &data); 

  // void binwrite(std::ostream& out, size_t data); 
  // size_t is 4 (32-bit architecture) or 8 bytes (64-bit architecture)
  // => don't use it

  std::ostream& write(std::ostream& out, char x);
  std::ostream& write(std::ostream& out, unsigned char x);
  std::ostream& write(std::ostream& out, short x);
  std::ostream& write(std::ostream& out, unsigned short x);
  std::ostream& write(std::ostream& out, long x);
  std::ostream& write(std::ostream& out, size_t x);
  std::ostream& write(std::ostream& out, float x);

  std::istream& read(std::istream& in, char& x);
  std::istream& read(std::istream& in, unsigned char& x);
  std::istream& read(std::istream& in, short& x);
  std::istream& read(std::istream& in, unsigned short& x);
  std::istream& read(std::istream& in, long& x);
  std::istream& read(std::istream& in, size_t& x);
  std::istream& read(std::istream& in, float& x);

  template<typename T>
  T read(std::istream& in)
  {
    T ret;
    read(in,ret);
    return ret;
  }

  template<typename T>
  T binread(std::istream& in)
  {
    T ret;
    binread(in,ret);
    return ret;
  }


  template<typename T>
  void 
  binwrite(std::ostream& out, std::vector<T> const& data)
  {
    binwrite(out,data.size());
    for (size_t i = 0; i < data.size(); i++)
      { binwrite(out,data[i]); }
  }

  template<typename T>
  void 
  binread(std::istream& in, std::vector<T>& data)
  {
    size_t s;
    binread(in,s);
    data.resize(s);
    for (size_t i = 0; i < s; i++)
      { binread(in,data[i]); }
  }

  template<typename K, typename V>
  void
  binread(std::istream& in, std::map<K,V>& data)
  {
    size_t s; K k; V v;
    binread(in,s);
    data.clear(); 
    // I have no idea why this is necessary, but it is, even when 
    // /data/ is supposed to be empty
    for (size_t i = 0; i < s; i++)
      {
	binread(in,k);
	binread(in,v);
	data[k] = v;
	// cerr << "* " << i << " " << k << " " << v << endl;
      }
  }

  template<typename K, typename V>
  void
  binwrite(std::ostream& out, std::map<K,V> const& data)
  {
    binwrite(out,data.size());
    for (typename std::map<K,V>::const_iterator m = data.begin(); 
	 m != data.end(); m++)
      {
	binwrite(out,m->first);
	binwrite(out,m->second);
      }
  }

  template<typename K, typename V>
  void
  binwrite(std::ostream& out, std::pair<K,V> const& data)
  {
    binwrite(out,data.first);
    binwrite(out,data.second);
  }

  template<typename K, typename V>
  void
  binread(std::istream& in, std::pair<K,V>& data)
  {
    binread(in,data.first);
    binread(in,data.second);
  }


  template<typename numtype>
  char const* 
  binread(char const* p, numtype& buf);
#if 0
  {
    static char mask = 127;
    
    if (*p < 0)     
      { 
        buf = (*p)&mask; 
        return ++p; 
      }
    buf = *p;
    if (*(++p) < 0) 
      {
        buf += numtype((*p)&mask)<<7;
        return ++p;
      }
    buf += numtype(*p)<<7;
    if (*(++p) < 0) 
      {
        buf += numtype((*p)&mask)<<14;
        return ++p;
      }
    buf += numtype(*p)<<14;
    if (*(++p) < 0) 
      {
        buf += numtype((*p)&mask)<<21;
        return ++p;
      }
    buf += numtype(*p)<<21;
    if (*(++p) < 0) 
      {
        buf += numtype((*p)&mask)<<28;
        return ++p;
      }
    buf += numtype(*p)<<28;
    if (*(++p) < 0) 
      {
        buf += numtype((*p)&mask)<<35;
        return ++p;
      }
    buf += numtype(*p)<<35;
    if (*(++p) < 0) 
      {
        buf += numtype((*p)&mask)<<42;
        return ++p;
      }
    buf += numtype(*p)<<42;
    if (*(++p) < 0) 
      {
        buf += numtype((*p)&mask)<<49;
        return ++p;
      }
    buf += numtype(*p)<<49;
    if (*(++p) < 0) 
      {
        buf += numtype((*p)&mask)<<56;
        return ++p;
      }
    buf += numtype(*p)<<56;
    if (*(++p) < 0) 
      {
        buf += numtype((*p)&mask)<<63;
        return ++p;
      }
    buf += numtype(*p)<<63;
    return ++p;
  }
#endif


} // end namespace ugdiss
#endif
