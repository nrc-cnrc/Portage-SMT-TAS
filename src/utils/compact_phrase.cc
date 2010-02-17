/**
 * @author Eric Joanis
 * @file compact_phrase.cc CompactPhrase implementation
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "portage_defs.h"
#include "compact_phrase.h"
#include <iostream>
#include <algorithm>

using namespace Portage;

#ifdef COMPACT_PHRASE_STATS
   static Uint shift_count(0);
   static vector<Uint> ref_count_stats(257,0);
   static vector<Uint> byte_count_stats(5,0);
   #define TALLY_REFS (++ref_count_stats[static_cast<unsigned char>(*data)])
   #define TALLY_SHIFTS (++shift_count)
#else
   #define TALLY_REFS
   #define TALLY_SHIFTS
#endif

ArrayMemPool<char, CompactPhrase::PoolMaxArraySize, 4096>
   CompactPhrase::mempool;

void CompactPhrase::alloc_data(Uint size) {
   if ( size <= PoolMaxArraySize ) {
      // For small objects, we have to have their size to free again with
      // mempool, so we store the allocated size in the first byte and set data
      // to the next one.
      Uint alloc_size;
      char* buffer = mempool.alloc_array(size+1, &alloc_size);
      *buffer = alloc_size;
      data = buffer + 1;
   } else {
      // For larger object, store 0xff as the size to mean it's actually be
      // allocated using new[] and should be deleted with delete [].
      char* buffer = new char[size+1];
      *buffer = 0xff;
      data = buffer + 1;
   }
}

void CompactPhrase::free_data() {
   char* buffer = data - 1;
   if ( *buffer == -1 ) {
      delete [] buffer;
   } else {
      Uint size = static_cast<unsigned char>(*buffer);
      mempool.free_array(buffer, size);
   }
   data = NULL;
}

CompactPhrase::CompactPhrase(const char* packed)
   : data(NULL)
{
   DBG("c3");
   alloc_data(strlen(packed)+2);
   *data = 1;
   TALLY_REFS;
   strcpy(data+1, packed);
}

CompactPhrase& CompactPhrase::operator=(const CompactPhrase& other) {
   DBG("=1");
   if ( this == &other ) return *this;
   clear();
   if ( other.data ) {
      if ( *(other.data) == -1 ) {
         // sharing this data will cause a ref-count overflow - duplicate
         // instead!
         DBG("+");
         alloc_data(strlen(other.data)+2);
         *data = 1;
         TALLY_REFS;
         #ifdef COMPACT_PHRASE_STATS
            ++ref_count_stats[256];
         #endif
         strcpy(data+1, other.data+1);
      } else {
         data = other.data;
         ++*data;
         TALLY_REFS;
      }
   }
   return *this;
}

void CompactPhrase::fromPhrase(const vector<Uint>& phrase) {
   clear();
   char buffer[phrase.size() * 5 + 1];
   char* pos(buffer);
   for ( Uint i(0); i < phrase.size(); ++i ) {
      // We encode unpacked[i] + 1 so that char(0) is never used, and can
      // therefore be used as null terminator.
      Uint to_pack(phrase[i] + 1);
      if ( to_pack == 0 ) {
         // Special handling for max_Uint, which overflows to 0 when doing +1
         *(pos++) = 0x80; *(pos++) = 0x80; *(pos++) = 0x80; *(pos++) = 0x80;
         *(pos++) = 0x10;
         #ifdef COMPACT_PHRASE_STATS
            ++byte_count_stats[4];
         #endif
      } else {
         #ifdef COMPACT_PHRASE_STATS
            if      ( to_pack <       0x80 ) ++byte_count_stats[0];
            else if ( to_pack <     0x4000 ) ++byte_count_stats[1];
            else if ( to_pack <   0x200000 ) ++byte_count_stats[2];
            else if ( to_pack < 0x10000000 ) ++byte_count_stats[3];
            else                             ++byte_count_stats[4];
         #endif
         while ( to_pack >= 0x80 ) {
            *(pos++) = 0x80 | (to_pack & 0x7f);
            to_pack >>= 7;
         }
         *(pos++) = to_pack & 0x7f;
      }
   }
   *(pos++) = 0x00;

   Uint data_size(pos - buffer + 1);
   alloc_data(data_size);
   *data = 1; // ref count
   TALLY_REFS;
   assert(strlen(buffer) < data_size - 1);
   strcpy(data+1,buffer);
   assert(strlen(data) < data_size);
}

void CompactPhrase::toPhrase(vector<Uint>& phrase, const char* packed) const {
   phrase.clear();
   if ( packed ) 
      for ( const_iterator it(packed); it != end(); ++it )
         phrase.push_back(*it);
   else
      for ( const_iterator it(begin()); it != end(); ++it )
         phrase.push_back(*it);
}

void CompactPhrase::clear() {
   if ( data ) {
      assert (*data != 0);
      if ( *data == 1 )
         free_data();
      else
         --*data;
      data = NULL;
   }
}

Uint CompactPhrase::size(const char* packed) const {
   if ( packed == NULL ) packed = c_str();   // Count *this by default
   // simple and clean, but too expensive!
   /*
   Uint count(0);
   for ( const_iterator it(packed); it != end(); ++it ) ++count;
   return count;
   */

   Uint count(0);
   const char* pos(packed);
   for ( ; *pos != 0x00; ++pos )
      if ( !(*pos & 0x80) ) ++count;

   // Since an "invalid" string where the last byte has its high bit set is
   // tolerated silently by other parts of this class, it should be tolerated
   // here too, but consistency, and to make any string a valid compactphrase.
   if ( pos > packed && (*(pos-1) & 0x80) ) ++count;

   return count;
}

