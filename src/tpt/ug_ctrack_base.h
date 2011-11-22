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



// Base class for corpus tracks. mmCtrack (memory-mapped Ctrack) and imCtrack (in-memory Ctrack)
// are derived from this class.

// (c) Ulrich Germann. All rights reserved.

#ifndef __ug_ctrack_base
#define __ug_ctrack_base

#include <string>

#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
#include "ug_vocab.h"

namespace ugdiss
{
  using namespace std;

  class Ctrack
  {
  protected:
    id_type numSent;
    id_type numWords;

  public:

    /** @return a pointer to beginning of sentence /sid/ */
    virtual 
    id_type const* 
    sntStart(size_t sid) const = 0; 

    /** @return end point of sentence /sid/ */
    virtual 
    id_type const* 
    sntEnd(size_t sid) const = 0; 

    /** @return length of sentence /sid/ */
    size_t sntLen(size_t sid) const;

    /** @return size of corpus in number of sentences */
    virtual size_t size() const = 0;      

    /** @return size of corpus in number of words/tokens */
    virtual size_t numTokens() const = 0; 

    /** @return string representation of sentence /sid/ */
    string str(id_type sid, TokenIndex const& T) const;
    string str(id_type sid, Vocab const& V) const;

    /** check that a sentence id is valid */
    void check_sid(id_type sid) const
    {
      if (sid >= size())
        cerr << efatal << "Encountered bad sentence id:" << sid
             << ". Expected < corpus size of " << size() << "."
             << exit_1;
    }

    /** Destructor should be virtual in abstract base class */
    virtual ~Ctrack() {}
  };
  

// #if 0
//   /** Functor that returns true if the key is greater than or equal to the word sequence
//    *  (suffix) that starts at the comparison point. Used for determining the lower bound
//    *  of a token range.
//    */
//   class
//   Ctrack::
//   sBelowRange
//   {
//   private:
//     Ctrack const& C;
//   public:
//     sBelowRange(Ctrack const& ct);
//     sBelowRange(Ctrack const* ct);
//     bool operator()(pair<id_type,uint16_t> const& pos, 
// 		    pair<id_type const*,id_type const*> const& key) const;
      
//     bool operator()(pair<id_type const*,id_type const*> const& key,
// 		    pair<id_type,uint16_t> const& pos) const;
//   };

//   /** Functor that returns true if the key is less than the word sequence (suffix) that 
//    *  starts at the comparison point. Used for determining the upper bound of a token 
//    *  range. 
//    */
//   class
//   Ctrack::
//   sAboveRange
//   {
//   private:
//     Ctrack const& C;
//   public:
//     sAboveRange(Ctrack const& ct);
//     sAboveRange(Ctrack const* ct);
//     bool operator()(pair<id_type,uint16_t> const& pos, 
// 		    pair<id_type const*,id_type const*> const& key) const;
      
//     bool operator()(pair<id_type const*,id_type const*> const& key,
// 		    pair<id_type,uint16_t> const& pos) const;
//   };

//   /** Functor that returns true if the key is greater than or equal to the word sequence
//    *  (prefix) that ends at the comparison point. Used for determining the lower bound
//    *  of a token range.
//    */
//   class
//   Ctrack::
//   pBelowRange
//   {
//   private:
//     Ctrack const& C;
//   public:
//     pBelowRange(Ctrack const& ct);
//     pBelowRange(Ctrack const* ct);
//     bool operator()(pair<id_type,uint16_t> const& pos, 
// 		    pair<id_type const*,id_type const*> const& key) const;
      
//     bool operator()(pair<id_type const*,id_type const*> const& key,
// 		    pair<id_type,uint16_t> const& pos) const;
//   };

//   /** Functor that returns true if the key is greater than or equal to the word sequence
//    *  (prefix) that ends at the comparison point. Used for determining the lower bound
//    *  of a token range.
//    */
//   class
//   Ctrack::
//   pAboveRange
//   {
//   private:
//     Ctrack const& C;
//   public:
//     pAboveRange(Ctrack const& ct);
//     pAboveRange(Ctrack const* ct);
//     bool operator()(pair<id_type,uint16_t> const& pos, 
// 		    pair<id_type const*,id_type const*> const& key) const;
      
//     bool operator()(pair<id_type const*,id_type const*> const& key,
// 		    pair<id_type,uint16_t> const& pos) const;
//   };
// #endif
}
#endif
