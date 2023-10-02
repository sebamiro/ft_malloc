#include "malloc.h"
#include <stdio.h>

size_t
align(size_t s, size_t mask)
{
    return (s + mask) & ~mask;
}

static bool
check_addr(block_t *block, void *block_addr, const void *p)
{
    printf("%p %p\n", block, block_addr);
    while (block && block_addr <= p) {
        if (block_addr == p)
            return true;
        block = block->next;
        block_addr = block + sizeof(block_t); 
    }
    return false;
}

bool
is_valid_sized(const void *p, size_t size)
{
    void *block_addr;
    block_t *block;

    printf("is_valid_block:\t%p %zu ", p, size);
    if (size > SMALL_ZONE) {
        block = g_malloc_pages.large;
        printf("LARGE ");
    }
    else if (size > TINY_ZONE) {
        block = (block_t*)g_malloc_pages.small + sizeof(page_t);
        printf("SMALL ");
    }
    else {
        block = (block_t*)g_malloc_pages.tiny + sizeof(page_t);
        printf("TINY ");
    }
    block_addr = (void*)block + sizeof(block_t);
    return check_addr(block, block_addr, p);
}

bool
is_valid_block(const void *p)
{
    void *block_addr;
    block_t *block;

    block = g_malloc_pages.large;
    block_addr = (void*)block + sizeof(block_t);
    if (check_addr(block, block_addr, p))
        return true;
    block = g_malloc_pages.small->alloc;
    block_addr = (void*)block + sizeof(block_t);
    if (check_addr(block, block_addr, p))
        return true;
    block = g_malloc_pages.tiny->alloc;
    block_addr = (void*)block + sizeof(block_t);
    if (check_addr(block, block_addr, p))
        return true;
    return false;
}

