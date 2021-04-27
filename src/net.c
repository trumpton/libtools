//
// net.c
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
// char *netpeerip(NET *sh)
// int netpeerport(NET *sh)
// int netlocalport(NET *sh)
//
// int netrdfdset(INET *sh, fd_set *rdfds, fd_set *wrfds, int *l)
// int netrdfdisset(INET *sh, fd_set *rfds, fd_set *wfds)
// int netsend(INET *sh, char *buf, int len)
// int netrecv(INET *sh, char *buf, int maxlen)
// int netclose(NET *sh)
//
// link with: -lssl -lcrypto 
//
// NOTES
//
// For SSL read connections, the underlying socket handle (fd) has two-way
// communications to allow the SSL handshake to take place.  In order to 
// make this as transparent as possible when non-blocking sockets are used
// the FD_SET should be replaced with netrdset, and the FD_ISSET should be
// replaced with the netrdfdisset function.  These functions manage the SSL
// connection's underlying need for additional reads (and writes).
// As SSL_read caches data from the socket, the underlying socket cannot be
// used in select to determine if more data is available.  The netrdfdset
// therefore checks for this, and adds /dev/null file descriptor to the 
// fd_set as and when necessary.
//

#define _GNU_SOURCE 

#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

//#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>


typedef struct {

  // Network socket management

  int isblocking ;     // True if connection is blocking
  int fd ;             // Socket file descriptor
  char *ipaddress ;    // Connected IP address
  int localport ;      // Local port number for connection
  int peerport ;       // Remote port number for connection

  // SSL connection management

  SSL *ssl;            // SSL object
  int certstatus ;     // SSL Connection status
  SSL_CTX *ctx;        // SSL Context

  // Non-blocking data stream management

  int sslwantwrite ;   // Flag so SSL_read can request write in select
  int sslhaspending ;  // Flag indicating SSL read can supply more data

  // Debug

  int keydumpenable ;
  int datadumpenable ;

} INET ;

//
static int _net_errno=-1 ;
static char _net_errcontext[64] ;

#define DEVNULL "/dev/null"
static int _net_devnull=-1 ;
static int _net_numconnections=0 ;

#define NET INET
#include "../net.h"

int _net_seterrno(INET *sh, char *context, enum net_errno_type type, int errcode) ;
int _net_disconnect(INET *sh) ;
void _net_commsdump(INET *sh, char *prefix, char *buf, int buflen) ;

typedef void (*SSL_CTX_keylog_cb_func)(const SSL *ssl, const char *line);
void _net_ssl_keylog(const SSL *ssl, const char *line);


//
// @brief Initialise network subsystem this only needs to be called once
// @return true on success
//

int _net_ssl_init()
{
  static int success=0 ;
  if (!success) {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    success = SSL_library_init();
  }
  return success ;
}

//
// @brief Connect to server
// @param(in) hostname Name of server to connect to - note 011 represents octal -> 9
// @param(in) port Port number on server
// @param(in) flags Type of connection to open (OPEN|TLS|SSL2|SSL3|NONBLOCK)
// @return Handle to NET structure, or NULL on failure (and sets errno)
//

