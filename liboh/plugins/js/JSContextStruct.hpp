
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
    JSContextStruct(JSObjectScript* parent)
        : jsObjScript(parent)
        {
            v8::HandleScope handle_scope;
            thisObject = v8::Persistent<v8::Object>::New(v8::Object::New());  //this object
                                                             //corresponds to
                                                             //the this
                                                             //parameter that
                                                             //will be available
                                                             //in the executed function.


            v8::Handle<v8::ObjectTemplate> tmpglobal = v8::ObjectTemplate::New();
            tmpglobal->Set(v8::String::New("debugBehramContext"),v8::String::New(" string for behram context"));
            mContext= v8::Context::New(NULL,tmpglobal);
            // bool setCorrect = mContext->Global()->SetInternalField(33, v8::String::New("another debugging string for you"));
            // assert(setCorrect);
            mContext->SetData(v8::String::New("Behram's data"));

        }
    
    ~JSContextStruct()
    {
        mContext.Dispose();
    }

    v8::Handle<v8::Value> executeScript(v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv);
    
    //data
    JSObjectScript* jsObjScript;
    v8::Persistent<v8::Context> mContext;
    v8::Persistent<v8::Object>  thisObject;
    
};


}//end namespace js
}//end namespace sirikata

#endif
