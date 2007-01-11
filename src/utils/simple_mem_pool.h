/**
 * @author Samuel Larkin
 * @file simple_mem_pool.h  A memory pool for same size and same type objects.
 * $Id$
 *
 * Memory pool that keeps a list of freed object and allocated one object at a time.
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 */
                    
#ifndef __SIMPLE_MEM_POOL_H__
#define __SIMPLE_MEM_POOL_H__

#include <boost/static_assert.hpp>

// add this flag at compile to change DEFAULT_MAX_MEM_POOL_SIZE
// -DDEFAULT_MAX_MEM_POOL_SIZE=500
#ifndef DEFAULT_MAX_SIMPLE_MEM_POOL_SIZE
#define DEFAULT_MAX_SIMPLE_MEM_POOL_SIZE 60000  ///< Maximum allowed free list size constant (at compile time)
#endif

namespace Portage {

/// Memory pool that keeps a list of freed object and allocated one object at a time.
template<class T>
class SimpleMemPool
{
   private:
      /// Structure used for linking the memory Unit
      struct falsePtr
      {
         falsePtr* next;  ///< next free unit
      };
      falsePtr* pList;  ///< free list

      /// Make sure our falsePtr fits inside T!
      BOOST_STATIC_ASSERT(sizeof(T) >= sizeof(falsePtr));
      /// And make sre the pointer inside falsePtr will be alligned
      BOOST_STATIC_ASSERT(sizeof(T) % sizeof(falsePtr*) == 0);

      // memory Unit count
      const unsigned int m_max_free_count;  ///< maximum allowed free list size
      unsigned int m_free_count;            ///< current free list size
      
   public:
      /**
       * Constructor.
       * @param max maximum allowed free list size
       */
      SimpleMemPool(const unsigned int max = DEFAULT_MAX_SIMPLE_MEM_POOL_SIZE)
      : pList(0)
      , m_max_free_count(max)
      , m_free_count(0)
      { }

      /// Destructor.  Frees the free list ;)
      ~SimpleMemPool()
      {
         while (pList != 0)
         {
            falsePtr* p = pList;
            pList = pList->next;
            std::free((T*)p);  // Since we used malloc we MUST use free.
            --m_free_count;
         }
         assert(m_free_count == 0);
      }

      /**
       * Gets one unit of memory.  Returns a free unit from the free list or
       * allocates one if none is available.
       */
      void* malloc()
      {
         if (pList == 0)
         {
            // Creates one unit if none is present in our link list.
            // We cannot use the operator new or else we'll end up in a
            // recusive loop of calls to operator new.
            return std::malloc(sizeof(T));
         }
         else
         {
            // We have at least one Unit already created thus return it.
            falsePtr* p = pList;
            pList = pList->next;
            return reinterpret_cast<void*>(p);
         }
      }

      /**
       * Frees a memory unit belonging to this memory pool.  Adds the memory
       * unit to the free list unless the maximum of allowed free memory
       * blocks have been reach in which case the unit is deleted.
       *
       * @param t unit to free
       */
      void free(void* t)
      {
         // For a reason of memory usage limitation we keep count of how many
         // Unit we have.  We don't want more then m_max_free_count of Unit at
         // one time.
         if (m_free_count >= m_max_free_count)
         {
            std::free(t);
         }
         else
         {
            falsePtr* p = reinterpret_cast<falsePtr*>(t);
            p->next = pList;
            pList = p;
            ++m_free_count;
         }
      }
}; // ends class memPool
}; // end namespace Portage

/**
 * Definition to convert class to manage its creation with a simple memory pool.
 * Helper macro that allows a class to override and automatically use
 * a particular pool of memory when new and delete are invoked.
 */
#define SIMPLEMEMPOOL_DECLARATION(class_name)\
private:\
   static SimpleMemPool<class_name> mp;\
public:\
   void* operator new(size_t s) { return mp.malloc(); }\
   void operator delete(void* p) { mp.free(p); }

/// Declaration to convert class to manage its creation with a simple memory pool.
#define SIMPLEMEMPOOL_INSTANTIATION(class_name) SimpleMemPool<class_name> class_name::mp;

#endif // __SIMPLE_MEM_POOL_H__

