/**
 * @author Eric Joanis
 * @file array_mem_pool-cc.h  Implementation of simple memory pool for arrays.
 * $Id$
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __ARRAY_MEM_POOL_CC_H__
#define __ARRAY_MEM_POOL_CC_H__

template<class T, unsigned Max, unsigned BlockSize>
T* ArrayMemPool<T,Max,BlockSize>::alloc_array(
   Uint size, Uint* actual_size_p
) {
   return new(alloc_array_no_ctor(size, actual_size_p)) T[size];
}

template<class T, unsigned Max, unsigned BlockSize>
T* ArrayMemPool<T,Max,BlockSize>::alloc_array_no_ctor(
   Uint size, Uint * actual_size_p
) {
   Uint dummy_actual_size(0);
   Uint& actual_size(actual_size_p ? *actual_size_p : dummy_actual_size);
   if ( size > Max ) {
      actual_size = size;

      char* storage = new char[sizeof(IndividualArray) + size * sizeof(T)];
      IndividualArray* header(reinterpret_cast<IndividualArray*>(storage));
      char* data(storage+sizeof(IndividualArray));

      if ( first_individual_array ) {
         assert(first_individual_array->prev == NULL);
         first_individual_array->prev = header;
      }
      header->next = first_individual_array;
      header->prev = NULL;
      first_individual_array = header;

      return reinterpret_cast<T*>(data);
   } else {
      actual_size = adjust_size(size);
      assert(actual_size >= size);
      assert(actual_size % AllocMultiple == 0);

      const Uint free_list_index = actual_size/AllocMultiple - 1;
      if ( free_lists[free_list_index] != NULL ) {
         // return the first free array from the free list
         falsePtr* ptr = free_lists[free_list_index];
         free_lists[free_list_index] =
            free_lists[free_list_index]->next;
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
   // Destructors must be called manually in all cases:
   //   - if size > Max, the delete[] call has to be on the same type the
   //     new[] call was made, i.e., on char*;
   //   - if size <= Max, we don't call delete at all.
   for ( Uint i = 0; i < size; ++i )
      to_free[i].~T();

   free_array_no_dtor(to_free, size);
}

template<class T, unsigned Max, unsigned BlockSize>
void ArrayMemPool<T,Max,BlockSize>::free_array_no_dtor(T* to_free, Uint size)
{
   if ( size > Max ) {
      assert(first_individual_array);
      char* data(reinterpret_cast<char*>(to_free));
      char* storage(data - sizeof(IndividualArray));
      IndividualArray* header(reinterpret_cast<IndividualArray*>(storage));

      if ( header->next ) {
         assert(header->next->prev == header);
         header->next->prev = header->prev;
      }
      if ( header->prev ) {
         assert(header->prev->next == header);
         header->prev->next = header->next;
      } else {
         assert(first_individual_array == header);
         first_individual_array = header->next;
      }

      delete [] storage;
   } else {
      size = adjust_size(size);

      // Put to_free on the appropriate free list.
      falsePtr* free = reinterpret_cast<falsePtr*>(to_free);
      free->next = free_lists[size/AllocMultiple-1];
      free_lists[size/AllocMultiple-1] = free;
   }
}

template<class T, unsigned Max, unsigned BlockSize>
ArrayMemPool<T,Max,BlockSize>::ArrayMemPool() 
   : first_block(NULL)
   , first_individual_array(NULL)
{
   for ( Uint i = 0; i < Max/AllocMultiple; ++i ) free_lists[i] = NULL;
   assert((AllocMultiple * sizeof(T)) % sizeof(falsePtr*) == 0);
   assert(AllocMultiple * sizeof(T) >= sizeof(falsePtr));
   assert(AllocMultiple > 0);
   // This assertion should logically be a BOOST_STATIC_ASSERT, but I don't
   // know how to do that with a constant that has to be calculated first, so
   // we assert it here instead.
   assert(Max % AllocMultiple == 0);
}

template<class T, unsigned Max, unsigned BlockSize>
void ArrayMemPool<T,Max,BlockSize>::clear()
{
   while ( first_block ) {
      Block* block = first_block->next;
      delete first_block;
      first_block = block;
   }
   assert(first_block == NULL);
   for ( Uint i = 0; i < Max/AllocMultiple; ++i ) free_lists[i] = NULL;
   while ( first_individual_array ) {
      char* storage(reinterpret_cast<char*>(first_individual_array));
      first_individual_array = first_individual_array->next;
      delete [] storage;
   }
   assert(first_individual_array == NULL);
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

