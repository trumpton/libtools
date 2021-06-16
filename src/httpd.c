//
//
// httpd.c
//
//
//

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdarg.h>

#include "../log.h"
#include "../mem.h"
#include "../str.h"


enum estate { URI, HEAD, BODY, COMPLETE, CLOSED, ERROR } ;

typedef struct { 
  int fd ;
  enum estate state ;
  int hasbody ;
  int bodylen ;
  int uricount ;
  mem *uri ;
  mem *body ;
  mem *transient ;
  mem *peeripaddress ;
  int peerport ;
  time_t connect_time ;
} IHTTPD ;

#define HTTPD IHTTPD
#include "../httpd.h"



// HTTPD Listener port and handle

int _httpd_listenport ;
int _httpd_listenfd ;

// Local functions

int _httpd_openlistenfd() ;
int _httpd_closelistenfd() ;

// Local constants

#define HTTPD_CONCURRENT_CONNECTIONS 16

///////////////////////////////////////////////////////////////////////
//
// @brief Initialises httpd server
// @param[in] port Port number to listen on
// @return true on success
//

int httpd_init(int port)
{
  _httpd_listenfd=-1 ;
  _httpd_listenport=port ;
  return httpd_listenfd() ;
}


///////////////////////////////////////////////////////////////////////
//
// @brief Returns httpd server listen handle
// @return listener handle
//

int httpd_listenfd()
{
  if (_httpd_listenfd>=0) return _httpd_listenfd ;
  else return _httpd_openlistenfd() ;
}


///////////////////////////////////////////////////////////////////////
//
// @brief Returns httpd server IP Address / port
// @return ip address / port
//

char * httpd_ipaddress()
{
  static char ipaddress[32] ;
 
  strcpy(ipaddress, "0.0.0.0") ;

  struct ifaddrs *addrs ;
  getifaddrs(&addrs);

  if (addrs) {

    struct ifaddrs *tmp = addrs;

    while (tmp) {

      if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
        struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
        char *addr = inet_ntoa(pAddr->sin_addr);
        if (addr && strncmp(addr, "127", 3)!=0) {
          strncpy(ipaddress, addr, sizeof(ipaddress)-1) ;
        }
      }

      tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs) ;

  }


  return ipaddress ;

}


int httpd_port()
{
  return _httpd_listenport ;
}



///////////////////////////////////////////////////////////////////////
//
// @brief Shuts down the http server
// @return true
//

int httpd_shutdown() 
{
  return _httpd_closelistenfd() ;
}



///////////////////////////////////////////////////////////////////////
//
// @brief Start HTTPD session
// param[in] listenfd File descriptor of listener
// return Handle of HTTPD session
//

#define BUFLEN 32768

IHTTPD *haccept(int listenfd)
{
  IHTTPD *hh=NULL ;
  struct sockaddr_in cli_addr;
  int clilen = sizeof(cli_addr);

  int sessionfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen);
  if (sessionfd<0) {
    goto error ;
  }

  int flags = fcntl(sessionfd,F_GETFL,0);
  if (flags<0) {
    goto error ;
  }
  fcntl(sessionfd, F_SETFL, flags | O_NONBLOCK);


  hh = (HTTPD *)mem_malloc(sizeof(HTTPD)) ;
  if (!hh) {
    goto error ;
  }

  // Store connection details

  char *ip = inet_ntoa(cli_addr.sin_addr);
  hh->peerport = ntohs(cli_addr.sin_port) ;
  hh->peeripaddress = mem_malloc(strlen(ip)+1) ;
  if (!hh->peeripaddress) {
    goto error ;
  }
  strcpy(hh->peeripaddress, ip) ;


  hh->transient = mem_malloc(BUFLEN) ;
  if (!hh->transient) {
    goto error ;
  }

  hh->fd = sessionfd ;
  hh->state = URI ;
  hh->connect_time = time(NULL) ;

  return hh ;

error:
  logmsg(LOG_CRIT, "Unable to accept connection - %s", strerror(errno)) ;
  if (sessionfd>=0) close(sessionfd) ;
  if (hh && hh->peeripaddress) mem_free((mem *)hh->peeripaddress) ;
  if (hh && hh->transient) mem_free((mem *)hh->transient) ;
  if (hh) mem_free((mem *)hh) ;
  return NULL ;


}

int hconnectiontime(IHTTPD *hh)
{
  if (!hh || hh->fd<0) return -1 ;
  return ((int)time(NULL)-hh->connect_time) ;
}


///////////////////////////////////////////////////////////////////////
//
// @brief Get peer network connection details
// param[in] hh Handle of HTTPD session
// return Peer network information
//

