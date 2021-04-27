//
// mdns.c
//
//

#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "../mdns.h"
#include "../rdata.h"

//
// UDP Message data structures
//

struct packethead {
  unsigned short id;
  unsigned short flags;
  unsigned short qdcount;
  unsigned short ancount;
  unsigned short nscount;
  unsigned short arcount;
} ;
#define PACKETHEADLEN 12

struct recordhead {
  unsigned short type;
  unsigned short flags;
  unsigned int ttl;
  unsigned short len;
} ;
#define RECORDHEADLEN 10

struct type21hdr {
  unsigned short priority ;
  unsigned short weight ;
  unsigned short port ;
} ;
#define TYPE21HDRLEN 6

struct ipv4address {
  unsigned char ip1 ;
  unsigned char ip2 ;
  unsigned char ip3 ;
  unsigned char ip4 ;
} ;
#define IPV4ADDRESSLEN 4

//
// @brief Prints packet contents to stdout
// @param[in] udpmsg UDP MDNS message packet
// @param[in] length of message
// @return true
//
int mdns_dump_packet(mem *udpmsg, int len)
{
  int i=0 ;
  printf("-BUF-0x%04X--------------------------------------------------------------------\n", len) ;
  
  while (i<len) {

    printf("%04X: ", i) ;

    for (int k=0; k<16; k++) {
      if ((i+k)<len) printf("%02X ", udpmsg[i+k]) ;
      else printf("   ") ;
    }

    printf("  ") ;

    for (int k=0; k<16; k++) {
      if ((i+k)<len) printf("%c", (udpmsg[i+k]>32 && udpmsg[i+k]<128)?udpmsg[i+k]:'.') ;
      else printf(" ") ;
    }

    i+=16 ;
    printf("\n") ;

  }
  printf("-----------------------------------------------------------------------------\n") ;
  return 1 ;
}



//
// @brief Parse UDP message and retrieve header
// @param[in] udpmsg UDP MDNS message packet
// @param[out] flags Flags field of message
// @param[out] qdcount Query count field of message
// @param[out] ancount Resource count field of message
// @param[out] nscount Name count field of message
// @param[out] arcount Additional record count field of message
// @return Position of end of packet header (at start of first record)
//

int mdns_get_packethead(mem *udpmsg, int *flags, int *qdcount, int *ancount, int *nscount, int *arcount)
{
  int udpbuflen=mem_length(udpmsg) ;

  if (udpbuflen<0) return 0 ;

  struct packethead *header = (struct packethead *)udpmsg ;

  *flags = ntohs(header->flags) ;
  *qdcount = ntohs(header->qdcount) ;
  *ancount = ntohs(header->ancount) ;
  *nscount = ntohs(header->nscount) ;
  *arcount = ntohs(header->arcount) ;

  return PACKETHEADLEN ;
}



//
// @brief Parse UDP record at offset and retrieve record header
// @param[in] udpmsg UDP MDNS message packet
// @param[in] p Current offset in message
// @param[out] name Name/scope of UDP record
// @param[out] type Type of UDP record
// @param[out] flags Flags for UDP record
// @param[out] ttl Time to live for UDP record
// @param[out] len Length of payload of UDP record
// @return Position of end of record header (at start of record payload)
//

int mdns_get_recordhead( mem *udpmsg, int p, mem *name, 
                        unsigned short *type, unsigned short *flags, 
                        unsigned int *ttl, unsigned short *len )
{

  mem *namebuf=NULL ;
  int ul=0, nl=0, hl=0 ;

  ul=mem_length(udpmsg) ;
  nl=mem_length(name) ;
  if (nl<0 || ul<0) goto error ;

  namebuf = mem_malloc(ul) ;
  if (!namebuf) goto error ;

  // Extract record head name - guaranteed to be terminated

  hl=rdata_extract(namebuf, udpmsg, p, ul) ;
  if (mem_length(name)<(strlen(namebuf)+1)) goto error ;
  strcpy(name, rdata_toname(namebuf, '.')) ;

  // Extract header parameters

  struct recordhead *header = (struct recordhead *) &udpmsg[p+hl] ;
  *type = ntohs(header->type) ;
  *flags = ntohs(header->flags) ;
  *ttl = ntohl(header->ttl) ;
  *len = ntohs(header->len) ;

  mem_free(namebuf) ;
  return hl+RECORDHEADLEN ;

error:

  mem_free(namebuf) ;
printf("ERROR\n") ;
  return -1 ;
}




