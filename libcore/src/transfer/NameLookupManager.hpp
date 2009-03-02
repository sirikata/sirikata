/*  Sirikata Transfer -- Content Distribution Network
 *  NameLookupManager.hpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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
/*  Created on: Jan 31, 2009 */

#ifndef SIRIKATA_NameLookup_HPP__
#define SIRIKATA_NameLookup_HPP__

#include "URI.hpp"
#include "ServiceLookup.hpp"
#include "DownloadHandler.hpp"

namespace Sirikata {
namespace Transfer {


/** The NameLookupManager handles the conversion from a named URI (filename) to
 * a hash and a URI to download, possibly from a "cloud". Note that NameLookupHandler
 * is generally a simple UDP-based protocol intended for small transfers. */
class NameLookupManager {
	ProtocolRegistry<NameLookupHandler> *mHandlers;

	/* NOTE: Is it acceptable to "trust" the client to have the correct hash here?
	 * If, for example, an attacker downloads "http://myrogueserver.com/0123456789somevalidhashstring"
	 * then conceivably that rogue data could be cached under any hash, and then any legitimate requests
	 * will get the rogue data.
	 */
	ProtocolRegistry<DownloadHandler> *mDownloadProto; // check if someone referenced a file by hash direct

public:
	/** Called with a temporary pointer to a fingerprint, or NULL if the lookup failed. */
	typedef std::tr1::function<void(const RemoteFileId *fingerprint)> Callback;

private:
	void gotNameLookup(const Callback &cb, const URI &origNamedUri, unsigned int which, ListOfServicesPtr services,
			const Fingerprint &hash, const std::string &str, bool success) {
		if (!success) {
			doNameLookup(cb, origNamedUri, which+1, services);
			return;
		}

		RemoteFileId rfid(hash, URI(origNamedUri.context(), str));
		addToCache(origNamedUri, rfid);
		cb(&rfid);
	}

	void hashedDownload(const Callback &cb, const URI &origNamedUri, ListOfServicesPtr services) {
		if (services && services->size() > 0) {
			// It is safe to cast this URI to a RemoteFileId????
			RemoteFileId rfid(origNamedUri);
			cb(&rfid);
		} else {
			cb(NULL);
		}
	}

	void doNameLookup(const Callback &cb, const URI &origNamedUri, unsigned int which, ListOfServicesPtr services) {
		if (!services || which >= services->size()) {
			SILOG(transfer,error,"None of the " << which << " URIContexts registered for " <<
					origNamedUri << " were successful for NameLookup.");
			/// Hashed download.
			if (mDownloadProto) {
				mDownloadProto->lookupService(origNamedUri.context(), std::tr1::bind(&NameLookupManager::hashedDownload, this, cb, origNamedUri, _1));
			} else {
				cb(NULL);
			}
			return;
		}
		URI lookupUri ((*services)[which].first, origNamedUri.filename());



		std::tr1::shared_ptr<NameLookupHandler> handler;
		lookupUri.getContext().setProto(mHandlers->lookup(lookupUri.proto(), handler));
		if (handler) {
			/// FIXME: Need a way of aborting a name lookup that is taking too long.
			handler->nameLookup(NULL, lookupUri,
				std::tr1::bind(&NameLookupManager::gotNameLookup, this, cb, origNamedUri, which, services, _1, _2, _3));
		} else {
			doNameLookup(cb, origNamedUri, which+1, services);
		}
	}

protected:
	/** To be overridden for any child class that intends to cache name lookups.
	 * Note that origNamedUri is the URI from before the service lookup step. */
	virtual void addToCache(const URI &origNamedUri, const RemoteFileId &toFetch) {
	}

	/** FIXME: Should these be overridden by a subclass. */
	virtual void unserialize() {
	}
	/** Called by the virtual destructor of this class. Should also be called periodically?
	 * FIXME: Not used yet. */
	virtual void serialize() {
	}

public:
	/** NameLookupManager constructor.
	 *
	 * @param nameProtocols  The NameLookupHandler protocol registry to be used.
	 * @param downloadProto  If non-null, and a mhash://.../ URI is passed into lookupHash, the
	 *                       input uri will be cast to a RemoteFileId and returned.
	 *                       If null, lookupHash returns a NULL RemoteFileId if this is not a named URI.
	 */
	NameLookupManager(ProtocolRegistry<NameLookupHandler> *nameProtocols, ProtocolRegistry<DownloadHandler> *downloadProto=NULL)
			: mHandlers(nameProtocols), mDownloadProto(downloadProto) {
	}

	/// Virtual destructor. Calls serialize().
	virtual ~NameLookupManager() {
		serialize();
	}

	/** Takes a URI, and tries to lookup the hash and download URI from it.
	 *
	 * @param namedUri A ServiceURI or a regular URI (depending on if serviceLookup is NULL)
	 * @param cb       The Callback to be called either on success or failure. */
	void lookupHash(const URI &namedUri, const Callback &cb) {
		mHandlers->lookupService(namedUri.context(), std::tr1::bind(&NameLookupManager::doNameLookup, this, cb, namedUri, 0, _1));
	}
};

}
}

#endif /* SIRIKATA_NameLookup_HPP__ */
