/**
 * @author Aaron Tikuisis
 * @file canoe_general.h  This file contains commonly used structures and
 *                        simple functions.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef CANOE_GENERAL_H
#define CANOE_GENERAL_H

#include <portage_defs.h>
#include <utility>
#include <ext/hash_map>
#include <string>
#include <vector>
#include <sstream>
#include <assert.h>

using namespace std;
using namespace Portage;

namespace Portage
{
   /// Default smoothed value for log(0)
   extern const double LOG_ALMOST_0;
   /// What Portage thinks is the value of almost infinity
   extern const double ALMOST_INFINITY;
   /// Indicates no pruning size limit
   extern const Uint NO_SIZE_LIMIT;
   /// Indicates no limit on maximum distortion cost
   extern const int NO_MAX_DISTORTION;

   /**
    * A range of words in a sentence [start, end):
    * start = first word in the range
    * end = first word NOT in the range, which must be <= the number of words
    *       in the sentence
    * Hence, the length of the range is simply (end - start). If the empty
    * range is not being allowed (which is generally the case), then end >
    * start.  The empty range is given when end == start.  end < start is
    * always invalid.
    */
   struct Range
   {
      Uint start;   ///< start index of the Range
      Uint end;     ///< end index of the Range (not included [start, end))

      /**
       * Constructor.  Constructs the rance [start, end)
       * @param s  start index
       * @param e  end index
       */
      Range(Uint s=0, Uint e=0): start(s), end(e) {}
      /**
       * Less operator.
       * @param other  right-hand side operand.
       * @return Returns true if this Range start before other or if they
       * start at the same index and this end before other.
       */
      bool operator<(const Range &other) const {
         return start < other.start ||
                (start == other.start && end < other.end);
      }
      /**
       * Equality operator.
       * @param other right-hand side operand.
       * @return Returns true if both Ranges starts and ends at the same
       *         indexes.
       */
      bool operator==(const Range &other) const {
         return start == other.start && end == other.end;
      } // operator==
      /**
       * Converts the Range to a readable format.
       * @return Returns a readable format of this Range.
       */
      string toString() const {
         ostringstream s;
         s << "[" << start << "," << end << ")";
         return s.str();
      }
   }; // Range

   /// Definition of a Phrase
   typedef vector<Uint> Phrase;

   /**
    * A set of Uint's is represented uniquely by a minimal set of Range's
    * which form a partition of the set; this partition set in turn is
    * uniquely represented as a sorted vector of the Range's.  All functions
    * which use UintSet have these restrictions (minimal partition size and
    * orderedness) as pre- and post-conditions.
    */
   typedef vector<Range> UintSet;

   /**
    * Creates the union of the set s of Uint's with all the Uint's in the
    * range r.  The result is stored in result.
    * @param result    The result of the operation is stored here.
    * @param s         A set of Uint's.
    * @param r         A contiguous set of Uint's
    */
   void addRange(UintSet &result, const UintSet &s, const Range &r);

   /**
    * Creates the set subtraction, the set s of Uint's minus all the Uint's in
    * the range r.  The result is stored in result.
    * @param result    The result of the operation is stored here.
    * @param s         A set of Uint's.
    * @param r         A contiguous set of Uint's
    */
   void subRange(UintSet &result, const UintSet &s, const Range &r);

   /**
    * Creates the set intersection, the set s of Uint's intersect all the
    * Uint's in the range r.  The result is stored in result.
    * @param result    The result of the operation is stored here.
    * @param s         A set of Uint's.
    * @param r         A contiguous set of Uint's
    */
   void intersectRange(UintSet &result, const UintSet &s, const Range &r);

   /**
    * Count the number of possible contiguous subranges in a UintSet
    * @param set       The set of Uints (as a set sequence of Ranges)
    * @return          Number of possible contiguous subranges
    */
   Uint countSubRanges(const UintSet &set);

   /**
    * Display a UintSet as a bit vector
    * @param set        The set of Uints to display
    * @param in_is_1    If true, displays 1 for elements in the set, - for
    *                   others; if false, reverses 1 and -.
    * @param length     Length of the bit vector to display.  If length is
    *                   smaller than the largest element in set, length is
    *                   set so the largest element can be displayed.
    * @return string containing the display.
    */
   string displayUintSet(const UintSet &set, bool in_is_1=true, Uint length=1);

   /**
    * Given an item associated with each subrange of some range [0, R), and
    * given a UintSet, this picks out all items whose associated range is a
    * subset of the Uint set.
    * @param result    All the items picked out are appended to this vector.
    * @param triangArray A triangular array whose (i, j)-th entry is the item
    *                  associated with the range [i, i + j - 1).
    * @param set       The set of Uint's.
    */
   template <class T>
   void pickItemsByRange(vector<T> &result, T **triangArray, const UintSet &set)
   {
      result.reserve(countSubRanges(set));
      for (UintSet::const_iterator it = set.begin(); it < set.end(); it++) {
         for (Uint i = it->start; i < it->end; i++) {
            for (Uint j = 0; j < it->end - i; j++) {
               result.push_back(triangArray[i][j]);
            }
         }
      }
   } // pickItemsByRange

   /**
    * Callable entity that creates a triangular array.
    */
   template <class T>
   class CreateTriangularArray
   {
    public:
      /**
       * Create a triangular array, a, of objects of type T.  The array has
       * length size, and the i-th entry has (size - i) elements.  This is
       * precisely the dimension needed for the return value of
       * PhraseDecoderModel::getPhraseInfo() when size = source sentence
       * length, for example.
       * The result must be deleted by its user, possibly by calling
       * DeleteTriangularArray<T>().
       * @param size        The first-dimension size of the array created
       * @return            The triangular array created.
       */
      T** operator()(Uint size) {
         T **result = new T *[size];
         for (Uint i = 0; i < size; i++) {
            result[i] = new T[size - i];
         }
         return result;
      }
   }; // CreateTriangularArray

   /**
    * Callable entity that deletes a triangular array.
    */
   template <class T>
   class DeleteTriangularArray
   {
    public:
      /**
       * Delete a triangular array created by CreateTriangularArray<T>(size).
       * @param a    The triangular array to delete
       * @param size Must be the same value as was used to create a
       */
      void operator()(T** a, Uint size) {
         for (Uint i = 0; i < size; ++i)
            delete [] a[i];
         delete [] a;
      }
   }; // DeleteTriangularArray

} // Portage

