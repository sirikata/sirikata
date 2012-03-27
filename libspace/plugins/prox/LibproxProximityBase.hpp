// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPROX_PROXIMITY_BASE_HPP_
#define _SIRIKATA_LIBPROX_PROXIMITY_BASE_HPP_

#include <sirikata/space/Proximity.hpp>
#include "CBRLocationServiceCache.hpp"
#include <prox/base/QueryEvent.hpp>

namespace Sirikata {

/** Base class for Libprox-based Proximity implementations, providing a bit of
 *  utility code that gets reused across different implementations.
 */
class LibproxProximityBase : public Proximity {
public:
    LibproxProximityBase(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr);
    ~LibproxProximityBase();

protected:
    typedef Prox::QueryEvent<ObjectProxSimulationTraits> QueryEvent;
    typedef std::deque<QueryEvent> QueryEventList;

    // Helper types & methods
    enum ObjectClass {
        OBJECT_CLASS_STATIC = 0,
        OBJECT_CLASS_DYNAMIC = 1,
        NUM_OBJECT_CLASSES = 2
    };
    static const std::string& ObjectClassToString(ObjectClass c);
    static BoundingBox3f aggregateBBoxes(const BoundingBoxList& bboxes);
    static bool velocityIsStatic(const Vector3f& vel);

    // Coalesces events, turning them effectively into one giant event (although
    // split across enough events that no event is too large). This gets rid of
    // any intermediate additions/removals that occurred as the cut was
    // refined/unrefined. The old results are destroyed and the new ones are
    // placed back in the query event queue.
    //
    // per_event indicates how many additions/removals to put in each
    // event. Since they are no longer forced to be together to be atomic, we
    // can pack them however we like.
    void coalesceEvents(QueryEventList& evts, uint32 per_event);

    // BOTH Threads: These are read-only.

    // To support a static/dynamic split but also support mixing them for
    // comparison purposes track which we are doing and, for most places, use a
    // simple index to control whether they point to different query handlers or
    // the same one.
    bool mSeparateDynamicObjects;
    int mNumQueryHandlers;
    // When using separate trees, how long to wait after an object becomes
    // static to move it into the static tree. This keeps us from moving things
    // in and out of trees frequently because of short stops (e.g. and avatar
    // stops for a few seconds while walking).
    Duration mMoveToStaticDelay;


    // MAIN Thread: Utility methods that should only be called from the main
    // strand

    // Server-to-server messages
    Router<Message*>* mProxServerMessageService;

    // Server-to-Object, Server-to-ObjectHost streams

    // ProxStreamInfo manages *most* of the state for sending data to
    // a client. This data is managed by the main thread, where
    // messaging is performed. See SeqNoInfo for how sequence numbers
    // are stored -- they need to be accessed in the Prox thread so
    // they are managed separately.
    template<typename EndpointType, typename StreamType>
    struct ProxStreamInfo {
    public:
        typedef std::tr1::shared_ptr<StreamType> StreamTypePtr;
        typedef std::tr1::shared_ptr<ProxStreamInfo> Ptr;
        typedef std::tr1::weak_ptr<ProxStreamInfo> WPtr;

        // Start a fresh ProxStreamInfo, which will require requesting
        // a new substream
        ProxStreamInfo()
         : iostream_requested(false), writing(false) {}
        // Start a ProxStreamInfo on an existing stream.
        ProxStreamInfo(StreamTypePtr strm)
         : iostream(strm), iostream_requested(true), writing(false) {}

        void disable() {
            if (iostream)
                iostream->close(false);
        }

        // Setup reading of frames from the stream. ProxStreamInfo
        // takes care of queueing up messages until complete frames
        // are available, giving just a callback per message.
        typedef std::tr1::function<void(String&)> FrameReceivedCallback;
        void readFramesFromStream(Ptr prox_stream, FrameReceivedCallback cb);

        // The actual stream we send on
        StreamTypePtr iostream;
        // Whether we've requested the iostream
        bool iostream_requested;

        // Outstanding data to be sent. FIXME efficiency
        std::queue<std::string> outstanding;
        // If writing is currently in progress
        bool writing;
        // Stored callback for writing
        std::tr1::function<void()> writecb;

        // Stored callback for reading frames
        FrameReceivedCallback read_frame_cb;
        // Backlog of data, i.e. incomplete frame
        String partial_frame;

        // Defined safely in cpp since these are only used from
        // LibproxProximityBase

        // Handle reads from the underlying stream, decoding frames
        // and invoking the read callback
        static void handleRead(WPtr w_prox_stream, uint8* data, int size);

        // The driver for getting data to the OH, initially triggered by sendObjectResults
        static void writeSomeObjectResults(Context* ctx, WPtr prox_stream);
        // Helper for setting up the initial proximity stream. Retries automatically
        // until successful.
        static void requestProxSubstream(LibproxProximityBase* parent, Context* ctx, const EndpointType& oref, Ptr prox_stream);
        // Helper that handles callbacks about prox stream setup
        static void proxSubstreamCallback(LibproxProximityBase* parent, Context* ctx, int x, const EndpointType& oref, StreamTypePtr parent_stream, StreamTypePtr substream, Ptr prox_stream_info);
    };

