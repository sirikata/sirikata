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

using namespace std;

typedef map<const char*, char*> keymap;

typedef pair<const char*, char*> keypair;


pANTLR3_STRING emerson_printAST(pANTLR3_BASE_TREE tree);
void emerson_createTreeMirrorImage(pANTLR3_BASE_TREE tree);
void emerson_createTreeMirrorImage2(pANTLR3_BASE_TREE tree);
void emerson_printRewriteStream(pANTLR3_REWRITE_RULE_TOKEN_STREAM);
char* emerson_compile(const char*);
char* emerson_compile_diag(const char*, FILE*);
int emerson_isAKeyword(const char*);
char* read_file(char*);


int emerson_init();
void insertKeywords();


/*
#ifdef __cplusplus
}
#endif
*/


#endif
