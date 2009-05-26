#include <space/Loc.hpp>
namespace Sirikata {
Loc::Loc() {
    
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
void Loc::processMessage(const ObjectReference*ref,MemoryReference message) {
    for (std::vector<MessageService*>::iterator i=mServices.begin(),ie=mServices.end();i!=ie;++i) {
        (*i)->processMessage(ref,message);
    }
}
void Loc::processMessage(const RoutableMessageHeader&header,MemoryReference message_body) {
    for (std::vector<MessageService*>::iterator i=mServices.begin(),ie=mServices.end();i!=ie;++i) {
        (*i)->processMessage(header,message_body);
    }
}
}
