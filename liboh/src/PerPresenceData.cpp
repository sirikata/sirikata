
#include <sirikata/core/util/Platform.hpp>

#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/VWObject.hpp>
#include <sirikata/oh/PerPresenceData.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>


namespace Sirikata{

    SpaceObjectReference PerPresenceData::id() const
    {
        return SpaceObjectReference(space, object);
    }


PerPresenceData::PerPresenceData(HostedObjectPtr _parent, const SpaceID& _space, const ObjectReference& _oref, const HostedObject::BaseDatagramLayerPtr&layer, const String& _query)
     : parent(_parent),
       space(_space),
       object(_oref),
       proxyManager(ProxyManager::construct( _parent, SpaceObjectReference(_space, _oref) )),
       query(_query),
       mSSTDatagramLayers(layer),
       updateFields(LOC_FIELD_NONE),
       requestEpoch(1),
       requestLoc( new SequencedPresenceProperties() ),
       rerequestTimer( Network::IOTimer::create(_parent->context()->ioService) ),
       latestReportedEpoch(0)
    {
    }

PerPresenceData::~PerPresenceData() {
    if (mSSTDatagramLayers)
        mSSTDatagramLayers->invalidate();
    // We no longer have this session, so none of the proxies are usable
    // anymore. We can't delete the ProxyManager, but we can clear it out and
    // trigger destruction events for the proxies it holds.
    proxyManager->destroy();

    rerequestTimer->cancel();
}

    ProxyManagerPtr PerPresenceData::getProxyManager()
    {
        return proxyManager;
    }

    void PerPresenceData::initializeAs(ProxyObjectPtr proxyobj) {
        assert(object == proxyobj->getObjectReference().object());
        mProxyObject = proxyobj;

        // Initialize request location information with sane defaults.
        requestLoc->reset();
        requestLoc->setLocation(proxyobj->verifiedLocation(), 0);
        requestLoc->setOrientation(proxyobj->verifiedOrientation(), 0);
        requestLoc->setBounds(proxyobj->verifiedBounds(), 0);
        requestLoc->setMesh(proxyobj->verifiedMesh(), 0);
        requestLoc->setPhysics(proxyobj->verifiedPhysics(), 0);

        proxyobj->isValid();
    }



}
