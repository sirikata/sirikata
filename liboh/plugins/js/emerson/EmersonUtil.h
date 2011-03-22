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
char* emerson_compile(const char* em_script_str);
char* emerson_compile(const char* em_script_str, FILE* dbg);
char* emerson_compile(const char*, int& errorNum);
char* emerson_compile(const char*, int& errorNum, FILE* dbg);

typedef void (*EmersonErrorFuncType)(struct ANTLR3_BASE_RECOGNIZER_struct*, pANTLR3_UINT8*);
char* emerson_compile(std::string _originalFile, const char* em_script_str, int& errorNum, EmersonErrorFuncType error_cb);
char* emerson_compile(std::string _originalFile, const char* em_script_str, int& errorNum, EmersonErrorFuncType error_cb, FILE* dbg);


/*
#ifdef __cplusplus
}
#endif
*/


#endif
