#include <space/Physics.hpp>

namespace Sirikata { namespace Space {

Physics::Physics(Space*space, Network::IOService*io, const SpaceObjectReference&nodeId, uint32 port):SpaceProxyManager(space,io),mReplyMessageService(nodeId,port) {
    mQueryTracker.forwardMessagesTo(&mReplyMessageService);
    initialize();
}
Physics::~Physics() {
    mQueryTracker.endForwardingMessagesTo(&mReplyMessageService);
    destroy();
}
Physics::ReplyMessageService::ReplyMessageService(const SpaceObjectReference&senderId, uint32 port):mSenderId(senderId),mPort(port){
    mSpace=NULL;
}
bool Physics::ReplyMessageService::forwardMessagesTo(MessageService*msg){
    if (!mSpace) {
        mSpace=msg;return true;
    }
    return false;
}
bool Physics::ReplyMessageService::endForwardingMessagesTo(MessageService*msg){
    if (mSpace==msg) {
        mSpace=NULL;
        return true;
    }
    return false;
}

void Physics::ReplyMessageService::processMessage(const RoutableMessageHeader&, MemoryReference) {
    NOT_IMPLEMENTED();
}

bool Physics::forwardMessagesTo(MessageService*msg) {
    return mReplyMessageService.forwardMessagesTo(msg);
}

bool Physics::endForwardingMessagesTo(MessageService*msg) {
    return mReplyMessageService.endForwardingMessagesTo(msg);
}
void Physics::processMessage(const RoutableMessageHeader&, MemoryReference) {
    //FIXME
    NOT_IMPLEMENTED();
}



} }




