//
// rdata.h
//
//

#ifndef _RDATA_DEFINED
#define _RDATA__DEFINED

#include "mem.h"

//
// @brief Internal function to extract an rdata sequence into a string
// @param[out] rdatadst Destination rdata buffer
// @param[in] src Source buffer
// @param[in] srcpos Start index within source buffer
// @param[in] srclen Maximum length of data to process
// @return Length of data processed
//
int rdata_extract(mem *rdatadst, unsigned char *src, int srcpos, int srclen) ;


//
// @brief Convert an rdata sequence produced by _extract_rdata_string to a printable name/path
// @param[inout] rdatabuf Buffer containing data to be converted.  On success, buf[0]='\0'.
// @param[in] separator Separator character for output, e.g. '.'
// @return Pointer to converted name.  Converted name valid as long as buf not overwritten/reused.
//
char *rdata_toname(mem *rdatabuf, char separator) ;


//
// @brief Get length of requested string within rdata buffer
// @param[in] rdatabuf Buffer containing rdata
// @param[in] index Index of string to query
// @return Length of requested string, or -1 if index invalid
//
int rdata_substring_len(mem *rdatabuf, int index) ;


//
// @brief Extract an individual rdata substring record by index
// @param[out] dst Output buffer
// @param[in] rdatabuf Source buffer containing the source rdata
// @param[in] index Index number of  the requested substring
// @return Length of substring created, or -1 on error
//
int rdata_substring_at(mem *dst, mem *rdatabuf, int index) ;

#endif

