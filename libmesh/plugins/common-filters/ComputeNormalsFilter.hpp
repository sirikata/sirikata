// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _LIBMESH_PLUGIN_COMMON_FILTERS_COMPUTE_NORMALS_FILTER_HPP_
#define _LIBMESH_PLUGIN_COMMON_FILTERS_COMPUTE_NORMALS_FILTER_HPP_

#include <sirikata/mesh/Filter.hpp>

namespace Sirikata {
namespace Mesh {

/** For meshes that don't already specify them, fills in normals computed based
 *  on winding order. Note that this *only* works for triangulated meshes!
 */
class ComputeNormalsFilter : public Filter {
public:
    static Filter* create(const String& args);

    virtual ~ComputeNormalsFilter() {}

    virtual FilterDataPtr apply(FilterDataPtr input);
};

} // namespace Mesh
} // namespace Sirikata

#endif //_LIBMESH_PLUGIN_COMMON_FILTERS_COMPUTE_NORMALS_FILTER_HPP_
