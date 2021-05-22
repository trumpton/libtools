//
// common/execute.h
//

#ifndef _EXECUTE_DEFINED
#define _EXECUTE_DEFINED

////////////////////////////////////////
//
// Executes script
//
// script - ascii command to execute script including arguments
// input  - null terminated ascii input
// output - buffer to store output
// outmax - maximum length of output data to store
// error  - buffer to store stderr output
// errmax - maximum length of error data to store
//
// RETURNS
//
//   0        - on success
//   non-zero - error response from script
//   -1       - on execution error
//   -2       - on timeout
//

int execute( char *script,
             char *input, 
             char *output, 
             int outmax, 
             char *error, 
             int errmax
) ;

#endif
