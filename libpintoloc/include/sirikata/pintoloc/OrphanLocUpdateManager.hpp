// Copyright (c) 2010 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_ORPHAN_LOC_UPDATE_MANAGER_HPP_
#define _SIRIKATA_ORPHAN_LOC_UPDATE_MANAGER_HPP_

#include <sirikata/pintoloc/Platform.hpp>
#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/Defs.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/util/BoundingSphere.hpp>
#include <sirikata/pintoloc/LocUpdate.hpp>
#include <sirikata/pintoloc/ProtocolLocUpdate.hpp>
#include <sirikata/core/util/PresenceProperties.hpp>
#include <sirikata/pintoloc/PresencePropertiesLocUpdate.hpp>

namespace Sirikata {

namespace Protocol {
namespace Loc {
class LocationUpdate;
}
}

/** OrphanLocUpdateManager tracks location updates/information for objects,
 *  making sure that location information does not get lost due to reordering of
 *  proximity and location messages from the space server. It currently handles
 *  two types of errors.
 *
 *  First, if a location message is received before the corresponding proximity
 *  addition event, it is stored for awhile in case the addition is received
 *  soon. (This is the reason for the name of the class -- the location update
 *  is 'orphaned' since it doesn't have a parent proximity addition.)
 *
 *  The second type of issue is when the messages that should be received are
 *  (remove object X, add object X, update location of X), but it is received as
 *  (update location of X, remove object X, add object X). In this case the
 *  update would have been applied and then lost. For this case, we save the
 *  state of objects for a short while after they are removed, including the
 *  sequence numbers. Then, we can reapply them after objects are re-added, just
 *  as we would with an orphaned update.
 *
 *  Loc updates are saved for short time and, if they aren't needed, are
 *  discarded. In all cases, sequence numbers are still used so possibly trying
 *  to apply old updates isn't an issue.
 */
class SIRIKATA_LIBPINTOLOC_EXPORT OrphanLocUpdateManager : public PollingService {
public:
    class Listener {
    public:
        virtual ~Listener() {}
        // You need something like this defined:
        //  virtual void onOrphanLocUpdate(const LocUpdate& lu, Foo foo);
        // in order to get callbacks from invokeOrphanUpdates1 or
        //  virtual void onOrphanLocUpdate(const LocUpdate& lu, Foo foo, Bar bar);
        // in order to get callbacks from invokeOrphanUpdates2.
    };

    OrphanLocUpdateManager(Context* ctx, Network::IOStrand* strand, const Duration& timeout);

    /** Add an orphan update to the queue and set a timeout for it to be cleared
     *  out.
     */
    void addOrphanUpdate(const SpaceObjectReference& observed, const Sirikata::Protocol::Loc::LocationUpdate& update);
    /**
       Take all fields in proxyPtr, and create an struct from
       them.
     */
    void addUpdateFromExisting(ProxyObjectPtr proxyPtr);
    /** Take all parameters from an object to backup for a short time.
     */
    void addUpdateFromExisting(
        const SpaceObjectReference& observed,
        const SequencedPresenceProperties& props
    );

    /** Gets all orphan updates for a given object. */
    // ListenerType would be Listener<QuerierIDType> but we want to support
    // optional extra params passed through to the callback.
    template<typename ListenerType, typename ExtraParamType1>
    void invokeOrphanUpdates1(const TimeSynced& sync, const SpaceObjectReference& proximateID, ListenerType* listener, ExtraParamType1 extra1) {
        ObjectUpdateMap::iterator it = mUpdates.find(proximateID);
        if (it == mUpdates.end()) return;

        const UpdateInfoList& info_list = it->second;
        for(UpdateInfoList::const_iterator info_it = info_list.begin(); info_it != info_list.end(); info_it++) {
            if ((*info_it)->value != NULL) {
                LocProtocolLocUpdate llu( *((*info_it)->value), sync );
                listener->onOrphanLocUpdate( llu, extra1 );
            }
            else if ((*info_it)->opd != NULL) {
                PresencePropertiesLocUpdate plu( (*info_it)->object.object(), *((*info_it)->opd) );
                listener->onOrphanLocUpdate( plu, extra1 );
            }
        }

        // Once we've notified of these we can get rid of them -- if they
        // need the info again they should re-register it with
        // addUpdateFromExisting before cleaning up the object.
        mUpdates.erase(it);
    }
    template<typename ListenerType, typename ExtraParamType1, typename ExtraParamType2>
    void invokeOrphanUpdates2(const TimeSynced& sync, const SpaceObjectReference& proximateID, ListenerType* listener, ExtraParamType1 extra1, ExtraParamType2 extra2) {
        ObjectUpdateMap::iterator it = mUpdates.find(proximateID);
        if (it == mUpdates.end()) return;

        const UpdateInfoList& info_list = it->second;
        for(UpdateInfoList::const_iterator info_it = info_list.begin(); info_it != info_list.end(); info_it++) {
            if ((*info_it)->value != NULL) {
                LocProtocolLocUpdate llu( *((*info_it)->value), sync );
                listener->onOrphanLocUpdate( llu, extra1, extra2 );
            }
            else if ((*info_it)->opd != NULL) {
                PresencePropertiesLocUpdate plu( (*info_it)->object.object(), *((*info_it)->opd) );
                listener->onOrphanLocUpdate( plu, extra1, extra2 );
            }
        }

        // Once we've notified of these we can get rid of them -- if they
        // need the info again they should re-register it with
        // addUpdateFromExisting before cleaning up the object.
        mUpdates.erase(it);
    }

    bool empty() const {
        return mUpdates.empty();
    }

private:
    virtual void poll();

    struct UpdateInfo {
        UpdateInfo(const SpaceObjectReference& obj, Sirikata::Protocol::Loc::LocationUpdate* _v, const Time& t)
         : object(obj), value(_v), opd(NULL), expiresAt(t)
        {}
        UpdateInfo(const SpaceObjectReference& obj, SequencedPresenceProperties* _v, const Time& t)
         : object(obj), value(NULL), opd(_v), expiresAt(t)
        {}
        ~UpdateInfo();

        SpaceObjectReference object;
        //Either value or opd will be non-null.  Never both.
        Sirikata::Protocol::Loc::LocationUpdate* value;
        SequencedPresenceProperties* opd;

        Time expiresAt;
    private:
        UpdateInfo();
    };
    typedef std::tr1::shared_ptr<UpdateInfo> UpdateInfoPtr;
    typedef std::vector<UpdateInfoPtr> UpdateInfoList;

    typedef std::tr1::unordered_map<SpaceObjectReference, UpdateInfoList, SpaceObjectReference::Hasher> ObjectUpdateMap;

    Context* mContext;
    Duration mTimeout;
    ObjectUpdateMap mUpdates;
}; // class OrphanLocUpdateManager

typedef std::tr1::shared_ptr<OrphanLocUpdateManager> OrphanLocUpdateManagerPtr;

} // namespace Sirikata

#endif
