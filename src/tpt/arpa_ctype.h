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
/* @author Samuel Larkin
 * @file arpa_ctype.h
 * @brief Facet that doesn't treat \v as a space delimiter.
 *
 */

#ifndef __ARPA_CTYPE_H__
#define __ARPA_CTYPE_H__

#include <locale>

/**
 * A facet that redefines space mark as been ' ' \t \l \r \n and excludes from
 * space the vertical tab \t.
 * Here's how to use it:
 * // Arpa format says that a record is space or tab separated.
 * static std::locale arpa_space(std::locale::classic(), new arpa_ctype);
 * istream.imbue(arpa_space);
 */
class arpa_ctype : public std::ctype<char> {
#ifndef Darwin
    mask my_table[UCHAR_MAX+1];
public:
    arpa_ctype(size_t refs = 0)  
        : std::ctype<char>(my_table, false, refs) 
    {
        // copy the original character classifaction table.
        std::copy(classic_table(), 
                  classic_table() + table_size, 
                  my_table);
#if 0        
        cerr << table_size << endl;
        cerr << space << endl;
        for (int i(0); i<UCHAR_MAX+1; ++i)
           cerr << i << ":" << my_table[i] << endl;
#endif        
        // A vertical tab is not considered a space in arpa.
        my_table['\v'] &= ~((mask)space);
#if 0        
        for (int i(0); i<UCHAR_MAX+1; ++i) {
           cerr << i << ":" << my_table[i] << endl;
           if (is(space, i)) cerr << "SPACE: " << i << endl;
        }
#endif        
    }

#else
public:
    bool is(mask m, char c) const {
      if((m == space) && (c == '\v'))
        return false;
      return std::ctype<char>::is(m, c);
    }
#endif // !Darwin
};

#endif  // __ARPA_CTYPE_H__
