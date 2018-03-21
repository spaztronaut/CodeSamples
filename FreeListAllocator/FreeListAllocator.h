#ifndef _BB_FREELIST_ALLOCATOR_H_ // [ _BB_FREELIST_ALLOCATOR_H_
#define _BB_FREELIST_ALLOCATOR_H_

#include "engine/memory/Allocator.h"

namespace bbengine
{
    namespace mem
    {
        class FreeListAllocator : public Allocator
        {
        public:

            FreeListAllocator( u32 heapSize );
            ~FreeListAllocator( );

            virtual void*   Allocate( u32 numBytes );
            virtual void*   AllocateAligned( u32 numBytes, const align_t alignment );
            virtual void    Free( void* ptr );
            virtual u32     GetBlockSize( void* ptr );

        private:

            FreeListAllocator( FreeListAllocator& );

            struct block_s
            {
                block_s*    next;
                u32         size;   // lowest order bit used as a "free" flag. since
                                    // sizes are only ever going to be 8 byte aligned
                                    // there will be unused lower order bits. bit
                                    // is set to 1 if in use and 0 if free
            };

            void*       m_heap;         // ptr to internal memory used for allocations
            block_s*    m_firstFree;    // head of list of address-ordered free blocks
        };
    }
}



#endif // ] _BB_FREELIST_ALLOCATOR_H_
