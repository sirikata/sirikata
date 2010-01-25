#include "SSTImpl.hpp"


#include <sirikata/network/Address.hpp>
#include "Address4.hpp"




#include "Random.hpp"

#include <boost/bind.hpp>


//Assume we have a send(void* data, int len) function and a handleRead(void*) function
//

namespace CBR {

template <>  std::map<Sirikata::UUID, boost::shared_ptr< BaseDatagramLayer<Sirikata::UUID> > > BaseDatagramLayer<Sirikata::UUID>::mDatagramLayerMap = std::map<Sirikata::UUID, boost::shared_ptr< BaseDatagramLayer<Sirikata::UUID> > > ();

template <> std::map<EndPoint<Sirikata::UUID>  , boost::shared_ptr< Connection<Sirikata::UUID> > > Connection<Sirikata::UUID>::mConnectionMap = std::map<EndPoint<Sirikata::UUID>  , boost::shared_ptr< Connection<Sirikata::UUID> > > ();

    
template <> std::map<EndPoint<Sirikata::UUID>  , ConnectionReturnCallbackFunction > Connection<Sirikata::UUID>::mConnectionReturnCallbackMap = std::map<EndPoint<Sirikata::UUID>  , ConnectionReturnCallbackFunction > ();

template <> std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > Connection<Sirikata::UUID>::mListeningConnectionsCallbackMap = std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > ();

template <> std::bitset<65536> Connection<Sirikata::UUID>::mAvailableChannels = std::bitset<65536> ();
template <> uint16 Connection<Sirikata::UUID>::mLastAssignedPort = 65530;


template <> std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > Stream<Sirikata::UUID>::mStreamReturnCallbackMap = std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > ();


template <> Mutex Stream<Sirikata::UUID>::mStreamCreationMutex = Mutex();

template <> Mutex Connection<Sirikata::UUID>::mStaticMembersLock = Mutex();


}
