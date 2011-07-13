// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_MESH_VISUAL_HPP_
#define _SIRIKATA_MESH_VISUAL_HPP_

#include <sirikata/mesh/Platform.hpp>
#include <sirikata/core/util/Sha256.hpp>
#include <stack>

namespace Sirikata {
namespace Mesh {

class SIRIKATA_MESH_EXPORT Visual {
  public:
    virtual ~Visual();

    String uri;
    SHA256 hash;
};

typedef std::tr1::shared_ptr<Visual> VisualPtr;
typedef std::tr1::weak_ptr<Visual> VisualWPtr;


} // namespace Mesh
} // namespace Sirikata

#endif //_SIRIKATA_MESH_VISUAL_HPP_
