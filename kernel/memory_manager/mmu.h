#ifndef MMU_H
#define MMU_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char      int8_t;
typedef signed short     int16_t;
typedef signed int       int32_t;
typedef signed long long int64_t;
typedef __typeof__(sizeof(0)) size_t;

#define NULL ((void *)0)

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
