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

/** A protocol handler for downloading files. Examples of implementations may
 * include HTTP, BitTorrent, SVN, S3 ... Currently only Curl (HTTP,FTP) is implemented. */
class DownloadHandler : public std::tr1::enable_shared_from_this<DownloadHandler> {
public:
	/** A handle to an active transfer. Currently its only use is to call abort().
	 * Since this contains a shared_ptr to the transfer, it may be necessary */
	class TransferData {
		std::tr1::shared_ptr<DownloadHandler> mParent;
	public:
		const std::tr1::shared_ptr<DownloadHandler> &getParent() {
			return mParent;
		}
		TransferData(const std::tr1::shared_ptr<DownloadHandler> &parent)
			:mParent(parent) {
		}
		virtual ~TransferData() {}

		virtual void abort() {}
	};
	/** Hold onto this if you want to be able to abort the transfer.
	 * Otherwise, it is safe to discard this. Note that nothing will be
	 * free'd as long as you hold a reference to the TransferData. */
	typedef std::tr1::shared_ptr<TransferData> TransferDataPtr;

	/**
	 * Virtual destructor. Note that the destructor should not be called until
	 * all active connections have ended, since TransferData contains a reference
	 * to this, and any active download may hold onto the TransferData.
	 *
	 * If you need to hold a reference to a  TransferData, make sure its mParent
	 * is set to NULL so as to avoid a circular reference.
	 */
	virtual ~DownloadHandler() {
	}

	/** Called upon completion. Note that stream() will call this once per packet.
	 *
	 * @param data      The a shared_ptr<DenseData> containing the requested data.
	 * @param           success is set to false on failure (when data is invalid).
	 */
	typedef std::tr1::function<void(DenseDataPtr data, bool success)> Callback;

	/** Downloads the given range of a file, and calls cb(data, success) upon
	 * completion or failure.
	 *
	 * @param uri      The entire URI to download (from ServiceLookup).
	 * @param bytes    What range to download. Currently this does not support
	 *                 multiple byteranges in one request.
	 * @param cb       The callback to be called when the download has completed.
	 *                 FIXME: 'data' may be non-null even if 'success' is false.
	 */
	virtual void download(TransferDataPtr *ptrRef, const URI &uri, const Range &bytes, const Callback &cb) {
		cb(DenseDataPtr(), false);
	}

	/** Downloads the given range of a file, and calls streamCB for each packet
	 * received. [Currently not implemented.]
	 *
	 * NOTE: In protocols such as bittorrent, the callback may be called with
	 * out-of-order data. This may not make sense for some applications.
	 * @see inOrderStream()
	 *
	 * @param uri      The entire URI to download (from ServiceLookup).
	 * @param bytes    What range to download. Currently this does not support
	 *                 multiple byteranges in one request.
	 * @param streamCB The callback to be called for each packet. If 'success' is
	 *                 false, it indicates either EOF or failed connection.
	 */
	virtual void stream(TransferDataPtr *ptrRef, const URI &uri, const Range &bytes, const Callback streamCB) {
		streamCB(DenseDataPtr(), false);
	}

	/** Returns true if stream() will receive data in order (such as in HTTP or FTP)
	 * FIXME: What about transfers that can lose packets (UDP video streaming)?
	 * Are they best handled by a different interface? Or should that be a boolean passed somewhere?
	 */
	virtual bool inOrderStream() const {
		return false;
	}
};

/** A protocol handler for converting a filename to a hash, so that it can be
 * downloaded and cached. Currently, the only implementation is HTTP, but this
 * could include a custom DNS-like protocol that runs on top of UDP.*/
class NameLookupHandler {
public:
	/** Passes a fingerprint as well as an unresolved URI you can use to download this.
	 *
	 * @param hash      The fingerprint of the resolved file.
	 * @param uriString The URI to download, to be applied to the original URIContext.
	 * @param success   If the name lookup succeeded.
	 */
	typedef std::tr1::function<void(const Fingerprint& hash, const std::string& uriString, bool success)> Callback;

	virtual ~NameLookupHandler() {
	}

	/** Performs a name lookup using this method, and calls cb whether it succeeded or not. */
	virtual void nameLookup(const URI &uri, const Callback &cb) {
		cb(Fingerprint(), std::string(), false);
	}
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
	std::map<std::string, std::tr1::shared_ptr<ProtocolType> > mHandlers;
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
	void setHandler(const std::string &proto, std::tr1::shared_ptr<ProtocolType> handler) {
		mHandlers[proto] = handler;
	}

	/** Removes a registered protocol handler. */
	void removeHandler(const std::string &proto) {
		mHandlers.remove(proto);
	}

	/** Looks up proto in the map. Returns NULL if no protocol handler was found. */
	std::tr1::shared_ptr<ProtocolType> lookup(const std::string &proto) {
		std::tr1::shared_ptr<ProtocolType> protoHandler (mHandlers[proto]);
		if (!protoHandler) {
			std::cerr << "No protocol handler registered for "<<proto<<std::endl;
		}
		return protoHandler;
	}

	void lookupService(const URIContext &context, const ServiceLookup::Callback &cb, bool allowProto=true) {
		if (allowProto && mAllowNonService &&
				mHandlers.find(context.proto()) != mHandlers.end()) {
			ListOfServicesPtr services(new ListOfServices);
			services->push_back(context);
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