const char* CompactPhrase::c_str() const {
   if ( data == NULL )
      return "";
   else
      return data+1;
}

Uint CompactPhrase::ref_count() const {
   if ( data )
      return static_cast<unsigned char>(*data);
   else
      return 0;
}

CompactPhrase::const_iterator& CompactPhrase::const_iterator::operator++() {
   if ( pos ) {
      if ( *pos == 0x00 ) {
         DBG("++ to NULL");
         pos = NULL;
      } else {
         DBG("\n++ ");
         current_val = 0;
         Uint current_shift(0);
         for ( ; *pos != 0x00; ++pos ) {
            TALLY_SHIFTS;
            current_val |= (*pos & 0x7f) << current_shift;
            if ( !(*pos & 0x80) ) {
               ++pos;
               break;
            } else {
               current_shift += 7;
            }
         }
         --current_val;
      }
   }
   return *this;
}

CompactPhrase::const_iterator CompactPhrase::const_iterator::operator++(int) {
   const_iterator copy(*this);
   operator++();
   return copy;
}

CompactPhrase::const_iterator& CompactPhrase::const_iterator::operator+=(int n)
{
   // Can't use Uint: would lead to unexpected behaviour when a caller
   // did += -2, instead of an explicit assert failure.
   assert(n >= 0);
   for ( int i(0); i < n; ++i )
      operator++();
   return *this;
}

CompactPhrase::const_reverse_iterator&
   CompactPhrase::const_reverse_iterator::operator++() {
   if ( pos ) {
      if ( pos == start_pos ) {
         pos = NULL;
         start_pos = NULL;
      } else {
         assert(pos > start_pos);
         current_val = 0;
         current_val |= (*(pos-1) & 0x7f);
         --pos;
         for ( ; pos > start_pos; --pos ) {
            if ( !(*(pos-1) & 0x80) ) {
               break;
            } else {
               current_val <<= 7;
               current_val |= (*(pos-1) & 0x7f);
            }
         }
         --current_val;
      }
   }
   return *this;
}

CompactPhrase::const_reverse_iterator
   CompactPhrase::const_reverse_iterator::operator++(int)
{
   const_reverse_iterator copy(*this);
   operator++();
   return copy;
}

