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

#include "tpt_pickler.h"
#include <cassert>
using namespace std;
using namespace ugdiss;

#ifdef CYGWIN
#define stat64  stat
#endif

namespace ugdiss
{
// template function for writing unsigned integers (short, long, long
// long)
  template <typename T>
  size_t
  binwrite_unsigned_integer(std::ostream& out, T data)
  {
    size_t cnt = 0;
    char c;
#ifdef LOG_WRITE_ACTIVITY
    size_t bytes_written = 1;
    cerr << "binwrite " << data;
#endif
    while (data >= 128)
      {
#ifdef LOG_WRITE_ACTIVITY
	bytes_written++;
#endif
	out.put(data%128);
	++cnt;
	data = data >> 7;
      }
    c = data;
    out.put(c|char(-128)); // set the 'stop' bit
    ++cnt;
#ifdef LOG_WRITE_ACTIVITY
    cerr << " in " << bytes_written << " bytes" << std::endl;
#endif
    return cnt;
  }

  template<typename T>
  void
  binread_unsigned_integer(std::istream& in, T& data)
  {
    char c, mask=127;
    in.clear();
    in.get(c);
    data = c&mask;

    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 14;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 21;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 28;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 35;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 42;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 49;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 56;
    if (c < 0) return;
    in.get(c);
    data += T(c&mask) << 63;
  }

  size_t
  binwrite(std::ostream& out, unsigned char data) 
  { 
    return binwrite_unsigned_integer(out, data);
  }

  size_t
  binwrite(std::ostream& out, unsigned short data)
  { 
    return binwrite_unsigned_integer(out, data);
  }

  size_t
  binwrite(std::ostream& out, unsigned long data)
  { 
    return binwrite_unsigned_integer(out, data);
  }

  size_t
  binwrite(std::ostream& out, unsigned long long data)
  { 
    return binwrite_unsigned_integer(out, data);
  }

#if __WORDSIZE == 64 || defined(Darwin)
  size_t
  binwrite(std::ostream& out, unsigned int data)
  { 
    return binwrite_unsigned_integer(out, data);
  }
#else 
  void 
  binwrite(std::ostream& out, size_t data)
  { 
    binwrite_unsigned_integer(out, data);
  }
#endif

