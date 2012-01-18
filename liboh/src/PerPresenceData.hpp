
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/VWObject.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>


#ifndef _SIRIKATA_PER_PRESENCE_DATA_HPP_
#define _SIRIKATA_PER_PRESENCE_DATA_HPP_

namespace Sirikata
{


class PerPresenceData
{
public:
    HostedObjectPtr parent;
    SpaceID space;
    ObjectReference object;
    ProxyObjectPtr mProxyObject;
    ProxyManagerPtr proxyManager;
    String query;
    HostedObject::BaseDatagramLayerPtr mSSTDatagramLayers;
    // Outstanding requests for loc updates.
    enum LocField {
        LOC_FIELD_NONE = 0,
        LOC_FIELD_LOC = 1,
        LOC_FIELD_ORIENTATION = 1 << 1,
        LOC_FIELD_BOUNDS = 1 << 2,
        LOC_FIELD_MESH = 1 << 3,
        LOC_FIELD_PHYSICS = 1 << 4
    };

    LocField updateFields;
    uint64 requestEpoch;
    // Requested location information, may or may not have been
    // accepted/applied by the space. We keep this as a shared_ptr so
    // we can return it in place of a down-casted
    // ProxyObjectPtr. This tracks the request epoch for each
    // component since they are requested independently so we need to
    // resolve differences for each component independently.
    SequencedPresencePropertiesPtr requestLoc;
    Network::IOTimerPtr rerequestTimer;

    // This tracks the latest epoch we've seen *reported* from the space server,
    // i.e. what requests the server has handled.
    uint64 latestReportedEpoch;

    typedef std::map<String, Simulation*> SimulationMap;
    SimulationMap sims;

    PerPresenceData(HostedObjectPtr _parent, const SpaceID& _space, const ObjectReference& _oref, const HostedObject::BaseDatagramLayerPtr& layer, const String& query);
    ~PerPresenceData();

    ProxyManagerPtr getProxyManager();

    SpaceObjectReference id() const;
    void initializeAs(ProxyObjectPtr proxyobj);

};

inline PerPresenceData::LocField operator|(PerPresenceData::LocField a, PerPresenceData::LocField b){
    return static_cast<PerPresenceData::LocField>(static_cast<int>(a) | static_cast<int>(b));
}
inline PerPresenceData::LocField operator|=(PerPresenceData::LocField& a, PerPresenceData::LocField b){
    return (a = static_cast<PerPresenceData::LocField>(static_cast<int>(a) | static_cast<int>(b)));
}
inline PerPresenceData::LocField operator&(PerPresenceData::LocField a, PerPresenceData::LocField b){
    return static_cast<PerPresenceData::LocField>(static_cast<int>(a) & static_cast<int>(b));
}

}//end namespace sirikata


#endif
