// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef SIRIKATA_CORE_TRANSFER_DATA_URI_HPP__
#define SIRIKATA_CORE_TRANSFER_DATA_URI_HPP__

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/transfer/URI.hpp>

namespace Sirikata {
namespace Transfer {

/** Data URIs encode data directly instead of referring to an external
 *  resource. Format: data:[<mediatype>][;base64],<data>.
 *
 *  NOTE: We currently only support base64 encoded URIs.
 */
class SIRIKATA_EXPORT DataURI {
public:
    typedef std::map<String, String> ParameterMap;

private:
    String mMediaType;
    ParameterMap mMediaTypeParameters;
    String mData;

    void parse(const String& uri);
public:
    explicit DataURI() {
    }

    /** Construct a data URI from a String. If the URI cannot be parsed (i.e. we
     *  cannot extract a valid scheme), the URI remains empty.
     */
    explicit DataURI(const String& uri);

    /** Construct a data URI from a String. If the URI cannot be parsed (i.e. we
     *  cannot extract a valid scheme), the URI remains empty.
     */
    explicit DataURI(const URI& uri);

    inline const String& mediatype() const {
        return mMediaType;
    }

    inline const ParameterMap& mediatypeParameters() const {
        return mMediaTypeParameters;
    }

    inline const String& data() const {
        return mData;
    }

    String toString() const;

    inline bool operator<(const DataURI &other) const {
        return (mMediaType < other.mMediaType ||
            (mMediaType < other.mMediaType && mData < other.mData));
    }

    inline bool operator==(const DataURI &other) const {
        return (mMediaType == other.mMediaType && mData < other.mData);
    }

    inline bool operator!=(const DataURI &other) const {
        return (mMediaType != other.mMediaType || mData != other.mData);
    }

    bool empty() const {
        return mData.empty();
    }

    operator bool() const {
        return !empty();
    }

    struct Hasher {
        size_t operator() (const DataURI& uri)const {
            return std::tr1::hash<std::string>()(uri.mMediaType);
        }
    };
};

inline std::ostream &operator<<(std::ostream &str, const DataURI &uri) {
    return str << uri.toString();
}

} // namespace Transfer
} // namespace Sirikata

#endif /* SIRIKATA_CORE_TRANSFER_URI_HPP__ */
