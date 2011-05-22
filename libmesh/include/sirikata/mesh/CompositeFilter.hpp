/*  Sirikata
 *  CompositeFilter.hpp
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

#ifndef _SIRIKATA_LIBMESH_COMPOSITE_FILTER_HPP_
#define _SIRIKATA_LIBMESH_COMPOSITE_FILTER_HPP_

#include <sirikata/mesh/Platform.hpp>
#include <sirikata/mesh/Filter.hpp>

namespace Sirikata {
namespace Mesh {

/** A CompositeFilter is essentially a pipeline of other filters. This makes it
 *  easy to specify a whole set of operations together, e.g. perform a full
 *  set of simplification procedures.
 */
class SIRIKATA_MESH_EXPORT CompositeFilter : public Filter {
public:
    class SIRIKATA_MESH_EXPORT Exception : std::exception {
    public:
        Exception(const String& msg);
        virtual ~Exception() throw();
        virtual const char* what() const throw();
    private:
        std::string _msg;
    };

    CompositeFilter();
    /** Constructor taking a sequence of arguments as
     *  [filter1_name, filter1_args, filter2_name, filter2_args, ...].
     *  Note that arguments cannot be omitted -- use an empty string if you do
     *  not need to pass any.
     */
    CompositeFilter(const std::vector<String>& names_and_args);
    virtual ~CompositeFilter() {}

    void add(const String& name, const String& args = "");

    virtual FilterDataPtr apply(FilterDataPtr input);

private:
    std::vector<FilterPtr> mFilters;
}; // class CompositeFilter

} // namespace Mesh
} // namespace Sirikata

#endif //_SIRIKATA_LIBMESH_COMPOSITE_FILTER_HPP_
