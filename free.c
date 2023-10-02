#include "malloc.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>

static void
free_mem(const size_t malloc_size, page_t *mem)
{
    if (mem->prev)
        mem->prev->next = mem->next;
    else
    {
        if (malloc_size)
            g_malloc_pages.tiny = mem->next;
        else
            g_malloc_pages.small = mem->next;
    }
    if (mem->next)
        mem->next->prev = mem->prev;
    munmap(mem, MALLOC_ZONE * malloc_size);

}

static inline void
free_little(block_t *block, const size_t malloc_size, page_t *mem)
{
    if (block->prev)
        block->prev->next = block->next;
    else
        mem->alloc = block->next;
    if (block->next)
        block->next->prev = block->prev;
    block->prev = NULL;
    block->next = mem->free;
    if (mem->free)
        mem->free->prev = block;
    mem->free = block;
    if (!mem->alloc)
        free_mem(malloc_size, mem);
}

static inline void
free_large(block_t *block)
{
    const size_t msize = align(block->size + sizeof *block, MASK_0XFFF);

    if (block->prev)
        block->prev->next = block->next;
    else
        g_malloc_pages.large = block->next;
    if (block->next)
        block->next->prev = block->prev;
    munmap(block, msize);
}

static void
free_block(block_t *block)
{
    page_t *mem;
    size_t size;

    printf("FreeBlock: %p\n", block);
    switch ((block->size > TINY_ZONE) + (block->size > SMALL_ZONE)) {
        case MALLOC_TINY: {
            printf("FreeBlock: MALLOC_TINY\n");
            mem = g_malloc_pages.tiny;
            size = TINY_ZONE;
            break;
        }
        case MALLOC_SMALL: {
            printf("FreeBlock: MALLOC_SMALL\n");
            mem = g_malloc_pages.small;
            size = SMALL_ZONE;
            break;
        }
        default: {
            printf("FreeBlock: MALLOC_LARGE\n");
            free_large(block);
            return ;
        } 
    }
    while (!((void*)block < (void*)mem + MALLOC_ZONE * size && (void*)block > (void*)mem))
        mem = mem->next;
    free_little(block, size, mem);
}

void
free(void *p)
{
    pthread_mutex_lock(&g_malloc_mutex);
    if (p && is_valid_block(p))
        free_block(p - sizeof(block_t));
    pthread_mutex_unlock(&g_malloc_mutex);
}
