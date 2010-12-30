#ifndef _SIRIKATA_JS_CONTEXT_STRUCT_HPP_
#define _SIRIKATA_JS_CONTEXT_STRUCT_HPP_

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>


namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;

struct JSContextStruct
{
    JSContextStruct(JSObjectScript* parent, const SpaceObjectReference& _sporef)
        : jsObjScript(parent),
          sporef(new SpaceObjectReference(_sporef)),
          mContext(v8::Context::New())
        {
        }
    
    ~JSContextStruct()
    {
        mContext.Dispose();
        delete sporef;
    }

    void executeScript(v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv);
    
    //data
    JSObjectScript* jsObjScript;
    SpaceObjectReference* sporef; //sporef associated with this presence.
    v8::Persistent<v8::Context> mContext;
    
};


}//end namespace js
}//end namespace sirikata

#endif