char *hpeeripaddress(IHTTPD *hh) 
{
  if (!hh) return "" ;
  else return hh->peeripaddress ;
}

int hpeerport(IHTTPD *hh) 
{
  if (!hh) return 0 ;
  else return hh->peerport ;
}



///////////////////////////////////////////////////////////////////////
//
// @brief Get session file descriptor
// param[in] hh Handle of HTTPD session
// return File descriptor associated with handle
//

int hfd(IHTTPD *hh)
{
  if (hh==NULL) return -1 ;
  else return (hh->fd) ;
}


///////////////////////////////////////////////////////////////////////
//
// @brief Receive data
// param[in] hh Handle of HTTPD session
// @return 0 - Still receiving, call back later
// @return -1 - Connection closed
// @return positive number - xxx http response code (200=OK)

int hrecv(IHTTPD *hh)
{
  if (hh==NULL || hh->state==CLOSED) return -1 ;
  
  char ch[2] ; ch[1]='\0' ;
  switch (recv(hh->fd, ch, 1, 0)) {

  case 0:  // connection closed

    hh->state=CLOSED ;
    return -1 ; // -1:Terminated
    break ;

  case -1: // connection terminated

    hh->state=ERROR ;
    return -1 ; // -1:Terminated
    break ;

  case 1:  // character received

    if (hh->state==ERROR) {
      return -1 ; // -1:Terminated
    }

    if (*ch=='\r') {

      // Ignore \r
      return 0 ; // 0:Continue

    } else {

      if (!str_strcat(hh->transient, ch)) {

        hh->state=ERROR ;
        return 431 ; // 431:HeaderOverflow

      }

      switch (hh->state) {

      case URI:

        if (*ch=='\n') {

          int urioffset = str_offseti(hh->transient, " ") ;
          if (urioffset<0) {
            hh->state=ERROR ;
            return 414 ; // 414:BadURI
          }
          int urilen = str_offset(&(hh->transient)[urioffset+1], " ") ;
          if (urilen<=0) {
            hh->state=ERROR ;
            return 414 ; // 414:BadURI
          } else {
            hh->transient[urioffset+urilen+1]='\0' ;
          }

          hh->hasbody = (tolower(*(hh->transient))=='p') ;

          str_replaceall(hh->transient, "\r", "") ;
          str_replaceall(hh->transient, "?", "\n") ;
          str_replaceall(hh->transient, "&", "\n") ;

          str_decode(hh->transient) ;

          str_insert(hh->transient, 0, urioffset+1, "") ;
          hh->uri = mem_malloc(strlen(hh->transient)+1) ;
          if (!hh->uri) return 500 ; // 500:InternalServerError

          if (!str_strcpy(hh->uri, hh->transient)) {
            logmsg(LOG_CRIT, "strcpy failed for some reason") ;
          }

          // Replace all \n with \0 and count number of params
          // which is number of \n + 1
          hh->uricount=1 ;
          int len=strlen(hh->uri) ;
          for (int i=0; i<len; i++) {
            if (hh->uri[i]=='\n') {
              hh->uri[i]='\0' ;
              hh->uricount++ ;
            }
          }

          hh->state=HEAD ;
          *(hh->transient) = '\0' ;
          return 0 ; // 0:Continue

        }
        break ;

      case HEAD:

        if (str_offset(hh->transient, "\n\n")>0) {

          if (hh->hasbody) {

            int lo = str_offseti(hh->transient, "Content-Length:") ;
            if (lo<0) {
              hh->state=ERROR ;
              return 411 ; // 411:LengthRequired
            }
            hh->bodylen = atoi(&(hh->transient[lo+16])) ;

            if (hh->bodylen==0) {

              // POST has no data
              hh->state = COMPLETE ;
              return 200 ; // 200:OK

            } else {

              hh->state = BODY ;
              *(hh->transient) = '\0' ;
              return 0 ; // 0:Continue

            }

          } else {

           hh->state = COMPLETE ;
           return 200 ; // 200:OK

          }

        }
          
        break ;

      case BODY:

        hh->bodylen-- ;

        if (hh->bodylen<=0) {

          hh->body = mem_malloc(strlen(hh->transient)+1) ;
          if (!hh->body) {
            hh->state=ERROR ;
            return 500 ; // 500:InternalServerError
          }
          str_strcpy(hh->body, hh->transient) ;

          hh->state=COMPLETE ;
          return 200 ; // 200:OK
        }

        break ;

      default:
        perror("hrecv: unexpected state") ;
        return 500 ; // 500:InternalServerError
        break ;

      }

    }

  }

  return 0 ;

}


