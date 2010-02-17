// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.
#ifndef __ugReadWriteValues_hh
#define __ugReadWriteValues_hh
#include <iostream>
#include <cassert>
#include "tpt_typedefs.h"
#include "tpt_pickler.h"

namespace ugdiss
{
  using namespace std;

  // Pure virtual base class that all ValWriters must be derived from
  template<typename T>
  class
  ValWriterBaseClass
  {
  public:
    virtual void operator()(ostream& out, T const& value) const = 0;

    // returns false if conversion to filepos_type is not supported or failed,
    // true otherwise
    virtual bool operator()(filepos_type& dest, T const& value) const = 0;
  };

  template<typename T>
  class
  ValReaderBaseClass
  {
  public:
    virtual void operator()(istream& in, T& value) const = 0;
    virtual char const* operator()(char const* p, T& value) const = 0;
  };

  // Basic Writer and Reader for count types:
  template<typename T>
  class
  GenericValueWriter : public ValWriterBaseClass<T>
  {
  public:
    void operator()(ostream& out, T const& value) const;
    bool operator()(filepos_type& dest, T const& value) const;
    filepos_type operator()(T const& value) const;
  };

  template<typename T>
  void
  GenericValueWriter<T>::
  operator()(ostream& out, T const& value) const
  {
    out.write(reinterpret_cast<char const*>(&value),sizeof(T));
  }

  template<typename T>
  class
  GenericValueReader : public ValReaderBaseClass<T>
  {
  public:
    void operator()(istream& in, T& value) const;
    char const* operator()(char const* p, T& value) const;
  };


//   template<>
//   bool
//   GenericValueWriter<count_type>::
//   operator()(filepos_type& dest, count_type const& value) const;


#if 0
  template<>
  class
  GenericValueWriter<count_type> : public ValWriterBaseClass<count_type>
  {
  public:
    void operator()(ostream& out, count_type const& value) const;
    bool operator()(filepos_type& dest, count_type const& value) const;
    filepos_type operator()(count_type const& value) const;
  };
#endif

  // template<>
  // void
  // GenericValueWriter<count_type>::
  // operator()(ostream& out, count_type const& value) const;

}

#endif
