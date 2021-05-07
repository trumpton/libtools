//
// httpd.h
//
// These functions are designed to provide a simple
// httpd server, which can accept get and post requests.
// All file descriptors are set to non-blocking and
// are expected to be used with the select function.
//
//
// Manage httpd server
//
//   int httpd_init(int port, int numconnections) ;
//   int httpd_listenfd() ;
//   int httpd_shutdown() ;
//
// Manage HTTPD session
//
//   HTTPD *haccept(int listenfd) ;
//   int hrecv(HTTPD *hh) ;
//   int hfd(HTTPD *hh) ;
//   char *hgeturi(HTTPD *hh) ;
//   char *hgeturiparam(HTTPD *hh, char *param) ;
//   char *hgetbody(HTTPD *hh) ;
//   int hclose(HTTPD *hh) ;
//

#ifndef _HTTPD_DEFINED
#define _HTTPD_DEFINED

#ifndef HTTPD
typedef struct {} HTTPD ;
#endif

///////////////////////////////////////////////////////////////////////
//
// @brief Initialises httpd server
// @param[in] port Port number to listen on
// @return true on success
//

int httpd_init(int port) ;


///////////////////////////////////////////////////////////////////////
//
// @brief Returns httpd server listen handle
// @return listener handle
//

int httpd_listenfd() ;


///////////////////////////////////////////////////////////////////////
//
// @brief Shuts down the http server
// @return true
//


///////////////////////////////////////////////////////////////////////
//
// @brief Returns httpd server IP Address / port
// @return ip address / port
//

char * httpd_ipaddress() ;
int httpd_port() ;


int httpd_shutdown() ;



///////////////////////////////////////////////////////////////////////
//
// @brief Start HTTPD session
// param[in] listenfd File descriptor of listener
// return Handle of HTTPD session
//

HTTPD *haccept(int listenfd) ;


///////////////////////////////////////////////////////////////////////
//
// @brief Get Length of time current connection has been established
// param[in] hh Handle of HTTPD session
// return Number of seconds, or -1 if not connected
//

int hconnectiontime(HTTPD *hh) ;


///////////////////////////////////////////////////////////////////////
//
// @brief Get session file descriptor
// param[in] hh Handle of HTTPD session
// return File descriptor associated with handle
//

int hfd(HTTPD *hh) ;


///////////////////////////////////////////////////////////////////////
//
// @brief Get peer network connection details
// param[in] hh Handle of HTTPD session
// return Peer network information
//

char *hpeeripaddress(HTTPD *hh) ;
int hpeerport(HTTPD *hh) ;


///////////////////////////////////////////////////////////////////////
//
// @brief Receive data (keep calling until it returns non-zero)
// param[in] hh Handle of HTTPD session
// @return 0 - Still receiving, call back later
// @return -1 - Connection closed
// @return positive number - xxx http response code (200=OK)

int hrecv(HTTPD *hh) ;


//
// @brief Returns base URI
// param[in] hh Handle of HTTPD session
// @return Transient pointer to uri, or NULL if hrecv incomplete
//

char *hgeturi(HTTPD *hh) ;


//
// @brief Returns nth URI ? Parameter
// param[in] hh Handle of HTTPD session
// param[in] n Index of parameter to return
// @return Transient pointer to parameter name or NULL if not found
//

char *hgeturiparamname(HTTPD *hh, int n) ;


//
// @brief Returns URI ? Parameter
// param[in] hh Handle of HTTPD session
// param[in] param Parameter to search for
// @return Transient pointer to parameter value, or NULL if not found
//

char *hgeturiparamstr(HTTPD *hh, char *param) ;


//
// @brief Returns URI ? Parameter
// param[in] hh Handle of HTTPD session
// param[in] param Parameter to search for
// @return Newly allocated string containing data or NULL if not found
//

int hgeturiparamint(HTTPD *hh, char *param, int *i) ;


//
// @brief Returns URI ? Parameter
// param[in] hh Handle of HTTPD session
// param[in] param Parameter to search for
// param[out] f pointer to float for result
// @return True on success
//

int hgeturiparamfloat(HTTPD *hh, char *param, float *f) ;



//
// @brief Returns request body
// param[in] hh Handle of HTTPD session
// @return Transient pointer to request body, or NULL if not present
//

char *hgetbody(HTTPD *hh) ;



///////////////////////////////////////////////////////////////////////
//
// @brief Send response message
// param[in] hh Handle of HTTPD session
// param[in] code Response code (e.g. 200)
// param[in] contenttype Content-type for response (or NULL if no body)
// param[in] body Contents for body (or NULL if no body)
// @return true on success

int hsend(HTTPD *hh, int code, char *contenttype, char *body, ...) ;


///////////////////////////////////////////////////////////////////////
//
// @brief Send binary response message
// param[in] hh Handle of HTTPD session
// param[in] code Response code (e.g. 200)
// param[in] contenttype Content-type for response (or NULL if no body)
// param[in] body Contents for body (or NULL if no body)
// param[in] bodylen Length of body (or 0 if NULL)
// @return true on success

int hsendb(HTTPD *hh, int code, char *contenttype, char *body, int bodylen) ;



//
// @brief Shutdown httpd server
// @return true on success
//

int hclose(HTTPD *hh) ;

#endif
