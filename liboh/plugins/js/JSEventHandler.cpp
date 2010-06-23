#include "JSEventHandler.hpp"
#include <v8.h>
#include "JSPattern.hpp"

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/odp/Defs.hpp>




namespace Sirikata {
namespace JS {

bool JSEventHandler::matches(v8::Handle<v8::Object> obj, v8::Handle<v8::Object> sender) const
{
    if (suspended)
        return false; //cannot match a suspended handler

    
    //get sender as obj reference
    v8::Local<v8::External> wrap;
    if (sender->InternalFieldCount() > 0)
        wrap = v8::Local<v8::External>::Cast(sender->GetInternalField(0));

    void* ptr = wrap->Value();
    ObjectReference* objRef1 = static_cast<ObjectReference*>(ptr);

    
    if (! this->sender->IsNull())
    {
        //have a sender to match.  see if it does
        if (this->sender->InternalFieldCount() > 0)
            wrap= v8::Local<v8::External>::Cast(this->sender->GetInternalField(0));

        ptr = wrap->Value();
        ObjectReference* objRef2 = static_cast<ObjectReference*> (ptr);


        if (! (objRef1->getAsUUID() == objRef2->getAsUUID()))
            return false;
        else
            std::cout<<"\n\nThe senders match\n\n";
    }
        

    //check if the pattern matches the obj
    for(PatternList::const_iterator pat_it = pattern.begin(); pat_it != pattern.end(); pat_it++)
    {
        if (!pat_it->matches(obj))
            return false;
    }
    return true;
}


//changes state of handler to suspended
void JSEventHandler::suspend()
{
    suspended = true;
}

void JSEventHandler::resume()
{
    suspended = false;
}

bool JSEventHandler::isSuspended()
{
    return suspended;
}


void JSEventHandler::printHandler()
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



