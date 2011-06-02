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

bool emerson_compile(const char* em_script_str,std::string& toCompileTo);
bool emerson_compile(const char* em_script_str,std::string& toCompileTo, FILE* dbg);
bool emerson_compile(const char*,std::string& toCompileTo, int& errorNum);
bool emerson_compile(const char*,std::string& toCompileTo, int& errorNum, FILE* dbg);

typedef void (*EmersonErrorFuncType)(struct ANTLR3_BASE_RECOGNIZER_struct*, pANTLR3_UINT8*);
bool emerson_compile(std::string _originalFile,const char* em_script_str,std::string& toCompileTo, int& errorNum, EmersonErrorFuncType error_cb);
bool emerson_compile(std::string _originalFile, const char* em_script_str,std::string& toCompileTo, int& errorNum, EmersonErrorFuncType error_cb, FILE* dbg);


/*
#ifdef __cplusplus
}
#endif
*/


#endif
