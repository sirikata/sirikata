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


    // MAIN Thread: Utility methods that should only be called from the main
    // strand

    // Server-to-server messages
    Router<Message*>* mProxServerMessageService;

    // Server-to-Object streams
    typedef SST::Stream<SpaceObjectReference>::Ptr ProxObjectStreamPtr;
    // ProxStreamInfo manages *most* of the state for sending data to
    // a client. This data is managed by the main thread, where
    // messaging is performed. See SeqNoInfo for how sequence numbers
    // are stored -- they need to be accessed in the Prox thread so
    // they are managed separately.
    struct ProxStreamInfo {
    public:
        ProxStreamInfo()
         : iostream_requested(false), writing(false) {}
        void disable() {
            if (iostream)
                iostream->close(false);
        }
        // The actual stream we send on
        ProxObjectStreamPtr iostream;
        // Whether we've requested the iostream
        bool iostream_requested;
        // Outstanding data to be sent. FIXME efficiency
        std::queue<std::string> outstanding;
        // If writing is currently in progress
        bool writing;
        // Stored callback for writing
        std::tr1::function<void()> writecb;
    };
    typedef std::tr1::shared_ptr<ProxStreamInfo> ProxStreamInfoPtr;
    typedef std::tr1::weak_ptr<ProxStreamInfo> ProxStreamInfoWPtr;

    // Utility for poll.  Queues a message for delivery, encoding it and putting
    // it on the send stream.  If necessary, starts send processing on the stream.
    void sendObjectResult(Sirikata::Protocol::Object::ObjectMessage*);
    // The driver for getting data to the OH, initially triggered by sendObjectResults
    void writeSomeObjectResults(ProxStreamInfoWPtr prox_stream);
    // Helper for setting up the initial proximity stream. Retries automatically
    // until successful.
    void requestProxSubstream(const UUID& objid, ProxStreamInfoPtr prox_stream);
    // Helper that handles callbacks about prox stream setup
    void proxSubstreamCallback(int x, const UUID& objid, ProxObjectStreamPtr parent_stream, ProxObjectStreamPtr substream, ProxStreamInfoPtr prox_stream_info);

    typedef std::tr1::unordered_map<UUID, ProxStreamInfoPtr, UUID::Hasher> ObjectProxStreamMap;
    ObjectProxStreamMap mObjectProxStreams;



    // PROX Thread - Should only be accessed in methods used by the prox thread
    Network::IOStrand* mProxStrand;

    CBRLocationServiceCache* mLocCache;

}; // class LibproxProximityBase

} // namespace Sirikata

#endif //_SIRIKATA_LIBPROX_PROXIMITY_BASE_HPP_
