// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_OHDP_SST_DECLS_HPP_
#define _SIRIKATA_LIBCORE_OHDP_SST_DECLS_HPP_

#include <sirikata/core/network/SSTDecls.hpp>
#include <sirikata/core/ohdp/Defs.hpp>

namespace Sirikata {

// Convenience typedefs in a separate namespace
namespace OHDPSST {
typedef Sirikata::SST::EndPoint<OHDP::SpaceNodeID> Endpoint;

typedef Sirikata::SST::BaseDatagramLayer<OHDP::SpaceNodeID> BaseDatagramLayer;
typedef std::tr1::shared_ptr<BaseDatagramLayer> BaseDatagramLayerPtr;
typedef std::tr1::weak_ptr<BaseDatagramLayer> BaseDatagramLayerWPtr;

typedef Sirikata::SST::Connection<OHDP::SpaceNodeID> Connection;
typedef std::tr1::shared_ptr<Connection> ConnectionPtr;
typedef std::tr1::weak_ptr<Connection> ConnectionWPtr;

typedef Sirikata::SST::Stream<OHDP::SpaceNodeID> Stream;
typedef std::tr1::shared_ptr<Stream> StreamPtr;
typedef std::tr1::weak_ptr<Stream> StreamWPtr;

typedef Sirikata::SST::ConnectionManager<OHDP::SpaceNodeID> ConnectionManager;

} // namespace OHDPSST

} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_OHDP_SST_DECLS_HPP_
