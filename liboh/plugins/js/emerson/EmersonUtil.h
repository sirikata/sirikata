#ifndef __EMERSON_UTIL__ 
#define __EMERSON_UTIL__


/*
#ifdef __cplusplus
extern "C" {
#endif
*/
#include<antlr3.h>
#include <map>
#include <stdio.h>
#include <string>



pANTLR3_STRING emerson_printAST(pANTLR3_BASE_TREE tree);
char* emerson_compile(const char*);
char* emerson_compile_diag(const char*, FILE*,int& errorNum);




/*
#ifdef __cplusplus
}
#endif
*/


#endif
