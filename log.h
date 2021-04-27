//
// log.h
//

#ifndef _LOG_DEFINED_
#define _LOG_DEFINED_

#include <syslog.h>
#define LOG_INT 999

#define LOGLEVEL_DEFAULT LOG_NOTICE
#define LOGLEVEL_DEFAULTSTR "notice"

// Max Line length for Snoop entries
// Max number of lines at top and bottom of snoop entries
// for non-showall requests
#define SNOOPMAXLINELEN 60
#define SNOOPLINECOUNT1 5
#define SNOOPLINECOUNT2 5


//
// Set log level to: 
//    emergency, alert, critical, error, warning, 
//    notice, info, debug
//
// returns true on success
// sets level to notice and returns false
//
int logsetlevel(char *level) ;

//
// Open log, using appname as descriptor
//
void logopen(char *appname) ;

//
// Appends message to log file.  type is one of:
//    LOG_EMERG, LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING, 
//    LOG_NOTICE, LOG_INFO, LOG_DEBUG, LOG_INT
//
void logmsg(int type, char *format, ...) ;

//
// Close the log file
//
void logclose() ;

//
// Set and Send Logs to Snoop
//
void logsetsnoopfd(int fd) ;
void logsnoopmsg(char *prefix, char *buf, int buflen, int showall) ;

#endif
