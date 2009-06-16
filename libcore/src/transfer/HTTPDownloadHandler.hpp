/*  Sirikata Transfer -- Content Distribution Network
 *  HTTPDownloadHandler.hpp
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
/*  Created on: Feb 8, 2009 */

#ifndef SIRIKATA_HTTPDownloadHandler_HPP__
#define SIRIKATA_HTTPDownloadHandler_HPP__

#include "HTTPRequest.hpp"
#include "DownloadHandler.hpp"

namespace Sirikata {
namespace Transfer {

/** A very simple subclass of DownloadHandler that wraps around the HTTPRequest
 * curl interface. It also provides a subclass of NameLookupHandler that reads
 * the fingerprint URI from a HTTP uri.
 * */
class HTTPDownloadHandler :
			public DownloadHandler,
			public NameLookupHandler,
			public std::tr1::enable_shared_from_this<HTTPDownloadHandler>
{

	template <class Base>
	class HTTPTransferData : public ProtocolData<Base> {
		HTTPRequestPtr http;
	public:
		HTTPTransferData(const std::tr1::shared_ptr<Base> &parent,
				const HTTPRequestPtr &httpreq)
			: ProtocolData<Base>(parent),
			  http(httpreq) {
		}

		virtual ~HTTPTransferData() {}

		virtual void abort() {
			http->abort();
		}
	};

	static void httpCallback(
			DownloadHandler::Callback callback,
			HTTPRequest* httpreq,
			const DenseDataPtr &recvData,
			bool success) {
		callback(recvData, success, httpreq->getFullLength());
	}

	struct IsSpace {
		bool operator()(const unsigned char c) {
			return std::isspace(c);
		}
	};

	static void nameCallback(
			NameLookupHandler::Callback callback,
			HTTPRequest* httpreq,
			const DenseDataPtr &data,
			bool success) {
		if (success) {
			cache_usize_type length = 0;
			const unsigned char *content = data->data();
			std::string receivedUri (content, content + data->length());
			receivedUri.erase(std::remove_if(receivedUri.begin(), receivedUri.end(), IsSpace()), receivedUri.end());
			URI temp(httpreq->getURI().context(), receivedUri);
			std::string shasum = temp.filename();
			{
				Fingerprint fp;
				bool success = false;
				try {
					success = true;
					fp = Fingerprint::convertFromHex(shasum);
				} catch (std::invalid_argument &e) {
					SILOG(transfer,error,"HTTP name lookup gave URI " << httpreq->getURI() << " which is not a hash URI!");
				}
				callback(fp, receivedUri, success);
                        }
		} else {
			SILOG(transfer,error,"HTTP name lookup failed for " << httpreq->getURI());
			callback(Fingerprint(), std::string(), false);
		}
	}

public:
	/** Simple wrapper around HTTPRequest to download a URL.
	 * The returned TransferDataPtr contains a shared reference to the HTTPRequest.
	 * If you hold onto it, you can abort the download. */
	virtual void download(DownloadHandler::TransferDataPtr *ptrRef,
			const URI &uri,
			const Range &bytes,
			const DownloadHandler::Callback &cb) {
		HTTPRequestPtr req (new HTTPRequest(uri, bytes));


		 //Vc9 needs this
		req->setCallback(
			std::tr1::bind(&HTTPDownloadHandler::httpCallback, cb, _1, _2, _3));
		// should call callback when it finishes.
		if (ptrRef) {
			/*
			 * Must set this before calling req->go() or else
			 * it is possible for the HTTPRequest to call cb() before go() returns,
			 * which will cause the object owning *ptrRef to be deleted.
			 */
			*ptrRef = DownloadHandler::TransferDataPtr(
				new HTTPTransferData<DownloadHandler>(shared_from_this(), req));
		}

		req->go(req);
	}
	/// FIXME: Unimplemented -- needs a better interface from HTTPRequest to work.
	virtual void stream(DownloadHandler::TransferDataPtr *ptrRef,
			const URI &uri,
			const Range &bytes,
			const DownloadHandler::Callback &cb) {
		// Does not work yet.
		cb(DenseDataPtr(), false, 0);
	}

	/// HTTP (as with most TCP protocols) returns packets in order.
	virtual bool inOrderStream() const {
		return true;
	}

	/** Provides a simple nameLookup service using HTTP. It expects a request
	 * that responds with a URI string and nothing else in the body (do not use
	 * a 302 HTTP code to do this, as curl will silently redirect).
	 * Also strips ASCII spaces/newlines from the body.
	 */
	virtual void nameLookup(NameLookupHandler::TransferDataPtr *ptrRef,
			const URI &uri,
			const NameLookupHandler::Callback &cb) {
		HTTPRequestPtr req (new HTTPRequest(uri, Range(true)));


		req->setCallback(
			std::tr1::bind(&HTTPDownloadHandler::nameCallback, cb, _1, _2, _3));

		if (ptrRef) {
			/*
			 * Must set this before calling req->go() or else
			 * it is possible for the HTTPRequest to call cb() before go() returns,
			 * which will cause the object owning *ptrRef to be deleted.
			 */
			*ptrRef = NameLookupHandler::TransferDataPtr(
				new HTTPTransferData<NameLookupHandler>(shared_from_this(), req));
		}
		req->go(req);
	}
};

}
}

#endif /* SIRIKATA_HTTPDownloadHandler_HPP__ */
