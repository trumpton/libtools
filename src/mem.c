
#include <malloc.h>
#include <string.h>
#include <errno.h>

#include "../mem.h"

#define MAGIC "\0~%&<?/."
#define MAGIC_LEN 8

/////////////////////////////////////
//
// Allocate memory
//
mem *mem_malloc(size_t len)
{
  // force allocations to be at least 1 byte
  if (len<=0) len++ ;

  size_t rlen=MAGIC_LEN+sizeof(size_t)+len+MAGIC_LEN ;
  mem *ptr=(mem *)malloc(rlen) ;

  if (!ptr) {
    errno=ENOMEM ;
    return NULL ;

  } else {

    mem *m1 ;    m1=ptr ; 
    size_t *sz ;  sz=(size_t *)(m1+MAGIC_LEN) ;
    char *data ;  data=(char *)(m1+MAGIC_LEN+sizeof(size_t)) ;
    mem *m2 ;    m2=(m1+MAGIC_LEN+sizeof(size_t)+len) ;

    bzero(ptr, rlen) ;
    memcpy(m1, MAGIC, MAGIC_LEN) ;
    memcpy(m2, MAGIC, MAGIC_LEN) ;
    sz[0]=len ;
    data[0]='\0' ;

    return (mem *)data ;

  }
}

/////////////////////////////////////
//
// Re-allocate memory
//
mem *mem_realloc(mem *ptr, size_t newlen) 
{
  size_t oldlen=mem_length(ptr) ;
  if (newlen==oldlen) return ptr ;
  mem *newptr=mem_malloc(newlen) ;
  if (newptr && oldlen>0) {
    memcpy(newptr, ptr, newlen>oldlen?oldlen:newlen) ;
    mem_free(ptr) ;
  }
  return newptr ;  
}

/////////////////////////////////////
//
// Dump the requested mem buffer to stderr
//
mem mem_dump(char *prefix, mem *ptr) 
{
  char *m1 ;    m1=(char *)(ptr-MAGIC_LEN-sizeof(size_t)) ; 
  size_t *sz ;  sz=(size_t *)(m1+MAGIC_LEN) ;

  fprintf(stderr, "%s: ", prefix) ;

  for (int i=0; i<MAGIC_LEN; i++) {
    fprintf(stderr, "%02X", m1[i]) ;
  }
  size_t len=mem_length(ptr) ;
  fprintf(stderr, " (%ld) '", len) ;
  int j=MAGIC_LEN+sizeof(size_t) ;
  for (int i=0; i<16 && i<len; i++) {
    fprintf(stderr, "%c", (m1[i+j]<32 || m1[i+j]>127)?'.':m1[i+j]) ;
  }
  fprintf(stderr, "' ") ;
  int k=MAGIC_LEN+sizeof(size_t)+len ;
  for (int i=0; i<MAGIC_LEN; i++) {
    fprintf(stderr, "%02X", m1[i+k]) ;
  }
  fprintf(stderr, "\n") ;
}


/////////////////////////////////////
//
// Returns length allocated or 0 on error
//
int mem_length(mem *ptr)
{
  if (!ptr) {

    return 0 ;

  } else {

    mem *m1 ;  m1=ptr-MAGIC_LEN-sizeof(size_t) ; 
    size_t *sz ;  sz=(size_t *)(m1+MAGIC_LEN) ;
    char *data ;  data=(char *)(m1+MAGIC_LEN+sizeof(size_t)) ;
    size_t len = sz[0] ;
    mem *m2 ;  m2=(m1+MAGIC_LEN+sizeof(size_t)+len) ;

    if (memcmp((void *)m1, MAGIC, MAGIC_LEN)!=0) return 0 ;
    if (memcmp((void *)m2, MAGIC, MAGIC_LEN)!=0) return 0 ;

    return sz[0] ;

  }
}

/////////////////////////////////////
//
// Free memory
//
int mem_free(mem *ptr) 
{
  if (!ptr) return 0;
  if (!mem_length(ptr)) {
    // Invalid pointer
    // Or buffer overrun
    return 0;
  }

  mem *m1 ;    m1=ptr-MAGIC_LEN-sizeof(size_t) ; 
  size_t *sz ;  sz=(size_t *)(m1+MAGIC_LEN) ;
  size_t len = sz[0] ;
  mem *m2 ;    m2=(m1+MAGIC_LEN+sizeof(size_t)+len) ;

  memcpy((void *)m1, "FREE\0\0\0\0\0\0\0", MAGIC_LEN) ;
  memcpy((void *)m2, "FREE\0\0\0\0\0\0\0", MAGIC_LEN) ;

  free((void *)m1) ;
  return 1 ;
}


