/*  Sirikata Transfer -- Content Distribution Network
 *  HTTPFormUploadHandler.hpp
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

#ifndef SIRIKATA_HTTPFormUploadHandler_HPP__
#define SIRIKATA_HTTPFormUploadHandler_HPP__

#include "HTTPRequest.hpp"
#include "UploadHandler.hpp"
#include "Range.hpp"
#include "URI.hpp"
#include "TransferData.hpp"

namespace Sirikata {
namespace Transfer {

/// Uses HTTP POST to upload names or files.
class HTTPFormUploadHandler :
			public UploadHandler,
			public NameUploadHandler,
			public std::tr1::enable_shared_from_this<HTTPFormUploadHandler>
{
	typedef std::tr1::function<void (bool)> Callback;

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
			const Callback &callback,
			HTTPRequest* httpreq,
			const DenseDataPtr &recvData,
			bool success) {
		callback(success);
	}

	template <class Base>
	void createRequest (HTTPRequestPtr &req,
			typename Base::TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const Callback&cb) {
		// Note: Here, the filename of the URI is the file to upload,
		// but the context is the location of the http form.
		req = HTTPRequestPtr(new HTTPRequest(URI(uri.context().toString(false)), Range(true)));
		req->setCallback(
			std::tr1::bind(&HTTPFormUploadHandler::finishedCallback, cb, _1, _2, _3));
		if (ptrRef) {
			// Must set this before calling req->go()
			*ptrRef = typename Base::TransferDataPtr(
				new HTTPTransferData<Base>(shared_from_this(), req));
		}

		for (ServiceParams::const_iterator iter = params.lower_bound("value:");
				iter != params.end() && (*iter).first.compare(0,6,"value:")==0;
				++iter) {
			req->addPOSTField((*iter).first.substr(6), (*iter).second);
		}
	}

public:
	/** Simple wrapper around HTTPRequest to upload a URL.
	 * The returned TransferDataPtr contains a shared reference to the HTTPRequest.
	 * If you hold onto it, you can abort the download. */
	virtual void upload(UploadHandler::TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const DenseDataPtr &uploadData,
			const UploadHandler::Callback &cb) {
		//graphics.stanford.edu/~danielrh/uploadsystem/
		//    ?value:uploadNeed=1&field:hash=MhashFile0&field:file=uploadFile0&
		//     field:user=author&field:password=password&auth:type=plaintext
		HTTPRequestPtr req;
		createRequest<UploadHandler> (req, ptrRef, params,
				uri, cb);
		req->addPOSTData(params["field:file"], uri.filename(), uploadData);
		//req->addPOSTField(params["field:hash"])
		if (!params["field:filename"].empty()) {
			req->addPOSTField(params["field:filename"], uri.filename());
		}
		if (!params["field:insert"].empty()) {
			req->addPOSTField(params["field:insert"],"on"); // checkbox
		}
		req->go(req);
	}

	virtual void remove(UploadHandler::TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const UploadHandler::Callback &cb) {
		HTTPRequestPtr req;
		createRequest<UploadHandler> (req, ptrRef, params,
				uri, cb);
		if (params["field:filename"].empty()) {
			req->addPOSTData(params["field:file"], uri.filename(),
					DenseDataPtr(new DenseData(Range(false))));
		} else {
			req->addPOSTField(params["field:filename"], uri.filename());
		}
		if (!params["field:remove"].empty()) {
			req->addPOSTField(params["field:remove"],"on"); // checkbox
		}
		req->go(req);
	}

	/** Provides a simple nameLookup service using HTTP. It expects a request
	 * that responds with a URI string and nothing else in the body (do not use
	 * a 302 HTTP code to do this, as curl will silently redirect).
	 * Also strips ASCII spaces/newlines from the body.
	 */
	virtual void uploadName(NameUploadHandler::TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const RemoteFileId &uploadId,
			const NameUploadHandler::Callback &cb) {
		HTTPRequestPtr req;
		createRequest<NameUploadHandler> (req, ptrRef, params,
				uri, cb);
		if (params["field:filename"].empty()) {
			req->addPOSTData(params["field:file"], uri.filename(),
					DenseDataPtr(new DenseData(uploadId.uri().toString())));
		} else {
			req->addPOSTField(params["field:file"], uploadId.uri().toString());
			req->addPOSTField(params["field:filename"], uri.filename());
		}
		if (!params["field:insert"].empty()) {
			req->addPOSTField(params["field:insert"],"on"); // checkbox
		}
		req->go(req);
	}

	virtual void removeName(NameUploadHandler::TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const NameUploadHandler::Callback &cb) {
		HTTPRequestPtr req;
		createRequest<NameUploadHandler> (req, ptrRef, params,
				uri, cb);
		if (params["field:filename"].empty()) {
			req->addPOSTData(params["field:file"], uri.filename(),
					DenseDataPtr(new DenseData(Range(false))));
		} else {
			req->addPOSTField(params["field:filename"], uri.filename());
		}
		if (!params["field:remove"].empty()) {
			req->addPOSTField(params["field:remove"],"on"); // checkbox
		}
		req->go(req);
	}
};


}
}

#endif /* SIRIKATA_HTTPFormUploadHandler_HPP__ */
