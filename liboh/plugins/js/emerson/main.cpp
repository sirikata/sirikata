// Example of a grammar for a trivial tree parser.
// Adapted from Java equivalent example, by Terence Parr
// Author: Jim Idle - April 2007
// Permission is granted to use this example code in any way you want, so long as
// all the original authors are cited.
//

// set ts=4,sw=4
// Tab size is 4 chars, indent is 4 chars

// Notes: Although all the examples provided are configured to be built
//        by Visual Studio 2005, based on the custom build rules
//        provided in $(ANTLRSRC)/code/antlr/main/runtime/C/vs2005/rulefiles/antlr3.rules
//        there is no reason that this MUST be the case. Provided that you know how
//        to run the antlr tool, then just compile the resulting .c files and this
//	  file together, using say gcc or whatever: gcc *.c -I. -o XXX
//	  The C code is generic and will compile and run on all platforms (please
//        report any warnings or errors to the antlr-interest newsgroup (see www.antlr.org)
//        so that they may be corrected for any platform that I have not specifically tested.
//
//	  The project settings such as additional library paths and include paths have been set
//        relative to the place where this source code sits on the ANTLR perforce system. You
//        may well need to change the settings to locate the includes and the lib files. UNIX
//        people need -L path/to/antlr/libs -lantlr3c (release mode) or -lantlr3cd (debug)
//
//        Jim Idle (jimi cut-this at idle ws)
//

// You may adopt your own practices by all means, but in general it is best
// to create a single include for your project, that will include the ANTLR3 C
// runtime header files, the generated header files (all of which are safe to include
// multiple times) and your own project related header files. Use <> to include and
// -I on the compile line (which vs2005 now handles, where vs2003 did not).
//

#include "EmersonUtil.h"
#include "Util.h"
#include "EmersonLexer.h"
#include "EmersonParser.h"
#include "EmersonTree.h"
#include "EmersonInfo.h"
#include "EmersonException.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace Emerson;
extern pANTLR3_UINT8* EmersonParserTokenNames;
extern EmersonInfo* _emersonInfo;

void myRecoverFromMismatchedSet(struct ANTLR3_BASE_RECOGNIZER_struct* _recognizer, pANTLR3_BITSET_LIST _follow)
{
}


void* myRecoverFromMismatchedToken(struct ANTLR3_BASE_RECOGNIZER_struct* _recognizer, ANTLR3_UINT32 _ttype, pANTLR3_BITSET_LIST _follow)
{
    return NULL;
}


void myDisplayRecognitionError(struct ANTLR3_BASE_RECOGNIZER_struct* recognizer, pANTLR3_UINT8* tokenNames)
{
  pANTLR3_EXCEPTION exception = recognizer->state->exception;

  //std::cout << "Exception type = " << exception->type << "\n";
  if(exception->nextException)
  {
    //std::cout << "There is another exception too.." << "\n\n";
  }
  /*
  string filename =
  linenumber
  character postion
  type of error
*/
  std::string filename = _emersonInfo->fileInfo().fileName();
  //uint32 line = _emersonInfo->fileInfo().line();
  uint32 line = exception->line;
  uint32 charPos = exception->charPositionInLine;

  std::stringstream err_msg;
  //char* tokens = (char*)(tokenNames);
  //std::string s(exception->token);
  //err_msg << "Error: " << filename << " at line " << line << " at position " << charPos << "near""\n";
  std::string expected(" ");
  pANTLR3_COMMON_TOKEN currToken = (pANTLR3_COMMON_TOKEN)exception->token;


  //std::string tokenText((char*)(currToken->tokText.chars));
  //cout << "token text = " << tokenText << "\n";
  err_msg << "Error: " << filename << " at line " << line << ":" << charPos << "\n";
  err_msg << "Additional Info: " << (char*)(exception->message) << "\n";
  throw EmersonException(err_msg.str());
}


