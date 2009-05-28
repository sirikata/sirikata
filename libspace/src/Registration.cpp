#include <space/Registration.hpp>
#include <Space_Sirikata.pbj.hpp>
#include <util/RoutableMessage.hpp>
namespace Sirikata {

Registration::Registration(const ObjectReference&registrationServiceIdentifier,const SHA256&privateKey):mRegistrationServiceIdentifier(registrationServiceIdentifier),mPrivateKey(privateKey) {
    
}

Registration::~Registration() {

}

bool Registration::forwardMessagesTo(MessageService*ms) {
    mServices.push_back(ms);
    return true;
}

bool Registration::endForwardingMessagesTo(MessageService*ms) {
    std::vector<MessageService*>::iterator where=std::find(mServices.begin(),mServices.end(),ms);
    if (where==mServices.end())
        return false;
    mServices.erase(where);
    return true;
}
void Registration::processMessage(const ObjectReference*ref,MemoryReference message){
    
    RoutableMessageHeader hdr;
    MemoryReference message_body=hdr.ParseFromArray(message.data(),message.size());
    if (!hdr.has_source_object()&&ref) {
        hdr.set_source_object(*ref);
    }
    this->processMessage(hdr,message_body);
}

void Registration::processMessage(const RoutableMessageHeader&header,MemoryReference message_body) {
    RoutableMessageBody body;
    if (body.ParseFromArray(message_body.data(),message_body.size())) {
        asyncRegister(header,body);
    }else{
        SILOG(registration,warning,"Unable to parse message body from message originating from "<<header.source_object());        
    }
}
void Registration::asyncRegister(const RoutableMessageHeader&header,const RoutableMessageBody& body) {
    RoutableMessageBody retval;
    if (body.has_id())
        retval.set_id(body.id());//make sure this is a return value for the particular query in question
    //for now do so synchronously in a very short-sighted manner.
    int num_messages=body.message_arguments_size();
    for (int i=0;i<num_messages;++i) {
        if (body.message_names(i)==body.getDefaultMessageName()||body.message_names(i)=="NewObj") {//Will anything contain the extra 'NewObj' information when the destination UUID
            //clearly indicatest the Registration servcie?
            Protocol::NewObj newObj;
            newObj.ParseFromString(body.message_arguments(i));
            if (newObj.has_requested_object_loc()&&newObj.has_bounding_sphere()) {
                unsigned char evidence[SHA256::static_size+UUID::static_size];
                std::memcpy(evidence,mPrivateKey.rawData().begin(),SHA256::static_size);
                std::memcpy(evidence+SHA256::static_size,newObj.object_uuid_evidence().getArray().begin(),UUID::static_size);
                RoutableMessageHeader destination_header;
                Protocol::RetObj retObj;
                std::string obj_loc_string;
                newObj.requested_object_loc().SerializeToString(&obj_loc_string);
                retObj.mutable_location().ParseFromString(obj_loc_string);
                retObj.set_bounding_sphere(newObj.bounding_sphere());
                retObj.set_object_reference(UUID(SHA256::computeDigest(evidence,sizeof(evidence)).rawData().begin(),UUID::static_size));
                destination_header.set_destination_object(ObjectReference(newObj.object_uuid_evidence()));
                destination_header.set_source_object(mRegistrationServiceIdentifier);
                if (retval.message_arguments_size()==0) {
                    retval.add_message_arguments(std::string());
                }else {
                    retval.message_arguments(0).resize(0);
                }
                retObj.SerializeToString(&retval.message_arguments(0));
                std::string return_message;
                retval.SerializeToString(&return_message);
                for (std::vector<MessageService*>::iterator i=mServices.begin(),ie=mServices.end();i!=ie;++i) {
                    (*i)->processMessage(destination_header,MemoryReference(return_message));
                }
            }else {
                SILOG(registration,warning,"Insufficient information in NewObj request"<<body.message_names(i));
            }
        }else {
            SILOG(registration,warning,"Do not understand message of type "<<body.message_names(i));
        }
    }
}
}
