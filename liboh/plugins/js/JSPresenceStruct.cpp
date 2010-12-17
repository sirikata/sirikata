
#include "JSPresenceStruct.hpp"
#include <v8.h>


void registerOnProxRemovedEventHandler(v8::Persistent<v8::Function>& cb)
{
    mOnProxRemovedEventHandler = cb;
}
    
void registerOnProxAddedEventHandler(v8::Persistent<v8::Function>& cb)
{
    mOnProxAddedEventHandler = cb;
}