INET *netconnect(char *hostname, int port, enum netflags flags)
{
  struct hostent *host;

  if (!hostname) {
    // Unable to set sh->errno
    return NULL ;
  }

  INET *sh = malloc(sizeof(INET)) ;
  if (!sh) {
    // Unable to set sh->errno
    return NULL ;
  }
  memset(sh, '\0', sizeof(INET)) ;

  sh->fd=-1 ;
  sh->localport=-1 ;
  sh->peerport=-1 ;
  sh->certstatus=X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT ; 
  sh->isblocking = !(flags&NONBLOCK) ;

  // Create underlying connection

  host = gethostbyname(hostname) ;
  if ( host == NULL ) {
    _net_seterrno(sh, "gethost", NET_ERR_INT, NET_ERR_BADA) ;
    goto fail ;
  }

  sh->fd = socket(AF_INET, SOCK_STREAM, 0);
  if ( sh->fd <= 0 ) {
    _net_seterrno(sh, "socket", NET_ERR_ERRNO, 0) ;
    goto fail ;
  }

  if (port<=0) {
    _net_seterrno(sh, "port", NET_ERR_INT, NET_ERR_BADP) ;
    goto fail ;
  }

  sh->peerport = port ;

  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(struct sockaddr_in));
  dest_addr.sin_family=AF_INET;
  dest_addr.sin_port=htons(sh->peerport);
  dest_addr.sin_addr.s_addr = *(unsigned long*)(host->h_addr);


  // Store resolved IP address

  char *i = inet_ntoa(dest_addr.sin_addr);
  if (!i) {
    _net_seterrno(sh, "ntoa", NET_ERR_INT, NET_ERR_BADA) ;
    goto fail ;
  }

  sh->ipaddress = malloc(strlen(i)+1) ;
  if (!sh->ipaddress) {
    _net_seterrno(sh, "ipaddress", NET_ERR_ERRNO, 0) ;
    goto fail ;
  }
  strcpy(sh->ipaddress, i) ;

  // Set non-blocking and connect to destination

  int fdoptions = fcntl(sh->fd,F_GETFL,0);

  if (fdoptions<0) {
    _net_seterrno(sh, "fcntl", NET_ERR_ERRNO, 0) ;
    goto fail ;
  }
  fcntl(sh->fd, F_SETFL, fdoptions | O_NONBLOCK);

  if ( connect(sh->fd, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr)) < 0 ) {

    int r=0, connected=0 ;

    if (errno!=EINPROGRESS) {

      _net_seterrno(sh, "connect", NET_ERR_ERRNO, 0) ; 
      goto fail ;

    } else do { 

      fd_set myset ;
      struct timeval tv ;
      tv.tv_sec = 2; 
      tv.tv_usec = 0; 
      FD_ZERO(&myset); 
      FD_SET(sh->fd, &myset); 

      r = select((sh->fd)+1, NULL, &myset, NULL, &tv); 

      if (r < 0 && errno != EINPROGRESS) {

        _net_seterrno(sh, "connect", NET_ERR_ERRNO, 0) ;
        goto fail ;

      } else if (r > 0) { 

        int valopt ;
        int lon = sizeof(int); 
        if (getsockopt(sh->fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
   
          _net_seterrno(sh, "getsockopt", NET_ERR_ERRNO, 0) ;
          goto fail ;

        }

        if (valopt) { 

          _net_seterrno(sh, "getsockopt", NET_ERR_ERRNO, valopt) ;
          goto fail ;
 
        } else {

          connected=1 ;

        }

      } else {

        _net_seterrno(sh, "connect", NET_ERR_INT, NET_ERR_TIMEOUT) ;
        goto fail ;

      }

    } while (!connected);

  }

 // Get local port

  struct sockaddr_in local_addr;
  int local_addr_len = sizeof(local_addr) ;
  memset(&local_addr, 0, sizeof(struct sockaddr_in));
  if (getsockname(sh->fd, (struct sockaddr *) &local_addr, &local_addr_len) < 0 ) {
    _net_seterrno(sh, "getsockname", NET_ERR_ERRNO, 0) ; 
     goto fail ;
  }
  sh->localport = ntohs(local_addr.sin_port) ;


  // Now establish SSL connection if required

  if (flags&TLS || flags&SSL2 || flags&SSL3) {

    // Initialise SSL

    _net_ssl_init() ;

    // Set client hello and announce SSLv3 & TLSv1

    const SSL_METHOD *method;
    method = SSLv23_client_method();
    if (!method) { 
      _net_seterrno(sh, "client_method", NET_ERR_SSL, 0) ;
      goto fail ; 
    }

    sh->ctx = SSL_CTX_new(method) ;
    if ( !sh->ctx ) { 
      _net_seterrno(sh, "ctx_new", NET_ERR_SSL, 0) ; 
      goto fail ; 
    }

    // Enable verification of full certificate chain

    if (!(flags&NOCERTCHAIN))
      SSL_CTX_set_default_verify_paths(sh->ctx) ;

    // Disable SSL if requested

    if (! (flags&SSL2) ) {
      SSL_CTX_set_options(sh->ctx, SSL_OP_NO_SSLv2);
    }

    if (! (flags&SSL3) ) {
      SSL_CTX_set_options(sh->ctx, SSL_OP_NO_SSLv3);
    }

    // XXXXXX May not be needed ...

//    if ( ! (flags&NONBLOCK) ) { 
//      SSL_CTX_set_mode(sh->ctx, SSL_MODE_ENABLE_PARTIAL_WRITE) ;
//  }

   // Enable key logging

   if (flags&DEBUGKEYDUMP) {
     sh->keydumpenable=1 ;
     SSL_CTX_set_keylog_callback(sh->ctx, _net_ssl_keylog);
   }

    // Create connection state object

    sh->ssl = SSL_new(sh->ctx);
    if (!sh->ssl) {
      _net_seterrno(sh, "ssl_new", NET_ERR_ERRNO, 0) ;
      goto fail ;
    }

    SSL_set_connect_state(sh->ssl); 

    // Attach SSL server to the socket

    SSL_set_fd(sh->ssl, sh->fd);


    // Establish SSL protocol connection

    int r ;
    while ( (r=SSL_connect(sh->ssl)) < 0 ) {

      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(sh->fd, &fds);

      int e=SSL_get_error(sh->ssl, r) ;

      switch (e) {

      case SSL_ERROR_WANT_READ:
         select(sh->fd + 1, &fds, NULL, NULL, NULL);
         break;

      case SSL_ERROR_WANT_WRITE:
         select(sh->fd + 1, NULL, &fds, NULL, NULL);
         break;

      default:
         _net_seterrno(sh, "ssl_connect", NET_ERR_SSL, r) ;
         goto fail ;
         break ;
      }

    }


    // Open /dev/null, which is used for select

    if ( !sh->isblocking && _net_devnull<0) {
      _net_devnull = open(DEVNULL, O_RDWR|O_NONBLOCK) ;
    }


  }

  // Restore blocking if required

  if ( ! (flags&NONBLOCK) ) {

    fcntl(sh->fd, F_SETFL, fdoptions);

  }

   if (flags&DEBUGDATADUMP && getenv("NETDUMPENABLE")) {
     sh->datadumpenable=1 ;
   }
  
  _net_numconnections++ ;

  return sh ;

fail: 
  _net_disconnect(sh) ;
  errno=EHOSTUNREACH ;
  return NULL ;
 
}


