//
// rdata.c
//

#include <string.h>
#include "../rdata.h"

//
// @brief Internal recursive function to extract an rdata sequence into a string
// @param[out] dst Destination buffer of length RDATABUFLEN
// @param[in] src Source buffer
// @param[in] i Start index within source buffer
// @param[in] l Maximum length of data to process
// @param[in] r Current level of recursion
// @return Length of data processed
//
int __recursive_extract_rdata_string(mem *dst, unsigned char *src, int i, int l, int r)
{
  // Safety check
  if (r>4) return 0 ;
  int dstlen=mem_length(dst) ;

  int starti=i ;
  int length ;
 
  if (r==0) dst[0]='\0' ;
  if (r==0 && l==0) l=dstlen-1 ;

// The compression scheme allows a domain name in a message to be
// represented as either:
//   - a sequence of labels ending in a zero octet
//   - a pointer
//   - a sequence of labels ending with a pointer

  do {

    length=(unsigned char)src[i] ;

    if (length>=0xC0) {

        int newi = (src[i]&0x3F)*256 + src[i+1] ;

        if (newi<starti) {
          // Follow backwards compression reference and append string
          __recursive_extract_rdata_string(dst, src, newi, (starti-newi), r+1) ;
        }
        i+=2 ;

      } else if (length>0) {

        if ( strlen(dst)+length+1 < dstlen) {
          // Copy length byte plus data
          strncat(dst, &src[i], length+1) ;
        }
        i+=length+1 ;

      } else if (length==0) {
        i++ ;
      }

  } while ( length<0xC0 && length!=0 && i<(starti+l) && i<(dstlen-1)) ;

  return i-starti ;
}


//
// @brief Internal function to extract an rdata sequence into a string
// @param[out] rdatadst Destination buffer
// @param[in] src Source buffer
// @param[in] i Start index within source buffer
// @param[in] l Maximum length of data to process
// @return Length of data processed
//
int rdata_extract(mem *rdatadst, unsigned char *src, int srcpos, int srclen)
{
  return __recursive_extract_rdata_string(rdatadst, src, srcpos, srclen, 0) ;
}


//
// @brief Convert an rdata sequence produced by _extract_rdata_string to a printable name/path
// @param[inout] rdatabuf Buffer containing data to be converted.  On success, buf[0]='\0'.
// @param[in] separator Separator character for output, e.g. '.'
// @return Pointer to converted name.  Converted name valid as long as buf not overwritten/reused.
//
char *rdata_toname(mem *rdatabuf, char separator)
{
  int p=0 ;
  int buflen=mem_length(rdatabuf) ;
  if (rdatabuf[0]!='\0') {
    do {
      int rl=rdatabuf[p] ;
      rdatabuf[p]=separator ;
      p+=rl+1 ;
    } while (p<(buflen-1) && rdatabuf[p]!='\0') ;
    rdatabuf[0]='\0' ;
  }
  return &rdatabuf[1] ;
}


//
// @brief Get length of requested string within rdata buffer
// @param[in] rdatabuf Buffer containing rdata
// @param[in] index Index of string to query
// @return Length of requested string, or -1 if index invalid
//
int rdata_substring_len(mem *rdatabuf, int index)
{
  int p=0 ;
  for (; index>0 && rdatabuf[p]!=0; index--, p=p+rdatabuf[p]+1) ;
  if (index>0) return -1 ;
  else return ((int)rdatabuf[p]) ;
}

//
// @brief Extract an individual rdata substring record by index
// @param[out] dst Output buffer
// @param[in] rdatabuf Source rdata buffer containing the source rdata
// @param[in] index Index number of  the requested substring
// @return Length of substring created, or -1 on error
//
int rdata_substring_at(mem *dst, mem *rdatabuf, int index) 
{
  int p=0 ;
  int dstlen=mem_length(dst);
  for (; index>0 && rdatabuf[p]!=0; index--, p=p+rdatabuf[p]+1) ;
  if (index>0 || rdatabuf[p]==0) return -1 ;
  int lc = ((unsigned int)rdatabuf[p]) ;
  strncpy(dst, &rdatabuf[p+1],(lc>dstlen-1)?(dstlen-1):lc) ;
  dst[lc]='\0' ;
  return lc ; 
}


