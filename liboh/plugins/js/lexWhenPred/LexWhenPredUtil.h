#ifndef __LEX_WHEN_PRED_UTIL__ 
#define __LEX_WHEN_PRED_UTIL__


/*
#ifdef __cplusplus
extern "C" {
#endif
*/
#include<antlr3.h>
#include <map>
#include <stdio.h>
#include <string>


pANTLR3_STRING lexWhenPred_printAST(pANTLR3_BASE_TREE tree);
char* lexWhenPred_compile(const char*);
char* lexWhenPred_compile_diag(const char*, FILE*);


/*
#ifdef __cplusplus
}
#endif
*/


#endif
