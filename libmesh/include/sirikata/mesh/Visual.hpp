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

/** Base class for visual objects that are passed through the system. Many types
 *  of objects may be used -- regular meshes, billboards, particle systems,
 *  depth+color images, etc -- so this only contains the most basic, common
 *  information.  All types are defined in libmesh, but different loaders and
 *  filters can be provided through plugins.
 */
class SIRIKATA_MESH_EXPORT Visual {
  public:
    virtual ~Visual();

    virtual const String& type() const = 0;

    String uri;
    SHA256 hash;
};

typedef std::tr1::shared_ptr<Visual> VisualPtr;
typedef std::tr1::weak_ptr<Visual> VisualWPtr;


} // namespace Mesh
} // namespace Sirikata

#endif //_SIRIKATA_MESH_VISUAL_HPP_
