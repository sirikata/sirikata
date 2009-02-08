/*  Sirikata Transfer -- Content Distribution Network
 *  ProtocolHandler.hpp
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

namespace Sirikata {
namespace Transfer {

class DownloadHandler : public std::tr1::enable_shared_from_this<DownloadHandler> {
public:
	class TransferData {
		std::tr1::shared_ptr<DownloadHandler> mParent;
	public:
		TransferData(const std::tr1::shared_ptr<DownloadHandler> &parent)
			:mParent(parent) {
		}
		virtual ~TransferData() {}

		virtual void abort() {}
	};
	typedef std::tr1::shared_ptr<TransferData> TransferDataPtr;

	virtual ~DownloadHandler() {
	}

	typedef std::tr1::function<void(DenseDataPtr data, bool success)> Callback;

	virtual TransferDataPtr download(const URI &uri, const Range &bytes, const Callback &cb) {
		cb(DenseDataPtr(), false);
		return TransferDataPtr();
	}
};

class NameLookupHandler {
public:
	typedef std::tr1::function<void(const Fingerprint&, const std::string&, bool success)> Callback;

	virtual ~NameLookupHandler() {
	}

	virtual void nameLookup(const URI &uri, const Callback &cb) {
		cb(Fingerprint(), std::string(), false);
	}
};

template <class ProtocolType>
class ProtocolRegistry {
	std::map<std::string, std::tr1::shared_ptr<ProtocolType> > mHandlers;

public:
	///// NOT THREAD SAFE!
	void setHandler(const std::string &proto, std::tr1::shared_ptr<ProtocolType> handler) {
		mHandlers[proto] = handler;
	}

	void removeHandler(const std::string &proto) {
		mHandlers.remove(proto);
	}

	std::tr1::shared_ptr<ProtocolType> lookup(const std::string &proto) {
		std::tr1::shared_ptr<ProtocolType> protoHandler (mHandlers[proto]);
		if (!protoHandler) {
			std::cerr << "No protocol handler registered for "<<proto<<std::endl;
		}
		return protoHandler;
	}
};

}
}

#endif /* SIRIKATA_ProtocolHandler_HPP__ */
