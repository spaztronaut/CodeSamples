#include "engine/memory/FreeListAllocator.h"
#include "engine/system/Assert.h"
#include <stdlib.h>

namespace bbengine
{
    namespace mem
    {
        #define FREE_BIT_MASK           0x01u
        #define IS_BLOCK_FREE(block)    !( block->size & FREE_BIT_MASK )
        #define ALIGNED_HEADER_SIZE     ( MemUtils_Align( sizeof( FreeListAllocator::block_s ), ALIGN_8 ) )
        #define MIN_ALLOC_SIZE          ( ALIGNED_HEADER_SIZE + ALIGNED_HEADER_SIZE )


        /*====================================================================

            FreeListAllocator::FreeListAllocator
            - allocates memory buffer based on heapSize
            - initializes internal free list

            TODO:
            - Allocate internal memory block from a parent custom allocator
              instead of using malloc and free

        ====================================================================*/
        FreeListAllocator::FreeListAllocator( u32 heapSize )
        {
            m_heap = malloc( heapSize );

            m_firstFree = ( block_s* )MemUtils_Align( ( u32 )m_heap, ALIGN_8 );
            m_firstFree->next = NULL;
            m_firstFree->size = heapSize - ALIGNED_HEADER_SIZE -
                                ( ( u32 )m_firstFree - ( u32 )m_heap );
        }


        /*====================================================================

            FreeListAllocator::~FreeListAllocator
            - releases memory held by internal buffer

        ====================================================================*/
        FreeListAllocator::~FreeListAllocator()
        {
            free( m_heap );
            m_heap = NULL;
        }


        /*====================================================================

            FreeListAllocator::Allocate( u32 numBytes)
            - Allocate 8-byte aligned memory of numBytes size.
            - @return: returns pointer to memory aligned block

        ====================================================================*/
        void* FreeListAllocator::Allocate( u32 numBytes )
        {
            return AllocateAligned( numBytes, ALIGN_8 );
        }


        /*====================================================================

            FreeListAllocator::AllocateAligned( u32 numBytes, const align_t alignment)
            - Allocate aligned memory of numBytes size.
            - @return: returns pointer to memory aligned block

            TODO:
            - Add calls to "Out of Memory" routines

        ====================================================================*/
        void* FreeListAllocator::AllocateAligned( u32 numBytes, const align_t alignment )
        {
            u32 sizeNeeded = numBytes;

            // make sure allocation is at least the size of block header.
            // should be using another allocator ( ie SlabAllocator ) for
            // smaller allocations.
            if( sizeNeeded < ALIGNED_HEADER_SIZE )
            {
                sizeNeeded = ALIGNED_HEADER_SIZE;
            }

            // make sure the requested allocation size is aligned and at
            // least 8 bytes + the aligned size of the block header
            sizeNeeded = MemUtils_Align( sizeNeeded, alignment ) + ALIGNED_HEADER_SIZE;

            block_s* prevBlock = NULL;
            block_s* block = m_firstFree;

            // iterate through all the free list blocks until one with enough
            // space is found. this block will be used for the allocation. Uses
            // a First Fit Policy
            while( block )
            {
                if( sizeNeeded <= block->size )
                {
                    break;
                }

                prevBlock = block;
                block = block->next;
            }

            if( block == NULL )
            {
                // No blocks large enough to fit memory request
                return NULL;
            }

            DEBUG_ASSERT( IS_BLOCK_FREE(block) && "Trying to allocate from a block of memory that is already in use" );

            // check to see if another allocation can be made after this one
            if( sizeNeeded + MIN_ALLOC_SIZE <= block->size )
            {
                // split the free block
                block_s* newBlock = ( block_s* )( ( byte* )block + sizeNeeded );
                // link the new free block into the free list
                newBlock->next = block->next;
                newBlock->size = block->size - sizeNeeded;

                // begin removing block from the free list. this is half of it,
                // need prevBlock for the other half of the removal process
                block->next = newBlock;
                // update the size of the block, taking into account the number
                // of bytes needed for the header of the block
                block->size = sizeNeeded - ALIGNED_HEADER_SIZE;
            }

            if( prevBlock )
            {
                // complete inserting any new blocks into the free list and
                // remove the current block from the free list
                prevBlock->next = block->next;
            }
            else
            {
                // if a previous block wasnt found bound on memory address, then
                // the first free block was grabbed from the list and the head
                // of the list now needs to be updated
                m_firstFree = m_firstFree->next;
            }

            block->next = NULL;

            // flag the block as being used
            block->size |= FREE_BIT_MASK;

            void* ret = ( void* )( ( byte* )block + ALIGNED_HEADER_SIZE );

            return ret;
        }


