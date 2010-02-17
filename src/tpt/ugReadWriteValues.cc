// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.

#include "ugReadWriteValues.h"
#include "tpt_pickler.h"

namespace ugdiss
{
  using namespace std;

  template<>
  void
  GenericValueWriter<count_type>::
  operator()(ostream& out, count_type const& value) const
  {
    binwrite(out,value);
  }

  template<>
  void
  GenericValueWriter<uint64_t>::
  operator()(ostream& out, uint64_t const& value) const
  {
    binwrite(out,value);
  }

  template<>
  bool
  GenericValueWriter<uint32_t>::
  operator()(filepos_type& out, uint32_t const& value) const
  {
    out = value;
    return 1; 
  }

  template<>
  void
  GenericValueWriter<char>::
  operator()(ostream& out, char const& value) const
  {
    out.put(value);
  }

#if 0
  template<>
  bool
  GenericValueWriter<count_type>::
  operator()(filepos_type& dest, count_type const& value) const
  {
    dest = value;
    return true;
  }
  
  template<>
  bool
  GenericValueWriter<char>::
  operator()(filepos_type& dest, char const& value) const
  {
    dest = value;
    return true;
  }
  
  template<>
  filepos_type 
  GenericValueWriter<count_type>::
  operator()(count_type const& value) const
  { 
    return value; 
  }

#endif

  template<>
  void
  GenericValueReader<char>::
  operator()(istream& in, char& value) const
  {
    value = in.get();
  }

  template<>
  void
  GenericValueReader<count_type>::
  operator()(istream& in, count_type& value) const
  {
    binread(in,value);
  }

  template<>
  char const*
  GenericValueReader<count_type>::
  operator()(char const* p, count_type& value) const
  {
    return binread(p,value);
  }

  template<>
  void
  GenericValueReader<uint64_t>::
  operator()(istream& in, uint64_t& value) const
  {
    binread(in,value);
  }

  template<>
  char const*
  GenericValueReader<uint64_t>::
  operator()(char const* p, uint64_t& value) const
  {
    return binread(p,value);
  }

#if 0
#if IS_64_BIT
  template<>
  void
  GenericValueReader<filepos_type>::
  operator()(istream& in, filepos_type& value) const
  {
    binread(in,value);
  }
#endif
#endif
}
  