// 
// @brief Obtain SSL certificate status
// @param(in) Handle of open connection
// @return Non-zero certificate status code or zero if ok
//

int netcertstatus(INET *sh)
{
  // Get peer certificate

  X509 *cert = SSL_get_peer_certificate(sh->ssl);
  if (cert) {
    sh->certstatus = SSL_get_verify_result(sh->ssl);
    X509_free(cert) ;
  } else {
    sh->certstatus = -1 ;
  }

  return (sh->certstatus) ;
}


// 
// @brief Get network connection state
// @param(in) Handle of open connection
// @return Returns true if network connection is connected
//

int netisconnected(INET *sh)
{
  if (!sh) return 0 ;
  return ( sh->fd >= 0 ) ;
}

//
// @brief Determine if connection is ready for reading
// @param(in) sh Handle of network connection
// @param(in) rfds Read fd set
// @param(in) wrds Write fd set - only required for SSL
// @return True if connection has pending data
//
int netrdfdisset(INET *sh, fd_set *rfds, fd_set *wfds)
{
  if (!sh || !rfds) {

    return 0 ;

  } else if (!sh->ssl) {

    return FD_ISSET(sh->fd, rfds) ;

  } else {

    // Return true if there is data left in the SSL_read
    // Or the SSL_read needs a write
    // Or the SSL_read needs a read

    return ( ( sh->sslhaspending ) || 
             ( sh->sslwantwrite && FD_ISSET(sh->fd, wfds) ) ||
             ( FD_ISSET(sh->fd, rfds) ) ) ;

  }

}



//
// @brief Add connection to fd_set if active
// @param(in) sh Handle of network connection
// @param(in) fds FD Set for select()
// @param(inout) l pointer to largest fd found
// @return number of connections added
//

int netrdfdset(INET *sh, fd_set *rdfds, fd_set *wrfds, int *l)
{
  int newfds=0 ;

  if (!sh || !rdfds || !l) return 0 ;

  if (!sh->ssl) {

    // Add non-ssl fd

    if (sh->fd>=0) {

      FD_SET(sh->fd, rdfds) ;
      if ( sh->fd > (*l) ) { (*l) = sh->fd ; }
    }

  } else {

    // Add DEVNULL if ssl has more data available

    if ( sh->sslhaspending ) {

      FD_SET(_net_devnull, wrfds) ;
      if ( _net_devnull > (*l) ) { (*l) = _net_devnull ; }

    }

    // Add ssl socket level fd

    if ( sh->fd >= 0 ) {

      FD_SET(sh->fd, rdfds) ; // Socket level fd
      if ( sh->fd > (*l) ) { (*l) = sh->fd ; }
      newfds++ ;

      if ( sh->sslwantwrite ) {

        FD_SET(sh->fd, wrfds) ; // Socket level fd
        if ( sh->fd > (*l) ) { (*l) = sh->fd ; }
        newfds++ ;

      }

    }

  }

  return (newfds) ;

}

