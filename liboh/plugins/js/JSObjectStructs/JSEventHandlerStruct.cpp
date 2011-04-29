#include "JSEventHandlerStruct.hpp"
#include <v8.h>
#include "../JSPattern.hpp"

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/odp/Defs.hpp>

#include "../JSObjects/JSFields.hpp"
#include "JSVisibleStruct.hpp"
#include "../JSLogging.hpp"
#include "JSContextStruct.hpp"
#include "JSSuspendable.hpp"
#include "../JSObjects/JSObjectsUtils.hpp"

namespace Sirikata {
namespace JS {

JSEventHandlerStruct::JSEventHandlerStruct(const PatternList& _pattern, v8::Persistent<v8::Object> _target, v8::Persistent<v8::Function> _cb, v8::Persistent<v8::Object> _sender, JSContextStruct* jscs)
 : JSSuspendable(),
   pattern(_pattern),
   target(_target),
   cb(_cb),
   sender(_sender),
   jscont(jscs)
{
    v8::HandleScope handle_scope;

    if (jscont != NULL)
        jscont->struct_registerSuspendable(this);

}


JSEventHandlerStruct::~JSEventHandlerStruct()
{
    if (! getIsCleared())
    {
        if (jscont != NULL)
            jscont->struct_deregisterSuspendable(this);

        target.Dispose();
        cb.Dispose();
        sender.Dispose();
    }
}

//sender should be of type VISIBLE (see template defined in JSObjectScriptManager
bool JSEventHandlerStruct::matches(v8::Handle<v8::Object> obj, v8::Handle<v8::Object> incoming_sender, const SpaceObjectReference& receiver)
{
    if (getIsSuspended() || getIsCleared())
        return false; //cannot match a suspended handler

    //decode the sender of the message
    String errorMessage = "[JS] Error encountered in matches of JSEventHandler.  Failed to decode sender of message as a visible object.  ";
    JSPositionListener* jsposlist = decodeJSPosListener(incoming_sender,errorMessage);

    if (jsposlist == NULL)
    {
        JSLOG(error,errorMessage);
        return false;
    }
    
    SpaceObjectReference* spref1 =  jsposlist->getToListenTo();


    //decode the expected sender
    if (! sender->IsNull())
    {
        String errorMessageExpectedSender = "[JS] Error encountered in matches of JSEventHandler.  Failed to decode expected sender of event handler.  ";
        JSPositionListener* jsplExpectedSender = decodeJSPosListener(sender, errorMessageExpectedSender);

        if (jsplExpectedSender == NULL)
        {
            JSLOG(error,errorMessageExpectedSender);
            return false;
        }

        SpaceObjectReference* spref2 = jsplExpectedSender->getToListenTo();

        //check if the senders match
        if ( (*spref1)  != (*spref2))  //the senders do not match.  do not fire
            return false;
    }

    
    //check if the pattern matches the obj
    for(PatternList::const_iterator pat_it = pattern.begin(); pat_it != pattern.end(); pat_it++)
    {
        if (! pat_it->matches(obj))
            return false;
    }

    //check if the message is *to* one of my presences
    if (jscont->canReceiveMessagesFor(receiver))
        return true;

    return false;
}



v8::Handle<v8::Value> JSEventHandlerStruct::suspend()
{
    if (getIsCleared())
    {
        JSLOG(info, "Error in suspend of JSEventHandlerStruct.cpp.  Called suspend even though the handler had previously been cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Called suspend on a handler that had already been cleared.")));
    }

    return JSSuspendable::suspend();
}

v8::Handle<v8::Value> JSEventHandlerStruct::resume()
{
    if (getIsCleared())
    {
        JSLOG(info, "Error in resume of JSEventHandlerStruct.cpp.  Called resume even though the handler had previously been cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Called resume on a handler that had already been cleared.")));
    }
    
    return JSSuspendable::resume();
}
v8::Handle<v8::Value> JSEventHandlerStruct::clear()
{
    target.Dispose();
    cb.Dispose();
    sender.Dispose();
    return JSSuspendable::clear();
}




void JSEventHandlerStruct::printHandler()
{
    if (getIsCleared())
    {
        std::cout<<"**Cleared.  No information.";
        return;
    }

    
    //print patterns
    for (PatternList::const_iterator pat_it = pattern.begin(); pat_it != pattern.end(); pat_it++)
    {
        if (getIsSuspended())
            std::cout<<"**Suspended  ";
        else
            std::cout<<"**Active     ";
   
        pat_it->printPattern();
    }


    /*
      FIXME: sender should really be a part of the pattern.
     */
    
    //print sender
    if (this->sender->IsNull())
        std::cout<<"Sender: any";
    else
    {
        //unwrap the sender to match
        v8::Local<v8::External>wrap;
        void* ptr;
        wrap= v8::Local<v8::External>::Cast(this->sender->GetInternalField(0));
        ptr = wrap->Value();
        ObjectReference* objRef = static_cast<ObjectReference*>(ptr);
        
        std::cout<<"Sender: "<<(objRef->getAsUUID()).toString();
    }
    std::cout<<"\n\n";
    
}


}}//end namespaces



