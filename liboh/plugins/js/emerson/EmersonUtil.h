#ifndef __EMERSON_UTIL__ 
#define __EMERSON_UTIL__

#include<antlr3.h>


pANTLR3_STRING emerson_printAST(pANTLR3_BASE_TREE tree);
void emerson_createTreeMirrorImage(pANTLR3_BASE_TREE tree);
void emerson_createTreeMirrorImage2(pANTLR3_BASE_TREE tree);
void emerson_printRewriteStream(pANTLR3_REWRITE_RULE_TOKEN_STREAM);

#endif
