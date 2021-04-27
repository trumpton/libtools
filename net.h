//
// net.h
//
// These functions are designed to provide a simple
// network client wrapper.  SSL is also supported.
//
// Manage Network session
//
// int netinit()
// NET *netopen(char *hostname, int port, net_flags flags)
// int netcerterror(NET *sh)
// char *netcerterrorstr(int certerrno)
// int netfd(NET *sh)
// char *netpeerip(NET *sh)
// int netpeerport(NET *sh)
// int netlocalport(NET *sh)
// int netsend(INET *sh, char *buf, int len)
// int netrecv(INET *sh, char *buf, int maxlen)
// int netclose(NET *sh)
//
// link with: -lssl -lcrypto 
//

#ifndef _NET_DEFINED
#define _NET_DEFINED

#include <stdio.h>

#ifndef NET
typedef struct {} NET ;
#endif

enum netflags {
  OPEN = 0,           // Default (non-SSL/TLS)
  TLS = 1,            // Enables TLS
  SSL2 = 2,           // Enable SSL2
  SSL3 = 4,           // Enable SSL3
  NOCERTCHAIN = 8,    // Prevents interrogation of certificate chain for SSL
  DEBUGDATADUMP = 16, // Debug data dumped to stdout if flag set and env variable NETDUMPENABLE
  DEBUGKEYDUMP = 32,  // Enables key dump to file in env variable SSLKEYLOGFILE
  NONBLOCK = 256      // Handles client connection as non-blocking
} ;

// errno types

enum net_errno_type {
  NET_ERR_ERRNO = 0000,
  NET_ERR_INT = 10000,
  NET_ERR_SSL = 20000
} ;

// Internal net errcodes, which supplement
// errno and the SSL errors

enum errorcodes {
  NET_ERR_OK = 0,            // No error
  NET_ERR_PTR,               // Invalid pointer
  NET_ERR_BADP,              // Invalid network port specified
  NET_ERR_BADA,              // Invalid network port specified
  NET_ERR_TIMEOUT,           // Timeout establishing connection
  NET_ERR_UNK                // Unknown Error
} ;


int netinit() ;


//
// @brief Connect to server
// @param(in) hostname Name of server to connect to
// @param(in) port Port number on server
// @param(in) flags Type of connection to open (OPEN|SSL2|SSL3|NONBLOCK)
// @return Handle to SSL structure, or NULL on failure (and sets errno)
//

NET *netconnect(char *hostname, int port, enum netflags flags) ;


// 
// @brief Obtain SSL certificate status
// @param(in) Handle of open connection
// @return Non-zero certificate status code or zero if ok
//

int netcertstatus(NET *sh) ;


// 
// @brief Obtain SSL error status
// @param(in) sslcertstatus Status from netcertstatus
// @return Pointer to static string containing error message
//

char *netcertstatusstr(int sslcertstatus) ;


// 
// @brief Obtain network connection errno
// @return Error number
//

int neterrno() ;


// 
// @brief Obtain Network connection error status
// @return Pointer to static string containing last error message
//

char *netstrerror() ;

//
// @brief Return context of last error
//

char *netstrerrorcontext() ;


// 
// @brief Get network connection state
// @param(in) Handle of open connection
// @return Returns true if network connection is connected
//

int netisconnected(NET *sh) ;


//
// @brief Add connection to fd_set if active
// @param(in) sh Handle of network connection
// @param(in) rdfds FD Set for select()
// @param(in) wrfds FD Set for select() - only required for SSL
// @param(inout) l pointer to largest fd found
// @return number of connections added
//

int netrdfdset(NET *sh, fd_set *rdfds, fd_set *wrfds, int *l) ;


//
// @brief Add connection to write fd_set if active
// @param(in) sh Handle of network connection
// @param(in) wrfds FD Set for select()
// @param(inout) l pointer to largest fd found
// @return number of connections added
//

int netwrfdset(NET *sh, fd_set *wrfds, int *l) ;


//
// @brief Determine if connection is ready for reading
// @param(in) sh Handle of network connection
// @param(in) rfds Read fd set
// @param(in) wrds Write fd set - only required for SSL
// @return True if connection has pending data
//

int netrdfdisset(NET *sh, fd_set *rfds, fd_set *wrfds) ;


//
// @brief Determine if connection is ready for writing
// @param(in) sh Handle of network connection
// @param(in) wrds Write fd set
// @return True when connection becomes ready to receive data
//

int netwrfdisset(NET *sh, fd_set *wrfds) ;


//
// @brief Obtain peer IP address
// @param(in) sh Handle of open connection
// @return Pointer to IP address string, or NULL if not connected
//

char *netpeerip(NET *sh) ;



//
// @brief Provide summary string of read and write socket fd_set info
//

char *netfdsetinfo(NET *sh, fd_set *rfds, fd_set *wfds) ;


//
// @brief Obtain peer port number
// @param(in) sh Handle of open connection
// @return Peer port number, or 0 if not connected
//

int netpeerport(NET *sh) ;


//
// @brief Obtain local port number
// @param(in) sh Handle of open connection
// @return Local port number, or 0 if not connected
//

int netlocalport(NET *sh) ;


//
// @brief Send data to network interface
// @param(in) sh Handle of open connection
// @param(in) buf Data to be sent
// @param(in) len Amount of data to send
// @return Number of bytes sent, or -1 on error
//
 
int netsend(NET *sh, char *buf, int len) ;


//
// @brief Receive data from network interface
// @param(in) sh Handle of open connection
// @param(in) buf Buffer to store response
// @param(in) maxlen Maximum number of bytes to read
// @return Number of bytes received. 0 indicates peer has closed, or -1 on error
//
 
int netrecv(NET *sh, char *buf, int maxlen) ;


//
// @brief Returns true if pending data
// @param(in) sh Handle of open connection
// @return true if connection open and has pending data
//

int nethaspending(NET *sh) ;


//
// @brief Close connection
// @param(in) sh Handle of open connection
// @return true on success, or false on error (setting errno)
//
int netclose(NET *sh) ;


#endif

