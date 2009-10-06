#ifndef _CBR_OBJECT_MESSAGE_QUEUE_HPP
#define _CBR_OBJECT_MESSAGE_QUEUE_HPP

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "ForwarderUtilityClasses.hpp"
#include "AbstractQueue.hpp"
namespace CBR{
class Forwarder;
/*
class ServerProtocolMessagePair;

struct ObjMessQBeginSend
{
    UUID dest_uuid;
    ServerProtocolMessagePair* data;
};
*/


class ObjectMessageQueue:public AbstractQueue<OutgoingMessage*> {
public:
    ObjectMessageQueue(SpaceContext* ctx, Forwarder*sm)
     : mContext(ctx),
       mForwarder(sm)
    {}

    virtual ~ObjectMessageQueue(){}
    virtual OutgoingMessage *&front()=0;
    virtual OutgoingMessage * pop()=0;
    virtual bool empty()const=0;


    virtual bool beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend& ) = 0;
    virtual void endSend(const ObjMessQBeginSend&, ServerID dest_server_id) = 0;

    virtual bool hasClient(const UUID&uuid) const=0;

    virtual void registerClient(const UUID& oid, float weight=1) = 0;
    virtual void unregisterClient(const UUID& oid) = 0;

    // Note: only public because the templated version of FairObjectMessageQueue requires it
    class ServerMessagePair {
    private:
        std::pair<ServerID,Network::Chunk> mPair;
        UniqueMessageID mID;
    public:
        ServerMessagePair(const ServerID&sid, const Network::Chunk&data,UniqueMessageID id):mPair(sid,data),mID(id){}
        //destructively modifies the data chunk to quickly place it in the queue
        ServerMessagePair(const ServerID&sid, Network::Chunk&data,UniqueMessageID id):mPair(sid,Network::Chunk()),mID(id){
            mPair.second.swap(data);
        }
        explicit ServerMessagePair(size_t size):mPair(0,Network::Chunk(size)),mID(0){

        }
        unsigned int size()const {
            return mPair.second.size();
        }
        bool empty() const {
            return size()==0;
        }
        ServerID dest() const {
            return mPair.first;
        }

        ServerID dest(ServerID newval) {
            mPair.first = newval;
            return mPair.first;
        }

        const Network::Chunk data() const {
            return mPair.second;
        }

        UniqueMessageID id() const {
            return mID;
        }
    };

protected:
    SpaceContext* mContext;
    Forwarder *mForwarder;
};
}
#endif
