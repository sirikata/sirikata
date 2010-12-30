
#include "JSContextStruct.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {


void JSContextStruct::executeScript(v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv)
{
    std::cout<<"\n\nFunction unfinished in executeScript of JSContextStruct.cpp\n\n";
    assert(false);
//    jsObjScript->executeInContext(mContext,funcToCall, args);
}


}//js namespace
}//sirikata namespace