///////////////////////////////////////////////////////////////////////
//
// @brief Returns base URI
// param[in] hh Handle of HTTPD session
// @return Transient pointer to uri, or NULL if hrecv incomplete
//

char *hgeturi(IHTTPD *hh)
{
  if (!hh || hh->state==ERROR) return NULL ;
  return hh->uri ;

}


///////////////////////////////////////////////////////////////////////
//
// @brief Returns nth URI ? Parameter
// param[in] hh Handle of HTTPD session
// param[in] n Index of parameter to return
// @return Transient pointer to parameter name or NULL if not found
//

char *hgeturiparamname(HTTPD *hh, int n) 
{
  static char staticresponse[128] ;

  if (n<0 || n>=hh->uricount) return NULL ;
  int p=0 ;

  // Skip to record  
  while (n>0) {
    while (hh->uri[p]!='\0') p++ ;
    p++ ;
    n-- ;
  }

  int i ;
  for (i=0; i<sizeof(staticresponse)-1 && hh->uri[p+i]!='\0' && hh->uri[p+i]!='='; i++) {
    staticresponse[i] = hh->uri[p+i] ;
  }
  staticresponse[i]='\0' ;

  return staticresponse;
}


///////////////////////////////////////////////////////////////////////
//
// @brief Returns URI ? Parameter
// param[in] hh Handle of HTTPD session
// param[in] param Parameter to search for
// @return Transient pointer to parameter, or NULL if not found
//

char *hgeturiparamstr(IHTTPD *hh, char *param)
{

  if (!hh || hh->state==ERROR) return NULL ;

  int index=0 ;
  int p=0 ;
  int lenparam=strlen(param) ;

  do {

    if ( strncmp( &(hh->uri[p]), param, lenparam ) == 0 &&
         hh->uri[p+lenparam]=='=') {

      return &(hh->uri[p+lenparam+1]) ;

    }

    index++ ;
    if (index<hh->uricount) {
      while (hh->uri[p]!='\0') p++ ;
      p++ ;
    }

  } while (index<hh->uricount) ;

  return NULL ;

}


//
// @brief Returns URI ? Parameter
// param[in] hh Handle of HTTPD session
// param[in] param Parameter to search for
// param[out] i Pointer to integer for result
// @return True on success
//

int hgeturiparamint(HTTPD *hh, char *param, int *i) 
{
  if (!hh || !i) return 0 ;
  char *p = hgeturiparamstr(hh, param) ;
  if (!p) return 0 ;
  (*i) = atoi(p) ;
  return 1 ;
}

//
// @brief Returns URI ? Parameter
// param[in] hh Handle of HTTPD session
// param[in] param Parameter to search for
// param[out] f pointer to float for result
// @return True on success
//

int hgeturiparamfloat(HTTPD *hh, char *param, float *f) 
{
  if (!hh || !f) return 0 ;
  char *p = hgeturiparamstr(hh, param) ;
  if (!p) return 0 ;
  sscanf(p, "%f", f) ;
  return 1 ;
}


///////////////////////////////////////////////////////////////////////
//
// @brief Returns request body
// param[in] hh Handle of HTTPD session
// @return Transient pointer to request body, or NULL if not present
//

char *hgetbody(IHTTPD *hh)
{

  if (!hh || hh->state==ERROR) return NULL ;

  if (!hh || hh->state!=COMPLETE) return NULL ;

  return (hh->body) ;

}


///////////////////////////////////////////////////////////////////////
//
// @brief Send response message
// param[in] hh Handle of HTTPD session
// param[in] code Response code (e.g. 200)
// param[in] contenttype Content-type for response (or NULL if no body)
// param[in] body Contents for body (or NULL if no body)
// @return true on success

int hsend(IHTTPD *hh, int code, char *contenttype, char *body, ...) 
{
  int response ;
  va_list args;
  char *messagebuf=NULL ;

  if (!hh || !contenttype || !body || *body=='\0') return 0 ;

  // Expand varargs

  va_start (args, body);
  size_t messagelen=vsnprintf(messagebuf, 0, body, args) ;

  messagebuf=malloc(messagelen+1) ;
  if (!messagebuf) return 0 ;

  va_start (args, body);
  vsnprintf(messagebuf, messagelen+1, body, args) ;

  response = hsendb(hh, code, contenttype, messagebuf, strlen(messagebuf)) ;

  free(messagebuf) ;

  return response ;

}

