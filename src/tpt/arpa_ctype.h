// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.
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
};

#endif  // __ARPA_CTYPE_H__