CompactPhrase::const_reverse_iterator&
   CompactPhrase::const_reverse_iterator::operator+=(int n)
{
   // Can't use Uint: would lead to unexpected behaviour when a caller
   // did += -2, instead of an explicit assert failure.
   assert(n >= 0);
   for ( int i(0); i < n; ++i )
      operator++();
   return *this;
}

void CompactPhrase::print_ref_count_stats() {
   #ifdef COMPACT_PHRASE_STATS
      cerr << "Ref count statistics for CompactPhrase" << endl;
      for ( Uint i(0); i < 257; ++i )
         if ( ref_count_stats[i] > 0 )
            cerr << i << " ref(s): " << ref_count_stats[i] << endl;

      cerr << "Byte count statistics for CompactPhrase" << endl;
      for ( Uint i(0); i < 5; ++i )
         if ( byte_count_stats[i] > 0 )
            cerr << (i+1) << " byte(s): " << byte_count_stats[i] << endl;
      
      cerr << "Shift count: " << shift_count << endl;
   #endif
}

bool CompactPhrase::test() {
   bool ok = true;
   vector<Uint> v;
   Uint pack_size(0);
   v.push_back(0);              pack_size += 1;
   v.push_back(1);              pack_size += 1;
   v.push_back(126);            pack_size += 1;
   v.push_back(127);            pack_size += 2;
   v.push_back(128);            pack_size += 2;
   v.push_back(0x3ffe);         pack_size += 2;
   v.push_back(0x3fff);         pack_size += 3;
   v.push_back(0x4000);         pack_size += 3;
   v.push_back(0x1ffffe);       pack_size += 3;
   v.push_back(0x1fffff);       pack_size += 4;
   v.push_back(0x200000);       pack_size += 4;
   v.push_back(0xffffffe);      pack_size += 4;
   v.push_back(0xfffffff);      pack_size += 5;
   v.push_back(0x10000000);     pack_size += 5;
   v.push_back(0xfffffffe);     pack_size += 5;
   v.push_back(0xffffffff);     pack_size += 5;
   DBG("from Phrase\n");

   // test Basic construction from a phrase
   CompactPhrase packed(v);
   if ( strlen(packed.c_str()) != pack_size ) {
      error(ETWarn, "Pack size not as expected; got %d, expected %d",
            strlen(packed.c_str()), pack_size);
      ok = false;
   }

   // test back()
   DBG("back()\n");
   if ( packed.back() != v.back() ) {
      error(ETWarn, "packed.back() != unpacked.back(): got %d, expected %d",
            packed.back(), v.back());
      ok = false;
   }

   // test front()
   DBG("front()\n");
   if ( packed.front() != v.front() ) {
      error(ETWarn, "packed.front() != unpacked.front(): got %d, expected %d",
            packed.front(), v.front());
      ok = false;
   }

   // test size()
   DBG("size()\n");
   if ( packed.size() != v.size() ) {
      error(ETWarn, "packed.size() != unpacked.size(): got %d, expected %d",
            packed.size(), v.size());
      ok = false;
   }

   // test empty()
   DBG("empty()\n");
   if ( packed.empty() ) {
      error(ETWarn, "packed.empty() != false when non-empty");
      ok = false;
   }
   CompactPhrase empty1;
   if ( ! empty1.empty() ) {
      error(ETWarn, "empty1.empty() == false when freshly created");
      ok = false;
   }
   if ( empty1.size() != 0 ) {
      error(ETWarn, "empty1.size() != 0 when freshly created");
      ok = false;
   }
   empty1.fromPhrase(vector<Uint>(0));
   if ( ! empty1.empty() ) {
      error(ETWarn, "empty1.empty() == false when from empty vector");
      ok = false;
   }
   if ( empty1.size() != 0 ) {
      error(ETWarn, "empty1.size() != 0 when from empty vector");
      ok = false;
   }

   // test conversion to a phrase, and thereby the forward iterator
   DBG("to Phrase\n");
   vector<Uint> v2;
   packed.toPhrase(v2);
   if ( v2 != v ) {
      error(ETWarn, "Unpacked vector<Uint> not the same as the original");
      cerr << "Original: " << join(v) << endl;
      cerr << "Re-unpacked: " << join(v2) << endl;
      ok = false;
   }

   // test to phrase from c_str
   DBG("to Phrase from c_str\n");
   vector<Uint> v3;
   packed.toPhrase(v3, packed.c_str());
   if ( v3 != v ) {
      error(ETWarn, "Unpacked vector<Uint> not the same as the original");
      cerr << "Original: " << join(v) << endl;
      cerr << "Re-unpacked: " << join(v3) << endl;
      ok = false;
   }

   // test the reverse iterator
   vector<Uint> v4;
   reverse(v2.begin(), v2.end());
   for ( CompactPhrase::const_reverse_iterator it(packed.rbegin());
         it != packed.rend(); ++it )
      v4.push_back(*it);
   if ( v2 != v4 ) {
      error(ETWarn, "Reverse iterated not same as unpacked and reversed");
      cerr << "Reverse iterated: " << join(v2) << endl;
      cerr << "Unpacked and reversed: " << join(v4) << endl;
      ok = false;
   }

   // test reference counting
   CompactPhrase packed2(packed);
   CompactPhrase packed3(packed2);
   if ( packed2.ref_count() != 3 ||
        packed3.ref_count() != 3 ||
        packed.ref_count() != 3 ) {
      error(ETWarn, "Ref count inconsistency: %d %d %d, should all be 3",
            packed.ref_count(), packed2.ref_count(), packed3.ref_count());
      ok = false;
   }
   if ( packed.c_str() != packed2.c_str() ||
        packed.c_str() != packed3.c_str() ) {
      error(ETWarn, "Shared data inconsistency: %x %x %x, should all be same",
            packed.c_str(), packed2.c_str(), packed3.c_str());
      ok = false;
   }

   packed2.fromPhrase(vector<Uint>(4,66));
   if ( packed.ref_count() != 2 ||
        packed2.ref_count() != 1 ||
        packed3.ref_count() != 2 ) {
      error(ETWarn, "Ref count inconsistency: %d %d %d, should be 2 1 2",
            packed.ref_count(), packed2.ref_count(), packed3.ref_count());
      ok = false;
   }
   if ( packed.c_str() == packed2.c_str() ||
        packed.c_str() != packed3.c_str() ) {
      error(ETWarn, "Shared data inconsistency: %x %x %x, should be X Y X",
            packed.c_str(), packed2.c_str(), packed3.c_str());
      ok = false;
   }

   { // sub scope, so packed4 disappears when this scope ends.
      CompactPhrase packed4(packed);
      if ( packed4.ref_count() != 3 ||
           packed.ref_count() != 3 ) {
         error(ETWarn, "Ref count inconsistency: %d %d, should be 3 3",
               packed.ref_count(), packed4.ref_count());
         ok = false;
      }
   }
   if ( packed.ref_count() != 2 ) {
      error(ETWarn, "Ref count not decreased correctly: %d should be 2",
            packed.ref_count());
      ok = false;
   }

   // reference count high enough so we have to duplicate
   CompactPhrase cpa[1000];
   for ( Uint i(0); i < 253; ++i ) cpa[i] = packed;
   if ( packed.ref_count() != 255 ) {
      error(ETWarn, "ref count not 255: %d (%x)",
            packed.ref_count(), packed.c_str());
      ok = false;
   }
   if ( packed.c_str() != cpa[252].c_str() ) {
      error(ETWarn, "mem not shared");
      ok = false;
   }
   cpa[253] = packed;
   CompactPhrase packed5(packed);
   if ( packed.ref_count() != 255 ) {
      error(ETWarn, "ref count not 255 (bis): %d (%x)",
            packed.ref_count(), packed.c_str());
      ok = false;
   }
   if ( packed.c_str() == cpa[253].c_str() ) {
      error(ETWarn, "mem is shared");
      ok = false;
   }
   //cpa[252]

   return ok;
}

const CompactPhrase::const_iterator CompactPhrase::end_iter(NULL);
const CompactPhrase::const_reverse_iterator CompactPhrase::end_rev_iter(NULL);
