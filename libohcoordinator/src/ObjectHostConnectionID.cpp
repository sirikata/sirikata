// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/ohcoordinator/ObjectHostConnectionID.hpp>
#include <sirikata/ohcoordinator/ObjectHostConnectionManager.hpp>

namespace Sirikata {

ObjectHostConnectionID::ObjectHostConnectionID()
        : conn(NULL)
{
}

ObjectHostConnectionID::ObjectHostConnectionID(ObjectHostConnection* _conn)
        : conn(_conn)
{
}

ObjectHostConnectionID::ObjectHostConnectionID(const ObjectHostConnectionID& rhs)
        : conn(rhs.conn)
{
}

ObjectHostConnectionID& ObjectHostConnectionID::operator=(const ObjectHostConnectionID& rhs) {
    conn = rhs.conn;
    return *this;
}

bool ObjectHostConnectionID::operator==(const ObjectHostConnectionID& rhs) const {
    return (conn == rhs.conn);
}

bool ObjectHostConnectionID::operator!=(const ObjectHostConnectionID& rhs) const {
    return (conn != rhs.conn);
}

ShortObjectHostConnectionID ObjectHostConnectionID::shortID() const {
    return conn->short_id;
}

} // namespace Sirikata
