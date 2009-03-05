#ifndef _CBR_SENDQUEUE_HPP
#define _CBR_SENDQUEUE_HPP
namespace CBR{
class SendQueue {
public:
    virtual ~SendQueue(){}
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg)=0;
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj)=0;
    virtual void service()=0;
};
}
#endif
