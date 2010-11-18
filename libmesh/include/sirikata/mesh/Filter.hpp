/*  Sirikata
 *  Filter.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_LIBMESH_FILTER_HPP_
#define _SIRIKATA_LIBMESH_FILTER_HPP_

#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/core/util/Factory.hpp>

namespace Sirikata {
namespace Mesh {

/** FilterData is the input and output of a Filter. In order to support filters
 *  with multiple inputs (e.g. simplification of multiple meshes), FilterData is
 *  a list of MeshdataPtrs.
 */
class SIRIKATA_MESH_EXPORT FilterData : public std::vector<MeshdataPtr> {
public:
    bool single() const { return this->size() == 1; }
    MeshdataPtr get() const { assert(single()); return at(0); }
}; // class FilterData
typedef std::tr1::shared_ptr<const FilterData> FilterDataPtr;
typedef std::tr1::shared_ptr<FilterData> MutableFilterDataPtr;

class SIRIKATA_MESH_EXPORT Filter {
public:
    virtual ~Filter() {}

    virtual FilterDataPtr apply(FilterDataPtr input) = 0;
}; // class Filter

class SIRIKATA_MESH_EXPORT FilterFactory :
        public AutoSingleton<FilterFactory>,
        public Factory1<Filter*, const String&>
{
public:
    static FilterFactory& getSingleton();
    static void destroy();
}; // class FilterFactory

} // namespace Mesh
} // namespace Sirikata

#endif //_SIRIKATA_LIBMESH_FILTER_HPP_
