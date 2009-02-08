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


class NameLookupManager {
	ServiceLookup *mServiceLookup;
	ProtocolRegistry<NameLookupHandler> *mHandlers;

public:
	typedef std::tr1::function<void(const RemoteFileId *fingerprint)> Callback;

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
	virtual void addToCache(const URI &origNamedUri, const RemoteFileId &toFetch) {
	}

	virtual void unserialize() {
	}
	virtual void serialize() {
	}

public:
	NameLookupManager(ServiceLookup *serviceLookup, ProtocolRegistry<NameLookupHandler> *nameProtocols)
		: mServiceLookup(serviceLookup), mHandlers(nameProtocols) {
	}

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

}}
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
namespace Sirikata {
namespace Transfer {


class CachedNameLookupManager : public NameLookupManager {
	typedef std::map<URI, RemoteFileId > NameMap;
	NameMap mLookupCache;
	boost::shared_mutex mMut;

protected:
	virtual void addToCache(const URI &origNamedUri, const RemoteFileId &toFetch) {
		boost::unique_lock<boost::shared_mutex> updatecache(mMut);
		mLookupCache.insert(NameMap::value_type(origNamedUri, toFetch));
	}

	virtual void unserialize() {
		// ...
	}
	virtual void serialize() {
		// ...
	}

public:
	CachedNameLookupManager(ServiceLookup *serviceLookup, ProtocolRegistry<NameLookupHandler> *nameProtocols)
		: NameLookupManager(serviceLookup, nameProtocols) {
	}

	virtual void lookupHash(const URI &namedUri, const Callback &cb) {
		{
			boost::shared_lock<boost::shared_mutex> lookuplock(mMut);
			NameMap::const_iterator iter = mLookupCache.find(namedUri);
			if (iter != mLookupCache.end()) {
				RemoteFileId rfid ((*iter).second); // copy, because the map could change.
				lookuplock.unlock();

				cb(&rfid);
				return;
			}
		}
		NameLookupManager::lookupHash(namedUri, cb);
	}
};

}
}

#endif /* SIRIKATA_NameLookup_HPP__ */
