/*  Sirikata Transfer -- Content Distribution Network
 *  ProtocolRegistry.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*  Created on: Feb 5, 2009 */

#ifndef SIRIKATA_ProtocolHandler_HPP__
#define SIRIKATA_ProtocolHandler_HPP__

#include "TransferData.hpp"
#include "ServiceLookup.hpp"
#include "URI.hpp"

namespace Sirikata {
namespace Transfer {

/** A handle to an active transfer. Currently its only use is to call abort().
 * Since this contains a shared_ptr to the transfer, it may be necessary.
 *
 * Hold onto this if you want to be able to abort the transfer.
 * Otherwise, it is safe to discard this. Note that nothing will be
 * free'd as long as you hold a reference to the TransferData. */

template <class ProtocolType> class ProtocolData {
	typedef std::tr1::shared_ptr<ProtocolType> ParentPtr;
	ParentPtr mParent;
public:
	typedef std::tr1::shared_ptr<ProtocolData<ProtocolType> > Ptr;

	const ParentPtr &getParent() {
		return mParent;
	}
	ProtocolData(const ParentPtr &parent)
		:mParent(parent) {
	}
	virtual ~ProtocolData() {}

	virtual void abort() {}
};

/** Class to handle associations from a basic internet protocol (http, ftp, ...) to a class that can download from it.
 *
 * Note that this does not yet handle security, so beware if you decide
 * to implement a "file:///" protocol, that someone doesn't try to
 * access files on your hard drive.
 *
 * There have been many security bugs in browsers due to these problems.
 */
template <class ProtocolType>
class ProtocolRegistry {
	typedef std::tr1::shared_ptr<ProtocolType> HandlerPtr;
	typedef std::map<std::string, std::pair<std::string, HandlerPtr> > HandlerMap;
	HandlerMap mHandlers;
	ServiceLookup *mServices;
	bool mAllowNonService;

public:
	explicit ProtocolRegistry() : mServices(NULL), mAllowNonService(true) {
	}

	ProtocolRegistry(ServiceLookup *services, bool allowNonService=false)
		: mServices(services), mAllowNonService(allowNonService) {
	}

	/** Sets the protocol handler. Note that there can currently be only one
	 * protocol handler per protocol per ProtocolType.
	 *
	 * Warning: NOT THREAD SAFE! Call this only at the beginning of the application.
	 *
	 * @param proto   A string, without a colon (e.g. "http")
	 * @param handler A shared_ptr to a descendent of ProtocolType. It is legal to register
	 *                the same instance for multiple protocols ("http" and "ftp").
	 */
	void setHandler(const std::string &proto, const std::string &mappedproto, const HandlerPtr &handler) {
		mHandlers[proto] = std::pair<std::string, HandlerPtr>(mappedproto, handler);
	}

	void setHandler(const std::string &proto, const HandlerPtr &handler) {
		mHandlers[proto] = std::pair<std::string, HandlerPtr>(proto, handler);
	}

	/** Removes a registered protocol handler. */
	void removeHandler(const std::string &proto) {
		mHandlers.remove(proto);
	}

	/** Looks up proto in the map. Returns NULL if no protocol handler was found. */
	const std::string &lookup(const std::string &proto, HandlerPtr &handlerPtr) const {
		typename HandlerMap::const_iterator iter = mHandlers.find(proto);
		if (iter == mHandlers.end()) {
			SILOG(transfer,error,"No protocol handler registered for "<<proto);
			return proto;
		}
		const std::pair<std::string, HandlerPtr> &protoHandler = (*iter).second;
		handlerPtr = protoHandler.second;
		return protoHandler.first;
	}

	void lookupService(const URIContext &context, const ServiceLookup::Callback &cb, bool allowProto=true) const {
		if (allowProto && mAllowNonService &&
				mHandlers.find(context.proto()) != mHandlers.end()) {
			ListOfServicesPtr services(new ListOfServices);
			services->push_back(ListOfServices::value_type(context, ServiceParams()));
			cb(services);
		} else if (mServices) {
			mServices->lookupService(context, cb);
		} else {
			cb(ListOfServicesPtr());
			/*
			ListOfServicesPtr services(new ListOfServices);
			services->push_back(context);
			cb(services);
			*/
		}
	}
};

}
}

#endif /* SIRIKATA_ProtocolHandler_HPP__ */
