/**
 * @author Eric Joanis
 * @file block_mem_pool.h  Simple memory pool with block allocated memory.
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
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __BLOCK_MEM_POOL_H__
#define __BLOCK_MEM_POOL_H__

#include "portage_defs.h"

/// add this flag at compile to change DEFAULT_BLOCK_SIZE_IN_BLOCK_MEM_POOL
/// -DDEFAULT_BLOCK_SIZE_IN_BLOCK_MEM_POOL=500
#ifndef DEFAULT_BLOCK_SIZE_IN_BLOCK_MEM_POOL
/// Maximum allowed objects per Block (at compile time)
#define DEFAULT_BLOCK_SIZE_IN_BLOCK_MEM_POOL 1024
#endif

namespace Portage {


/**
 * This memory pool is intended for data that is typically allocated in many
 * individual allocations but deleted all at once.  Using this pool will result
 * is significant reduction of memory usage of your program, because of the
 * elimination of most of the overhead malloc uses to track things.
 *
 * A free list is provided for completeness, but it is very inefficient to free
 * each element individually.  If you can, you should call clear() instead, or
 * just let the OS clean up.
*/
template <class T>
class BlockMemPool {

 public:

   /// Constructor
   BlockMemPool() : first(NULL), free_list(NULL) {}

   /// Destructor
   ~BlockMemPool() { clear(); }

   /// Allocate a new T object
   T* alloc();

   /**
    * Clear all previously allocated objects.  Make sure you are no longer
    * using them!!!
    */
   void clear();

   /**
    * Use this only if you plan to release a few objects and reuse them later.
    * These will not really be released until you call clear().
    */
   void release(T* t);

 private:

   static const Uint BlockSize;  ///< Number of object of type T in a Block.
   /**
    * Memory blocks - these save us tons of malloc overhead and are the point
    * of this memory pool.
    */
   class Block {
      Uint next_unused;   ///< Next free object in this block.
      Block* next;        ///< Next block in the list of blocks.
      T pool[BlockSize];  ///< Array of objects of type T.
    public:
      /// Constructor.
      Block(Block* next) : next_unused(0), next(next) {}
      /// Destructor.
      ~Block() { if ( next ) delete next; next = NULL; }
      /// Verify if all the objects from the block are used.
      bool is_full() const { return next_unused >= BlockSize; }
      /// Get the next object from the pool.
      T* get_next() { return &(pool[next_unused++]); }
   };
   Block* first;   ///< Pointer to the newest allocated Block.

   /**
    * Zero-overhead free list structure - usability sugar, not a core
    * functionality of this pool.
    */
   struct falsePtr {
      falsePtr* next;  ///< Pointer to the next block.
   };
   falsePtr* free_list;  ///< List of free objects.

   /// Make sure our falsePtr fits inside T!
   BOOST_STATIC_ASSERT(sizeof(T) >= sizeof(falsePtr));
   /// And make sre the pointer inside falsePtr will be alligned.
   BOOST_STATIC_ASSERT(sizeof(T) % sizeof(falsePtr*) == 0);

}; // BlockMemPool

template <class T>
const Uint BlockMemPool<T>::BlockSize = DEFAULT_BLOCK_SIZE_IN_BLOCK_MEM_POOL;

template <class T>
T* BlockMemPool<T>::alloc() { 
   if ( free_list ) {
      // return the first free element, doing in place construction
      falsePtr* ptr = free_list;
      free_list = free_list->next;
      return new(reinterpret_cast<void*>(ptr)) T;
   } else {
      if ( !first || first->is_full() )
         first = new Block(first);
      // No need to do in place construction, because T's contructor is
      // called when we allocated the block.
      return first->get_next();
   }
}

template <class T>
void BlockMemPool<T>::clear() {
   if ( first ) delete first;
   first = NULL;
}

template <class T>
void BlockMemPool<T>::release(T* t) {
   t->~T();
   falsePtr* p = reinterpret_cast<falsePtr*>(t);
   p->next = free_list;
   free_list = p;
}


}; // Portage

#endif //  __BLOCK_MEM_POOL_H__


