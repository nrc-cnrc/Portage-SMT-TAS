#ifndef __ug_corpus_token_h
#define __ug_corpus_token_h

// This file defines a few simple token classes for use with the Ttrack/TSA template classes
// - SimpleWordId is a simple wrapper around an integer ID
// - L2R_Token defines next() for building suffix arrays
// - R2L_Token defines next() for building prefix arrays

// This code is part of the re-factorization of the earlier non-template implementation of "corpus tracks"
// and suffix and prefix arrays over them as template classes.
// (c) 2007-2009 Ulrich Germann

#include "tpt_typedefs.h"
#include "ug_ttrack_base.h"

namespace ugdiss
{
  /** Simple wrapper around id_type for use with the Ttrack/TSA template classes */
  class SimpleWordId
  {
    id_type theID;
  public:
    /// Default constructor
    SimpleWordId() : theID(0) {}
    /// Constructor from an id_type.
    SimpleWordId(id_type id) : theID(id) {}
    /// Cast operator so we can use a SimpleWordId as if it was a id_type
    operator id_type () const { return theID; }
    /// Assignment operator so we can use a SimpleWordId as if it was a id_type
    SimpleWordId& operator=(id_type id) { theID = id; return *this; }
    /// Explicitly ask for the ID.
    id_type id() const { return theID; }
    /// three way comparison operator
    int cmp(SimpleWordId other) const;
  };


  /** Token class for suffix arrays */
  template<typename T>
  class
  L2R_Token : public T
  {
  public:
    typedef T Token;

    L2R_Token() : T() {};
    L2R_Token(id_type id) : T(id) {};

    L2R_Token const* next() const { return this+1; }

    /** return a pointer to the end of a sentence; used as a stopping criterion during 
     *  comparison of suffixes; see Ttrack::cmp() */
    template<typename TTRACK_TYPE>
    L2R_Token const* stop(TTRACK_TYPE const& C, id_type sid) const 
    { 
      return reinterpret_cast<L2R_Token<T> const*>(C.sntEnd(sid)); 
    }

    bool operator<(T const& other)  const { return this->cmp(other)  < 0; }
    bool operator>(T const& other)  const { return this->cmp(other)  > 0; }
    bool operator==(T const& other) const { return this->cmp(other) == 0; }
    bool operator!=(T const& other) const { return this->cmp(other) != 0; }
  };

  /** Token class for prefix arrays */
  template<typename T>
  class
  R2L_Token : public T
  {
  public:
    typedef T Token;
    R2L_Token const* next() const { return this-1; }

    template<typename TTRACK_TYPE>
    R2L_Token const* stop(TTRACK_TYPE const& C, id_type sid) const 
    { 
      return reinterpret_cast<R2L_Token<T> const*>(C.sntStart(sid)-1); 
    }

    bool operator<(T const& other)  const { return this->cmp(other)  < 0; }
    bool operator>(T const& other)  const { return this->cmp(other)  > 0; }
    bool operator==(T const& other) const { return this->cmp(other) == 0; }
    bool operator!=(T const& other) const { return this->cmp(other) != 0; }
  };

}
#endif
