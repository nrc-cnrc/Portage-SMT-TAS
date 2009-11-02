/**
 * @author Eric Joanis and Evan Stratford
 * @file binio.h Specializations to read/write maps in binary format
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef PORTAGE_BINIO_MAPS_H
#define PORTAGE_BINIO_MAPS_H

#include "portage_defs.h"
#include "binio.h"
#include <map>
#include <tr1/unordered_map>

namespace Portage {

namespace BinIO {

   /// Specialization: read and write std::map in binary mode.
   template <typename K, typename V>
   struct Impl<map<K, V> > {
      static void writebin_impl(ostream& os, const map<K, V>& m) {
         Uint n(m.size());
         writebin(os, n);
         typename map<K, V>::const_iterator iter;
         for (iter = m.begin(); iter != m.end(); ++iter) {
            writebin(os, iter->first);
            writebin(os, iter->second);
         }
      }
      static void readbin_impl(istream& is, map<K, V>& m) {
         Uint n;
         readbin(is, n);
         m.clear();
         for (Uint i = 0; i < n; i++) {
            K k; readbin(is, k);
            readbin(is, m[k]);
         }
      }
   };

   /// Specialization: read and write tr1::unordered_map in binary mode.
   template <typename K, typename V> 
   struct Impl<tr1::unordered_map<K, V> > {
      static void writebin_impl(ostream& os, const std::tr1::unordered_map<K, V>& m) {
         Uint n(m.size());
         writebin(os, n);
         typename tr1::unordered_map<K, V>::const_iterator iter;
         for (iter = m.begin(); iter != m.end(); ++iter) {
            writebin(os, iter->first);
            writebin(os, iter->second);
         }
      }
      static void readbin_impl(istream& is, std::tr1::unordered_map<K, V>& m) {
         Uint n;
         readbin(is, n);
         m.clear();
         m.rehash(n);
         for (Uint i = 0; i < n; i++) {
            K k; readbin(is, k);
            readbin(is, m[k]);
         }
      }
   };

} // BinIO

} // Portage

#endif // PORTAGE_BINIO_MAPS_H
