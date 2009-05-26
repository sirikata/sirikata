#include <space/Registration.hpp>
#include <Space_Sirikata.pbj.hpp>
#include <util/RoutableMessage.hpp>
namespace Sirikata {

Registration::Registration() {
    
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
void Registration::processMessage(const ObjectReference*ref,MemoryReference message) {
    RoutableMessageHeader hdr;
    MemoryReference message_body=hdr.ParseFromArray(message.data(),message.size());
    if (!hdr.has_source_object()&&ref) {
        hdr.set_source_object(*ref);
    }
    this->processMessage(hdr,message_body);
}
void Registration::processMessage(const RoutableMessageHeader&header,MemoryReference message_body) {
    //FIXME


    for (std::vector<MessageService*>::iterator i=mServices.begin(),ie=mServices.end();i!=ie;++i) {
        (*i)->processMessage(header,message_body);
    }
}
}
