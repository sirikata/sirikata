#ifndef _CBR_SENDQUEUE_HPP
#define _CBR_SENDQUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"

namespace CBR{
class SendQueue {
public:
    virtual ~SendQueue(){}
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg)=0;
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj)=0;
    virtual void service(const Time& t)=0;
    virtual bool hasServerRegistered(ServerID sid)const{
        return false;
    }
    virtual void registerServer(ServerID sid, float weight=1) = 0;
    virtual void registerClient(UUID oid,float weight=1) = 0;

};
}
#endif
