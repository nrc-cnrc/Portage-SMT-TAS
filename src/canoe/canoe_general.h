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
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef CANOE_GENERAL_H
#define CANOE_GENERAL_H

#include <portage_defs.h>
#include <utility>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <compact_phrase.h>

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

   /// Indicates no limit on maximum levenshtien cost
   extern const int NO_MAX_LEVENSHTEIN;

   /// Indicates no limit on maximum itg distortion
   extern const int NO_MAX_ITG;

   /// Definition of a Phrase.
   /// Can be VectorPhrase or CompactPhrase, both defined in
   /// utils/compact_phrase.h
   typedef VectorPhrase Phrase;
   //typedef CompactPhrase Phrase;

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
       * @return Returns true if *this starts before other or if both
       * start at the same index and *this ends before other.
       */
      bool operator<(const Range &other) const {
         return start < other.start ||
                (start == other.start && end < other.end);
      }
      /**
       * Equality operator.
       * @param other right-hand side operand.
       * @return Returns true iff both Ranges start and end at the same
       *         indexes.
       */
      bool operator==(const Range &other) const {
         return start == other.start && end == other.end;
      }
      /// Inequality operator.
      bool operator!=(const Range &other) const {
         return !operator==(other);
      }
      /**
       * Converts the Range to a readable format.
       * @return Returns a readable format of this Range.
       */
      string toString() const {
         ostringstream s;
         s << "[" << start << "," << end << ")";
         return s.str();
      }

      /// Return the length of a range
      Uint size() const { return end - start; }
   }; // Range

   /**
    * Output a range in a readable format
    */
   inline ostream& operator<<(ostream& os, const Range& r) {
      os << '[' << r.start << ", " << r.end << ')';
      return os;
   }

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
    * Test whether a Range and a UintSet are equivalent
    * @param s  UintSet
    * @param r  range
    * @return true iff, conceptually, s == r
    */
   inline bool operator==(const UintSet &s, Range r) {
      return s.size() == 1 && s[0] == r;
   }

   /**
    * Count the number of possible contiguous subranges in a UintSet
    * @param set       The set of Uints (as a set sequence of Ranges)
    * @return          Number of possible contiguous subranges
    */
   Uint countSubRanges(const UintSet &set);

   /**
    * count the number of words in set.
    */
   Uint countWords(const UintSet &set);

   /**
    * Display a UintSet as a bit vector, e.g., --111--11-
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
    * Display a UintSet as a sequence of ranges, e.g., "[2, 5) [7, 9)".
    * @param os where to display the uintset
    * @param s  what to convert to a readable format
    * @return os
    */
   inline ostream& operator<<(ostream& os, const UintSet& set) {
      os << join(set); return os;
   }

   /**
    * Given an item associated with each subrange of some range [0, R), and
    * given a UintSet, this picks out all items whose associated range is a
    * subset of the Uint set.
    * @param result    All the items picked out are appended to this vector.
    * @param triangArray A triangular array whose (i, j)-th entry is the item
    *                  associated with the range [i, i + j + 1).
    * @param set       The set of Uint's.
    */
   template <class T>
   void pickItemsByRange(vector<const T*> &result, T **triangArray, const UintSet &set)
   {
      result.reserve(countSubRanges(set));
      for (UintSet::const_iterator it = set.begin(); it < set.end(); it++) {
         for (Uint i = it->start; i < it->end; i++) {
            for (Uint j = 0; j < it->end - i; j++) {
               result.push_back(&triangArray[i][j]);
            }
         }
      }
   } // pickItemsByRange

   /// This namespace groups together all functions related to triangular
   /// arrays.  We don't want to use a class, because triangular arrays are
   /// double pointers, not a proper object.
   /// Note: it is recommended you don't "use namespace TriangArray", but use
   /// instead the qualified TriangArray::\<fn\>() syntax, so that your calls to
   /// these methods remain self-documenting.
   namespace TriangArray {

      /**
       * Get the element in triangArray for range [i,j).
       * Arguably not very useful, this function is intended primarily as
       * documentation, helping to clarify how to use a triangular array.
       * @pre 0 <= i < j <= sent_size
       */
      template <class T>
      inline T& Elem(T ** triangArray, Uint i, Uint j) {
         assert (j > i);
         // also assert j <= sentSize, but this can't be checked
         return triangArray[i][j - i - 1];
      }

      /**
       * Get the element in triangArray for range r.
       * Arguably not very useful, this function is intended primarily as
       * documentation, helping to clarify how to use a triangular array.
       * @pre 0 <= r.start < r.end <= sent_size
       */
      template <class T>
      inline T& Elem(T ** triangArray, Range r) {
         return Elem(triangArray, r.start, r.end);
      }

      /**
       * Callable entity that creates a triangular array.
       */
      template <class T>
      class Create
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
          * @param size The first-dimension size of the array created
          * @return     The triangular array created.
          */
         T** operator()(Uint size) {
            T **result = new T *[size];
            for (Uint i = 0; i < size; i++) {
               result[i] = new T[size - i];
            }
            return result;
         }
      }; // TriangArray::Create()

      /**
       * Callable entity that deletes a triangular array.
       */
      template <class T>
      class Delete
      {
       public:
         /**
          * Delete a triangular array created by TriangArray::Create<T>(size).
          * @param a    The triangular array to delete
          * @param size Must be the same value as was used to create a
          */
         void operator()(T** a, Uint size) {
            for (Uint i = 0; i < size; ++i)
               delete [] a[i];
            delete [] a;
         }
      }; // TrianArray::Delete()

   } // TriangArray

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
      T result = T(0);
      for (Uint i = 0; i < size; i++) {
         if (a[i] != T(0) && b[i] != T(0)) result += a[i] * b[i];
      } // for
      return result;
   } // dotProduct

} // Portage


#endif // CANOE_GENERAL_H
