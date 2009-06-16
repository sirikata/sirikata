/*  Sirikata Transfer -- Content Distribution Network
 *  CachedNameLookupManager.hpp
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

#ifndef SIRIKATA_CachedNameLookupManager_HPP__
#define SIRIKATA_CachedNameLookupManager_HPP__

#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>

#include "NameLookupManager.hpp"

namespace Sirikata {
namespace Transfer {

/** A subclass of NameLookupManager to handle caching of name lookup requests.
 * Note that this also intends to handle serializing to disk for offline access
 * to a space, However this part is not written yet. Perhaps it is more
 * appropriately handled by a subclass. */
class CachedNameLookupManager : public NameLookupManager {
	typedef std::map<URI, RemoteFileId > NameMap;
	NameMap mLookupCache;
	boost::shared_mutex mMut;

protected:
	/// Unimplemented.
	virtual void unserialize() {
		// ...
	}
	/// Unimplemented.
	virtual void serialize() {
		// ...
	}

public:
	CachedNameLookupManager(ServiceManager<NameLookupHandler> *nameProtocols, ServiceManager<DownloadHandler> *downloadServ=NULL)
		: NameLookupManager(nameProtocols, downloadServ) {
	}

	virtual void removeFromCache(const URI &origNamedUri) {
		boost::unique_lock<boost::shared_mutex> updatecache(mMut);
        NameMap::iterator iter = mLookupCache.find(origNamedUri);
        if (iter != mLookupCache.end()) {
            mLookupCache.erase(iter);
        }
	}

	virtual void addToCache(const URI &origNamedUri, const RemoteFileId &toFetch) {
		boost::unique_lock<boost::shared_mutex> updatecache(mMut);
		mLookupCache.insert(NameMap::value_type(origNamedUri, toFetch));
	}

	virtual void lookupHash(const URI &namedUri, const Callback &cb) {
		{
			boost::shared_lock<boost::shared_mutex> lookuplock(mMut);
			NameMap::const_iterator iter = mLookupCache.find(namedUri);
			if (iter != mLookupCache.end()) {
				RemoteFileId rfid ((*iter).second); // copy, because the map could change.
				lookuplock.unlock();

				cb(namedUri, &rfid);
				return;
			}
		}
		NameLookupManager::lookupHash(namedUri, cb);
	}
};

}
}

#endif /* SIRIKATA_CachedNameLookupManager_HPP__ */
