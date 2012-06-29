// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPINTOLOC_TIME_SYNCED_HPP_
#define _SIRIKATA_LIBPINTOLOC_TIME_SYNCED_HPP_

#include <sirikata/pintoloc/Platform.hpp>

namespace Sirikata {

/** TimeSynced can translate times between local and a remote server. */
class SIRIKATA_LIBPINTOLOC_EXPORT TimeSynced {
public:
    virtual ~TimeSynced() {}

    virtual Time localTime(const Time& t) const = 0;
    virtual Time remoteTime(const Time& t) const = 0;
}; // class TimeSynced


/** Trivial implementation of TimeSynced which does no translation -- either
 *  both components are on the same node or they are already synced in some
 *  other way.
 */
class SIRIKATA_LIBPINTOLOC_EXPORT NopTimeSynced : public TimeSynced {
public:
    virtual ~NopTimeSynced() {}

    virtual Time localTime(const Time& t) const { return t; }
    virtual Time remoteTime(const Time& t) const { return t; }
}; // class TimeSynced

} // namespace Sirikata

#endif //_SIRIKATA_LIBPINTOLOC_TIME_SYNCED_HPP_
