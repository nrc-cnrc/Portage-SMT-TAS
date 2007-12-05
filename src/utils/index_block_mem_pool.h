/**
 * @author Eric Joanis
 * @file index_block_mem_pool.h  Simple memory pool with block allocated memory
 *                          and small index substitute for pointers.
 * $Id$
 *
 * This memory pool is intended for data that is typically allocated in many
 * individual allocations but deleted all at once.  Using this pool will result
 * is significant reduction of memory usage of your program, because of the
 * elimination of most of the overhead malloc uses to track things.
 *
 * A free list is provided for completeness, but it is very inefficient to free
 * each element individually.  If you can, you should call clear() instead, or
 * just let the OS clean up.
 *
 * An integral index is provided for each pointer, for smaller storage than
 * full-size pointers.  Useful on 64 bit machines in particular.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __BLOCK_MEM_POOL_H__
#define __BLOCK_MEM_POOL_H__

//#define DEBUG_PORTAGE_INDEX_BLOCK_POOL
#ifdef DEBUG_PORTAGE_INDEX_BLOCK_POOL
   #define IBPDEBUG(expr) expr
#else
   #define IBPDEBUG(expr) 
#endif

#include "portage_defs.h"

namespace Portage {

namespace IndexBlockMemPoolConstants {
   const Uint BlockBits = 10;
   const Uint BlockSize = 1 << BlockBits;
   const Uint ElementMask = ~(Uint(0)) >> (8*sizeof(Uint)-BlockBits);
   const Uint BlockMask = ~ElementMask;
}

/**
 * This memory pool is intended for data that is typically allocated in many
 * individual allocations but deleted all at once.  Using this pool will result
 * is significant reduction of memory usage of your program, because of the
 * elimination of most of the overhead malloc uses to track things.
 *
 * A free list is provided for completeness, but it is very inefficient to free
 * each element individually.  If you can, you should call clear() instead, or
 * just let the OS clean up.
 *
 * An integral index is provided for each pointer, for smaller storage than
 * full-size pointers.  Useful on 64 bit machines in particular.
 */
template <class T>
class IndexBlockMemPool {

 public:

   /// Constructor.
   IndexBlockMemPool() : free_list(NULL) {}

   /// Destructor.
   ~IndexBlockMemPool() { clear(); }

   /**
    * Allocate a new T object.
    * The object will already have been constructed with T's default
    * constructor.
    * index will contain a value that can be used to recover the pointer again
    * using get_ptr().  These indices are consecutive, starting at 0, so if you
    * know you have fewer than 2^16 T objects, you can safely store them in an
    * unsigned short.
    */
   T* alloc(Uint& index);

   /**
    * Get the actual pointer for a given index.
    * @param  index
    */
   T* get_ptr(Uint index) const;

   /**
    * Clear all previously allocated objects.  Make sure you are no longer
    * using them!!!  Destroys each allocated T object.
    */
   void clear();

   /**
    * Destroy and release object at index.
    * Use this only if you plan to release a few objects and reuse them later.
    * These will not really be released until you call clear().
    */
   void release(Uint index);

   static const Uint NoIndex; ///< largest value - may never get used.

 private:

   /// Memory blocks - these save us tons of malloc overhead and are the point
   /// of this memory pool.
   class Block {
      Uint next_unused;  ///< Next unused memory unit in this block.
      /// List of memory units in this block
      T pool[IndexBlockMemPoolConstants::BlockSize];
    public:
      /// Constructor.
      Block() : next_unused(0) {}
      /// Destructor.
      ~Block() {}
      /// Checks if all the available units are taken.
      /// @return true if there is no available units from this block.
      bool is_full() const {
         return next_unused >= IndexBlockMemPoolConstants::BlockSize;
      }
      /**
       * Returns the next free memory unit.  Be careful to check if there is
       * some free unit by calling is_full().
       * @param index return the index of the return memory unit.
       * @return a free unit of memory.
       */
      T* get_next(Uint& index) {
         index = next_unused; return &(pool[next_unused++]);
      }
      /**
       * Retrieves a memory unit based on index.  Must have already been
       * retrieved.
       * @param index index of the memory unit to retrieve.
       * @return memory unit.
       */
      T* get_ptr(Uint index) const {
         assert(index < next_unused);
         return const_cast<T*>(&(pool[index]));
      }
   };
   vector<Block*> blocks;  ///< List of Blocks in this memory pool.

   /// Zero-overhead free list structure - usability sugar, not a core
   /// functionality of this pool
   struct falsePtr {
      falsePtr* next;   ///< Pointer to the next free block of memory.
      Uint index;       ///< Index of the memory unit and pool combined.
   };
   falsePtr* free_list;  ///< List of free memory Unit.

}; // IndexBlockMemPool

template <class T>
T* IndexBlockMemPool<T>::alloc(Uint& index) { 
   if ( free_list ) {
      // return the first free element, doing in place construction
      falsePtr* ptr = free_list;
      index = ptr->index;
      free_list = free_list->next;
      return new(reinterpret_cast<void*>(ptr)) T;
   } else {
      if ( blocks.empty() || blocks.back()->is_full() )
         blocks.push_back(new Block);
      // No need to do in place construction, because T's contructor is
      // called when we allocated the block.
      Uint element_index;
      T* ptr = blocks.back()->get_next(element_index);
      index = (Uint(blocks.size()-1) << IndexBlockMemPoolConstants::BlockBits)
               | element_index;
      if ( index == NoIndex )
         error(ETFatal, "Exceeded system limits in IndexBlockMemPool");
      IBPDEBUG(printf("ALLOC: index=%x element_index=%x block_index=%x\n",
                       index, element_index, Uint(blocks.size()-1)));
      return ptr;
   }
}

template <class T>
T* IndexBlockMemPool<T>::get_ptr(Uint index) const {
   using namespace IndexBlockMemPoolConstants;
   Uint element_index = index & ElementMask;
   Uint block_index = (index & BlockMask) >> BlockBits;
   assert(block_index < blocks.size());
   IBPDEBUG(printf("GET_PTR: index=%x element_index=%x block_index=%x\n",
                    index, element_index, block_index));
   IBPDEBUG(printf("ElementMask=%x BlockMask=%x BlockBits=%d\n",
                    ElementMask, BlockMask, BlockBits));
   return blocks[block_index]->get_ptr(element_index);
}

template <class T>
void IndexBlockMemPool<T>::clear() {
   // Problem: if there are elements on the free list, they already had their
   // destructor called, so this will destroy them a second time, resulting in
   // unpredictable behaviour.  To avoid this, we must re-allocate all the
   // freed elements (thus calling their constructors), before clearing the
   // blocks.
   Uint dummy_index;
   while ( free_list ) alloc(dummy_index);

   // Now we can safely delete all the blocks.
   for ( typename vector<Block *>::iterator it = blocks.begin();
         it != blocks.end(); ++it )
      delete *it;
   blocks.resize(0);
}

template <class T>
void IndexBlockMemPool<T>::release(Uint index) {
   T* t_ptr = get_ptr(index);
   t_ptr->~T();
   falsePtr* p = reinterpret_cast<falsePtr*>(t_ptr);
   p->next = free_list;
   p->index = index;
   free_list = p;
}

template <class T>
const Uint IndexBlockMemPool<T>::NoIndex = ~(Uint(0));

} // Portage

#endif //  __BLOCK_MEM_POOL_H__


