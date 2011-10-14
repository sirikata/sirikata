// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_ODP_SST_HPP_
#define _SIRIKATA_LIBCORE_ODP_SST_HPP_

#include <sirikata/core/network/SSTImpl.hpp>

namespace Sirikata {

// Convenience typedefs in a separate namespace
namespace ODPSST {
typedef Sirikata::SST::EndPoint<SpaceObjectReference> Endpoint;
typedef Sirikata::SST::BaseDatagramLayer<SpaceObjectReference> BaseDatagramLayer;
typedef Sirikata::SST::Connection<SpaceObjectReference> Connection;
typedef Sirikata::SST::Stream<SpaceObjectReference> Stream;
typedef Sirikata::SST::ConnectionManager<SpaceObjectReference> ConnectionManager;
} // namespace ODPSST

// SpaceObjectReference/ODP-specific implementation
namespace SST {
#if SIRIKATA_PLATFORM == SIRIKATA_WINDOWS
SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT BaseDatagramLayer<SpaceObjectReference>;
SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Connection<SpaceObjectReference>;
SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Stream<SpaceObjectReference>;
#endif
} // namespace SST

} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_ODP_SST_HPP_
