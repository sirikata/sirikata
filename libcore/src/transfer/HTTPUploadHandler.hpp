/*  Sirikata Transfer -- Content Distribution Network
 *  HTTPUploadHandler.hpp
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
/*  Created on: Feb 26, 2009 */

#ifndef SIRIKATA_HTTPUploadHandler_HPP__
#define SIRIKATA_HTTPUploadHandler_HPP__

namespace Sirikata {
namespace Transfer {

/** A very simple subclass of UploadHandler that wraps around the HTTPRequest
 * curl interface. It also provides a subclass of NameUploadHandler that reads
 * the fingerprint URI from a HTTP uri.
 * */
class HTTPUploadHandler :
			public UploadHandler,
			public NameUploadHandler,
			public std::tr1::enable_shared_from_this<HTTPUploadHandler>
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

	static void finishedCallback(
			DownloadHandler::Callback callback,
			HTTPRequest* httpreq,
			const DenseDataPtr &recvData,
			bool success) {
		callback(success);
	}

public:
	/** Simple wrapper around HTTPRequest to download a URL.
	 * The returned TransferDataPtr contains a shared reference to the HTTPRequest.
	 * If you hold onto it, you can abort the download. */
	virtual void upload(TransferDataPtr *ptrRef,
			const URI &uri,
			const DenseDataPtr &uploadData,
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
			*ptrRef = TransferDataPtr(
				new HTTPTransferData<DownloadHandler>(shared_from_this(), req));
		}

		req->go(req);
	}

	/** Provides a simple nameLookup service using HTTP. It expects a request
	 * that responds with a URI string and nothing else in the body (do not use
	 * a 302 HTTP code to do this, as curl will silently redirect).
	 * Also strips ASCII spaces/newlines from the body.
	 */
	virtual void uploadName(const URI &uri, const RemoteFileId &uploadId, const NameLookupHandler::Callback &cb) {
		HTTPRequestPtr req (new HTTPRequest(uri, Range(true)));


		req->setCallback(
			std::tr1::bind(&HTTPUploadHandler::finishedCallback, cb, _1, _2, _3));
		req->go(req);
	}
};


}
}

#endif /* SIRIKATA_HTTPUploadHandler_HPP__ */