///////////////////////////////////////////////////////////////////////
//
// @brief Send binary response message
// param[in] hh Handle of HTTPD session
// param[in] code Response code (e.g. 200)
// param[in] contenttype Content-type for response (or NULL if no body)
// param[in] body Contents for body (or NULL if no body)
// param[in] bodylen Length of body (or 0 if NULL)
// @return true on success

int hsendb(IHTTPD *hh, int code, char *contenttype, char *body, int bodylen) 
{

  if ( !hh || hh->fd < 0 ) return 0 ;

  int success=0 ;
  int headlen=0 ;
  mem *head ;

  head = mem_malloc(8192) ;
  if (!head) goto fail ;

  str_strcpy(head, "HTTP/1.1 ") ;
  str_intcat(head, code) ;
  if (code<200) { str_strcat(head, " Info\r\n") ; }
  else if (code<300) { str_strcat(head, " OK\r\n") ; }
  else if (code<400) { str_strcat(head, " Redirect\r\n") ; }
  else if (code<500) { str_strcat(head, " Client Error\r\n") ; }
  else { str_strcat(head, " Server Error\r\n") ; }
  str_strcat(head, "Connection: close\r\n") ;

  if (body && contenttype) {
    str_strcat(head, "Content-Type: ") ;
    str_strcat(head, contenttype) ;
    str_strcat(head, "\r\nContent-Length: ") ;
    str_intcat(head, bodylen) ;
#ifndef NOCORS
    str_strcat(head, "\r\nAccess-Control-Allow-Origin: *") ;
    str_strcat(head, "\r\nAccess-Control-Allow-Headers: *") ;
    str_strcat(head, "\r\nAccess-Control-Allow-Methods: *") ;
#endif
    str_strcat(head, "\r\n") ;
  }

  if (!str_strcat(head, "\r\n")) goto fail ;

  headlen = strlen(head) ;

  
  success = (write(hh->fd, head, headlen)==headlen) ;

  int written, writepos=0 ;

  do {
    written = write(hh->fd, &body[writepos], bodylen-writepos) ;
    if (written<0 && (errno==EAGAIN || errno==EWOULDBLOCK) ) {
      usleep(50) ;
      written=0 ;
    } else {
      writepos+=written ;
    }
  } while (writepos<bodylen && written>=0) ;


fail:

  mem_free(head) ;
  return success ;


}


///////////////////////////////////////////////////////////////////////
//
// @brief Shutdown httpd server
// @return true on success
//

int hclose(IHTTPD *hh)
{

  if (!hh || !mem_free(hh->transient)) return 0 ;
  mem_free(hh->peeripaddress) ;
  mem_free(hh->uri) ;
  mem_free(hh->body) ;
  close(hh->fd) ;
  return mem_free((mem *)hh) ;

}



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//
// Local Functions
//


///////////////////////////////////////////////////////////////////////
//
// @brief Open listener network connection
// return File descriptor for listener, or -1 on failure
//

int _httpd_openlistenfd()
{
  struct sockaddr_in srv;

  _httpd_closelistenfd() ;

  // Create socket

  if ( (_httpd_listenfd = socket(AF_INET , SOCK_STREAM , 0)) < 0 ){
    perror("_httpd_openlistenfd: error creating socket");
    _httpd_listenfd=-1 ;
    return -1 ;
  }
  int flag_on = 1;
  setsockopt(_httpd_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag_on, sizeof(flag_on));

  // Server (input) settings

  memset(&srv, 0, sizeof(srv));
  srv.sin_family = AF_INET;
  srv.sin_port = htons(_httpd_listenport);
  srv.sin_addr.s_addr = htons(INADDR_ANY) ;

  // Bind server to port

  if( bind(_httpd_listenfd, (struct sockaddr*) &srv, sizeof(srv)) < 0 ) {

    perror("_httpd_openlistenfd: error binding to socket");
    close(_httpd_listenfd);
    _httpd_listenfd=-1 ;
    return -1 ;

  }

  // Set non-blocking

  int flags = fcntl(_httpd_listenfd,F_GETFL,0);
  assert(flags != -1);
  fcntl(_httpd_listenfd, F_SETFL, flags | O_NONBLOCK);

  listen(_httpd_listenfd, HTTPD_CONCURRENT_CONNECTIONS) ;

  // And return handle

  return _httpd_listenfd ; 
}



///////////////////////////////////////////////////////////////////////
//
// @brief Close listener network connection
// return true
//

int _httpd_closelistenfd()
{
  if (_httpd_listenfd>=0) close(_httpd_listenfd) ;
  _httpd_listenfd=-1 ;
  return 1 ;
}


