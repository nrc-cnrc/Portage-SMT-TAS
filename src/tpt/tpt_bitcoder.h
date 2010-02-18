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
#ifndef __bitCoder_hh
#define __bitCoder_hh

#include <vector>
#include <cassert>
#include <byteswap.h>
#include <string>
#include <endian.h>
#include "bitwise.h"
#include "tpt_typedefs.h"

namespace ugdiss 
{
  using namespace std;

  /** Class to write numbers in an extending code with an arbitrary sequence
   *  of bit blocks to a string and read from a char* pointer
   */
  template<typename T>
  class BitCoder
  {
    vector<T> threshold;  /** for determining how many blocks are needed */
    vector<T> modMask;    /** used in breaking numbers into blocks and
			   *  setting/reading the stop flag */
    vector<uint32_t> block; /** stores the block sequence */

  public:
    /** Constructor */
    BitCoder() {};
    BitCoder(vector<uint32_t>  const& b)
    {
      setBlocks(b);
    }

    void setBlocks(vector<uint32_t>  const& b)
    {
      threshold.resize(b.size());
      modMask.resize(b.size());
      block = b;
      T one = 1,th=1;
      for (size_t i = 0; i < b.size(); i++)
        {
          threshold[i] = (th<<=b[i])-1;
          modMask[i] = one<<b[i];
        }
    }

  private:
    /** read a block of bits
     *  @param src     memory address of where to start reading
     *  @param offset  bit offset     of where to start reading
     *  @param numBits number of bits to read
     *  @param dest    destination of the number
     *  @return a pair of new start position (src) and bit offset
     */
    pair<char const*, unsigned char>
    readBits(char const* src, unsigned char offset, size_t numBits, T& dest)
    {
      assert(numBits <= sizeof(T)*8);
      pair<char const*, size_t> retval;
      retval.second = (offset+numBits)%8;    // new offset
      retval.first = src+(offset+numBits)/8; // new "base pointer"
      
#if __BYTE_ORDER==__LITTLE_ENDIAN
      dest = *reinterpret_cast<T const*>(src);
      switch (sizeof(T))
        {
        case 2: dest = bswap_16(dest); break;
        case 4: dest = bswap_32(dest); break;
        case 8: dest = bswap_64(dest); break;
        default: ;
        }
      // cout << "dest=" << bitpattern(dest) << " offset=" << int(offset) << endl;
      dest <<= offset;
      // cout << "dest=" << bitpattern(dest) << " offset=" << int(offset) << endl;
      if (offset+numBits > sizeof(T)*8)
        dest += uchar(*(retval.first)) >> (8-offset);
      dest >>= sizeof(T)*8 - numBits;
#else
      assert(0); 
      // not yet implemented; 
      // probably all we need is to skip the byte swapping
#endif
      // cout << "dest(2)=" << bitpattern(dest) << endl;
      return retval;
    }
    
