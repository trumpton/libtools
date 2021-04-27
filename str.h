//
// str.h
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

#ifndef _STR_DEFINED_
#define _STR_DEFINED_

#include "mem.h"

//
// Inserts / replaces data within a string
//
// @param[inout] source Null terminated string to modify
// @param[in] at Position to start operation
// @param[in] removelen Amount of data to remove from source
// @param[in] insertdata Data to insert at position 'at'
// @return Updates source and returns true on success
//

int str_insert(mem *source, int at, int removelen, char *insertdata) ;


//
// Replace the _first_ instance of a string
// 
// @param[inout] source Null terminated string to modify
// @param[in] from String to be replaced
// @param[in] to String to replace with
// @return true on success
//

int str_replace(mem *source, char *from, char *to) ;


//
// Replace all instances of a string
// 
// @param[inout] source Null terminated string to modify
// @param[in] from String to be replaced
// @param[in] to String to replace with
// @return true on success
//

int str_replaceall(mem *source, char *from, char *to) ;


//
// Replace the _first_ case insensitive instance of a string
// 
// @param[inout] source Null terminated string to modify
// @param[in] from Case insensitive string to be replaced
// @param[in] to String to replace with
// @return true on success
//

int str_replacei(mem *source, char *from, char *to) ;


//
// Replace all case insensitive instances of a string
// 
// @param[inout] source Null terminated string to modify
// @param[in] from Case insensitive string to be replaced
// @param[in] to String to replace with
// @return true on success
//

int str_replacealli(mem *source, char *from, char *to) ;


//
// Searches a string for a substring
//
// @param[in] haystack String to use for search
// @param[in] needle String to search for
// @return offset of needle in haystack or -1 if not found
//

int str_offset(char *haystack, char *needle) ;

//
// Searches a string for a substring ignoring case
//
// @param[in] haystack Case insensitive string to use for search
// @param[in] needle String to search for
// @return offset of needle in haystack or -1 if not found
//

int str_offseti(char *haystack, char *needle) ;


//
// Copies source string into destination
//
// @param[out] dst Destination string
// @param[in] src Source string
// @return true on success
//

int str_strcpy(mem *dst, char *src) ;


//
// Copies source string up to termination into destination
// If termination not found, copies entire string
//
// @param[out] dst Destination string
// @param[in] src Source string
// @param[in] term Termination string to search for in src
// @return true on success
//

int str_strtcpy(mem *dst, char *src, char *term) ;


//
// Copies up to n characters of a source string
//
// @param[out] dst Destination string
// @param[in] src Source string
// @param[in] n Maximum length of string to copy
// @return true on success
//

int str_strncpy(mem *dst, char *src, int n) ;


//
// Append one string onto another
//
// @param[out] dst Destination string
// @param[in] src Source string to append
// @return true on success
//

int str_strcat(mem *dst, char *src) ;


//
// Append a decimal integer onto a string
//
// @param[out] dst Destination string
// @param[in] i Integer to append
// @return true on success
//

int str_intcat(mem *dst, int i) ;


//
// Perform a http % decode of a string
//
// @param[inout] str String to modify
// @return true on success
//

int str_decode(mem *str) ;


#endif

