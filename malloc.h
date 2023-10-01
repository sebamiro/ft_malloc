#ifndef MALLOC_H
# define MALLOC_H

# include <sys/mman.h>

# include <pthread.h>

# define SMALL_ZONE		(1 << 10)
# define TINY_ZONE		(1 << 6)
# define MALLOC_ZONE	(1 << 7)
# define MASK_0XFFF		(1 << 12) - 1

typedef enum
{
    MALLOC_TINY,
    MALLOC_SMALL,
    MALLOC_LARGE
} malloc_type_t;

typedef struct s_block
{
    struct s_block	*next;
    struct s_block	*prev;
    size_t			size;
    size_t			align;
} block_t;

typedef struct s_page
{
    struct s_page	*next;
    struct s_page	*prev;
    block_t			*alloc;
    block_t			*free;
} page_t;

typedef struct s_m_pages
{
    page_t	*tiny;
    page_t	*small;
    block_t	*large;
} t_m_pages;

extern t_m_pages g_malloc_pages;
extern pthread_mutex_t g_malloc_mutex;

void *malloc(size_t size);

#endif
