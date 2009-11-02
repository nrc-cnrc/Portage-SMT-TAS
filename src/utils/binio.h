/**
 * @author Eric Joanis, based on code by Samuel Larkin and Evan Stratford
 * @file binio.h Utilities for fast binary reading and writing of data.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef PORTAGE_BINIO_H
#define PORTAGE_BINIO_H

#include "portage_defs.h"
#include "movable_trait.h"
#include <boost/static_assert.hpp>
#include <vector>
#include <iostream>

namespace Portage {

/**
 * Read and write data in binary mode.
 *
 * Uses fast block operations if T is safe-movable, and recursive operations
 * otherwise.
 *
 * The API consists of these function templates, found at the end of this file:
 *    void writebin(ostream&, const T&);
 *    void writebin(ostream&, const T[], Uint);
 *    void readbin(istream&, T&);
 *    void readbin(istream&, T[], Uint);
 *
 * T can be any primitive type, std::pair, std::vector, std::map,
 * std::tr1::unordered_map or any class that provides writebin() and readbin().
 * Any data structure that is effectively a tree can be supported.
 * Cycles are not handled directly by this library, and neither are DAGs,
 * however, or even pointers.  Those would have to be handled explicitely by
 * the client class's writebin() and readbin() methods.
 *
 * To invoke writebin() on objects of type MyType or containers of such
 * objects, you must do one of three things:
 *    1) make sure MyType is safe movable (see movable_trait.h for
 *       details) and declare it so like this: SAFE_MOVABLE(MyType).
 *       In this case, the object will be block-copied, and vectors or
 *       arrays of MyType obects will also be efficiently copied together
 *       using block memory operations, for both writebin() and readbin().
 * or
 *    2) if MyType is not safe movable, provide these two public methods:
 *          void writebin(ostream& os) const;
 *          void readbin(istream&is);
 *       which should correctly serialize and deserialize MyType.
 * or
 *    3) If you don't own MyType and it's not safe movable, write a template
 *       specialization of BinIO::Impl<MyType> which must implement
 *          static void writebin_impl(ostream& os, const MyType& v);
 *          static void readbin_impl(istream& is, MyType& v);
 *       (See the specialization for vector below, or map/unordered_map in
 *       binio_maps.h for examples of how to do this.)
 *
 * Caveat: not always safe if you use pointers to base classes.  For example,
 * this code will yield incorrect results:
 *   class S { ... };
 *   SAFE_MOVABLE(S);
 *   class NS : public S { ... };
 *   S* p = new NS();
 *   writebin(os, *p);
 * If you need to call writebin() on base-class pointers, do not declare S
 * to be safe movable, write a virtual writebin() method instead.  However,
 * to read the resulting stream back in, it will be necessary to resolve the
 * ultimate type of the object before calling readbin().
 * For this reason, it is preferable to call writebin() on the ultimate
 * class of each object.  Also, no specialization is offered for writing out
 * containers of pointers, since they could not be written correctly in a
 * generic way.
 * See the HMMJumpStrategy class hierarchy in the tm module for examples of
 * handling reading and writing a class hierarchy; not that the BinIO library
 * is only partially used there.
 *
 * Caveat: error handling is minimalist: writebin() has no error handling;
 * readbin() only indirectly handles errors, insofar as the istream's error
 * bits can be checked after invoking readbin().  See readbin()'s documentation
 * for details.
 */
namespace BinIO {

   using std::istream;
   using std::ostream;

   // forward declarations of the main functions of this library
   template<typename T> void writebin(ostream& os, const T& v);
   template<typename T> void readbin(istream& is, T& v);

   /**
    * Instances of safe_writebin() and safe_readbin() that handle safe-movable
    * data types take a SafeMovableType argument that directs the overloading
    * mechanism to the right instance.
    *
    * movable_trait_t<T>::safe_t() resolves to SafeMovableType if T is
    * safe-movable, i.e., if movable_trait<T>::safe == true.
    */
   typedef ConstIntType<true> SafeMovableType;

   /** 
    * Instances of safe_writebin() and safe_readbin() that handle non
    * safe-movable data types take a SafeMovableType argument that directs the
    * overloading mechanism to the right instance.
    *
    * movable_trait_t<T>::safe_t() resolves to NonSafeMovableType if T is
    * not safe-movable, i.e., if movable_trait<T>::safe == false.
    */
   typedef ConstIntType<false> NonSafeMovableType;

