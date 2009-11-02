/**
 * $Id$
 * @author Eric Joanis
 * @file bivector.h Bidirectional vector, with negative and positive indices
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef BI_VECTOR_H
#define BI_VECTOR_H

#include "portage_defs.h"
#include "binio.h"
#include "errors.h"
#include <boost/operators.hpp>
#include <numeric>
#include <vector>

namespace Portage {

/// Vector abstraction with valid indices ranging from negative to positive
/// values, with all values implicitely being T(0) until modified.
template <class T>
class BiVector {

   /// Underlying storage
   vector<T> storage;
   /// Storage offset: storage[0] contains this->[-offset], this->[0] is
   /// stored in storage[offset].
   int offset;

 public:

   /**
    * Default constructor.
    */
   BiVector() : offset(0) {}

   /**
    * Construct a BiVector with values from start to end, inclusively,
    * allocated and set to init.
    * @param first index of first element to allocate and initialize
    * @param last  index of last element to allocate and initialize
    * @param init  initial value for each value in [first,last]
    * @pre first <= last
    */
   BiVector(int first, int last, T init = T(0)) {
      setRange(first, last, init);
   }

   /**
    * Set a range of values, allocating storage for them if necessary.
    * The range [first,last], inclusive of first and last, is set to value.
    * Does nothing if first > last.
    * Intended to replace reserve() and resize() of regular vectors - since a
    * bivector is conceptually unbounded, those methods would not make sense,
    * so we provide setRange to do the job.
    * @param first  first index to set
    * @param last   last index to set
    * @param value  value to give to indices [first,last] (inclusive range)
    */
   void setRange(int first, int last, T value = T(0)) {
      if ( first > last ) return;
      if ( storage.empty() ) {
         storage.resize(last-first+1, value);
         offset = -first;
      } else {
         // Deal with allocation of new memory via setAt()
         setAt(first) = value;
         setAt(last) = value;
         // Now set the values in the rest of the range reasonably efficiently
         fill(storage.begin()+first+offset+1,
              storage.begin()+last+1+offset, value);
      }
   }

   /**
    * swap the contents of two BiVectors
    */
   void swap(BiVector& that) {
      std::swap(offset, that.offset);
      storage.swap(that.storage);
   }

   /**
    * Reinitialize all elements to zero.
    */
   void clear() { storage.clear(); }

   /**
    * Return the number of elements actually allocated.
    */
   Uint size() const { return storage.size(); }

   /**
    * Return the index of the first element actually allocated.
    * If none are allocated, size() == 0 and last() == first() - 1.
    */
   int first() const { return -offset; }

   /**
    * Return the index of the last element actually allocated.
    * If none are allocated, size() == 0 and last() == first() - 1.
    */
   int last() const { return -offset + storage.size() - 1; }

   /**
    * Syntactic sugar - returns the sum of the elements in the vector
    */
   T sum() const {
      return std::accumulate(begin(), end(), T());
   }

   /**
    * Test if *this and that have the same values.
    * first() and last() need not match, as long as operator[] will return the
    * same thing for all indices i, *this and that are considered equal.
    * @param that  BiVector to compare *this with
    * @return true iff *this and that have the same values.
    */
   bool operator==(const BiVector<T>& that) const {
      for ( int i   = min(first(), that.first()),
                end = max(last(), that.last()) + 1;
            i < end; ++i )
         if ( operator[](i) != that[i] )
            return false;
      return true;
   }

   /**
    * @returns !(*this == that)
    */
   bool operator!=(const BiVector<T>& that) const {
      return !operator==(that);
   }

   /**
    * Add that to self, index by index.
    * @param that  bivector to add to self
    * @return ref to self
    */
   BiVector<T>& operator+=(const BiVector<T>& that) {
      for ( int i(that.first()), that_last(that.last()); i <= that_last; ++i )
         setAt(i) += that[i];
      return *this;
   }
   
   /**
    * Get the value at i for reading only.
    * @param i position to read
    * @return the value at i, which is implicitely T(0) if it was
    *         never modified.
    */
   const T operator[](int i) const {
      if ( i + offset < 0 || i + offset >= int(storage.size()) )
         return T(0);
      else
         return storage[i + offset];
   }

   /**
    * This reference class allows for an operator[] which only allocates memory
    * if the result written to via operator=().
    */
   class reference {
      BiVector<T>* parent;
      int i;
      reference(BiVector<T>* parent, int i) : parent(parent), i(i) {};
      friend class BiVector<T>;
     public:
      /// Get the value of parent[i] without causing any allocation.
      operator T() const { return (*const_cast<const BiVector<T>*>(parent))[i]; }
      /// Set the value of parent[i], allocating memory as necessary
      reference& operator=(const T& value) { parent->setAt(i) = value; return *this; }
   };

   /**
    * Get a reference to the value at i; will only cause actual allocation if
    * used as an lvalue in an assignment operation.
    * @param i position to read
    * @return reference to the value at i
    */
   reference operator[](int i) { return reference(this, i); }
      
   /**
    * Get a modifiable reference to the value at i.  Allocates it if it wasn't
    * already allocated.
    * @param i position to fetch
    * @return a modifiable reference to the value at i.
    */
   T& setAt(int i) {
      if ( storage.empty() ) offset = -i;
      if ( i + offset < 0 ) {
         // need to insert space at the beginning of storage
         const int new_elements(-(i+offset));
         //assert(new_elements > 0);
         storage.insert(storage.begin(), new_elements, T(0));
         offset += new_elements;
      } else if ( i + offset >= int(storage.size()) ) {
         // need to insert space at the end of storage
         const int new_elements(i+offset - int(storage.size()) + 1);
         //assert(new_elements > 0);
         storage.insert(storage.end(), new_elements, T(0));
      }
      //assert( i + offset >= 0 && i + offset < int(storage.size()) );
      return storage[i+offset];
   }

   /**
    * Write self to a human readable stream
    * @param os        stream to write self to
    * @param precision precision to use on the stream (leave untouched if 0)
    */
   void write(ostream& os, Uint precision = 0) const {
      streamsize old_precision = os.precision();
      if ( precision != 0 ) os.precision(precision);
      int first_non_zero(first());
      int last_non_zero(last());
      while ( last_non_zero >= first_non_zero &&
              operator[](last_non_zero) == T(0) )
         --last_non_zero;
      while ( first_non_zero <= last_non_zero &&
              operator[](first_non_zero) == T(0) )
         ++first_non_zero;
      os << first_non_zero << " " << last_non_zero;
      for ( int i(first_non_zero); i <= last_non_zero; ++i )
         os << " " << operator[](i);
      os << nf_endl;
      os.precision(old_precision);
   }

   /**
    * Read self from a human readable stream, as created by write()
    * @param is stream to read and re-initialize self from
    * @return true if read successfully, false if stream is somehow corrupt.
    */
   bool read(istream& is) {
      int first_non_zero, last_non_zero;
      is >> first_non_zero >> last_non_zero;
      clear();
      setRange(first_non_zero, last_non_zero);
      for ( int i(first_non_zero); i <= last_non_zero; ++i )
         is >> setAt(i);
      string line;
      if ( ! getline(is, line) ) {
         error(ETWarn, "Corrupt stream missing newline in BiVector<T>::read() "
               "in %s:%d", __FILE__, __LINE__);
         return false;
      }
      if ( line != "" ) {
         error(ETWarn, "Corrupt stream has extra stuff in BiVector<T>::read() "
               "in %s:%d: %s", __FILE__, __LINE__, line.c_str());
         return false;
      }
      return true;
   }
   
   /**
    * Write self to a binary stream
    * @param os        stream to write self to
    */
   void writebin(ostream& os) const {
      BinIO::writebin(os, offset);
      BinIO::writebin(os, storage);
   }

   /**
    * Read self from a binary stream
    * @param is        stream to read self from
    */
   void readbin(istream& is) {
      BinIO::readbin(is, offset);
      BinIO::readbin(is, storage);
   }

   class const_iterator;

   /// Return a const_iterator starting at the first allocated value
   const_iterator begin() const {
      return const_iterator(this, first());
   }

   /// Return a const_iterator after the last allocated value
   const_iterator end() const {
      return const_iterator(this, last()+1);
   }

   /// const iterator over a BiVector.
   class const_iterator
      : public boost::input_iterator_helper<const_iterator, T>
   {
      const BiVector<T>* bv;
      int pos;
      friend class BiVector<T>;
      /// Constructor for use by encapsulating class
      const_iterator(const BiVector<T>* bv, int pos) : bv(bv), pos(pos) {}

    public:
      /// Prefix ++: move to next element.
      const_iterator& operator++() { ++pos; return *this; }

      /// efficient arbitrary increment
      const_iterator& operator+=(int n) { pos += n; return *this; }

      /// Get the value at the current position
      T operator*() const { return (*bv)[pos]; }

      /// Test for equality
      bool operator==(const const_iterator& other) const {
         return bv == other.bv && pos == other.pos;
      }

   }; // class const_iterator

}; // class BiVector<T>

} // Portage

#endif // BI_VECTOR_H
