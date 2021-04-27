//
// common/str.c
//
// String management functions
//
// int str_insert(mem *source, int at, int removelen, char *insertdata)
// int str_replace(mem *source, char *from, char *to)
// int str_replaceall(mem *source, char *from, char *to)
// int str_replacei(mem *source, char *from, char *to)
// int str_replacealli(mem *source, char *from, char *to)
// int str_offset(char *haystack, char *needle)
// int str_offseti(char *haystack, char *caseinsensitiveneedle)
// int str_strcpy(mem *dst, char *src)
// int str_strtcpy(mem *dst, char *src, char *term)
// int str_strncpy(mem *dst, char *src, int n)
// int str_strcat(mem *destination, char *source)
// int str_intcat(mem *destination, int number)
//
// Note that these functions expects the string space to have been allocated
// with mem_malloc
//

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "../mem.h"
#include "../str.h"


//
// Inserts / replaces data within a string
//
// @param[inout] source Null terminated string to modify
// @param[in] at Position to start operation
// @param[in] removelen Amount of data to remove from source
// @param[in] insertdata Data to insert at position 'at'
// @return Updates source and returns true on success
//

int str_insert(mem *source, int at, int removelen, char *insertdata) 
{
  int maxlen=mem_length(source) ;
  int insertlen=strlen(insertdata) ;
  int len=strlen(source) ;

  // Sanity Checks and checks for sufficient buffer space
  if (maxlen<0) return 0 ;
  if (insertlen>0 && !insertdata) 0 ;
  if ( (maxlen-len) <= (insertlen-removelen) ) return 0 ;
  if (removelen<0 || insertlen<0 || len>maxlen || len<0 || maxlen<=0 || at>len) return 0 ;

  if (insertlen!=removelen) {

    // Move the right hand side of the data to the correct position
    int leftgap=at ;
    int oldrightgap=at+removelen ;
    int newrightgap=at+insertlen ;

    if (oldrightgap>newrightgap) {
      memmove(&source[newrightgap], &source[oldrightgap], (size_t)(maxlen-oldrightgap)) ;
    } else {
      memmove(&source[newrightgap], &source[oldrightgap], (size_t)(maxlen-newrightgap)) ;
    }

  }

  // Insert requested data
  if (insertlen>0) {
    memmove(&source[at], insertdata, insertlen) ;
  }

  return 1 ;
}


//
// Replace the _first_ instance of a string
// 
// @param[inout] source Null terminated string to modify
// @param[in] from String to be replaced
// @param[in] to String to replace with
// @return true on success
//

int str_replace(mem *source, char *from, char *to)
{
  int k=str_offset(source, from) ;
  if (k>=0) {
    return str_insert(source, k, strlen(from), to) ;
  } else {
    return 0 ;
  }
}


//
// Replace all instances of a string
// 
// @param[inout] source Null terminated string to modify
// @param[in] from String to be replaced
// @param[in] to String to replace with
// @return true on success
//

int str_replaceall(mem *source, char *from, char *to)
{
  int k=0, o=0 ;
  do {
    if ( (o=str_offset(&source[k], from)) >=0 ) {
      k+=o ;
      if (!str_insert(source, k, strlen(from), to)) return 0 ;
    }
  } while (o>0) ;
  return 1 ;
}

//
// Replace the _first_ case insensitive instance of a string
// 
// @param[inout] source Null terminated string to modify
// @param[in] from Case insensitive string to be replaced
// @param[in] to String to replace with
// @return true on success
//

int str_replacei(mem *source, char *from, char *to)
{
  int k=str_offseti(source, from) ;
  if (k>=0) {
    return str_insert(source, k, strlen(from), to) ;
  } else {
    return 0 ;
  }
}



//
// Replace all case insensitive instances of a string
// 
// @param[inout] source Null terminated string to modify
// @param[in] from Case insensitive string to be replaced
// @param[in] to String to replace with
// @return true on success
//

int str_replacealli(mem *source, char *from, char *to)
{
  int k=0, o=0 ;
  do {
    if ( (o=str_offseti(&source[k], from)) >=0 ) {
      k+=o ;
      if (!str_insert(source, k, strlen(from), to)) return 0 ;
    }
  } while (o>0) ;
  return 1 ;
}



