#include <space/Loc.hpp>
#include "Space_Sirikata.pbj.hpp"
#include "util/RoutableMessage.hpp"
namespace Sirikata {
Loc::Loc(const ObjectReference&locServiceIdentifier,const ObjectReference& registrationServiceIdentifier):mLocServiceIdentifier(locServiceIdentifier),mRegistrationServiceIdentifier(registrationServiceIdentifier) {
    
}

Loc::~Loc() {

}

bool Loc::forwardMessagesTo(MessageService*ms) {
    mServices.push_back(ms);
    return true;
}

bool Loc::endForwardingMessagesTo(MessageService*ms) {
    std::vector<MessageService*>::iterator where=std::find(mServices.begin(),mServices.end(),ms);
    if (where==mServices.end())
        return false;
    mServices.erase(where);
    return true;
}


void Loc::processMessage(const ObjectReference*ref,MemoryReference message){
    
    RoutableMessageHeader hdr;
    MemoryReference message_body=hdr.ParseFromArray(message.data(),message.size());
    if (!hdr.has_source_object()&&ref) {
        hdr.set_source_object(*ref);
    }
    this->processMessage(hdr,message_body);
}
void Loc::processMessage(const ObjectReference&object_reference,const Protocol::ObjLoc&loc){
    RoutableMessageBody body;
    body.add_message_arguments(std::string());
    loc.SerializeToString(&body.message_arguments(0));
    std::string message_body;
    body.SerializeToString(&message_body);
    RoutableMessageHeader destination_header;
    destination_header.set_source_object(mLocServiceIdentifier);
    destination_header.set_destination_object(object_reference);
    
    for (std::vector<MessageService*>::iterator i=mServices.begin(),ie=mServices.end();i!=ie;++i) {
        (*i)->processMessage(destination_header,MemoryReference(message_body));
    }
   
}
void Loc::processMessage(const RoutableMessageHeader&header,MemoryReference message_body) {
    RoutableMessageBody body;
    if (body.ParseFromArray(message_body.data(),message_body.size())) {
        int num_args=body.message_arguments_size();
        if (header.source_object()==mRegistrationServiceIdentifier) {
            for (int i=0;i<num_args;++i) {
                Protocol::RetObj retObj;
                if (retObj.ParseFromString(body.message_arguments(i))&&retObj.has_location()) {
                    processMessage(ObjectReference(retObj.object_reference()),retObj.location());
                }else {
                    SILOG(loc,warning,"Loc:Unable to parse RetObj message body originating from "<<header.source_object());
                }
            }           
        }else {
            for (int i=0;i<num_args;++i) {
                Protocol::ObjLoc objLoc;
                if (objLoc.ParseFromString(body.message_arguments(i))) {
                    processMessage(ObjectReference(header.source_object()),objLoc);
                }else {
                    SILOG(loc,warning,"Loc:Unable to parse ObjLoc message body originating from "<<header.source_object());
                }
            }           
            
        }
    }else {
        SILOG(loc,warning,"Loc:Unable to parse message body originating from "<<header.source_object());
    }
}
}
