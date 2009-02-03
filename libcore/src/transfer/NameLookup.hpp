/*  Sirikata Transfer -- Content Transfer management system
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
 *  * Neither the name of Iridium nor the names of its contributors may
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

namespace Sirikata {
namespace Transfer {


class ServiceLookup {
public:
	typedef std::tr1::function1<void, const std::vector<URIContext> &> Callback;
private:
	ServiceLookup *mNext;

private:
	virtual void addToCache(const std::vector<URIContext> &toCache) {
		//
	}

public:

	ServiceLookup(ServiceLookup *next=NULL)
			:mNext(next) {
	}

	virtual void lookupService(const URIContext &context, const Callback &cb) {
		if (mNext) {
			mNext->lookupService(context, cb);
		}
	}
};

class OptionManagerServiceLookup : public ServiceLookup {
public:
	virtual void lookupService(const URIContext &context, const Callback &cb) {

	}
};

class CachedServiceLookup {
	typedef std::map<URIContext, std::vector<URIContext> > SerivceMap;
	ServiceMap mLookupCache;
	boost::shared_mutex mMut;

public:
	virtual void lookupService(const URIContext &context, const Callback &cb) {
		{
			boost::shared_lock<boost::shared_mutex> lookuplock(mMut);
			ServiceMap::const_iterator iter = mLookupCache.find(namedUri);
			if (iter != NameMap.end()) {
				cb(*iter);
				return;
			}
		}
	}
};


class NameLookup {
public:
	typedef std::tr1::function1<void, const RemoteFileId &> Callback;

private:
	NameLookup *mNext;

public:
	NameLookup(NameLookup *next=NULL)
			:mNext(next) {
	}

	virtual void lookupHash(const URI &namedUri, const Callback &cb) {
		if (mNext) {
			mNext->lookupHash(namedUri, cb);
		}
	}
};

class CachedNameLookup : public NameLookup {
	typedef std::map<URI, RemoteFileId > NameMap;
	NameMap mLookupCache;
	boost::shared_mutex mMut;
public:
	void unserialize() {
		// ...
	}
	void serialize() {
		// ...
	}
	virtual void lookupHash(const URI &namedUri, const Callback &cb) {
		{
			boost::shared_lock<boost::shared_mutex> lookuplock(mMut);
			NameMap::const_iterator iter = mLookupCache.find(namedUri);
			if (iter != NameMap.end()) {
				RemoteFileId rfid = (*iter).second; // copy, because the map could change.
				lookuplock.unlock();

				cb(rfid);
				return;
			}
		}
		NameLookup::lookup(namedUri, cb);
	}
};

}
}

#endif /* SIRIKATA_NameLookup_HPP__ */
