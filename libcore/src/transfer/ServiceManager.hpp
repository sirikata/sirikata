/*  Sirikata Transfer -- Content Distribution Network
 *  ServiceManager.hpp
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
/*  Created on: Apr 21, 2009 */

#ifndef SIRIKATA_ServiceManager_HPP__
#define SIRIKATA_ServiceManager_HPP__

#include "ServiceLookup.hpp"
#include "ProtocolRegistry.hpp"

namespace Sirikata {
namespace Transfer {


template <class ProtocolType>
class ServiceManager {
public:
	typedef ServiceLookup::Callback Callback;
private:
	ServiceLookup * const mServices;
	typedef ProtocolRegistry<ProtocolType> ProtoReg;
	ProtoReg * const mProtocol;

public:
	ServiceManager(ServiceLookup *services, ProtoReg *protocol)
		: mServices(services), mProtocol(protocol) {
	}

	inline ServiceLookup * getServiceLookup() const {
		return mServices;
	}

	inline ProtoReg * getProtocolRegistry() const {
		return mProtocol;
	}

	bool getNextProtocol(ServiceIterator *iter, ServiceIterator::ErrorType reason, const URI &uri,
			URI &outURI, ServiceParams &params, typename ProtoReg::HandlerPtr &protoHandler) const {
		bool ret;
		do {
			outURI = uri;
			ret = iter->tryNext(reason, outURI, params);
			if (!ret) {
				return false;
			}
			outURI.getContext().setProto(getHandler(outURI.proto(), protoHandler));
			if (!protoHandler) {
				reason = ServiceIterator::UNSUPPORTED;
			}
		} while(!protoHandler && ret);
		return true;
	}

	void lookupService(const URIContext &context, const Callback &cb) const {
		if (mServices) {
			mServices->lookupService(context, cb);
		} else {
			cb(new SimpleServiceIterator(ListOfServicesPtr()));
		}
	}

	std::string getHandler(const std::string &proto, typename ProtoReg::HandlerPtr &retHandler) const {
		return mProtocol->lookup(proto, retHandler);
	}
};

}
}

#endif /* SIRIKATA_ServiceManager_HPP__ */

