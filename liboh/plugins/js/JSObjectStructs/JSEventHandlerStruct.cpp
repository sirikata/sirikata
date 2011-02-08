#include "JSEventHandlerStruct.hpp"
#include <v8.h>
#include "../JSPattern.hpp"

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/odp/Defs.hpp>

#include "../JSObjects/JSFields.hpp"
#include "JSVisibleStruct.hpp"


namespace Sirikata {
namespace JS {

//sender should be of type ADDRESSABLE (see template defined in JSObjectScriptManager
bool JSEventHandlerStruct::matches(v8::Handle<v8::Object> obj, v8::Handle<v8::Object> sender) const
{
    if (suspended)
        return false; //cannot match a suspended handler

    //decode the sender of the message
    String errorMessage = "[JS] Error encountered in matches of JSEventHandler.  Failed to decode sender of message as a visible object.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(sender, errorMessage);
    if (jsvis == NULL)
    {
        SILOG(js,error,errorMessage);
        return false;
    }
    
    SpaceObjectReference* spref1 =  jsvis->whatIsVisible;


    //decode the expected sender
    if (! this->sender->IsNull())
    {
        String errorMessageExpectedSender = "[JS] Error encountered in matches of JSEventHandler.  Failed to decode expected sender of event handler.  ";
        JSVisibleStruct* jsvisExpectedSender = JSVisibleStruct::decodeVisible(this->sender, errorMessageExpectedSender);

        if (jsvisExpectedSender == NULL)
        {
            SILOG(js,error,errorMessageExpectedSender);
            return false;
        }

        SpaceObjectReference* spref2 = jsvisExpectedSender->whatIsVisible;

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

    return true;
}





//changes state of handler to suspended
void JSEventHandlerStruct::suspend()
{
    suspended = true;
}

void JSEventHandlerStruct::resume()
{
    suspended = false;
}

bool JSEventHandlerStruct::isSuspended()
{
    return suspended;
}


void JSEventHandlerStruct::printHandler()
{
    //print patterns
    for (PatternList::const_iterator pat_it = pattern.begin(); pat_it != pattern.end(); pat_it++)
    {
        if (suspended)
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



