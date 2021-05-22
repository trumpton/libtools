
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "../execute.h"

int _execute_process_open(char *command, int *chin, int *chout, int *cherr) ;
int _execute_process_close(int pid, int *chout, int *chin, int *cherr) ;
int _execute_set_nonblock(int fd) ;

////////////////////////////////////////////////////////////////////////////////
//
// Executes script, provides input text
// and stores output in output buffer
// and returns stored length.
// Or -1 on error
// Or -2 on timeout
// Or -3 on output buffer overflow
// Or -4 on error reading from script's stdout
// Or -5 on error outputting to script
//

int execute(char *script, char *input, char *output, int outmax, char *error, int errmax) 
{
  int chout=-1, chin=-1, cherr=-1 ;
  int pid=-1 ;
  int ip=0, op=0, ep=0 ;
  struct timeval tv;
  int st=0 ;

  if (!script || !input || !output || outmax<=0 || errmax<0) return -1 ;

  *output='\0' ;
  if (error && errmax>0) *error='\0' ;

  pid=_execute_process_open(script, &chin, &chout, &cherr) ;

  if (pid<0) {

    printf("Script execution failed: '%s'\n", script) ;
    return -1 ;

  } else {

    int ld=-1 ;
    fd_set rfds, wfds ;
    int finished=0 ;

    _execute_set_nonblock(chout) ;
    _execute_set_nonblock(chin) ;
    _execute_set_nonblock(cherr) ;

    do {

      int lp = 0;
      FD_ZERO(&rfds);
      FD_ZERO(&wfds);

      if (chin >=0) {
        FD_SET(chin, &wfds);
        if (chin > lp) lp = chin;
      }

      if (chout >= 0) {
        FD_SET(chout, &rfds);
        if (chout > lp) lp = chout;
      }

      if (cherr >=0) {
        FD_SET(cherr, &rfds);
        if (cherr > lp) lp = cherr;
      }

      tv.tv_sec = 5;
      tv.tv_usec = 0;
      int n = select(lp + 1, &rfds, &wfds , NULL, &tv);

      if ( chin>=0 && FD_ISSET(chin, &wfds) ) {
        // handle write to script
        switch (write(chin, &input[ip++], 1)) {
        case -1:
        case 0: // log error writing to script
          close(chin) ;
          chin=-1 ;
          st=-5 ;
          break ;
        case 1: // OK, character written to script input
          if (input[ip]=='\0') {
            close(chin) ;
            chin=-1 ;
          }
          break ;
        }
      }

      // handle read from script stderr
      if ( cherr>=0 && FD_ISSET(cherr, &rfds) ) {
        char ch ;
        switch (read(cherr, &ch, 1)) {
        case -1: // Quietly close file
        case 0:
          close(cherr) ;
          cherr  =-1 ;
          break ;
        case 1: // Output up to errmax characters
          if (!error || ep>=(errmax-1)) {
            // Skip buffer overflows 

          } else if (ch=='\t') {
            error[ep++]=' ' ;

          } else if (ch=='\n') {
            error[ep++]='.' ;
            if (ep<(errmax-1)) error[ep++]=' ' ;

          } else if (ch<32 || ch>127) {
            // Skip non-printable characters

          } else {

            error[ep++]=ch ;
          }
          error[ep]='\0' ;
          break ;
        }
      }

      // handle read from script stdout
      if ( chout>=0 && FD_ISSET(chout, &rfds) ) {
        char ch ;
        switch (read(chout, &ch, 1)) {
        case -1: // Log error reading
          close(chout) ;
          chout=-1 ;
          st=-4 ;
          break ;
        case 0: // End of file, stop processing
          close(chout) ;
          chout=-1 ;
          break ;
        case 1: // Store character or error on overflow
          if (op<(outmax-1)) {
            output[op++]=ch ;
            output[op]='\0' ; 
          } else {
            finished=1 ;
            st=-3 ;
          }
          break ;
        }
      }

      if (n==0 && !finished) {
        // Handle error timeout
        finished=1 ;
        st=-2 ;
        // TODO error, timeout
      }

      if (chout<0 && cherr<0) {
        finished=(1==1) ;
      }

    } while (!finished) ;

  }

  int clst=_execute_process_close(pid, &chin, &chout, &cherr) ;

  if (error && errmax>64) {
    switch (st) {
      case -1: strcpy(error, "script fork error") ; break ;
      case -2: strcpy(error, "script timeout") ; break ;
      case -3: strcpy(error, "script response buffer overflow") ; break ;
      case -4: strcpy(error, "script error receiving response data") ; break ;
      case -5: strcpy(error, "script error writing data") ; break ;
    }
  }

  return (st<0) ? st : clst ;
}

////////////////////////////////////////////////////////////////////////////////
//
// Support (local) functions
//

// Set file descriptor to non-blocking

int _execute_set_nonblock(int fd)
{
  int fl=fcntl(fd, F_GETFL, 0) ;
  if (fl<0) { return 0 ; }
  if (fcntl(fd, F_SETFL, fl|O_NONBLOCK)<0) { return 0 ; }
  return 1 ;
}


// Launch process and return PID or -1 on error

int _execute_process_open(char *command, int *chin, int *chout, int *cherr) {

  int tochild[2], fromchild[2], errfromchild[2];
  if (pipe(tochild)<0 || pipe(fromchild)<0 || pipe(errfromchild)) {
    return -1 ;
  }
  int pid=fork() ;
  switch (pid) {
  default:
    // Parent
    *chout=fromchild[0]; *chin=tochild[1] ; *cherr=errfromchild[0] ;
    close(tochild[0]); close(fromchild[1]); close(errfromchild[1]) ;
    if (*chout>=0 && *chin>=0 && *cherr>=0) return pid ;
    break ;
  case -1:
    // Fork error
    break ;
  case 0:
    // Forked child
    dup2(tochild[0], 0); dup2(fromchild[1], 1); dup2(errfromchild[1], 2) ;
    close(tochild[0]); close(tochild[1]);
    close(fromchild[0]); close(fromchild[1]);
    close(errfromchild[0]); close(errfromchild[1]);
    _execute_set_nonblock(0) ; _execute_set_nonblock(1) ; _execute_set_nonblock(2) ;
    execl( "/bin/sh", "sh", "-c", command, NULL );
    _exit(1);
  }
  if (*cherr>=0) close(*cherr) ;
  if (*chin>=0) close(*chin) ;
  if (*chout>=0) close(*chout) ;
  (*chout)=-1 ; (*chin)=-1 ; (*cherr)=-1 ;
  return -1;  
}

// Close process and return process return code (0=success)

int _execute_process_close(int pid, int *chin, int *chout, int *cherr)
{
  int status=0;
  if (*chout>=0) close(*chout);
  if (*chin>=0) close(*chin);
  if (*cherr>=0) close(*cherr);
  if (pid>0) waitpid(pid, &status, 0);
  (*chout)=-1 ; (*chin=-1) ; (*cherr)=-1 ;
  return status/256 ;
}