//
// @brief Provide summary string of read and write socket fd_set info
//

char *netfdsetinfo(INET *sh, fd_set *rfds, fd_set *wfds)
{
  static char str[32] ;
  sprintf(str, "R: %c%c W: %c",
        (FD_ISSET(sh->fd, rfds)) ? 'S' : '-',
        sh->ssl ? sh->sslhaspending ? 'O' : '-' : ' ',
        (FD_ISSET(sh->fd, wfds)) ? 'S' : '-') ;
  return str ;
}

//
// @brief Obtain peer IP address
// @param(in) sh Handle of open connection
// @return Pointer to IP address string, or NULL if not connected
//

char *netpeerip(INET *sh)
{
  if (!sh) return NULL ;
  else return sh->ipaddress ;
}

//
// @brief Obtain peer port number
// @param(in) sh Handle of open connection
// @return Peer port number, or 0 if not connected
//

int netpeerport(NET *sh) 
{
  if (!sh) return 0 ;
  else return sh->peerport ;
}


//
// @brief Obtain local port number
// @param(in) sh Handle of open connection
// @return Local port number, or 0 if not connected
//

int netlocalport(NET *sh)
{
  if (!sh) return 0 ;
  else return sh->localport ;
}

//
// @brief Disconnect connection
// @param(in) Handle of open connection
// @return true on success, or false on error (setting errno)
//
int _net_disconnect(INET *sh)
{
  if (!sh) return 0 ;

  if (sh->ssl) SSL_free(sh->ssl);
  else if (sh->fd >=0 ) close(sh->fd);
  if (sh->ipaddress) free(sh->ipaddress) ;
  if (sh->ctx) SSL_CTX_free(sh->ctx);

  sh->ssl = NULL ;
  sh->fd = -1 ;
  sh->ipaddress = NULL ;
  sh->ctx = NULL ;

  return 1 ;
}


//
// @brief Close Network connection
// @param(in) Handle of open connection
// @return true on success, or false on error (setting errno)
//
int netclose(INET *sh)
{
  if (!sh) return 0 ;

  _net_disconnect(sh) ;
  free(sh) ;

  _net_numconnections-- ;
  assert(_net_numconnections >= 0) ;
  if (_net_numconnections==0 && _net_devnull>=0) {
    close(_net_devnull) ;
    _net_devnull=-1 ;
  }

  return 1 ;
}


//
// @brief Send data to network interface
// @param(in) sh Handle of open connection
// @param(in) buf Data to be sent
// @param(in) len Amount of data to send
// @return Number of bytes sent, or -1 on error
//
 
int netsend(INET *sh, char *buf, int len)
{
  if (sh->ssl) {
    int r = SSL_write(sh->ssl, buf, len) ;
    _net_commsdump(sh, (r<=0)?">!":"> ", buf, len) ;
    _net_seterrno(sh, "netsend", NET_ERR_SSL, r) ;
    return r ;
  } else if (sh->fd) {
    int r = send(sh->fd, buf, len, 0) ;
    _net_commsdump(sh, (r<=0)?">!":"> ", buf, len) ;
    _net_seterrno(sh, "netsend", NET_ERR_ERRNO, 0) ;
  } else {
    errno = EBADF ;
    _net_seterrno(sh, "netsend", NET_ERR_ERRNO, 0) ;
    return -1 ;
  }
}


//
// @brief Receive data from network interface
// @param(in) sh Handle of open connection
// @param(in) buf Buffer to store response
// @param(in) maxlen Maximum number of bytes to read
// @return Number of bytes received, or -1 on error
//


// SSL_read doesn't return properly until entire encrypted chunk has been read
// so you can't read 4 bytes, then n bytes.  Hence flagging the need for more
// reads with sh->sslhaspending.

