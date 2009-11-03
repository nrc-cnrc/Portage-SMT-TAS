/**
 * @author Eric Joanis
 * @file array_mem_pool.h  Simple memory pool for arrays.
 * $Id$
 *
 * This class is intended for use with small arrays that will be frequently
 * allocated and deallocated, as can be expected for variable sized arrays.
 * It allocates small sized arrays in larger blocks, with a free-list to
 * reassign freed small arrays before using new ones again.
 *
 * Only small sized arrays are allocated this way, larger ones are
 * transparently allocated with new [] and freed with delete [], since their
 * malloc overhead is much less significant.  The treshold is a template
 * parameter.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __ARRAY_MEM_POOL_H__
#define __ARRAY_MEM_POOL_H__

#include "portage_defs.h"
#include <boost/static_assert.hpp>

namespace Portage {

/// Utility template to calculate the appropriate allocation multiple
/// Value not explicitly specialized here are conditions under which
/// AllocMultiple cannot be correctly calculated.
template<int MODULO> struct CalcAllocMultiple { enum {novalue = 0}; };
template<> struct CalcAllocMultiple<0> { enum {value = 1}; };
template<> struct CalcAllocMultiple<1> { enum {value = sizeof(void*)}; };
template<> struct CalcAllocMultiple<2> { enum {value = sizeof(void*)/2}; };
template<> struct CalcAllocMultiple<4> { enum {value = sizeof(void*)/4}; };

/**
 * Simple memory pool for arrays.  This class is intended for use with small
 * arrays that will be frequently allocated and deallocated, as can be expected
 * for variable sized arrays.  It allocates small sized arrays in larger
 * blocks, with a free-list to reassign freed small arrays before using new
 * ones again.
 */
template<
   class T,             // Type of object to pool
   unsigned Max = 32,   // Largest array size to get from pool; larger arrays
                        // will be alloc'd using new[] and freed using delete[]
   unsigned BlockSize = 1024 // number of elements to allocate in each block
>
class ArrayMemPool {

 public:

   /**
    * Get a new array of size elements.
    * actual_size will contain the actual allocated size, usually size, unless
    * integrity restrictions force the pool to allocate more.
    * @param size  Requested array size
    * @param[out] actual_size  If non-null, will be actual size of ret'd array
    * @return  Returns a memory unit from the pool
    */
   T* alloc_array(Uint size, Uint* actual_size = NULL);

   /**
    * Get a new *uninitialized* array of size elements.
    * Use this faster method if you're going to re-assign all values anyway.
    * @param size  Requested array size
    * @param[out] actual_size  If non-null, will be actual size of ret'd array
    * @return  Returns a memory unit from the pool
    */
   T* alloc_array_no_ctor(Uint size, Uint* actual_size = NULL);

   /**
    * Free an array of size elements.
    * Unlike delete[], we require that you remember the size to_free had when
    * you first called alloc_array.  You may use size or actual_size.
    * @param to_free  Pointer ot the memory unit to free
    * @param size  size of the array to free
    */
   void free_array(T* to_free, Uint size);

   /**
    * Free to_free, without calling destructors on all the elements.
    * Use this faster method if you don't need destructors called.
    * @param to_free  Pointer ot the memory unit to free
    * @param size  size of the array to free
    */
   void free_array_no_dtor(T* to_free, Uint size);

   /// Constructor.
   ArrayMemPool();

   /// Destructor.
   ~ArrayMemPool() { clear(); }

   /**
    * Clears all the memory allocated in the pool's blocks - make sure this is
    * done after the structures are no longer used!
    * Note: previously existing memory leaks have been fixed - this method
    * correctly clears all memory allocated via the mem pool.
    */
   void clear();

 private:

   /// Block of memory, where they come from the first time they are allocated.
   struct Block {
      Uint next_unused; ///< index of the next free unit in this Block
      Block * next;     ///< Pointer to the next Block

      // pool is intentionally after a word-aligned pointer type: this way pool
      // itself is alligned on the word boundary, which is necessary for many
      // types T.
      char pool[sizeof(T) * BlockSize];  ///< Block's list of Unit
      /// Constructor.
      /// @param next pointer to the next Block
      Block(Block* next) : next_unused(0), next(next) {}
      /// Get the number of free units in the Block.
      /// @return Returns the number of free unit in the Block.
      Uint free_slots() const { return BlockSize - next_unused; }
      void* get_next_array(Uint size);
   };

   Block* first_block;  ///< List of Blocks

   /// Array allocated outside a block, stored in a doubly linked list so we
   /// can manage freeing them individually (via free_array*()) or globally
   /// (via clear()).
   /// Just like falsePtr, we overlay this on a char[] allocation.
   struct IndividualArray {
      /// Previous individual array allocated
      IndividualArray* prev;
      /// Next individual array allocated
      IndividualArray* next;
   };

   /// List of individually allocated arrays
   IndividualArray* first_individual_array;

   /// Free list structure - called false pointer because we reinterpret cast
   /// a T* to a falsePtr when a T[] is put on a free list.
   struct falsePtr {
      falsePtr* next;  ///<  Next free memory unit in the free list
   };

   /**
    * Alignment size.
    * In order to make sure a falsePtr can be placed at the beginning of any
    * allocated array, we must guarantee two things:
    *  1- the smallest allocation is as large as sizeof(falsePtr), and
    *  2- each allocation is aligned on a sizeof(falsePtr) boundary.
    * To do so, we round any requested allocation to AllocMultiple, which is
    * calculated in a way to satisfy 1 and 2 above.
    */
   static const Uint AllocMultiple =
      CalcAllocMultiple<sizeof(T) % sizeof(void*)>::value;

   /// given size, calculate an allowable allocation size.
   static Uint adjust_size(Uint size);

   /// The free lists themselves.  There has to be Max/AllocMultiple of them,
   /// one for each size of block we can generate.
   falsePtr * free_lists[Max/AllocMultiple];

   /// This static assert validates that the alignment of pool in Block is OK.
   typedef struct { char spacer; T t_val; } T_AllignmentTest;
   //BOOST_STATIC_ASSERT(0 == offsetof(struct Block, pool)
   //                         % offsetof(struct T_AllignmentTest, t_val));

   // Make sure a falsePtr fits in a T
   BOOST_STATIC_ASSERT(sizeof(T)*AllocMultiple >= sizeof(falsePtr*));

   /// Make sure BlockSize and Max are in at least the minimally acceptable
   /// ratio of 10:1, 
   BOOST_STATIC_ASSERT(Max * 10 <= BlockSize);

}; // class ArrayMemPool



#include "array_mem_pool-cc.h"

} // namespace Portage


#endif // __ARRAY_MEM_POOL_H__

