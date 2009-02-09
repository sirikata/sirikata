/*  Sirikata Transfer -- Content Distribution Network
 *  NameLookup.hpp
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

namespace Sirikata {
namespace Transfer {


/** The NameLookupManager handles the conversion from a named URI (filename) to
 * a hash and a URI to download, possibly from a "cloud". Note that NameLookupHandler
 * is generally a simple UDP-based protocol intended for small transfers. */
class NameLookupManager {
	ServiceLookup *mServiceLookup;
	ProtocolRegistry<NameLookupHandler> *mHandlers;

public:
	/** Called with a temporary pointer to a fingerprint, or NULL if the lookup failed. */
	typedef std::tr1::function<void(const RemoteFileId *fingerprint)> Callback;

private:
	void gotNameLookup(Callback cb, URI origNamedUri, unsigned int which, ListOfServicesPtr services,
			const Fingerprint &hash, const std::string &str, bool success) {
		if (!success) {
			doNameLookup(cb, origNamedUri, which+1, services);
			return;
		}

		RemoteFileId rfid(hash, URI(origNamedUri.context(), str));
		addToCache(origNamedUri, rfid);
		cb(&rfid);
	}

	void doNameLookup(Callback cb, URI origNamedUri, unsigned int which, ListOfServicesPtr services) {
		if (which >= services->size()) {
			std::cerr << "None of the " << which << " URIContexts registered for " <<
					origNamedUri << " were successful for NameLookup." << std::endl;
			cb(NULL);
			return;
		}
		URI lookupUri ((*services)[which], origNamedUri.filename());
		std::tr1::shared_ptr<NameLookupHandler> handler = mHandlers->lookup(lookupUri.proto());
		if (handler) {
			handler->nameLookup(lookupUri,
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
	 * @param serviceLookup  If NULL, no service lookup will be done, and you are expected to
	 *                       pass in resolved URIs into lookupHash().
	 * @param nameProtocols  The NameLookupHandler protocol registry to be used.
	 */
	NameLookupManager(ServiceLookup *serviceLookup, ProtocolRegistry<NameLookupHandler> *nameProtocols)
			: mServiceLookup(serviceLookup), mHandlers(nameProtocols) {
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
		if (mServiceLookup) {
			mServiceLookup->lookupService(namedUri.context(), std::tr1::bind(&NameLookupManager::doNameLookup, this, cb, namedUri, 0, _1));
		} else {
			ListOfServicesPtr services(new ListOfServices);
			services->push_back(namedUri.context());
			doNameLookup(cb, namedUri, 0, services);
		}
	}
};

}
}

#endif /* SIRIKATA_NameLookup_HPP__ */