  void 
  binread(std::istream& in, unsigned short& data)
  {
    assert(sizeof(data)==2);
    char c, mask=127;
    in.clear();
    in.get(c);
    data = c&mask;
    if (c < 0) return;
    in.get(c);
    data += uint16_t(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += uint16_t(c&mask) << 14;
  }

  void 
  binread(std::istream& in, unsigned int& data)
  {
    assert(sizeof(data) == 4);
    char c, mask=127;
    in.clear();
    in.get(c);
    data = c&mask;
    if (c < 0) return;
    in.get(c);
    data += uint32_t(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += uint32_t(c&mask) << 14;
    if (c < 0) return;
    in.get(c);
    data += uint32_t(c&mask) << 21;
    if (c < 0) return;
    in.get(c);
    data += uint32_t(c&mask) << 28;
  }

  void 
  binread(std::istream& in, unsigned long& data)
  {
#if __WORDSIZE == 32
    assert(sizeof(unsigned long)==4);
#else
    assert(sizeof(unsigned long)==8);
#endif
    char c, mask=127;
//     if (!in.get(c))
//       {
// 	std::cerr << "read error at position " << in.tellg() << std::endl;
// 	assert(false);
//       }
    in.get(c);
    data = c&mask;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 14;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 21;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 28;
#if __WORDSIZE == 64
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 35;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 42;
    if (c < 0) return;
    in.get(c);

    data += static_cast<unsigned long long>(c&mask) << 49;
    if (c < 0) return;
    in.get(c);
    
    data += static_cast<unsigned long long>(c&mask) << 56;
    if (c < 0) return;
    in.get(c);
    
    data += static_cast<unsigned long long>(c&mask) << 63;
#endif
  }

  void 
  binread(std::istream& in, unsigned long long& data)
  {
    assert(sizeof(unsigned long long)==8);
    char c, mask=127;
    in.get(c);
    data = c&mask;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 7;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 14;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 21;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 28;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 35;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 42;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 49;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 56;
    if (c < 0) return;
    in.get(c);
    data += static_cast<unsigned long long>(c&mask) << 63;
  }

  // writing and reading strings ...
  size_t
  binwrite(std::ostream& out, std::string const& s)
  {
    size_t len = s.size();
    size_t cnt = ugdiss::binwrite(out,len);
    out.write(s.c_str(),len);
    return cnt+len;
  }
  
  void
  binread(std::istream& in, std::string& s)
  {
    size_t len;
    ugdiss::binread(in,len);
    if (!in) return;
    char buf[len+1];
    in.read(buf,len);
    buf[len] = 0;
    s = buf;
  }
  
//   std::ostream& 
//   write(std::ostream& out, char x) { out.write(&x,sizeof(x)); return out; }
  
//   std::ostream& 
//   write(std::ostream& out, unsigned char x) 
//   { out.write(reinterpret_cast<char*>(&x),sizeof(x)); return out; }

//   std::ostream& 
//   write(std::ostream& out, unsigned short x)
//   { x = htons(x); out.write(reinterpret_cast<char*>(&x),sizeof(x)); return out; }

//   std::ostream&
//   write(std::ostream& out, short x)
//   { x = htons(x); out.write(reinterpret_cast<char*>(&x),sizeof(x)); return out; }
  
//   std::ostream&
//   write(std::ostream& out, long x)
//   { x = htonl(x); out.write(reinterpret_cast<char*>(&x),sizeof(x)); return out; }

//   std::ostream&
//   write(std::ostream& out, size_t x)
//   { x = htonl(x); out.write(reinterpret_cast<char*>(&x),sizeof(x)); return out; }

//   std::ostream&
//   write(std::ostream& out, float x)
//   { 
//     assert(sizeof(float) == sizeof(size_t));
//     size_t y = htonl(*(reinterpret_cast<size_t*>(&x))); 
//     out.write(reinterpret_cast<char*>(&y),sizeof(y)); 
//     return out; 
//   }

//   std::istream&
//   read(std::istream& in,char& x)
//   { in.read(&x,sizeof(x)); return in; }

//   std::istream&
//   read(std::istream& in,unsigned char& x)
//   { in.read(reinterpret_cast<char*>(&x),sizeof(char)); return in; }

//   std::istream&
//   read(std::istream& in,short& x)
//   { in.read(reinterpret_cast<char*>(&x),sizeof(short)); x = ntohs(x); return in; }

//   std::istream&
//   read(std::istream& in,unsigned short& x)
//   { in.read(reinterpret_cast<char*>(&x),sizeof(short)); x = ntohs(x); return in; }

//   std::istream&
//   read(std::istream& in,long& x)
//   { in.read(reinterpret_cast<char*>(&x),sizeof(long)); x = ntohl(x); return in; }

//   std::istream&
//   read(std::istream& in,size_t& x)
//   { in.read(reinterpret_cast<char*>(&x),sizeof(size_t)); x = ntohl(x); return in; }

//   std::istream&
//   read(std::istream& in,float& x)
//   { 
//     assert(sizeof(size_t) == sizeof(float));
//     size_t y = ntohl(read<size_t>(in));
//     x = *(reinterpret_cast<float*>(&y)); 
//     return in; 
//   }

  size_t
  binwrite(std::ostream& out, float x)
  { 
    // IMPORTANT: this is not robust against the big/little endian 
    // issue. 
    out.write(reinterpret_cast<char*>(&x),sizeof(float));
    return sizeof(float);
  }
  
  void
  binread(std::istream& in, float& x)
  { 
    // IMPORTANT: this is not robust against the big/little endian 
    // issue. 
    in.read(reinterpret_cast<char*>(&x),sizeof(x)); 
  }
  

  template<>
  char const* 
  binread<uint32_t>(char const* p, uint32_t& buf)
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
        buf += uint32_t((*p)&mask)<<7;
        return ++p;
      }
    buf += uint32_t(*p)<<7;
    if (*(++p) < 0) 
      {
        buf += uint32_t((*p)&mask)<<14;
        return ++p;
      }
    buf += uint32_t(*p)<<14;
    if (*(++p) < 0) 
      {
        buf += uint32_t((*p)&mask)<<21;
        return ++p;
      }
    buf += uint32_t(*p)<<21;
    assert(*(++p) < 0);
    buf += uint32_t((*p)&mask)<<28;
    return ++p;
  }

  template<>
  char const* 
  binread<filepos_type>(char const* p, filepos_type& buf)
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
        buf += filepos_type((*p)&mask)<<7;
        return ++p;
      }
    buf += filepos_type(*p)<<7;
    if (*(++p) < 0) 
      {
        buf += filepos_type((*p)&mask)<<14;
        return ++p;
      }
    buf += filepos_type(*p)<<14;
    if (*(++p) < 0) 
      {
        buf += filepos_type((*p)&mask)<<21;
        return ++p;
      }
    buf += filepos_type(*p)<<21;
    if (*(++p) < 0) 
      {
        buf += filepos_type((*p)&mask)<<28;
        return ++p;
      }
    buf += filepos_type(*p)<<28;
    if (*(++p) < 0) 
      {
        buf += filepos_type((*p)&mask)<<35;
        return ++p;
      }
    buf += filepos_type(*p)<<35;
    if (*(++p) < 0) 
      {
        buf += filepos_type((*p)&mask)<<42;
        return ++p;
      }
    buf += filepos_type(*p)<<42;
    if (*(++p) < 0) 
      {
        buf += filepos_type((*p)&mask)<<49;
        return ++p;
      }
    buf += filepos_type(*p)<<49;
    if (*(++p) < 0) 
      {
        buf += filepos_type((*p)&mask)<<56;
        return ++p;
      }
    buf += filepos_type(*p)<<56;
    assert(*(++p) < 0);
    buf += filepos_type((*p)&mask)<<63;
    return ++p;
  }

  template<>
  char const* 
  binread<float>(char const* p, float& buf)
  {
    buf = *reinterpret_cast<float const*>(p);
    return p+sizeof(float);
  }

} // end namespace ugdiss
