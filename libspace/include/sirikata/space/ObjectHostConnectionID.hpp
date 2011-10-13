// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBSPACE_OBJECT_HOST_CONNECTION_ID_HPP_
#define _SIRIKATA_LIBSPACE_OBJECT_HOST_CONNECTION_ID_HPP_

#include <sirikata/space/Platform.hpp>

namespace Sirikata {

class ObjectHostConnection;
class ObjectHostConnectionManager;

// Short version of ObjectHostConnectionID. This requires a map lookup to
// actually send data, but is useful as a non-opaque integral type for use as an
// enpoint identifier in protocols.
typedef uint32 ShortObjectHostConnectionID;

/** Unique identifier for an object host connected to this space server. It is
 *  only valid on this server, is opaque, and *will* change, even if the same
 *  object host connects again.
 *
 *  Implementation note: this is an opaque wrapper around an
 *  ObjectHostConnection*. This provides zero overhead "lookup" of the
 *  ObjectHostConnection* for sending data but doesn't expose internal
 *  implementation.
 */
class SIRIKATA_SPACE_EXPORT ObjectHostConnectionID {
public:
    ObjectHostConnectionID();
    ObjectHostConnectionID(const ObjectHostConnectionID& rhs);
    ObjectHostConnectionID& operator=(const ObjectHostConnectionID& rhs);

    bool operator==(const ObjectHostConnectionID& rhs) const;
    bool operator!=(const ObjectHostConnectionID& rhs) const;

    // Implementation in ObjectHostConnectionManager.cpp
    ShortObjectHostConnectionID shortID() const;
private:
    friend class ObjectHostConnectionManager;

    ObjectHostConnectionID(ObjectHostConnection* _conn);

    ObjectHostConnection* conn;
};

} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_OBJECT_HOST_CONNECTION_ID_HPP_
