
#include <stdio.h>
#include <string.h>
#include "../mem.h"
#include "../str.h"


int main()
{
 int pass=1 ;
 int passcount=0 ;
 int totaltests=0 ;

 mem *m = mem_malloc(14) ;

 printf("\n-------------------------------------\n") ;
 printf("\nSTRCPY / REPLACE / REPLACEALL\n\n") ;

 str_strcpy(m, "abcdeabcdefgh") ;

 printf("BEFORE replace: %s\n", m) ;

 str_replaceall(m, "de", "Y") ; 
 str_replace(m, "bc", "XXXX") ;
 str_replace(m, "fgh", "FGH") ;
 str_replace(m, "fg", "ZZZ") ; // Would overflow

 printf("AFTER replace: %s\n", m) ;

 if (strcmp(m, "aXXXXYabcYFGH")==0) {
   printf("Test Passes\n") ;
   passcount++ ;
 } else {
   pass=0 ;
 }

 totaltests++ ;

 printf("\n-------------------------------------\n") ;

 mem_free(m) ;

 if (pass) {
    printf("All %d tests passed\n", passcount) ;
 } else {
    printf("%d/%d tests FAILED\n", totaltests-passcount, totaltests) ;
 }

 return pass ;
}