   /**
    * Generic implementation template for writebin and readbin.
    *
    * This base template already supports basic types, strings and pairs,
    * as well as classes that provide readbin() and writebin() methods.
    * Write your own template specializations to handle specific containers for
    * which you cannot add member methods writebin() and readbin().
    *
    * Specializations for std::map and std::tr1::unordered_map are provided in
    * binio_maps.h.  A specialization for std::vector is provided further down
    * in this file.
    *
    * Implementation Note: we use a template class instead of a template
    * function for the underlying implementation so that we can take advantage
    * of the partial specialization mechanism of classes, which is not
    * available for functions, and thus avoid all the pitfalls of function
    * template overloading.  (In particular, an early draft of this class based
    * on function templates could not correctly handle vectors of
    * unordered_maps, because of strange limitations of function overloading,
    * whereas it worked trivially with the current design based on Impl
    * template class.)
    */
   template<typename T>
   struct Impl {
   private:
      /// Write a safe-movable value in binary mode
      template<typename T1>
      static void safe_writebin(ostream& os, const T1& v, SafeMovableType) {
         BOOST_STATIC_ASSERT(movable_trait<T1>::safe);
         os.write((char*)&v, sizeof(v));
      }

      /// Write a non safe-movable value in binary mode; T1 must implement
      ///    void writebin(ostream&) const.
      template<typename T1>
      static void safe_writebin(ostream& os, const T1& v, NonSafeMovableType) {
         v.writebin(os);
      }

      /// Write a pair in binary mode for non safe-movable T's.  (The
      /// safe-movable case is already handled by the generic safe case above.)
      template<typename T1, typename T2>
      static void safe_writebin(ostream& os, const pair<T1,T2>& v, NonSafeMovableType) {
         writebin(os, v.first);
         writebin(os, v.second);
      }

      /**
       * Specialization: Write a string in binary mode. string is not
       * safe-movable, so we only need to cover that case.
       */
      static void safe_writebin(ostream& os, const string& s, NonSafeMovableType) {
         Uint n(s.size());
         writebin(os, n);
         os.write(s.data(), n);
      }

      /// Read a safe-movable value in binary mode
      template<typename T1>
      static void safe_readbin(istream& is, T1& v, SafeMovableType) {
         BOOST_STATIC_ASSERT(movable_trait<T1>::safe);
         is.read((char*)&v, sizeof(v));
      }

      /// Read a non safe-movable value in binary mode; T1 must implement
      ///    void readbin(istream&);
      template<typename T1>
      static void safe_readbin(istream& is, T1& v, NonSafeMovableType) {
         v.readbin(is);
      }

      /// Read a pair in binary mode for non safe-movable T's.  (The
      /// safe-movable case is arleady handled by the generic safe case above.)
      template<typename T1, typename T2>
      static void safe_readbin(istream& is, pair<T1,T2>& v, NonSafeMovableType) {
         readbin(is, v.first);
         readbin(is, v.second);
      }

      /// Read a string in binary mode, strings are not safe-movable
      static void safe_readbin(istream& is, string& s, NonSafeMovableType) {
         Uint n;
         readbin(is, n);
         char* buf = new char[n];
         is.read(buf, n);
         s.assign(buf, n);
         delete[] buf;
      }

   public:
      /// Underlying implementation of BinIO::writebin()
      static void writebin_impl(ostream& os, const T& v) {
         safe_writebin(os, v, typename movable_trait_t<T>::safe_t());
      }
      /// Underlying implementation of BinIO::readbin()
      static void readbin_impl(istream& is, T& v) {
         safe_readbin(is, v, typename movable_trait_t<T>::safe_t());
      }
   };


   // ====================================================================
   // Implementation for vectors
   // ====================================================================

   /**
    * Template specialization: read/write a vector in binary mode, using fast
    * block operations if T is safe-movable, or looping through each element
    * otherwise.  T should be default constructible.
    */
   template<typename T>
   struct Impl<vector<T> > {
    private:
      /// Write a vector in binary mode, optimized and specialized for
      /// safe-movable T
      static void safe_writebin_vector(ostream& os, const vector<T>& v, SafeMovableType) {
         BOOST_STATIC_ASSERT(movable_trait<T>::safe);
         Uint s(v.size());
         writebin(os, s);
         if (s>0)
            os.write((char*)&v[0], s*sizeof(T));
      }

      /// Write a vector in binary mode, specialized for safely writing non
      /// safe-movable T
      static void safe_writebin_vector(ostream& os, const vector<T>& v, NonSafeMovableType) {
         Uint s(v.size());
         writebin(os, s);
         for ( Uint i = 0; i < s; ++i )
            writebin(os, v[i]);
      }
      
