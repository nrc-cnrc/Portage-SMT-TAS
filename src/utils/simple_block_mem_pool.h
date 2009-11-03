/**
 * @author Eric Joanis, based on simple_mem_pool by Samuel Larkin
 * @file simple_block_mem_pool.h  Memory pool allocated in blocks.
 * $Id$
 *
 * SimpleBlockMemPool for same size and same type objects allocated in blocks.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */
                    
#ifndef __SIMPLE_BLOCK_MEM_POOL_H__
#define __SIMPLE_BLOCK_MEM_POOL_H__

#include <boost/static_assert.hpp>

namespace Portage {

/// Memory pool where memory is allocated in Blocks rather then per objects.
template<class T>
class SimpleBlockMemPool
{
   private:
      /// Structure used for linking the memory Unit.
      struct falsePtr
      {
         falsePtr* next;  ///< Pointer to the next free object.
      };
      falsePtr* free_list;  ///< List of free objects.

      /// Memory blocks - these save us tons of malloc overhead and are the
      /// point of this variant of the memory pool concept
      static const Uint BlockSize;
      class Block {
         /// Index of the next unused memory unit in the Block.
         Uint next_unused;
         Block* next;  ///< Pointer to the next Block.
         char pool[sizeof(T) * BlockSize];  ///< List of memory unit.
       public:
         /// Constructor.
         Block(Block* next) : next_unused(0), next(next) {}
         /// Destructor.
         ~Block() { if ( next ) delete next; next = NULL; }
         /// Checks if there is some free memory unit in the block.
         /// @return Returns true if there is some free memory unit in the
         /// block.
         bool is_full() const { return next_unused >= BlockSize; }
         /// Gets the next free memory unit.  Be sure to check for
         /// availability first with is_full().
         /// @return Returns a free memory unit.
         void* get_next() { return &(pool[sizeof(T)*(next_unused++)]); }
      };
      Block* first;  ///< List of memory Blocks

      /// Make sure our falsePtr fits inside T!
      BOOST_STATIC_ASSERT(sizeof(T) >= sizeof(falsePtr));
      /// And make sre the pointer inside falsePtr will be alligned
      BOOST_STATIC_ASSERT(sizeof(T) % sizeof(falsePtr*) == 0);

   public:
      /// Constructor.
      SimpleBlockMemPool()
      : free_list(NULL)
      , first(NULL)
      { }

      /// Destructor.
      ~SimpleBlockMemPool() { clear(); }

      /// Clears the Blocks of the memory pool.
      void clear() {
         if ( first ) delete first;
         first = NULL;
         free_list = NULL;
      }

      /**
       * Gets a memory unit.  Tries to get a memory unit from the free list if
       * there is any available, if none are available then we create memory
       * for it.
       * @return Returns a memory unit.
       */
      void* malloc() {
         if (free_list == NULL) {
            // Get a new Unit off the first allocation block
            if ( !first || first->is_full() )
               first = new Block(first);
            return first->get_next();
         } else {
            // We have at least one Unit already created thus return it.
            falsePtr* p = free_list;
            free_list = free_list->next;
            return reinterpret_cast<void*>(p);
         }
      }

      /**
       * Deletes a memory unit.  Actually returns the memory unit to a free
       * list so we can reuse it later.
       * @param t memory unit to release.
       */ 
      void free(void* t) {
         falsePtr* p = reinterpret_cast<falsePtr*>(t);
         p->next = free_list;
         free_list = p;
      }
}; // ends class memPool

template <class T>
const Uint SimpleBlockMemPool<T>::BlockSize = 1024;

} // end namespace Portage

/// Macro to convert class_name to use SimpleBlockMemPool as class_name memory
/// allocator.
#define SIMPLEBLOCKMEMPOOL_DECLARATION(class_name)\
private:\
   static SimpleBlockMemPool<class_name> mp;\
public:\
   void* operator new(size_t s) { return mp.malloc(); }\
   void operator delete(void* p) { mp.free(p); }\
   static void delete_all() { mp.clear(); } \
private:

#define SIMPLEBLOCKMEMPOOL_INSTANTIATION(class_name) SimpleBlockMemPool<class_name> class_name::mp;

#endif // __SIMPLE_BLOCK_MEM_POOL_H__

