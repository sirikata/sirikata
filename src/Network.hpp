#include "sirikata/util/Platform.hpp"
#include "sirikata/network/Stream.hpp"
namespace CBR {
class Network {

public:

    typedef Sirikata::Network::Address Address;
    typedef Sirikata::Network::Chunk Chunk;

    virtual void listen (const Sirikata::Network::Address&)=0;
    virtual bool sendTo(const Sirikata::Network::Address&,const Sirikata::Network::Chunk&, bool reliable, bool ordered, int priority)=0;
    virtual Sirikata::Network::Chunk*receiveOne()=0;
};
}