//
// @brief Extract nameptr from UDP message record (type 0x0C)
// @param[out] ptr Destination memory buffer for PTR string
// @param[in] udpmsg UDP MDNS message packet
// @param[in] p Offset in udpmsg of start of rdata record payload
// @param[in] len Maximum length of rdata record
// @return true if data extracted
//

int mdns_get_rdatanameptr(mem *ptr, mem *udpmsg, int p, int len) 
{
  int nl=0 ;
  int pl=mem_length(ptr) ;
  int ul=mem_length(udpmsg) ;

  if (ul<0 || pl<0 || pl<len) goto error ;

  mem *buf = mem_malloc(pl) ;
  if (!buf) goto error ;

  nl=rdata_extract(buf, udpmsg, p, len) ;  

  if (nl<0) goto error ;

  strcpy(ptr, rdata_toname(buf, '.')) ;

  mem_free(buf) ;
  return 1 ;

error:
  mem_free(buf) ;
  return 0 ;
}




//
// @brief Extract field from an rdata TXT sequence in a UDP message record (type 0x10)
// @param[out] dst Destination memory buffer for field string contents (excluding prefix)
// @param[in] udpmsg UDP MDNS message packet
// @param[in] p Offset in udpmsg of start of rdata record payload
// @param[in] search Search prefix for field
// @param[in] len Maximum length of rdata record
// @return true if data found
//

int mdns_get_rdatafield(mem *dst, mem *udpmsg, int p, char *search, int len) 
{
  int found=0 ;
  int ul=mem_length(udpmsg) ;
  int dl=mem_length(dst) ;
  if (ul<0 || dl<0 || dl<len) goto error ;

  int i=0 ;
  int sslen=0 ;
  int searchlen=strlen(search) ;
  mem *substring = mem_malloc(dl) ;

  do {
    sslen = rdata_substring_at(substring, &udpmsg[p], i++) ;
    if (sslen>0 && strncmp(substring, search, searchlen)==0) {found=1 ; }
  } while (!found && sslen>=0) ;

  if (found) {
    strcpy(dst, &substring[searchlen]) ;
  }

finish:
  mem_free(substring) ;
  return found ;

error:
  found=0 ;
  goto finish ;

}



//
// @brief Extract service data from a UDP message record (type 0x21)
// @param[out] networkname Destination memory buffer for the service network name
// @param[out] port Destination for the service port
// @param[in] udpmsg UDP MDNS message packet
// @param[in] p Offset in udpmsg of start of rdata record payload
// @return true if data found
//

int mdns_get_service(mem *networkname, unsigned short *port, mem *udpmsg, int p, int len) 
{
  int ul=mem_length(udpmsg) ;
  int nl=mem_length(networkname) ;
  if (ul<0 || nl<0) goto error ;

  struct type21hdr *header = (struct type21hdr *) &udpmsg[p] ;
  int headerlen = TYPE21HDRLEN ;

  *port = ntohs(header->port) ;

  return mdns_get_rdatanameptr(networkname, udpmsg, p+headerlen, len-headerlen) ;

error:
  return 0 ;
}




//
// @brief Extract IP address from a UDP message record (type 0x01)
// @param[out] ipaddress Destination memory buffer for the IP address
// @param[in] udpmsg UDP MDNS message packet
// @param[in] p Offset in udpmsg of start of rdata record payload
// @return true if data found
//

int mdns_get_ipaddress(mem *ipaddress, mem *udpmsg, int p, int len) 
{
  char addrbuf[33] ;
  if (mem_length(udpmsg)<0) goto error ;
  if (mem_length(ipaddress)<sizeof(addrbuf)) goto error ;

  struct ipv4address *ipbuf = (struct ipv4address *) &udpmsg[p] ;
  sprintf(addrbuf, "%d.%d.%d.%d", ipbuf->ip1, ipbuf->ip2, ipbuf->ip3, ipbuf->ip4) ;
  strcpy(ipaddress, addrbuf) ;

  return 1 ;

error:
  return 0 ;
}

