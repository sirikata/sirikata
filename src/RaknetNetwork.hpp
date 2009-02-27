#include "Network.hpp"
#include "raknet/MessageIdentifiers.h"
#include "raknet/RakNetworkFactory.h"
#include "raknet/RakPeerInterface.h"
#include "raknet/RakNetStatistics.h"
#include "raknet/RakNetTypes.h"
#include "raknet/BitStream.h"

bool operator==(const Sirikata::Network::Address&a, 
                const Sirikata::Network::Address&b) ;
namespace CBR {
class RaknetNetwork {
    class AddressHasher {
    public:
        size_t operator()(const Sirikata::Network::Address& address)const{
            return std::tr1::hash<Sirikata::String>()(address.getHostName())^
                std::tr1::hash<Sirikata::String>()(address.getService());
        }
    };
    class AddressEquals {
    public:
        size_t operator()(const Sirikata::Network::Address& a,const Sirikata::Network::Address& b)const{
            return a.getHostName()==b.getHostName()&&a.getService()==b.getService();
        }
    };
    class Connection{
    public:
        RakPeerInterface*mPeer;
        std::vector<std::pair<Sirikata::Network::Chunk,std::pair<PacketPriority,PacketReliability> > >mToBeSent;
        Connection() {
            mPeer=NULL;
        }
    };
    Sirikata::Network::Chunk *makeChunk(RakPeerInterface*,Packet*);
    std::tr1::shared_ptr<RakPeerInterface> mListener;
    typedef std::list<std::pair<Sirikata::Network::Address,std::tr1::weak_ptr<Connection> > > SocketList;
    SocketList mConnectingSockets;
    typedef std::tr1::unordered_map<Sirikata::Network::Address,std::tr1::shared_ptr<Connection>, AddressHasher, AddressEquals > ConnectionMap;
    ConnectionMap mOutboundConnectionMap;
    Sirikata::Network::Address mCurrentInspectionAddress;
    bool eraseOne(SocketList::iterator&i);
public:
    RaknetNetwork();
    virtual void listen (const Sirikata::Network::Address&);
    virtual bool sendTo(const Sirikata::Network::Address&,const Sirikata::Network::Chunk&, bool reliable, bool ordered, int priority);
    virtual Sirikata::Network::Chunk*receiveOne();
   
};

}
