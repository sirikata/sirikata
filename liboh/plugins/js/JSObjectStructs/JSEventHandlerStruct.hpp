#ifndef __SIRIKATA_JS_EVENT_HANDLER_STRUCT_HPP__
#define __SIRIKATA_JS_EVENT_HANDLER_STRUCT_HPP__

#include <v8.h>
#include <vector>
#include "../JSPattern.hpp"
#include "JSSuspendable.hpp"

namespace Sirikata{
namespace JS{


struct JSEventHandlerStruct 
{
    JSEventHandlerStruct(const PatternList& _pattern, v8::Persistent<v8::Object> _target, v8::Persistent<v8::Function> _cb, v8::Persistent<v8::Object> _sender)
     : pattern(_pattern), target(_target), cb(_cb), sender(_sender), suspended(false)
    {}


    bool matches(v8::Handle<v8::Object> obj, v8::Handle<v8::Object> sender) const;
    
    void printHandler();
    void suspend();
    void resume();
    bool isSuspended();



    
    PatternList pattern;
    v8::Persistent<v8::Object> target;
    v8::Persistent<v8::Function> cb;
    v8::Persistent<v8::Object> sender;
    bool suspended;
};


}} //end namepsaces


#endif
