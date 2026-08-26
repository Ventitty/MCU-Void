#ifndef MMU_H
#define MMU_H

#include <stddef.h>
#include <stdint.h>

#define HEADER_SIZE ((size_t)sizeof(block_t))
#define ALIGN_UP(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))

typedef struct block {
    size_t size;
    int    free;
    struct block *next;
} block_t;

void  mm_init(void);
void *sbrk(size_t size);
void *nmap(size_t size);
void  unmap(void *ptr);

#endif /* MMU_H */