/**
 * Output a range in a readable format (useful in debugging).
 */
#define RANGEOUT(range) '[' << (range).start << ", " << (range).end << ')'

/**
 * Converts a UintSet to a readable string.
 * @param s  what to convert to a readable format
 * @return Returns a readable representation of s
 */
inline string UINTSETOUT(const UintSet &s)
{
   if (s.empty()) return "";
   ostringstream sout;
   UintSet::const_iterator it = s.begin();
   while (true) {
      sout << RANGEOUT(*it);
      ++it;
      if (it == s.end()) break;
      sout << " ";
   } // while
   return sout.str();
} // UINTSETOUT


namespace __gnu_cxx
{
   /// Callable entity to create hash values for string.
   template<>
   class hash<string>
   {
    public:
      /**
       * Generates a hash value for a string
       * @param s  string to hash
       * @return Returns a hash value for s
       */
      Uint operator()(const string &s) const {
         return hash<const char *>()(s.c_str());
      } // operator()
   }; // hash<string>
} // __gnu_cxx

/**
 * Calculates the partial/total dot product between two vectors.
 * Note: If types T and U are not the same, T should be the type with higher
 * precision, since it is also the return type.
 * @param a     left-hand side operand
 * @param b     right-hand side operand
 * @param size  number of elements to use for the dot product
 */
template<class T, class U>
T dotProduct(const vector<T> &a, const vector<U> &b, Uint size)
{
   assert(size <= a.size());
   assert(size <= b.size());
   T result = (T)0;
   for (Uint i = 0; i < size; i++) {
      if (a[i] != (T)0 && b[i] != (T)0) result += a[i] * b[i];
   } // for
   return result;
} // dotProduct

#endif // CANOE_GENERAL_H
