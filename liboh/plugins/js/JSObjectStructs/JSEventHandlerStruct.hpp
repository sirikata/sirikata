#ifndef __SIRIKATA_JS_EVENT_HANDLER_STRUCT_HPP__
#define __SIRIKATA_JS_EVENT_HANDLER_STRUCT_HPP__

#include <v8.h>
#include <vector>
#include "../JSPattern.hpp"
#include "JSSuspendable.hpp"
#include <sirikata/core/util/SpaceObjectReference.hpp>

namespace Sirikata{
namespace JS{

class JSContextStruct;

struct JSEventHandlerStruct : public JSSuspendable
{
    JSEventHandlerStruct(const PatternList& _pattern, v8::Persistent<v8::Function> _cb, v8::Persistent<v8::Object> _sender, JSContextStruct* jscs,bool issusp);

    ~JSEventHandlerStruct();
    bool matches(v8::Handle<v8::Object> obj, v8::Handle<v8::Object> sender, const SpaceObjectReference& receiver );

    static JSEventHandlerStruct* decodeEventHandlerStruct(v8::Handle<v8::Value> toDecode, String& errorMessage);

    void printHandler();

    v8::Handle<v8::Value>getAllData();
    
    virtual v8::Handle<v8::Value> suspend();
    virtual v8::Handle<v8::Value> resume();
    virtual v8::Handle<v8::Value> clear();


    PatternList pattern;
    v8::Persistent<v8::Function> cb;
    v8::Persistent<v8::Object> sender;
    JSContextStruct* jscont;
};


}} //end namepsaces


#endif
