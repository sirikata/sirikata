/*  Sirikata Transfer -- Content Distribution Network
 *  UploadHandler.hpp
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

#ifndef SIRIKATA_UploadHandler_HPP__
#define SIRIKATA_UploadHandler_HPP__

#include "ProtocolRegistry.hpp"

namespace Sirikata {
namespace Transfer {

/** A protocol handler for downloading files. Examples of implementations may
 * include HTTP, BitTorrent, SVN, S3 ... Currently only Curl (HTTP,FTP) is implemented. */
class UploadHandler {
public:
	/** Hold onto this if you want to be able to abort the transfer.
	 * Otherwise, it is safe to discard this. Note that nothing will be
	 * free'd as long as you hold a reference to the TransferData. */
	typedef ProtocolData<UploadHandler>::Ptr TransferDataPtr;

	/**
	 * Virtual destructor. Note that the destructor should not be called until
	 * all active connections have ended, since TransferData contains a reference
	 * to this, and any active download may hold onto the TransferData.
	 *
	 * If you need to hold a reference to a  TransferData, make sure its mParent
	 * is set to NULL so as to avoid a circular reference.
	 */
	virtual ~UploadHandler() {
	}

	/** Called upon completion. Note that stream() will call this once per packet.
	 *
	 * @param success is set to false on failure (when data is invalid).
	 */
	typedef std::tr1::function<void(bool success)> Callback;

	/** Downloads the given range of a file, and calls cb(data, success) upon
	 * completion or failure.
	 *
	 * @param uri      The entire URI to download (from ServiceLookup).
	 * @param bytes    What range to download. Currently this does not support
	 *                 multiple byteranges in one request.
	 * @param cb       The callback to be called when the download has completed.
	 *                 FIXME: 'data' may be non-null even if 'success' is false.
	 */
	virtual void upload(TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const DenseDataPtr &contents,
			const Callback &cb) = 0;

	/** Deletes a file.
	 * @see upload
	 */
	virtual void remove(TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const Callback &cb) = 0;
};
typedef std::tr1::shared_ptr<UploadHandler> UploadHandlerPtr;

/** A protocol handler for converting a filename to a hash, so that it can be
 * downloaded and cached. Currently, the only implementation is HTTP, but this
 * could include a custom DNS-like protocol that runs on top of UDP.*/
class NameUploadHandler {
public:
	typedef ProtocolData<NameUploadHandler>::Ptr TransferDataPtr;

	/** Passes a fingerprint as well as an unresolved URI you can use to download this.
	 *
	 * @param success   If the name lookup succeeded.
	 */
	typedef std::tr1::function<void(bool success)> Callback;

	virtual ~NameUploadHandler() {
	}

	/** Performs a name lookup using this method, and calls cb whether it succeeded or not. */
	virtual void uploadName(TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const RemoteFileId &upload,
			const Callback &cb) = 0;

	/** Performs a name deletion using this method, and calls cb whether it succeeded or not. */
	virtual void removeName(TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const Callback &cb) = 0;
};
typedef std::tr1::shared_ptr<NameUploadHandler> NameUploadHandlerPtr;


}
}

#endif /* SIRIKATA_UploadHandler_HPP__ */
