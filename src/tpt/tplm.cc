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
#include "tplm.h"
#include "tpt_pickler.h"

namespace Portage
{
  using namespace ugdiss;
  using namespace std;

  /** auxiliary function for use in qlm_tightfind() 
   *  seek to the midpoint in range [start,stop)
   *  @param in data stream
   *  @param start start of the search range
   *  @param stop of the search range
   *  @return true if everything went as planned, false on error
   */
  bool 
  qlm_find_midpoint(std::istream& in, 
                    std::ios::pos_type start, 
                    std::ios::pos_type stop)
  {
    typedef unsigned char uchar;
    in.seekg((start+stop)/2); 
    uchar x = in.get(); x &= 128;
    while (in.good() && in.tellg() < stop && (x == (uchar(in.peek())&128))) 
      in.get();
    return !(in.eof() || in.bad() || in.tellg() == stop);
  }
  
  /** auxiliary function for use in qlm_tightfind() 
   *  search linearly for key in range [start,stop), assign associated value 
   *  to val
   */
  bool 
  qlm_linear_search(std::istream& in, 
                    filepos_type start, 
                    std::ios::pos_type stop, 
                    uint64_t key, 
                    id_type& val)
  { 
    in.seekg(start);
    uint64_t foo = tightread(in,stop);
    while (in.tellg() < stop && (foo>>8) < key) // skip all smaller values
      foo = tightread(in,stop);
    if ((foo>>8) != key)
      return false; // not found
    val = foo%256;
    return true;
  }

  /** auxiliary function for use in qlm_tightfind() 
   *  search for key in range [start,stop), assign associated value to val
   *  @param in data stream to search in
   *  @param start start of search range in /in/
   *  @param stop  end if search range
   *  @param key   value to search for
   *  @param val   where to store associated value
   *  @return true if the value is found
   */
  bool
  qlm_tightfind(std::istream&   in, 
                filepos_type start, 
                filepos_type  stop, 
                uint64_t       key, 
                id_type&       val)
  { 
    if (start==stop) return false;
    if (start+1==stop) return false;
    assert(stop>start);
    
    static unsigned int const granularity = sizeof(filepos_type)*5; 
    // granularity: point where we should switch to linear search,
    // because otherwise we might skip over the entry we are looking for
    // because we land right in the middle of it.
    
    // cout << "start=" << start << endl;
    // cout << "stop=" << stop << endl;

    if (stop > start + granularity)
      if (!qlm_find_midpoint(in,start,stop)) 
	return false; // something went wrong (empty index?)
    
    if (stop <= start + granularity || in.tellg() == std::ios::pos_type(stop))
      { // If the search range is very short, find_midpoint might skip the
	// entry we are loking for. In this case, we can afford a linear
	// search
	return qlm_linear_search(in,start,stop,key,val);
      }
    
    // perform binary search
    filepos_type curpos = in.tellg();  // find_midpoint got us here
    
    uint64_t foo = tightread(in,stop); // read the value at the midpoint
    uint64_t tmpid = foo>>8;

    // cout << "key at " << curpos << " is " << tmpid << endl;
    if (tmpid == key) 
      {
	val  = foo%256; 
	return true; // done, found 
      }
    else if (tmpid > key) 
      { // look in the lower half
	return qlm_tightfind(in,start,curpos,key,val);
      }
    else
      { // look in the upper half
	if (in.rdbuf()->in_avail() == 0 
            || in.tellg() == std::ios::pos_type(stop))
	  return false;
	return qlm_tightfind(in,in.tellg(),stop,key,val);
      }
  }
  
#if 0
  /** this specialization is for IRSTLM-style quantized LMs that have exactly one byte
   *  to encode value indices */
  template<> 
  bool
  LMtpt<uchar>::
  Entry::
  find_pidx(id_type key, id_type& val) const
  {
    return qlm_tightfind(*file,startPIdx,stopPIdx,key,val);
  }
#endif

  /** this specialization is for LMs that have allow value indices of up to 2^^28 */
  template<> 
  bool
  LMtpt<id_type>::
  Entry::
  find_pidx(id_type key, id_type& val) const
  {

#if 0
    file->seekg(startPIdx);
    while (file->tellg() < std::ios::pos_type(stopPIdx))
      {
        id_type wid = tightread(*file,stopPIdx);
        id_type pid = tightread(*file,stopPIdx);
        cout << setw(12) << wid << " " << setw(12) << pid << endl;
      }
#endif

#if 0
    uint32_t x,y;
    for (char const* q = startPIdx; q != stopPIdx;)
      {
        q = tightread4(q,stopPIdx,x);
        q = tightread4(q,stopPIdx,y);
        cout << "x = " << x << " y = " << y << endl;
      }
#endif 

    char const* p = tightfind_noflags(startPIdx,stopPIdx,key);
    if (!p)
      return false;
    tightread(p,stopPIdx,val);
    return true;
//     if (!tightfind_noflags(*file,startPIdx,stopPIdx,key))
//       return false;
//     val = tightread(*file,stopPIdx); // binread(*file,val);
//     return true;
  }
  
#if 0
  template<>
  void
  LMtpt<uchar>::
  Reader::
  operator()(istream& in, LMtpt<uchar>::Entry& v) const
  {
    v.file = &in;
    in.read(reinterpret_cast<char*>(&(v.bo_idx)),1);
    size_t foo;
    binread(in,foo);
    v.startPIdx = in.tellg();
    v.stopPIdx  = v.startPIdx+foo;
    in.seekg(v.stopPIdx); // relevant for endData in RoTrieNode()!
  }
#endif

#if 0
  template<>
  void
  LMtpt<id_type>::
  Reader::
  operator()(istream& in, LMtpt<id_type>::Entry& v) const
  {
    v.file = &in;
    binread(in,v.bo_idx);
    size_t foo;
    binread(in,foo);
    v.startPIdx = in.tellg();
    v.stopPIdx  = v.startPIdx+foo;
    in.seekg(v.stopPIdx); // relevant for endData in RoTrieNode()!
  }
#endif

  template<>
  char const*
  LMtpt<id_type>::
  Reader::
  operator()(char const* p, LMtpt<id_type>::Entry& v) const
  {
    // v.file = &in;
    p = binread(p,v.bo_idx);
    uint64_t foo;
    p = binread(p,foo);
    v.startPIdx = p;
    v.stopPIdx  = v.startPIdx+foo;
    return v.stopPIdx;
  }
}
