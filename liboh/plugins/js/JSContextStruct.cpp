
#include "JSContextStruct.hpp"
#include "JSObjectScript.hpp"
#include <v8.h>


namespace Sirikata {
namespace JS {


v8::Handle<v8::Value> JSContextStruct::executeScript(v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv)
{
    return jsObjScript->executeInContext(mContext,thisObject,funcToCall, argc,argv);
}


}//js namespace
}//sirikata namespace
