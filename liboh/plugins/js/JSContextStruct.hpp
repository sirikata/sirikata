
//#include "JSObjectScript.hpp"

#ifndef _SIRIKATA_JS_CONTEXT_STRUCT_HPP_
#define _SIRIKATA_JS_CONTEXT_STRUCT_HPP_

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <vector>

namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;

struct JSContextStruct
{
    JSContextStruct(JSObjectScript* parent, bool sendEveryone, bool recvEveryone, bool proxQueries)
     : jsObjScript(parent),
       mContext(v8::Context::New())
        {
            lkjs;
            add to global an object that can do all these things;
            lkjs;
        }
    
    ~JSContextStruct()
    {
        mContext.Dispose();
    }

    v8::Handle<v8::Value> executeScript(v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv);
    
    //data
    JSObjectScript* jsObjScript;
    v8::Persistent<v8::Context> mContext;
    
};


}//end namespace js
}//end namespace sirikata

#endif
