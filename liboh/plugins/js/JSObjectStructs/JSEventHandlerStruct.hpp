#ifndef __SIRIKATA_JS_EVENT_HANDLER_STRUCT_HPP__
#define __SIRIKATA_JS_EVENT_HANDLER_STRUCT_HPP__

#include <v8.h>
#include <vector>
#include "../JSPattern.hpp"
#include "JSSuspendable.hpp"
#include "JSContextStruct.hpp"

namespace Sirikata{
namespace JS{


struct JSEventHandlerStruct : public JSSuspendable
{
    JSEventHandlerStruct(const PatternList& _pattern, v8::Persistent<v8::Object> _target, v8::Persistent<v8::Function> _cb, v8::Persistent<v8::Object> _sender, JSContextStruct* jscs);
    
    ~JSEventHandlerStruct();
    bool matches(v8::Handle<v8::Object> obj, v8::Handle<v8::Object> sender, const SpaceObjectReference& receiver );
    
    void printHandler();

    virtual v8::Handle<v8::Value> suspend();
    virtual v8::Handle<v8::Value> resume();
    virtual v8::Handle<v8::Value> clear();


    PatternList pattern;
    v8::Persistent<v8::Object> target;
    v8::Persistent<v8::Function> cb;
    v8::Persistent<v8::Object> sender;
    JSContextStruct* jscont;
};


}} //end namepsaces


#endif
