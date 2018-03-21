#ifndef _BB_ALLOCATOR_H_ // [ _BB_ALLOCATOR_H_
#define _BB_ALLOCATOR_H_

#include "engine/system/System.h"
#include "engine/memory/MemoryUtils.h"

namespace bbengine
{
    namespace mem
    {
        // Base Allocator class. ALL Allocators must inherit from and implement Allocator
        class Allocator
        {
        public:
            // allocate a block of memory with 8-byte alignment
            virtual void*   Allocate( u32 numBytes ) = 0;
            // allocate a block of memory with a specific alignment
            virtual void*   AllocateAligned( u32 numBytes, const align_t alignment ) = 0;
            // free the block of memory associated with ptr
            virtual void    Free( void* ptr ) = 0;
            // returns the size of the block of memory that ptr points to
            virtual u32     GetBlockSize( void* ptr ) = 0;
        };
    }
}


#endif // ] _BB_ALLOCATOR_H_
