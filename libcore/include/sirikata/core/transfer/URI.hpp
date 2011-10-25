// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef SIRIKATA_CORE_TRANSFER_URI_HPP__
#define SIRIKATA_CORE_TRANSFER_URI_HPP__

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {
namespace Transfer {

/** A URI is very generic: it is only assumed to have a scheme, a colon, and a
 *  scheme specific part. This class is mainly for passing around the raw URI
 *  and providing minimal support for converting it into more specific versions,
 *  e.g. URLs, data URIs, etc.
 */
class URI {
    String mURI;
    String mScheme; // Derived from mURI and cached.

public:
    explicit URI() {
    }

    /** Construct a URI from a String. If the URI cannot be parsed (i.e. we
     *  cannot extract a valid scheme), the URI remains empty.
     */
    explicit URI(const String& uri)
     : mURI(uri)
    {
        // Find the first colon
        String::size_type colon_pos = mURI.find(':');
        if (colon_pos == String::npos) {
            mURI = "";
            return;
        }
        // FIXME we should validate the characters in the scheme.
        mScheme = mURI.substr(0, colon_pos);
    }

    inline const String& scheme() const {
        return mScheme;
    }

    inline String schemeSpecificPart() const {
        return mURI.substr(mScheme.size()+1);
    }

    inline const String& toString () const {
        return mURI;
    }

    inline bool operator<(const URI &other) const {
        return mURI < other.mURI;
    }

    inline bool operator==(const URI &other) const {
        return mURI == other.mURI;
    }

    inline bool operator!=(const URI &other) const {
        return mURI != other.mURI;
    }

    bool empty() const {
        return mURI.empty();
    }

    operator bool() const {
        return !empty();
    }

    struct Hasher {
        size_t operator() (const URI& uri)const {
            return std::tr1::hash<std::string>()(uri.mURI);
        }
    };
};

inline std::ostream &operator<<(std::ostream &str, const URI &uri) {
    return str << uri.toString();
}

} // namespace Transfer
} // namespace Sirikata

#endif /* SIRIKATA_CORE_TRANSFER_URI_HPP__ */
