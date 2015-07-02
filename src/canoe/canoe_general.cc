/**
 * @author Aaron Tikuisis
 * @file canoe_general.cc  This file contains the implementation of commonly
 *                         used simple functions.
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 *
 * Functions which really ought not be inlined extracted from canoe_general.h
 * by Eric Joanis
 */

#include "canoe_general.h"

namespace Portage {
   ostream& to_JSON(ostream& out, const Phrase& phrase, const Voc& voc) {
      out << '[';
      for (Uint i(0); i<phrase.size(); ++i) {
         if (i>0) out << ',';
         out << to_JSON(string(voc.word(phrase[i])));
      }
      out << ']';
      return out;
   }

   void addRange(UintSet &result, const UintSet &s, const Range &r)
   {
      result.clear();

      // Copy over everything before r
      UintSet::const_iterator it = s.begin();
      for (; it < s.end() && it->end < r.start; it++)
         result.push_back(*it);

      // The current range in s is the first one that overlaps or comes after r
      // The start of the new range is the minimum of the start of the current
      // range in s and of r.
      Uint curStart;
      if (it == s.end())
         curStart = r.start;
      else
         curStart = min(it->start, r.start);

      Uint curEnd;
      // Proceed to the first range in s that doesn't touch r
      for (; it < s.end() && it->start <= r.end; it++) {}

      // *(it - 1) is the last range that overlaps or comes before r
      // The end of the new range is the maximum of the end of *(it - 1) and of r.
      if (it == s.begin())
         curEnd = r.end;
      else
         curEnd = max((*(it - 1)).end, r.end);

      // Add the new range
      result.push_back(Range(curStart, curEnd));

      // Copy over everything that comes after r
      for (; it < s.end(); it++)
         result.push_back(*it);
   } // addRange

   void subRange(UintSet &result, const UintSet &s, const Range &r)
   {
      result.clear();

      for (UintSet::const_iterator it = s.begin(); it < s.end(); it++)
      {
         if (it->start > r.end || it->end < r.start)
         {
            // No overlap, so copy unaffected
            result.push_back(*it);
         } else
         {
            /* Overlap, so need to delete r from *it and store the result
               So, we have a picture as follows:
               *it                      [        )
               - r                         [  )
                                     ---------------
               = *it \setminus r        [  )  [  )
            */

            if (it->start < r.start)
            {
               // The left range is non-empty, so add it
               result.push_back(Range(it->start, r.start));
            }
            if (it->end > r.end)
            {
               // The right range is non-empty, so add it
               result.push_back(Range(r.end, it->end));
            }
         }
      }
   } // subRange

   void intersectRange(UintSet &result, const UintSet &s, const Range &r)
   {
      result.clear();

      for (UintSet::const_iterator it = s.begin(); it < s.end(); ++it)
      {
         if (it->end > r.start && r.end > it->start)
         {
            Uint curStart = max(it->start, r.start);
            Uint curEnd = min(it->end, r.end);
            result.push_back(Range(curStart, curEnd));
         }
      }
   }

   bool isSubset(Range s1, const UintSet &s2) {
      UintSet::const_iterator it2 = s2.begin();
      while (it2 != s2.end() && it2->end < s1.end) ++it2;
      if (it2 == s2.end()) return false;
      assert(it2->end >= s1.end);
      if (it2->start > s1.start) return false;
      return true;
   }

   bool isSubset(const UintSet &s1, const UintSet &s2) {
      UintSet::const_iterator it2 = s2.begin();
      for (UintSet::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1) {
         while (it2 != s2.end() && it2->end < it1->end) ++it2;
         if (it2 == s2.end()) return false;
         assert(it2->end >= it1->end);
         if (it2->start > it1->start) return false;
      }
      return true;
   }

   bool isDisjoint(Range r, const UintSet &s) {
      UintSet::const_iterator it = s.begin();
      while (it != s.end() && r.start >= it->end) ++it;
      if (it == s.end()) return true;
      return (r.end <= it->start);
   }

   Uint countSubRanges(const UintSet &set) {
      Uint subRanges = 0;
      for (UintSet::const_iterator it = set.begin(); it < set.end(); ++it) {
         Uint len = it->end - it->start;
         subRanges += len * (len + 1) / 2;
      }
      return subRanges;
   }

   Uint countWords(const UintSet &set) {
      Uint count(0);
      for (UintSet::const_iterator it = set.begin(); it < set.end(); ++it) {
         assert (it->end >= it->start);
         count += it->end - it->start;
      }
      return count;
   }

   string displayUintSet(const UintSet &set, bool in_is_1, Uint length) {
      string result;
      result.reserve(length);
      Uint pos(0);
      for ( UintSet::const_iterator it(set.begin()), end(set.end());
            it != end; ++it ) {
         for ( ; pos < it->start; ++pos )
            result += in_is_1 ? "-" : "1";
         for ( ; pos < it->end; ++pos )
            result += in_is_1 ? "1" : "-";
      }
      for ( ; pos < length; ++pos )
         result += in_is_1 ? "-" : "1";
      return result;
   }

   const Voc* GlobalVoc::the_voc = NULL;
   void GlobalVoc::set(const Voc* voc) { assert(the_voc == NULL); the_voc = voc; }
   void GlobalVoc::clear() { assert(the_voc != NULL); the_voc = NULL; }

} // namespace Portage