    typedef ODPSST::Stream::Ptr ProxObjectStreamPtr;
    typedef ProxStreamInfo<ObjectReference, ODPSST::Stream> ProxObjectStreamInfo;
    typedef std::tr1::shared_ptr<ProxObjectStreamInfo> ProxObjectStreamInfoPtr;
    typedef OHDPSST::Stream::Ptr ProxObjectHostStreamPtr;
    typedef ProxStreamInfo<OHDP::NodeID, OHDPSST::Stream> ProxObjectHostStreamInfo;
    typedef std::tr1::shared_ptr<ProxObjectHostStreamInfo> ProxObjectHostStreamInfoPtr;

    // Utility for implementations. Start listening on the stream and
    // read each Network::Frame, emitting a callback for each.
    void readFramesFromObjectStream(const ObjectReference& oref, ProxObjectStreamInfo::FrameReceivedCallback cb);
    void readFramesFromObjectHostStream(const OHDP::NodeID& node, ProxObjectHostStreamInfo::FrameReceivedCallback cb);

    // Utility for poll.  Queues a message for delivery, encoding it and putting
    // it on the send stream.  If necessary, starts send processing on the stream.
    void sendObjectResult(Sirikata::Protocol::Object::ObjectMessage*);
    void sendObjectHostResult(const OHDP::NodeID& node, Sirikata::Protocol::Object::ObjectMessage*);

    // Helpers that are protocol-specific
    bool validSession(const ObjectReference& oref) const;
    bool validSession(const OHDP::NodeID& node) const;
    ProxObjectStreamPtr getBaseStream(const ObjectReference& oref) const;
    ProxObjectHostStreamPtr getBaseStream(const OHDP::NodeID& node) const;
    // Use these to setup ProxStreamInfo's when the client initiates
    // the stream that will be used to communicate with it.
    void addObjectProxStreamInfo(ODPSST::Stream::Ptr);
    void addObjectHostProxStreamInfo(OHDPSST::Stream::Ptr);

    // Handle various events in the main thread that are triggered in the prox thread
    void handleAddObjectLocSubscription(const UUID& subscriber, const UUID& observed);
    void handleRemoveObjectLocSubscription(const UUID& subscriber, const UUID& observed);
    void handleRemoveAllObjectLocSubscription(const UUID& subscriber);
    void handleAddOHLocSubscription(const OHDP::NodeID& subscriber, const UUID& observed);
    void handleRemoveOHLocSubscription(const OHDP::NodeID& subscriber, const UUID& observed);
    void handleRemoveAllOHLocSubscription(const OHDP::NodeID& subscriber);
    void handleAddServerLocSubscription(const ServerID& subscriber, const UUID& observed, SeqNoPtr seqPtr);
    void handleRemoveServerLocSubscription(const ServerID& subscriber, const UUID& observed);
    void handleRemoveAllServerLocSubscription(const ServerID& subscriber);

    typedef std::tr1::unordered_map<UUID, ProxObjectStreamInfoPtr, UUID::Hasher> ObjectProxStreamMap;
    ObjectProxStreamMap mObjectProxStreams;

    typedef std::tr1::unordered_map<OHDP::NodeID, ProxObjectHostStreamInfoPtr, OHDP::NodeID::Hasher> ObjectHostProxStreamMap;
    ObjectHostProxStreamMap mObjectHostProxStreams;


    // PROX Thread - Should only be accessed in methods used by the prox thread
    Network::IOStrand* mProxStrand;

    CBRLocationServiceCache* mLocCache;

    // Query-Type-Agnostic AggregateListener Interface -- manages adding to Loc
    // and passing to AggregateManager, but you need to delegate to these
    // yourself since the AggregateListener interface depends on the type of
    // query/query handler being used.
    virtual void aggregateCreated(const UUID& objid);
    virtual void aggregateChildAdded(const UUID& objid, const UUID& child, const BoundingSphere3f& bnds);
    virtual void aggregateChildRemoved(const UUID& objid, const UUID& child, const BoundingSphere3f& bnds);
    virtual void aggregateBoundsUpdated(const UUID& objid, const BoundingSphere3f& bnds);
    virtual void aggregateDestroyed(const UUID& objid);
    virtual void aggregateObserved(const UUID& objid, uint32 nobservers);
    // Helper for updating aggregates
    void updateAggregateLoc(const UUID& objid, const BoundingSphere3f& bnds);

    // Command handlers
    virtual void commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;
    virtual void commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;
    virtual void commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;
    virtual void commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;

}; // class LibproxProximityBase

} // namespace Sirikata

#endif //_SIRIKATA_LIBPROX_PROXIMITY_BASE_HPP_