    /** write bit block to a string
     *  @param dest where to write
     *  @param offset bit offset on the last byte of the string
     *  @param x stores the bit block to write (flush right)
     *  @param numBits how many bits to store 
     *         (from the right edge, written left to right)
     *  @return new offset
     */ 
    size_t 
    writeBits(string& dest, size_t offset, T x, size_t numBits)
    {
      assert(numBits);
#if __BYTE_ORDER==__LITTLE_ENDIAN
      // size_t btw = (numBits-1)/8; // bytes to write - 1
      unsigned char* p    = reinterpret_cast<unsigned char*>(&x)+(numBits-1)/8; 
      unsigned char* stop = reinterpret_cast<unsigned char*>(&x); // that's the little end
      if (numBits%8)
        x <<= 8-(numBits%8);
#if 0
      cout << "before:  " << bitpattern(dest,offset) << " (" 
           << p-stop+1 << " bytes to write)" << endl;
      cout << "x=       " << bitpattern(x) << " (" 
           << numBits << " bits to write)" << endl;
      cout << "offset=" << offset << endl;
#endif
      for(;p>=stop;p--) 
        {
          if (offset)
            {
              assert(dest.size());
              *dest.rbegin() += (*p>>offset);
	      if (p>stop || numBits%8 > 8-offset || numBits%8==0)
		dest += *p<<(8-offset);
#if 0
              cout << "then[1]: " << bitpattern(dest) 
		   << " " << p-stop 
		   << " " << (offset+numBits)%8
		   << endl;
#endif
            }
          else
            {
              dest += *p;
              // cout << "then[2]: "<< bitpattern(dest) << endl;
            }
        } 
      // cout << "final:   " << bitpattern(dest) << endl;
#else
      assert(0); 
      // not yet implemented 
#endif
      // cout << "offset=" << (offset+numBits)%8 << endl;
      return (offset+numBits)%8;
    }
  public:
    /** write a number to a string
     *  @param dest destination string
     *  @param offset bit offset on the last char of that string
     *  @param x the number to write
     *  @return the new bit offset
     */
    size_t 
    writeNumber(string& dest, size_t offset, T x) 
    {
      size_t lastBlock=0;
      while (x >= threshold[lastBlock]) 
      {
        // cout << x << " " << threshold[lastBlock] << endl;
        lastBlock++;
        assert(lastBlock < block.size());
      }
      T buf[lastBlock+1];
      for (int i = lastBlock; i > 0;i--)
        {
          buf[i] = x % modMask[i];
#if 0
          cout << (bitpattern(modMask[i])) << " (b[" << i << "]=" << block[i] << ")" << endl;
          cout << (bitpattern(x)) << " [" << x << "]" << endl;
          cout << (bitpattern(buf[i])) << " {" << buf[i] << "}" << endl;
#endif
          x >>= block[i];
        }
      buf[0] = x;
#if 0
      cout << bitpattern(modMask[0]) << " (" << block[0] << ")" << endl;
      cout << bitpattern(x) << " [" << x << "]" << endl;
      cout << bitpattern(buf[0]) << " {" << buf[0] << "}" << endl;
#endif
      size_t i = 0;
      for (; i < lastBlock; i++)
        {
          offset = writeBits(dest,offset,buf[i],block[i]+1);
          // cout << offset << " (offset)" << endl;
        }
      if (i+1 == block.size())
        offset = writeBits(dest,offset,buf[lastBlock],block[lastBlock]);
      else
        {
          buf[lastBlock] |= modMask[lastBlock];
          offset = writeBits(dest,offset,buf[lastBlock],block[lastBlock]+1);
        }
      return offset;
    }

    /** read a number from a memory location
     *  @param p the memory location and the bit offset in that location
     *  @param x variable passed by reference to store the number
     *  @return pair of new src and new offset (after reading the number)
     */
    pair<char const*, unsigned char>
    readNumber(pair<char const*, unsigned char> const p, T& x) 
    {
      assert(block.size());
      // single block encoding:
      if (block.size()==1)
        return readBits(p.first,p.second,block[0],x);
      
      // multiple block encoding
      pair<char const*, unsigned char> r; 
      r = readBits(p.first,p.second,block[0]+1,x);
      T m = modMask[0];
      // cout << "readNum: " << bitpattern(x) << " " << block[0]+1 << " bits" << endl;
      // cout << "m=:      " << bitpattern(m) << endl;
      // cout << r.first-src << " " << r.second << endl;

      if (x&m)
        {
          x %= m;
          return r;
        }
      size_t blast=block.size()-1;
      T buf;
      // cout << "x = " << bitpattern(x) << endl;
      for (size_t i = 1; i < blast; i++)
        {
          size_t b = block[i];
          m = modMask[i];
          x <<= b;
          // cout << "x = " << bitpattern(x) << " <<" << b << endl;
          r = readBits(r.first,r.second,b+1,buf);
          if (buf&m)
            {
              x += buf%m;
              return r;
            }
          x += buf;
        }
      x <<= block[blast];
      // cout << "x = " << bitpattern(x) << " <<" << block[blast] << endl;
      r = readBits(r.first,r.second,block[blast],buf);
      // cout << "b = " << bitpattern(buf) << " pre-final" << endl;
      // cout << "x = " << bitpattern(x) << " pre-final" << endl;
      x += buf;
      // cout << "x = " << bitpattern(x) << " final" << endl;
      return r;
    }

    /** read a number from a memory location
     *  @param src the memory location
     *  @param offset the bit offset in that location
     *  @param x variable passed by reference to store the number
     *  @return pair of new src and new offset (after reading the number)
     */
    pair<char const*, unsigned char>
    readNumber(char const* src, unsigned char offset, T& x)
    {
      return readNumber(pair<char const*, unsigned char>(src,offset),x);
    }
    
  };
}
#endif
