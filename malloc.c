#include "malloc.h"
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

t_m_pages g_malloc_pages = {0};
pthread_mutex_t g_malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void*
new_block(block_t **free, block_t **alloc, const size_t size)
{
    block_t *block;

    block = *free;
    if ((*free = block->next))
        (*free)->prev = NULL;
    if ((block->next = *alloc))
        (*alloc)->prev = block;
    (*alloc) = block;
    block->size = size;
    return block + 1;
}

static inline void
init_mem_zone(page_t **page, page_t *mem, const size_t zone_size)
{
    block_t *free_block;

    free_block = (void *)mem + sizeof(page_t);
    mem->prev = NULL;
    if ((mem->next = *page))
        mem->next->prev = mem;
    *page = mem;
    mem->alloc = NULL;
    mem->free = free_block;
    while ((void*)free_block + (zone_size + sizeof(block_t)) * 2 < (void*)mem + zone_size * MALLOC_ZONE)
    {
        free_block->next = (void*)free_block + sizeof(block_t) + zone_size;
        free_block->next->prev = free_block;
        free_block = free_block->next;
    }
    free_block->next = NULL;
}

static inline void*
malloc_little(page_t **page, const size_t zone_size, const size_t size)
{
    page_t *mem;

    mem = *page;
    while (mem && !mem->free)
        mem = mem->next;
    if (!mem) {
        if ((mem = mmap(0, zone_size * MALLOC_ZONE,
                        PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0)) == MAP_FAILED)
            return NULL;
        init_mem_zone(page, mem, zone_size);
    }
    return new_block(&mem->free, &mem->alloc, align(size, 31));
}

static void*
malloc_large(size_t size)
{
    const size_t msize = align(size + sizeof(block_t), getpagesize() - 1);
    block_t *block;

    if ((block = mmap(0, msize * 100,
            PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0)) == MAP_FAILED)
        return NULL;
    block->size = align(size, 31);
    block->prev = NULL;
    if ((block->next = g_malloc_pages.large))
        g_malloc_pages.large->prev = block;
    g_malloc_pages.large = block;
    return (void*)block + sizeof(block_t);
}

void *
malloc(size_t size)
{
    void *p;

    if (!size)
        return NULL;
    pthread_mutex_lock(&g_malloc_mutex);
    printf("malloc:\t%zu ", size);
    switch ((size > TINY_ZONE) + (size > SMALL_ZONE)) {
        case MALLOC_TINY: {
            p = malloc_little(&g_malloc_pages.tiny, TINY_ZONE, size);
            printf("TINY\n");
            break;
        }
        case MALLOC_SMALL: {
                               printf("SMALL\n");
            p = malloc_little(&g_malloc_pages.small, SMALL_ZONE, size);
            break;
        }
        default: {
            p = malloc_large(size);
            printf("LARGE\n");
            break;
        } 
    }
    printf(" in %p\n", p);
    pthread_mutex_unlock(&g_malloc_mutex);
    return p;
}