int netrecv(INET *sh, char *buf, int maxlen)
{
  if (sh->ssl && sh->isblocking) {

    int r = SSL_read(sh->ssl, buf, maxlen) ;
    _net_commsdump(sh, (r<=0)?"<!":"< ", buf, r) ;
    return r ;

  } else if (sh->ssl && !sh->isblocking) {

    int r = SSL_read(sh->ssl, buf, maxlen) ;

    if (r > 0) {

      // Data was returned.  Assert flag if even more available

      sh->sslhaspending = nethaspending(sh) ;
      _net_commsdump(sh, (r<=0)?"<!":"< ", buf, r) ;
      return r ;

    } else {

      switch(SSL_get_error(sh->ssl, r)) {

        case SSL_ERROR_WANT_READ:

          // Nothing required, sh->fd is always added to fd_set

          return 0 ;
          break ;

        case SSL_ERROR_WANT_WRITE:

          // SSL read required, flag to be added to write set

          sh->sslwantwrite=1 ;
          return 0 ;
          break ;

        default:

          _net_seterrno(sh, "netrecv", NET_ERR_SSL, r) ;
          return -1 ;
          break ;

      }

    }


  } else if (sh->fd) {

    int r = recv(sh->fd, buf, maxlen, 0) ;
    _net_commsdump(sh, (r<=0)?"<!":"< ", buf, r) ;
    _net_seterrno(sh, "netrecv", NET_ERR_ERRNO, 0) ;
    if (r==0) r=-1 ;
    return r ;

  } else {

    errno= EBADF ;
    _net_seterrno(sh, "netrecv", NET_ERR_ERRNO, 0) ;
    return -1 ;

  }
}

//
// @brief Returns true if pending data
// @param(in) sh Handle of open connection
// @return true if connection open and has pending data
//

int nethaspending(NET *sh)
{
  if (sh->ssl) {

    int r = SSL_pending(sh->ssl) ;
    _net_seterrno(sh, "nethaspending", NET_ERR_SSL, r) ;
    return (r > 0) ;

  } else if (sh->fd) {

    char ch ;
    int r = recv(sh->fd, &ch, 1, MSG_PEEK) ;
    _net_seterrno(sh, "nethaspending", NET_ERR_ERRNO, 0) ;
    return ( r > 0 ) ;

  }
 
}



int neterrno()
{
  return _net_errno ;
}


char *netcertstatusstr(int statusno)
{
  switch(statusno) {
  case X509_V_OK: return "certificate ok" ;
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT: return "unable to get issuer certificate" ;
  case X509_V_ERR_UNABLE_TO_GET_CRL: return "unable to get certificate CRL" ;
  case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE: return "unable to decrypt certificate's signature" ;
  case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE: return "unable to decrypt CRL's signature" ;
  case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY: return "unable to decode issuer public key" ;
  case X509_V_ERR_CERT_SIGNATURE_FAILURE: return "certificate signature failure" ;
  case X509_V_ERR_CRL_SIGNATURE_FAILURE: "CRL signature failure" ;
  case X509_V_ERR_CERT_NOT_YET_VALID: return "certificate is not yet valid" ;
  case X509_V_ERR_CERT_HAS_EXPIRED: return "certificate has expired" ;
  case X509_V_ERR_CRL_NOT_YET_VALID: return "CRL is not yet valid" ;
  case X509_V_ERR_CRL_HAS_EXPIRED: return "CRL has expired" ;
  case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD: return "format error in certificate's notAfter field" ;
  case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD: return "format error in CRL's lastUpdate field" ;
  case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD: return "format error in CRL's nextUpdate field" ;
  case X509_V_ERR_OUT_OF_MEM: return "out of memory" ;
  case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT: return "self signed certificate" ;
  case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN: return "self signed certificate in certificate chain" ;
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY: return "unable to get local issuer certificate" ;
  case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE: return "unable to verify the first certificate" ;
  case X509_V_ERR_CERT_CHAIN_TOO_LONG: return "certificate chain too long" ;
  case X509_V_ERR_CERT_REVOKED: return "certificate revoked" ;
  case X509_V_ERR_INVALID_CA: return "invalid CA certificate" ;
  case X509_V_ERR_PATH_LENGTH_EXCEEDED: return "path length constraint exceeded" ;
  case X509_V_ERR_INVALID_PURPOSE: return "unsupported certificate purpose" ;
  case X509_V_ERR_CERT_UNTRUSTED: return "certificate not trusted" ;
  case X509_V_ERR_CERT_REJECTED: return "certificate rejected" ;
  case X509_V_ERR_SUBJECT_ISSUER_MISMATCH: return "subject issuer mismatch" ;
  case X509_V_ERR_AKID_SKID_MISMATCH: return "authority and subject key identifier mismatch" ;
  case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH: return "authority and issuer serial number mismatch" ;
  case X509_V_ERR_KEYUSAGE_NO_CERTSIGN: return "key usage does not include certificate signing" ;
  case X509_V_ERR_APPLICATION_VERIFICATION: return "application verification failure" ;
  case -1: return "unable to get peer certificate" ;
  default: return "unknown error" ;
  }
}

