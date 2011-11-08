// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPROX_PROXIMITY_BASE_HPP_
#define _SIRIKATA_LIBPROX_PROXIMITY_BASE_HPP_

#include <sirikata/space/Proximity.hpp>
#include "CBRLocationServiceCache.hpp"

namespace Sirikata {

/** Base class for Libprox-based Proximity implementations, providing a bit of
 *  utility code that gets reused across different implementations.
 */
class LibproxProximityBase : public Proximity {
public:
    LibproxProximityBase(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr);
    ~LibproxProximityBase();

protected:
    // Helper types & methods
    enum ObjectClass {
        OBJECT_CLASS_STATIC = 0,
        OBJECT_CLASS_DYNAMIC = 1,
        NUM_OBJECT_CLASSES = 2
    };
    static const std::string& ObjectClassToString(ObjectClass c);
    static BoundingBox3f aggregateBBoxes(const BoundingBoxList& bboxes);
    static bool velocityIsStatic(const Vector3f& vel);


    // BOTH Threads: These are read-only.

    // To support a static/dynamic split but also support mixing them for
    // comparison purposes track which we are doing and, for most places, use a
    // simple index to control whether they point to different query handlers or
    // the same one.
    bool mSeparateDynamicObjects;
    int mNumQueryHandlers;


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

        ProxStreamInfo()
         : iostream_requested(false), writing(false) {}
        void disable() {
            if (iostream)
                iostream->close(false);
        }
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

        // Defined safely in cpp since these are only used from LibproxProximityBase
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

    // Utility for poll.  Queues a message for delivery, encoding it and putting
    // it on the send stream.  If necessary, starts send processing on the stream.
    void sendObjectResult(Sirikata::Protocol::Object::ObjectMessage*);
    void sendObjectHostResult(const OHDP::NodeID& node, Sirikata::Protocol::Object::ObjectMessage*);

    // Helpers that are protocol-specific
    bool validSession(const ObjectReference& oref) const;
    bool validSession(const OHDP::NodeID& node) const;
    ProxObjectStreamPtr getBaseStream(const ObjectReference& oref) const;
    ProxObjectHostStreamPtr getBaseStream(const OHDP::NodeID& node) const;

    typedef std::tr1::unordered_map<UUID, ProxObjectStreamInfoPtr, UUID::Hasher> ObjectProxStreamMap;
    ObjectProxStreamMap mObjectProxStreams;

    typedef std::tr1::unordered_map<OHDP::NodeID, ProxObjectHostStreamInfoPtr, OHDP::NodeID::Hasher> ObjectHostProxStreamMap;
    ObjectHostProxStreamMap mObjectHostProxStreams;


    // PROX Thread - Should only be accessed in methods used by the prox thread
    Network::IOStrand* mProxStrand;

    CBRLocationServiceCache* mLocCache;

}; // class LibproxProximityBase

} // namespace Sirikata

#endif //_SIRIKATA_LIBPROX_PROXIMITY_BASE_HPP_
