
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/core/util/QueryTracker.hpp>
#include <sirikata/proxyobject/VWObject.hpp>
#include <sirikata/oh/ObjectHostProxyManager.hpp>


#ifndef _SIRIKATA_PER_PRESENCE_DATA_HPP_
#define _SIRIKATA_PER_PRESENCE_DATA_HPP_

namespace Sirikata
{

class SIRIKATA_EXPORT PerPresenceData
{
public:
    HostedObject* parent;
    SpaceID space;
    ObjectReference object;
    ProxyObjectPtr mProxyObject;
    ProxyObject::Extrapolator mUpdatedLocation;
    QueryTracker* tracker;
    ObjectHostProxyManagerPtr proxyManager;
    bool validSpaceObjRef;
    

    PerPresenceData(HostedObject* _parent, const SpaceID& _space, const ObjectReference& _oref);
    PerPresenceData(HostedObject* _parent, const SpaceID& _space);

    void populateSpaceObjRef(const SpaceObjectReference& sporef);
        
    ObjectHostProxyManagerPtr getProxyManager();
    
    SpaceObjectReference id() const;    
    void initializeAs(ProxyObjectPtr proxyobj);
    void destroy(QueryTracker *tracker) const;
    
};

}//end namespace sirikata


#endif
