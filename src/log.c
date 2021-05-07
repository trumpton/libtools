//
// log.c
//
//

#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "../log.h"


static char *_config_logname=NULL ;
FILE * _log_snoopfh=NULL ;
int _log_level=0 ;

void logopen(char *appname)
{
  _log_snoopfh=NULL ;
  #ifdef DEBUG
    _config_logname=appname ;
  #else
    openlog(appname, LOG_PID, LOG_DAEMON);
  #endif
}

void logmsg(int level, char *format, ...)
{
#ifdef DEBUG

  if (level<_log_level) {

    time_t currenttime=time(NULL) ;
    struct tm *tmb ;
    tmb=localtime(&currenttime) ;

    printf("%02d:%02d:%02d %s (%d) %s:", 
      tmb->tm_hour, tmb->tm_min, tmb->tm_sec, 
      _config_logname, 
      getpid(),  
      (level==LOG_EMERG)?"emergency":
      (level==LOG_ALERT)?"alert":
      (level==LOG_CRIT)?"critical":
      (level==LOG_ERR)?"error":
      (level==LOG_WARNING)?"warning":
      (level==LOG_NOTICE)?"notice":
      (level==LOG_INFO)?"info":
      (level==LOG_DEBUG)?"debug":
      (level==LOG_INT)?"internal":
      "??????") ;
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);  
    printf("\n") ;
    fflush(stdout) ;
  }

#else

  if (level!=LOG_INT) {
    va_list args ;
    va_start(args, format) ;
    vsyslog(level, format, args) ;
    va_end(args) ;
  }

#endif

}

void logsetsnoopfd(int fd) 
{
  if (_log_snoopfh!=NULL) {
    fclose(_log_snoopfh) ;
  }
  _log_snoopfh=fdopen(fd,"wb") ;
}

void logsnoopmsg(char *prefix, char *buf, int buflen, int showall)
{
  if (!_log_snoopfh) return ;
  time_t currenttime=time(NULL) ;
  struct tm *tmb ;
  tmb=localtime(&currenttime) ;
  int newline=(1==1) ;
  int i=0 ;
  int l=0 ;
  enum snoop_state { LS_ALL, LS_TOP } state ;
  int linecount=SNOOPLINECOUNT1 ;
  if (showall) state=LS_ALL ;
  else state=LS_TOP ;

  fprintf(_log_snoopfh, "%02d:%02d:%02d %s ", 
  tmb->tm_hour, tmb->tm_min, tmb->tm_sec,prefix) ;
  newline=(1==0) ;

  // Calculate start of last line
  int llp ;
  for (llp=buflen-1; llp>0 && buf[llp]!='\n'; llp--) ;
  if (llp>0) llp++ ;

  for (i=0,l=0,newline=(1==0); i<buflen; i++,l++) {

    if (l==SNOOPMAXLINELEN && buf[i]!='\n') {
      fprintf(_log_snoopfh, " ...") ;
    } 

    if (buf[i]=='\r') {
      // Ingore carriage returns

    } else if (buf[i]=='\n') {
      // Output newline and mark next entry for data/time output
      fprintf(_log_snoopfh, "\n") ;
      newline=(1==1) ; l=0 ;

    } else if (buf[i]>=32 && buf[i]<=127 && l<SNOOPMAXLINELEN) {
      // Valid character, so output it
      fprintf(_log_snoopfh,"%c",buf[i]) ;

    } else if (l<SNOOPMAXLINELEN) {
      // Output '.' for non-printable character
      fprintf(_log_snoopfh,".") ;

    }

    if (state==LS_TOP && linecount<=0 && newline) {

      // count back SNLC2 newlines
      int j, lc ;
      for (j=buflen,lc=SNOOPLINECOUNT2; lc>0 && j>i; j--) {
        if (buf[j]=='\n') lc-- ;
      }

      // If haven't counted back to current position, skip
      if (j>i) { 
        fprintf(_log_snoopfh, "%02d:%02d:%02d %s ........\n", 
        tmb->tm_hour, tmb->tm_min, tmb->tm_sec,prefix) ;
        i=j+1 ; 
      }

      // From now on, output everything remaining
      state=LS_ALL ;
    }

    // If on the last line, show the end of it
    if ((i+1)==llp) {
      newline=(1==1) ;
      if (buflen-i>SNOOPMAXLINELEN) { i=buflen-SNOOPMAXLINELEN ; }
    }

    if (newline) {
      fprintf(_log_snoopfh, "%02d:%02d:%02d %s ", 
      tmb->tm_hour, tmb->tm_min, tmb->tm_sec,prefix) ;
      newline=(1==0) ;
      linecount-- ;
    }
 
  }

  fprintf(_log_snoopfh, "\n\n") ;
  fflush(_log_snoopfh) ;
}


void logclose() {
  closelog() ;
  if (_log_snoopfh) {
    fclose(_log_snoopfh) ;
    _log_snoopfh=NULL ;
  }
}

int logsetlevel(char *sysloglevel) {

  if (!sysloglevel || sysloglevel[0]=='\0') {
    // User has not specified log level, so quietly use default
    logmsg(LOG_NOTICE, "log level set to: %s", LOGLEVEL_DEFAULTSTR) ;
    setlogmask(LOG_UPTO(LOGLEVEL_DEFAULT)) ;
    return (1==1) ;
  }

  if (strcmp(sysloglevel, "emergency")==0) {
    setlogmask(LOG_UPTO(LOG_EMERG)) ;
    _log_level=LOG_EMERG ;
  } else if (strcmp(sysloglevel, "alert")==0) {
    setlogmask(LOG_UPTO(LOG_ALERT)) ;
    _log_level=LOG_ALERT ;
  } else if (strcmp(sysloglevel, "critical")==0) {
    setlogmask(LOG_UPTO(LOG_CRIT)) ;
    _log_level=LOG_CRIT ;
  } else if (strcmp(sysloglevel, "error")==0) {
    setlogmask(LOG_UPTO(LOG_ERR)) ;
    _log_level=LOG_ERR ;
  } else if (strcmp(sysloglevel, "warning")==0) {
    setlogmask(LOG_UPTO(LOG_WARNING)) ;
    _log_level=LOG_WARNING ;
  } else if (strcmp(sysloglevel, "notice")==0) {
    setlogmask(LOG_UPTO(LOG_NOTICE)) ;
    _log_level=LOG_NOTICE ;
  } else if (strcmp(sysloglevel, "info")==0) {
    setlogmask(LOG_UPTO(LOG_INFO)) ;
    _log_level=LOG_INFO ;
  } else if (strcmp(sysloglevel, "debug")==0) {
    setlogmask(LOG_UPTO(LOG_DEBUG)) ;
    _log_level=LOG_DEBUG ;
  } else if (strcmp(sysloglevel, "internal")==0) {
    setlogmask(LOG_UPTO(LOG_DEBUG)) ;
    _log_level=LOG_INT ;
  } else {
    // Unrecognised log level, so default to NOTICE
    setlogmask(LOG_UPTO(LOG_NOTICE)) ;
    logmsg(LOG_WARNING, "unrecognised log level in config file, defaulting to 'notice'") ;
    return (1==0) ;
  }

  logmsg(LOG_NOTICE, "log level set to: %s", sysloglevel) ;
  return (1==1) ;
}

