
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/VWObject.hpp>
#include <sirikata/oh/ObjectHostProxyManager.hpp>


#ifndef _SIRIKATA_PER_PRESENCE_DATA_HPP_
#define _SIRIKATA_PER_PRESENCE_DATA_HPP_

namespace Sirikata
{


class PerPresenceData
{
public:
    HostedObject* parent;
    SpaceID space;
    ObjectReference object;
    ProxyObjectPtr mProxyObject;
    ProxyObject::Extrapolator mUpdatedLocation;
    ObjectHostProxyManagerPtr proxyManager;
    bool validSpaceObjRef;
    SolidAngle queryAngle;
    
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
    TimedMotionVector3f requestLocation;
    TimedMotionQuaternion requestOrientation;
    BoundingSphere3f requestBounds;
    String requestMesh;
    String requestPhysics;
    Network::IOTimerPtr rerequestTimer;

    typedef std::map<String, TimeSteppedSimulation*> SimulationMap;
    SimulationMap sims;

    PerPresenceData(HostedObject* _parent, const SpaceID& _space, const ObjectReference& _oref,const SolidAngle& qAngle);
    PerPresenceData(HostedObject* _parent, const SpaceID& _space,const SolidAngle& qAngle);
    ~PerPresenceData();

    void populateSpaceObjRef(const SpaceObjectReference& sporef);

    ObjectHostProxyManagerPtr getProxyManager();

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