//
// Searches a string for a substring
//
// @param[in] haystack String to use for search
// @param[in] needle String to search for
// @return offset of needle in haystack or -1 if not found
//

int str_offset(char *haystack, char *needle)
{
  char *pos = strstr(haystack, needle) ;
  if (!pos) {
    return -1 ;
  } else {
    return (int)(pos-haystack) ;
  }
}


//
// Searches a string for a substring ignoring case
//
// @param[in] haystack Case insensitive string to use for search
// @param[in] needle String to search for
// @return offset of needle in haystack or -1 if not found
//

int str_offseti(char *haystack, char *needle)
{
  char *pos = strcasestr(haystack, needle) ;
  if (!pos) {
    return -1 ;
  } else {
    return (int)(pos-haystack) ;
  }
}


//
// Copies source string into destination
//
// @param[out] dst Destination string
// @param[in] src Source string
// @return true on success
//

int str_strcpy(mem *dst, char *src) 
{
  int srclen=strlen(src) ;
  int dstbuf=mem_length(dst) ;
  if (srclen>(dstbuf-1)) return 0 ; 
  strcpy(dst, src) ;
  return 1 ;
}


//
// Copies source string up to termination into destination
// If termination not found, copies entire string
//
// @param[out] dst Destination string
// @param[in] src Source string
// @param[in] term Termination string to search for in src
// @return true on success
//

int str_strtcpy(mem *dst, char *src, char *term)
{
  int dstlen=strlen(dst) ;
  int srclen=strlen(src) ;
  int dstbuf=mem_length(dst) ;
  int t=str_offseti(src, term) ;
  if (t<0) t=srclen ;

  if ( t < (dstbuf-1) ) {

    strncpy(dst, src, t) ;
    dst[t]='\0' ;
    return 1 ;

  } else {

    return 0 ;

  }

}


//
// Copies up to n characters of a source string
//
// @param[out] dst Destination string
// @param[in] src Source string
// @param[in] n Maximum length of string to copy
// @return true on success
//

int str_strncpy(mem *dst, char *src, int n)
{
  int dstlen=strlen(dst) ;
  int srclen=strlen(src) ;
  int dstbuf=mem_length(dst) ;

  if ( n < (dstbuf-1) ) {

    strncpy(dst, src, n) ;
    dst[n]='\0' ;
    return 1 ;

  } else {

    return 0 ;

  }

}


//
// Append one string onto another
//
// @param[out] dst Destination string
// @param[in] src Source string to append
// @return true on success
//

int str_strcat(mem *dst, char *src)
{
  int dstbuf=mem_length(dst) ;

  if (dstbuf<0) {
    perror("str_strcat - invalid dst") ;
    return 0 ;
  }

  int dstlen=strlen(dst) ;
  int srclen=strlen(src) ;

  if (srclen < (dstbuf-dstlen)-1) {

    strcat(dst, src) ;
    return 1 ;

  } else {

    return 0 ;

  }

}


//
// Append a decimal integer onto a string
//
// @param[out] dst Destination string
// @param[in] i Integer to append
// @return true on success
//

int str_intcat(mem *dst, int i) 
{
  char num[16] ;
  sprintf(num, "%d", i) ;
  return (strcat(dst, num)!=NULL) ;
}


//
// Perform a http % decode of a string
//
// @param[inout] str String to modify
// @return true on success
//

int htod(unsigned char ch) {
  int r=0 ;
  if (ch>='0' && ch<='9') { return (ch-'0') ; }
  ch=tolower(ch) ;
  if (ch>='a' && ch<='f') { return (ch-'a'+10) ; }
  return 0 ;
}

int str_decode(mem *str)
{
  int i=0 ;
  char s[2] ; s[1]='\0' ;
  do {
    if (str[i]=='+') str[i]=' ' ;
    else if (str[i]=='%' && str[i+1]!='\0' && str[i+2]!='\0') {
      *s = htod(str[i+1])*16 + htod(str[i+2]) ;
      str_insert(str, i, 3, s) ;
    }
    i++ ; 
  } while (str[i]!='\0') ;
  return 1 ;
}

