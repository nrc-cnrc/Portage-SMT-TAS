#ifndef __ug_tsa_array_entry_h
#define __ug_tsa_array_entry_h

#include "ug_ttrack_position.h"

// (c) 2007-2010 Ulrich Germann
// implementation of stuff related to ArrayEntries
// this file should only be included via ug_tsa_base.h, 
// never by itself

namespace ugdiss 
{
  namespace tsa
  {
    // template<typename TOKEN>
    class
    // TSA<TOKEN>::
    ArrayEntry : public ttrack::Position
    {
    public:
      char const* pos;
      char const* next;
      ArrayEntry();

      ArrayEntry(char const* p);
      
      template<typename TSA_TYPE>
      ArrayEntry(TSA_TYPE const* S, char const* p);

      // TOKEN const* suffixStart(TSA<TOKEN> const* S) const;
      // TOKEN const* suffixEnd(TSA<TOKEN> const* S) const;
    };

    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------

    template<typename TSA_TYPE>
    ArrayEntry::
    ArrayEntry(TSA_TYPE const* S, char const* p)
    {
      S->readEntry(p,*this);
    }

#if 0
    //---------------------------------------------------------------------------

    template<typename TOKEN>
    TOKEN const*
    TSA<TOKEN>::
    ArrayEntry::
    suffixStart(TSA<TOKEN> const* S) const
    {
      if (!pos) return NULL;
      return S->corpus->sntStart(sid)+offset;
    }

    //---------------------------------------------------------------------------

    template<typename TOKEN>
    TOKEN const*
    TSA<TOKEN>::
    ArrayEntry::
    suffixEnd(TSA<TOKEN> const* S) const
    {
      if (!pos) return NULL;
      return S->corpus->sntEnd(sid);
    }
#endif
  } // end of namespace tsa
} // end of namespace ugdiss
#endif