        /*====================================================================

            FreeListAllocator::Free( void* ptr )
            - frees the specified block of memory and returns it to the internal
              free list
            - coalesces/joins adjacent free blocks of memory
            - sorts free blocks of memory based on address

            TODO:
            - Can attempt to validate ptr. At the moment, nothing is preventing
              the user from trying to free an invalid ptr (ie Check that it is
              aligned, add additional meta-data for tracking, byte patterns)
            - Fail an assertion if trying to free a NULL pointer
            - Fail an assertion if trying to free a block of memory that has
              already been freed

        ====================================================================*/
        void FreeListAllocator::Free( void* ptr )
        {
            if ( ptr == NULL )
            {
                // trying to free a NULL ptr
                return;
            }

            // get the block header for the ptr
            block_s* block = ( block_s* )( ( byte* )ptr - ALIGNED_HEADER_SIZE );

            if ( IS_BLOCK_FREE(block) )
            {
                // block has already been freed
                return;
            }

            // flag the block as being free
            block->size = block->size & ~FREE_BIT_MASK;

            // add block to free list and perform coalescense
            block_s* prevBlock = NULL;
            block_s* nextBlock = m_firstFree;

            // find adjacent blocks based on memory address
            while( nextBlock && nextBlock < block )
            {
                prevBlock = nextBlock;
                nextBlock = nextBlock->next;
            }

            if( prevBlock )
            {
                prevBlock->next = block;

                // check to see if prevBlock and the current block are adjacent
                u32 nextAddr = ( u32 )prevBlock + prevBlock->size + ALIGNED_HEADER_SIZE;

                if( nextAddr == ( u32 )block )
                {
                    // combine the two blocks
                    prevBlock->size += block->size + ALIGNED_HEADER_SIZE;
                    prevBlock->next = nextBlock;
                    // update the block as a whole so we can join with nextBlock if needed
                    block = prevBlock;
                }
            }
            else
            {
                // if a prevBlock wasn't found, this block that is currently being freed
                // is at a lower memory address than all other free blocks and should be
                // at the front of the free list
                m_firstFree = block;
            }

            if( nextBlock )
            {
                block->next = nextBlock;

                // check to see if the current block and nextBlock are adjacent
                u32 nextAddr = ( u32 )block + block->size + ALIGNED_HEADER_SIZE;

                if( nextAddr == ( u32 )nextBlock )
                {
                    // combine the two blocks
                    block->size += nextBlock->size + ALIGNED_HEADER_SIZE;
                    block->next = nextBlock->next;
                }
            }
        }


        /*====================================================================

            FreeListAllocator::GetBlockSize( void* ptr )
            - @return: size of specified block of memory

        ====================================================================*/
        u32 FreeListAllocator::GetBlockSize( void* ptr )
        {
            DEBUG_ASSERT( ptr != NULL && "Trying to get size of a NULL ptr" );

            // get pointer to associated block header
            block_s* block = ( block_s* )( ( byte* )ptr - ALIGNED_HEADER_SIZE );

            return block->size & ~FREE_BIT_MASK;
        }
    }
}
