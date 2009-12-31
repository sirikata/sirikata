#include <space/Physics.hpp>
#include <space/Space.hpp>
#include <util/KnownServices.hpp>
#include "Space_Sirikata.pbj.hpp"
#include <util/RoutableMessage.hpp>
namespace Sirikata { namespace Space {

Physics::Physics(Space*space, Network::IOService*io, const SpaceObjectReference&nodeId, uint32 port):SpaceProxyManager(space,io),mReplyMessageService(nodeId,port), mBounds(BoundingBox3d3f::null()),mQueryId(0) {
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

void Physics::setBounds(const BoundingBox3d3f &bounds) {

    if (mBounds!=bounds) {
        RoutableMessageBody body;
        RoutableMessageHeader proxHeader;
        proxHeader.set_destination_port(Services::GEOM);
        proxHeader.set_destination_object(ObjectReference::spaceServiceID());
        proxHeader.set_destination_space(mSpace->id());
        if (mBounds!=BoundingBox3d3f::null()) {
            //destroy query
            Protocol::DelProxQuery proxQuery;
            proxQuery.set_query_id(mQueryId);
            String proxQueryStr;
            proxQuery.SerializeToString(&proxQueryStr);
            body.add_message("DelProxQuery", proxQueryStr);            
            clearQuery(mQueryId);
            mQueryId++;            
        }
        if (bounds!=BoundingBox3d3f::null()) {
            //create query
            BoundingSphere3d newBounds=bounds.toBoundingSphere();
            Protocol::NewProxQuery proxQuery;
            proxQuery.set_query_id(mQueryId);
            proxQuery.set_absolute_center(newBounds.center());
            proxQuery.set_max_radius(newBounds.radius());
            String proxQueryStr;
            proxQuery.SerializeToString(&proxQueryStr);
            body.add_message("NewProxQuery", proxQueryStr);
        }
        String bodyStr;
        body.SerializeToString(&bodyStr);
        mReplyMessageService.processMessage(proxHeader, MemoryReference(bodyStr));
        mBounds=bounds;
    }
}

void Physics::ReplyMessageService::processMessage(const RoutableMessageHeader&rmh, MemoryReference body) {
    RoutableMessageHeader header(rmh);
    header.set_source_port(Services::PHYSICS);
    header.set_source_object(mSenderId.object());
    header.set_source_space(mSenderId.space());//FIXME is setting space truly necessary here--it's from this space--is that not implied
    mSpace->processMessage(header,body);
}

bool Physics::forwardMessagesTo(MessageService*msg) {
    return mReplyMessageService.forwardMessagesTo(msg);
}

bool Physics::endForwardingMessagesTo(MessageService*msg) {
    return mReplyMessageService.endForwardingMessagesTo(msg);
}
void Physics::processMessage(const RoutableMessageHeader&underspecifiedHeader, MemoryReference bodyData) {
    RoutableMessageHeader header(underspecifiedHeader);
    header.set_source_space(mSpace->id());
    header.set_destination_space(mSpace->id());
    SILOG(cppoh,debug,"** Message from: " << header.source_object() << " port " << header.source_port() << " to "<<header.destination_object()<<" port " << header.destination_port());
    /// Handle Return values to queries we sent to someone:
    if (header.has_reply_id()) {
        mTracker.processMessage(header, bodyData);
        return; // Not a message for us to process.
    }

    /// Parse message_names and message_arguments.
    
    RoutableMessageBody msg;
    msg.ParseFromArray(bodyData.data(), bodyData.length());
    int numNames = msg.message_size();

    for (int i = 0; i < numNames; ++i) {
        std::string name = msg.message_names(i);
        MemoryReference body(msg.message_arguments(i));
        processRPC(header, name, body, NULL);
    }
    
    if (header.has_id()) {
        NOT_IMPLEMENTED();//we don't reply to anything for now        
    }
}

void Physics::addQueryInterest(uint32 query_id, const SpaceObjectReference &id){
    if (query_id>=mQueryId) {//since we always increment our query numbers, we can ignore adds for defunct query numbers
        this->SpaceProxyManager::addQueryInterest(query_id,id);
    }
}

} }




