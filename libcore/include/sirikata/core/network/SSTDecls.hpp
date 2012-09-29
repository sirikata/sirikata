// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_NETWORK_SST_DECLS_HPP_
#define _SIRIKATA_LIBCORE_NETWORK_SST_DECLS_HPP_

#include <sirikata/core/util/Platform.hpp>

// Forward declarations and typedefs for SST, e.g. for headers that don't
// actually need the definitions.

namespace Sirikata {
namespace SST {

template <typename EndObjectType>
class EndPoint;

template <class EndPointType>
class SIRIKATA_EXPORT Connection;

template <class EndPointType>
class SIRIKATA_EXPORT Stream;

template <typename EndPointType>
class SIRIKATA_EXPORT BaseDatagramLayer;

template <typename EndPointType>
class ConnectionManager;

template <class EndPointType>
class ConnectionVariables;

} // namespace SST
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_NETWORK_SST_DECLS_HPP_
