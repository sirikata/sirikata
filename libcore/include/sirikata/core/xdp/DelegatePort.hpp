// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_XDP_DELEGATE_PORT_HPP_
#define _SIRIKATA_LIBCORE_XDP_DELEGATE_PORT_HPP_

#include <sirikata/core/xdp/Defs.hpp>

namespace Sirikata {
namespace XDP {

class DelegateService;

/** An implementation of XDP::Port, templatized on an Endpoint type, that
 *  handles all the bookkeeping and sanity checking, but delegates real
 *  operations to the user of the class via some callback functions registered
 *  at creation.
 *
 *  This class works in conjunction with DelegateService.
 */
template<typename EndpointType, typename DelegateServiceType, typename PortType = Sirikata::XDP::Port<EndpointType> >
class DelegatePort : public PortType {
public:
    typedef EndpointType Endpoint;
    typedef PortType Port;
    typedef DelegateServiceType DelegateService;
    typedef typename DelegateServiceType::MessageHandler MessageHandler;

    typedef std::tr1::function<bool(const Endpoint&, MemoryReference payload)> SendFunction;

    /** Create a new DelegatePort with the given properties which handles sends
     *  via send_func.
     *  \param parent the parent DelegateService that manages this port
     *  \param ep the endpoint identifier for this port, must be fully qualified
     *  \param send_func functor to invoke to send messages
     */
    DelegatePort(DelegateService* parent, const Endpoint& ep, SendFunction send_func)
        : mParent(parent),
        mEndpoint(ep),
        mSendFunc(send_func),
        mFromHandlers(),
        mInvalidated(false)
        {
        }

    virtual ~DelegatePort() {
        // Get it out of the parent map to ensure we won't get any more messages
        if (!mInvalidated)
            mParent->deallocatePort(this);
    }


    // Port Interface
    virtual const Endpoint& endpoint() const { return mEndpoint; }
    virtual bool send(const Endpoint& to, MemoryReference payload) {
        if (mInvalidated) return false;
        return mSendFunc(to, payload);
    }
    virtual void receiveFrom(const Endpoint& from, const MessageHandler& cb) {
        mFromHandlers[from] = cb;
    }

    /** Deliver a message via this port.  Returns true if a receiver was found
     *  for the message, false if none was found and it was ignored.
     */
    bool deliver(const Endpoint& src, const Endpoint& dst, MemoryReference data) const {
        if (mInvalidated) return false;

        // See Port documentation for details on this ordering.
        if (tryDeliver(src, src, dst, data)) return true;
        if (tryDeliver(Endpoint(src.space(), Endpoint::Identifier::any(), src.port()), src, dst, data)) return true;
        if (tryDeliver(Endpoint(SpaceID::any(), Endpoint::Identifier::any(), src.port()), src, dst, data)) return true;
        if (tryDeliver(Endpoint(src.space(), src.id(), PortID::any()), src, dst, data)) return true;
        if (tryDeliver(Endpoint(src.space(), Endpoint::Identifier::any(), PortID::any()), src, dst, data)) return true;
        if (tryDeliver(Endpoint(SpaceID::any(), Endpoint::Identifier::any(), PortID::any()), src, dst, data)) return true;

        return false;
    }

    /** Invalidate this port. This effectively disables the port. This is used
     *  by the parent DelegateService to stop the Port from doing anything
     *  dangerous after the parent is deleted, but doesn't actually delete this
     *  port since others might still hold a pointer to it.
     */
    void invalidate() {
        // Mark and remove from parent
        mInvalidated = true;
        mParent->deallocatePort(this);
    }

private:
    // Worker method for deliver, tries to deliver to the handler for this exact
    // endpoint.
    // \deprecated Prefer the version using both Endpoints

    bool tryDeliver(const Endpoint& src_match_ep, const Endpoint& src_real_ep, const Endpoint& dst, MemoryReference data) const {
        typename ReceiveFromHandlers::const_iterator rit = mFromHandlers.find(src_match_ep);

        if (rit == mFromHandlers.end())
            return false;

        const MessageHandler& handler = rit->second;

        handler(src_real_ep, dst, data);

        return true;
    }


    typedef std::tr1::unordered_map<Endpoint, MessageHandler, typename Endpoint::Hasher> ReceiveFromHandlers;

    DelegateService* mParent;
    Endpoint mEndpoint;
    SendFunction mSendFunc;
    ReceiveFromHandlers mFromHandlers;
    bool mInvalidated;
}; // class DelegatePort

} // namespace XDP
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_XDP_DELEGATE_PORT_HPP_
