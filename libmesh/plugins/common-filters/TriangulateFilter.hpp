/*  Sirikata
 *  TriangulateFilter.hpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LIBMESH_PLUGIN_COMMON_FILTERS_TRIANGULATE_FILTER_HPP_
#define _LIBMESH_PLUGIN_COMMON_FILTERS_TRIANGULATE_FILTER_HPP_

#include <sirikata/mesh/Filter.hpp>

namespace Sirikata {
namespace Mesh {

/** TriangulateFilter converts non-triangulated meshes, e.g. triangle fans and
 *  triangle strips, into triangle list representations. This is useful as a
 *  preprocess to filters which require triangle representations.
 *
 *  You can control which types it converts using the parameter: 'trifan',
 *  'tristrip', or 'all'. 'all' is the default.
 */
class TriangulateFilter : public Filter {
public:
    static Filter* create(const String& args);

    TriangulateFilter(bool tristrips, bool trifans);
    virtual ~TriangulateFilter() {}

    virtual FilterDataPtr apply(FilterDataPtr input);

private:
    bool mTriStrips;
    bool mTriFans;
};

} // namespace Mesh
} // namespace Sirikata

#endif //_LIBMESH_PLUGIN_COMMON_FILTERS_TRIANGULATE_FILTER_HPP_