      /// Read a vector in binary mode, specialized for safe-movable T 
      static void safe_readbin_vector(istream& is, vector<T>& v, SafeMovableType) {
         BOOST_STATIC_ASSERT(movable_trait<T>::safe);
         Uint s(0);
         readbin(is, s);
         if (s>0) {
            v.resize(s);
            is.read((char*)&v[0], s*sizeof(T));
         }
         else {
            v.clear();
         }
      }

      /// Read a vector in binary mode, specialized for safely reading non
      /// safe-movable T 
      static void safe_readbin_vector(istream& is, vector<T>& v, NonSafeMovableType) {
         Uint s;
         readbin(is, s);
         v.clear();
         v.resize(s);
         for (Uint i = 0; i < s; i++)
            readbin(is, v[i]);
      }

    public:
      /// writebin() implementation for vectors
      static void writebin_impl(ostream& os, const vector<T>& v) {
         safe_writebin_vector(os,v,typename movable_trait_t<T>::safe_t());
      }
      /// readbin() implementation for vectors
      static void readbin_impl(istream& is, vector<T>& v) {
         safe_readbin_vector(is, v, typename movable_trait_t<T>::safe_t());
      }
   };


   // ====================================================================
   // Implementation for arrays
   // ====================================================================

   /// Write an array of safe-movable T's
   template<typename T>
   void safe_writebin_array(ostream& os, const T* a, Uint n, SafeMovableType) {
      BOOST_STATIC_ASSERT(movable_trait<T>::safe);
      os.write((char*)a, sizeof(T) * n);
   }

   /// Write an array of non safe-movable T's
   template <typename T>
   void safe_writebin_array(ostream& os, const T* a, Uint n, NonSafeMovableType) {
      for (Uint i = 0; i < n; i++)
         writebin(os, *(a+i));
   }

   /// Read an array of safe-movable T's
   template<typename T>
   void safe_readbin_array(istream& is, T* a, Uint n, SafeMovableType) {
      BOOST_STATIC_ASSERT(movable_trait<T>::safe);
      is.read((char*)a, sizeof(T) * n);
   }

   /// Read an array of non safe-movable T's
   template<typename T>
   void safe_readbin_array(istream& is, T* a, Uint n, NonSafeMovableType) {
      for (Uint i = 0; i < n; i++)
         readbin(is, *(a+i));
   }


   // ====================================================================
   // Client API starts here
   // ====================================================================

   /**
    * Write a value in binary mode.  Uses fast block operations if T is
    * safe-movable, and recursive operations otherwise.
    *
    * Caveat: T must be the ultimate type of v, it cannoe be a superclass.  See
    * the BinIO namespace documentation for more details.
    *
    * @param os  stream to write to
    * @param v   value to write in binary format
    */
   template<typename T>
   void writebin(ostream& os, const T& v) {
      Impl<T>::writebin_impl(os, v);
   }

   /**
    * Read a value in binary mode.  Uses fast block operations if T is
    * safe-movable, and recursive operations otherwise.
    *
    * Detecting errors: When an error occurs during reading, the failbit or
    * badbit (and possibly eofbit) of is should get set.  In this case, the
    * contents of v are undefined.  You can detect errors like this:
    *    readbin(is, myvar);
    *    if ( !is ) { handle/report the error -- myvar is not usable }
    * Equivalently: if ( is.bad() ) { ... }.
    * Beware, the following is not quite equivalent: if ( !is.good() ) { ... }.
    * This is not quite equivalent because is.good() also returns false if
    * is.eof() is true, even if v was read succesfully.
    *
    * Caveat: T must be the ultimate type of v, it cannot be a superclass.
    * E.g., this code will yield unpredictable results:
    *   class NS : public S { ... };
    *   S* p = new NS();
    *   readbin(os, *p);
    *
    * @param is       stream to read from
    * @param[out] v   value to read in binary format
    */
   template<typename T>
   void readbin(istream& is, T& v) {
      Impl<T>::readbin_impl(is, v);
   }

   /**
    * Overload: Read an array in Binary mode.
    * @param is      stream to read from
    * @param[out] a  pointer to array of values to read in binary format
    *                a must point to enough allocated storage to hold the
    *                results
    * @param n       size of a, i.e., number of elements to read
    */
   template<typename T>
   void readbin(istream& is, T* a, Uint n) {
      safe_readbin_array(is, a, n, typename movable_trait_t<T>::safe_t());
   }

   /**
    * Overload: Write an array in Binary mode.
    * @param os   stream to write to
    * @param a    pointer to array of values to write in binary format
    * @param n    size of a, i.e., number of elements to write
    */
   template<typename T>
   void writebin(ostream& os, T* a, Uint n) {
      safe_writebin_array(os, a, n, typename movable_trait_t<T>::safe_t());
   }

} // BinIO

} // Portage

#endif // PORTAGE_BINIO_H
