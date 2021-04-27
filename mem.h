//
//
// common/mem.h
//
// Memory allocation and management functions
//
//
// mem *mem_malloc(size_t len)                 - allocate memory buffer
// int mem_length(mem *ptr)                    - report allocated buffer length
// int mem_free(mem *ptr)                      - free memory buffer
// mem mem_dump(char *prefix, mem *ptr)       - dump memory buffer to stderr
// mem *mem_realloc(mem *ptr, size_t newlen)  - reallocates new space


#ifndef _MEMORY_DEFINED_
#define _MEMORY_DEFINED

#include <stddef.h>

// mem* is essentially a pointer to a block of memory
typedef unsigned char mem ;

//
// Allocate memory
//
// Note that if the length supplied is less than or equal to zero,
// a one byte buffer is allocated.
//
// Note that the returned pointer has other information associated
// with it, and the standard realloc and free functions will
// not work with it.
//
// Returns pointer to a null terminated memory block or NULL
// 
// 
mem *mem_malloc(size_t len) ;

//
// Return the length
//
// Returns allocated memory length or 0 on error
// i.e. when the buffer is not allocated, or has been corruted.
//
int mem_length(mem *ptr) ;


//
// Free memory
//
// Returns true on success
//
// Note that if a buffer overrun is detected, the memory will not
// be freed.  It is recommended that the completed program is run
// through valgrind, so that buffers with overruns can be identified.
//
int mem_free(mem *ptr) ;


//
// Reallocate memory
//
// Returns new buffer or NULL
//
mem *mem_realloc(mem *ptr, size_t newlen) ;

//
// Debug - dump allocated memory to stdout
//
mem mem_dump(char *prefix, mem *ptr) ; 


#endif



