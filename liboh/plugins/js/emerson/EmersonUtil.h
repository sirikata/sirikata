#ifndef __EMERSON_UTIL__
#define __EMERSON_UTIL__



#include<antlr3.h>
#include <map>
#include <stdio.h>
#include <string>
#include <boost/thread/mutex.hpp>


pANTLR3_STRING emerson_printAST(pANTLR3_BASE_TREE tree);

typedef std::map<int, int> EmersonLineMap;




class EmersonInfo;

class EmersonUtil
{

public:    
    
    typedef void (*EmersonErrorFuncType)(struct ANTLR3_BASE_RECOGNIZER_struct*, pANTLR3_UINT8*);


    static bool emerson_compile(
        std::string _originalFile, const char* em_script_str,
        std::string& toCompileTo, int& errorNum,
        EmersonErrorFuncType error_cb, EmersonLineMap* lineMap);


    static bool emerson_compile(
        std::string _originalFile, const char* em_script_str,
        std::string& toCompileTo, int& errorNum,
        EmersonErrorFuncType error_cb, FILE* dbg,
        EmersonLineMap* lineMap);
    
private:
    
    static bool emerson_compile(
        const char*,std::string& toCompileTo, int& errorNum, FILE* dbg,
        EmersonLineMap* lineMap,EmersonInfo* );

    
};


#endif
