// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_TRANSFER_REMOTEFILEID_HPP_
#define _SIRIKATA_CORE_TRANSFER_REMOTEFILEID_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/transfer/Defs.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/transfer/URL.hpp>

namespace Sirikata {
namespace Transfer {

/** Container for both a fingerprint and a download URI, as is most
 * commonly be used in CacheLayer for file downloads.
 */
class RemoteFileId {
	Fingerprint mHash;
	URI mURI;

public:
    RemoteFileId() {
    }

    RemoteFileId(const Fingerprint &fingerprint, const URI &uri)
     : mHash(fingerprint), mURI(uri) {
    }

    RemoteFileId(const Fingerprint &fingerprint, const URLContext &context)
     : mHash(fingerprint), mURI(URL(context, fingerprint.convertToHexString()).toString())
    {
    }

    explicit RemoteFileId(const URL &fingerprinturl)
     : mURI(fingerprinturl.toString())
    {
        const std::string &path = fingerprinturl.filename();
        mHash = Fingerprint::convertFromHex(path);
    }

	inline std::string toString() const {
                return mURI.toString();
	}

	/// accessor for the hashed value
	inline const Fingerprint &fingerprint() const {
		return mHash;
	}

	/// const accessor for the full string URI
	inline const URI &uri() const {
		return mURI;
	}
	/// accessor for the full URI
	inline URI &uri() {
		return mURI;
	}
	inline bool operator== (const RemoteFileId &other) const {
	       return uri() == other.uri();
	}
        inline bool operator!= (const RemoteFileId &other) const {
               return uri() != other.uri();
        }
        inline bool operator< (const RemoteFileId &other) const {
               return uri() < other.uri();
        }
};



} // namespace Transfer
} // namespace Sirikata

#endif //_SIRIKATA_CORE_TRANSFER_REMOTEFILEID_HPP_
