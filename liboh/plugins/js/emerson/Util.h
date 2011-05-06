#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include<antlr3.h>
#include <map>
#include <stdio.h>
#include <string>

using namespace std;

typedef map<const char*, const char*> keymap;
typedef pair<const char*, const char*> keypair;


int emerson_isAKeyword(const char*);
char* read_file(const char*);
std::string emerson_escapeSingleQuotes(const char* stringSequence);
std::string emerson_escapeMultiline(const char* stringSequence);
std::string replaceAllInstances(std::string initialString, std::string toReplace, std::string toReplaceWith);
int emerson_init();
void insertKeywords();
void emerson_printRewriteStream(pANTLR3_REWRITE_RULE_TOKEN_STREAM);
void emerson_createTreeMirrorImage(pANTLR3_BASE_TREE tree);
void emerson_createTreeMirrorImage2(pANTLR3_BASE_TREE tree);
pANTLR3_STRING emerson_printAST(pANTLR3_BASE_TREE tree, pANTLR3_UINT8*parserTokenNames);

#endif
