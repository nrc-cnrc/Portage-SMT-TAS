/**
 * @author Eric Joanis
 * @file array_mem_pool-cc.h  Implementation of simple memory pool for arrays.
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __ARRAY_MEM_POOL_CC_H__
#define __ARRAY_MEM_POOL_CC_H__

template<class T, unsigned Max, unsigned BlockSize>
const Uint ArrayMemPool<T,Max,BlockSize>::AllocMultiple =
   (sizeof(T) % sizeof(falsePtr*) == 0) ? 1 :
   (sizeof(T) % sizeof(falsePtr*) == 1) ? sizeof(falsePtr*) :
   (sizeof(T) % sizeof(falsePtr*) == 2) ? sizeof(falsePtr*) / 2 :
   (sizeof(T) % sizeof(falsePtr*) == 4) ? sizeof(falsePtr*) / 4 :
   0; // Can never happen since we asserted 0,1,2,4 were the possible values

template<class T, unsigned Max, unsigned BlockSize>
T* ArrayMemPool<T,Max,BlockSize>::alloc_array(Uint size, Uint & actual_size)
{
   return new(alloc_array_no_ctor(size, actual_size)) T[size];
}

template<class T, unsigned Max, unsigned BlockSize>
T* ArrayMemPool<T,Max,BlockSize>::alloc_array_no_ctor(
   Uint size, Uint & actual_size
) {
   if ( size > Max ) {
      actual_size = size;
      return reinterpret_cast<T*>(new char[size * sizeof(T)]);
   } else {
      actual_size = adjust_size(size);
      assert(actual_size >= size);

      if ( free_lists[actual_size-1] != NULL ) {
         // return the first free array from the free list
         falsePtr* ptr = free_lists[actual_size-1];
         free_lists[actual_size-1] = free_lists[actual_size-1]->next;
         return reinterpret_cast<T*>(ptr);
      } else {
         // Allocate a new block if there isn't enough space on the first one
         if ( ! first_block || first_block->free_slots() < actual_size ) {
            // Put the odd-sized block on the free-list, so we don't lose it
            Uint free_slots = first_block ? first_block->free_slots() : 0;
            if ( free_slots >= AllocMultiple ) {
               free_slots -= free_slots % AllocMultiple;
               free_array_no_dtor(
                  reinterpret_cast<T*>(first_block->get_next_array(free_slots)),
                  free_slots);
            }
            // And allocate a new block
            first_block = new Block(first_block);
         }

         // Get some memory from the block and return it.
         return reinterpret_cast<T*>(first_block->get_next_array(actual_size));
      }
   }
}

template<class T, unsigned Max, unsigned BlockSize>
void ArrayMemPool<T,Max,BlockSize>::free_array(T* to_free, Uint size)
{
   if ( size > Max ) {
      delete [] to_free;
   } else {
      size = adjust_size(size);

      // Since we're not going to call delete, we need to call T's
      // destructor explicitely for each element
      for ( Uint i = 0; i < size; ++i )
         to_free[i].~T();

      // Put to_free on the appropriate free list.
      falsePtr* free = reinterpret_cast<falsePtr*>(to_free);
      free->next = free_lists[size-1];
      free_lists[size-1] = free;
   }
}

template<class T, unsigned Max, unsigned BlockSize>
void ArrayMemPool<T,Max,BlockSize>::free_array_no_dtor(T* to_free, Uint size)
{
   if ( size > Max ) {
      delete [] reinterpret_cast<char*>(to_free);
   } else {
      size = adjust_size(size);

      // Put to_free on the appropriate free list.
      falsePtr* free = reinterpret_cast<falsePtr*>(to_free);
      free->next = free_lists[size-1];
      free_lists[size-1] = free;
   }
}

template<class T, unsigned Max, unsigned BlockSize>
ArrayMemPool<T,Max,BlockSize>::ArrayMemPool() 
   : first_block(NULL)
{
   for ( Uint i = 0; i < Max; ++i ) free_lists[i] = NULL;
   assert((AllocMultiple * sizeof(T)) % sizeof(falsePtr*) == 0);
   assert(AllocMultiple * sizeof(T) >= sizeof(falsePtr));
   assert(AllocMultiple > 0);
}

template<class T, unsigned Max, unsigned BlockSize>
void ArrayMemPool<T,Max,BlockSize>::clear()
{
   while ( first_block ) {
      Block* block = first_block->next;
      delete first_block;
      first_block = block;
   }
   for ( Uint i = 0; i < Max; ++i ) free_lists[i] = NULL;
}

template<class T, unsigned Max, unsigned BlockSize>
void* ArrayMemPool<T,Max,BlockSize>::Block::get_next_array(Uint size)
{
   assert(size <= free_slots());
   void* result = &(pool[next_unused*sizeof(T)]);
   next_unused += size;
   return result;
}

template<class T, unsigned Max, unsigned BlockSize>
Uint ArrayMemPool<T,Max,BlockSize>::adjust_size(Uint size) {
   // Handle size==0 gracefully
   if ( size < AllocMultiple ) return AllocMultiple;
   // Handle size not a multiple of AllocMultiple
   if ( (size % AllocMultiple) != 0 )
      return size + AllocMultiple - (size % AllocMultiple);
   return size;
}






#endif // __ARRAY_MEM_POOL_CC_H__

