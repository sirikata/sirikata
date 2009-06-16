/*  Sirikata Transfer -- Content Distribution Network
 *  DownloadHandler.hpp
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

#ifndef SIRIKATA_DownloadHandler_HPP__
#define SIRIKATA_DownloadHandler_HPP__

#include "ProtocolRegistry.hpp"

namespace Sirikata {
namespace Transfer {

/** A protocol handler for downloading files. Examples of implementations may
 * include HTTP, BitTorrent, SVN, S3 ... Currently only Curl (HTTP,FTP) is implemented. */
class DownloadHandler {
public:
	typedef ProtocolData<DownloadHandler>::Ptr TransferDataPtr;

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
	 * @param success   is set to false on failure (when data is invalid).
	 * @param fileSize  The total size of the file, if available. Otherwise, 0.
	 */
	typedef std::tr1::function<void(DenseDataPtr data, bool success, cache_usize_type fileSize)> Callback;

	/** Downloads the given range of a file, and calls cb(data, success) upon
	 * completion or failure.
	 *
	 * @param uri      The entire URI to download (from ServiceLookup).
	 * @param bytes    What range to download. Currently this does not support
	 *                 multiple byteranges in one request.
	 * @param cb       The callback to be called when the download has completed.
	 *                 FIXME: 'data' may be non-null even if 'success' is false.
	 */
	virtual void download(TransferDataPtr *ptrRef, const URI &uri, const Range &bytes, const Callback &cb) = 0;
	/*{
		cb(DenseDataPtr(), false);
	}*/

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
	virtual void stream(TransferDataPtr *ptrRef, const URI &uri, const Range &bytes, const Callback &streamCB) = 0;
	/* {
		streamCB(DenseDataPtr(), false);
	} */

	/** Checks the existence of a file. The DenseData parameter of callback should be ignored
	 * and may be NULL.  The default version calls download with an empty range.
	 */
	virtual void exists(TransferDataPtr *ptrRef, const URI &uri, const Callback &cb) {
		download(ptrRef, uri, Range(false), cb);
	}

	/** Returns true if stream() will receive data in order (such as in HTTP or FTP)
	 * FIXME: What about transfers that can lose packets (UDP video streaming)?
	 * Are they best handled by a different interface? Or should that be a boolean passed somewhere?
	 */
	virtual bool inOrderStream() const {
		return false;
	}
};
typedef std::tr1::shared_ptr<DownloadHandler> DownloadHandlerPtr;

/** A protocol handler for converting a filename to a hash, so that it can be
 * downloaded and cached. Currently, the only implementation is HTTP, but this
 * could include a custom DNS-like protocol that runs on top of UDP.*/
class NameLookupHandler {
public:
	typedef ProtocolData<NameLookupHandler>::Ptr TransferDataPtr;

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
	virtual void nameLookup(TransferDataPtr *ptrRef, const URI &uri, const Callback &cb) {
		cb(Fingerprint(), std::string(), false);
	}
};
typedef std::tr1::shared_ptr<NameLookupHandler> NameLookupHandlerPtr;


}
}

#endif /* SIRIKATA_DownloadHandler_HPP__ */