// Main entry point for this example
//
int main	(int argc, char *argv[])
{
    // Now we declare the ANTLR related local variables we need.
    // Note that unless you are convinced you will never need thread safe
    // versions for your project, then you should always create such things
    // as instance variables for each invocation.
    // -------------------

    // Name of the input file. Note that we always use the abstract type pANTLR3_UINT8
    // for ASCII/8 bit strings - the runtime library guarantees that this will be
    // good on all platforms. This is a general rule - always use the ANTLR3 supplied
    // typedefs for pointers/types/etc.
    //
    const char*	    fName = "./input";

    // The ANTLR3 character input stream, which abstracts the input source such that
    // it is easy to provide input from different sources such as files, or
    // memory strings.
    //
    // For an ASCII/latin-1 memory string use:
    //	    input = antlr3NewAsciiStringInPlaceStream (stringtouse, (ANTLR3_UINT64) length, NULL);
    //
    // For a UCS2 (16 bit) memory string use:
    //	    input = antlr3NewUCS2StringInPlaceStream (stringtouse, (ANTLR3_UINT64) length, NULL);
    //
    // For input from a file, see code below
    //
    // Note that this is essentially a pointer to a structure containing pointers to functions.
    // You can create your own input stream type (copy one of the existing ones) and override any
    // individual function by installing your own pointer after you have created the standard
    // version.
    //
    pANTLR3_INPUT_STREAM	    input;

    // The lexer is of course generated by ANTLR, and so the lexer type is not upper case.
    // The lexer is supplied with a pANTLR3_INPUT_STREAM from whence it consumes its
    // input and generates a token stream as output.
    //
    pEmersonLexer			    lxr;

    // The token stream is produced by the ANTLR3 generated lexer. Again it is a structure based
    // API/Object, which you can customise and override methods of as you wish. a Token stream is
    // supplied to the generated parser, and you can write your own token stream and pass this in
    // if you wish.
    //
    pANTLR3_COMMON_TOKEN_STREAM	    tstream;

    // The Lang parser is also generated by ANTLR and accepts a token stream as explained
    // above. The token stream can be any source in fact, so long as it implements the
    // ANTLR3_TOKEN_SOURCE interface. In this case the parser does not return anything
    // but it can of course specify any kind of return type from the rule you invoke
    // when calling it.
    //
    pEmersonParser			    psr;

    // The parser produces an AST, which is returned as a member of the return type of
    // the starting rule (any rule can start first of course). This is a generated type
    // based upon the rule we start with.
    //
    EmersonParser_program_return	    emersonAST;


    // The tree nodes are managed by a tree adaptor, which doles
    // out the nodes upon request. You can make your own tree types and adaptors
    // and override the built in versions. See runtime source for details and
    // eventually the wiki entry for the C target.
    //
    pANTLR3_COMMON_TREE_NODE_STREAM	nodes;

    // Finally, when the parser runs, it will produce an AST that can be traversed by the
    // the tree parser: c.f. LangDumpDecl.g3t
    //
    pEmersonTree		    treePsr;

    // Create the input stream based upon the argument supplied to us on the command line
    // for this example, the input will always default to ./input if there is no explicit
    // argument.
    //
    int c;
    bool verbose= false;
    for(int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-f")
            fName = argv[++i];
        else if (std::string(argv[i]) == "-v")
            verbose = true;
    }

    // if (argc < 2 || argv[1] == NULL)
    // {
    //     fName	="./input"; // Note in VS2005 debug, working directory must be configured
    // }
    // else
    // {
    //     fName	= argv[1];
    // }


    // Create the input stream using the supplied file name
    // (Use antlr3AsciiFileStreamNew for UCS2/16bit input).
    //

    emerson_init();

    const char* em_script = read_file(fName);
    string em_script_str = string(em_script);
    delete[] em_script;
    string em_script_str_new = em_script_str;
    if(em_script_str.at(em_script_str.size() -1) != '\n')
    {
        em_script_str_new.push_back('\n');
    }


    int errorNum = 0;
    FILE* dbgFile = NULL;
    if (verbose)
        dbgFile = stderr;

    // wrap with with


    try
    {
        char* js_str = emerson_compile(std::string(fName), (const char*)em_script_str_new.c_str(), errorNum, &myDisplayRecognitionError, dbgFile);
        if (js_str){
            std::cout<<js_str;
        }
    }
    catch(EmersonException e)
    {
        std::cerr << e.msg() << "\n";
        return errorNum;
    }

    return errorNum;
}