char *netstrerrorcontext() 
{
  if (_net_errno<0) return "" ;
  else return _net_errcontext ;
}

char *netstrerror()
{

  if (_net_errno<1000) {

    return strerror(_net_errno) ;

  } else if ( _net_errno >= NET_ERR_INT && _net_errno < NET_ERR_INT+1000 ) {

    switch (_net_errno-NET_ERR_INT) {
    case NET_ERR_OK: return "OK" ;
    case NET_ERR_PTR: return "invalid pointer" ;
    case NET_ERR_BADP: return "invalid port number" ;
    case NET_ERR_BADA: return "invalid address" ;
    case NET_ERR_TIMEOUT: return "timeout establishing connection" ;
    default: return "unknown error" ;
    }

  } else if ( _net_errno >= NET_ERR_SSL && _net_errno < NET_ERR_SSL+1000 ) {

    switch (_net_errno-NET_ERR_SSL) {
    case SSL_ERROR_NONE: return "OK" ; 
    case SSL_ERROR_SSL: return "non-recoverable fatal error in SSL library" ;
    case SSL_ERROR_WANT_READ: return "want read: insufficient data available at this time" ;
    case SSL_ERROR_WANT_WRITE: return "want write: was unable to send all data at this time" ;
    case SSL_ERROR_WANT_X509_LOOKUP: return "want x509 lookup: operation did not complete, try after lookup" ;
    case SSL_ERROR_SYSCALL: return strerror(errno) ;
    case SSL_ERROR_ZERO_RETURN: return "zero return: peer has closed the connection" ;
    case SSL_ERROR_WANT_CONNECT: return "want connect: operation did not complete, try again" ;
    case SSL_ERROR_WANT_ACCEPT: return "want accept: operation did not complete, try again" ;
    case SSL_ERROR_WANT_CLIENT_HELLO_CB: return "want hello:" ;
    case SSL_ERROR_WANT_ASYNC: return "want async:" ;
    default: return "unknown error" ;
    }

  } else {

    return "unknown error" ;

  }

}

int _net_seterrno(INET *sh, char *context, enum net_errno_type type, int errcode) 
{
  if (!sh) return 0 ;

  _net_errcontext[0]='\0' ;
  if (context && strlen(context)<sizeof(_net_errcontext)-1) {
    strcpy(_net_errcontext, context) ;
  }

  if (type == NET_ERR_ERRNO && errcode>0) { 

    _net_errno = errcode ;

  } else if (type == NET_ERR_ERRNO) {

    _net_errno = errno ;

  } else if (type == NET_ERR_INT) {

    _net_errno = NET_ERR_INT + errcode ;

  } else if (!sh->ssl) {

    _net_errno = NET_ERR_INT + NET_ERR_PTR ; 

  } else if (type == NET_ERR_SSL) { 

    _net_errno = NET_ERR_SSL + SSL_get_error(sh->ssl, errcode) ;
    if (_net_errno == NET_ERR_SSL) _net_errno=errno ;

  } else {

    _net_errno = NET_ERR_INT + NET_ERR_UNK ;

  }

  return 1 ;

}


void _net_commsdump(INET *sh, char *prefix, char *buf, int buflen)
{
  if (!sh->datadumpenable) return ;

  for (int i=0; i<buflen; i+=16) {
    fprintf(stderr, " %s %s:%d - ", prefix, sh->ipaddress, sh->peerport  ) ;
    for (int j=0; j<16; j++) {
      if (i+j<buflen) {
        fprintf(stderr, "%02X", (unsigned char)buf[i+j]) ;
      } else {
        fprintf(stderr, "  ") ;
      }
    }
    fprintf(stderr, "  ") ;
    for (int j=0; j<16; j++) {
      if (i+j<buflen) {
        fprintf(stderr, "%c", (buf[i+j]>=32 && buf[i+j]<=127)?(buf[i+j]):'.') ;
      } else {
        fprintf(stderr, " ") ;
      }
    }
    fprintf(stderr, "\n") ;
  }
  if (buflen%16) fprintf(stderr, "\n") ;
}

void _net_ssl_keylog(const SSL *ssl, const char *line)
{
  char *keylog = getenv("SSLKEYLOGFILE") ;
  if (!keylog) return ;
  FILE *kf = fopen(keylog,"a") ;
  if (!kf) return ;
  fprintf(kf, "%s\n", line) ;
  fclose(kf) ;
}

