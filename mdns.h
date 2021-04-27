//
// mdns.h
//
//

#ifndef _MDNS_DEFINED_
#define _MDNS_DEFINED_

#include "mem.h"

//
// @brief Prints packet contents to stdout
// @param[in] upmsg UDP MDNS message packet
// @param[in] length of message
// @return true
//
int mdns_dump_packet(mem *upmsg, int len) ;


//
// @brief Parse UDP message and retrieve header
// @param[in] upmsg UDP MDNS message packet
// @param[out] flags Flags field of message
// @param[out] qdcount Query count field of message
// @param[out] ancount Resource count field of message
// @param[out] nscount Name count field of message
// @param[out] arcount Additional record count field of message
// @return Position of end of packet header (at start of first record)
//

int mdns_get_packethead(mem *upmsg, int *flags, int *qdcount, int *ancount, int *nscount, int *arcount) ;



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
                        unsigned int *ttl, unsigned short *len ) ;



//
// @brief Extract nameptr from UDP message record (type 0x0C)
// @param[out] ptr Destination memory buffer for PTR string
// @param[in] udpmsg UDP MDNS message packet
// @param[in] p Offset in udpmsg of start of rdata record payload
// @param[in] len Maximum length of rdata record
// @return true if data extracted
//

int mdns_get_rdatanameptr(mem *ptr, mem *udpmsg, int p, int len) ;




//
// @brief Extract field from an rdata TXT sequence in a UDP message record (type 0x10)
// @param[out] dst Destination memory buffer for field string contents (excluding prefix)
// @param[in] udpmsg UDP MDNS message packet
// @param[in] p Offset in udpmsg of start of rdata record payload
// @param[in] search Search prefix for field
// @param[in] len Maximum length of rdata record
// @return true if data found
//

int mdns_get_rdatafield(mem *dst, mem *udpmsg, int p, char *search, int len) ;



//
// @brief Extract service data from a UDP message record (type 0x21)
// @param[out] networkname Destination memory buffer for the service network name
// @param[out] port Destination for the service port
// @param[in] udpmsg UDP MDNS message packet
// @param[in] p Offset in udpmsg of start of rdata record payload
// @param[in] len Maximum length of record
// @return true if data found
//

int mdns_get_service(mem *networkname, unsigned short *port, mem *udpmsg, int p, int len) ;




//
// @brief Extract IP address from a UDP message record (type 0x01)
// @param[out] ipaddress Destination memory buffer for the IP address
// @param[in] udpmsg UDP MDNS message packet
// @param[in] p Offset in udpmsg of start of rdata record payload
// @param[in] len Maximum length of record
// @return true if data found
//

int mdns_get_ipaddress(mem *ipaddress, mem *udpmsg, int p, int len) ;



#endif

