/*  Sirikata
 *  TransformFilter.cpp
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

#include "TransformFilter.hpp"
#include <boost/lexical_cast.hpp>

namespace Sirikata {
namespace Mesh {

Filter* TransformFilter::createTranslate(const String& args) {
    Vector3f offset = boost::lexical_cast<Vector3f>(args);
    return new TransformFilter( Matrix4x4f::translate(offset) );
}

Filter* TransformFilter::createScale(const String& args) {
    float sc = boost::lexical_cast<float>(args);
    return new TransformFilter( Matrix4x4f::scale(sc) );
}

Filter* TransformFilter::createRotate(const String& args) {
    Quaternion rot = boost::lexical_cast<Quaternion>(args);
    return new TransformFilter( Matrix4x4f::rotate(rot) );
}

TransformFilter::TransformFilter(const Matrix4x4f& xform)
 : mTransform(xform)
{
}

FilterDataPtr TransformFilter::apply(FilterDataPtr input) {
    // To apply the transform, we just add a new node and put all root nodes as
    // children of it.

    for(FilterData::const_iterator md_it = input->begin(); md_it != input->end(); md_it++) {
        MeshdataPtr md = *md_it;

        Node new_root(NullNodeIndex, mTransform);
        new_root.children = md->rootNodes;

        NodeIndex new_root_index = md->nodes.size();
        md->nodes.push_back(new_root);

        md->rootNodes.clear();
        md->rootNodes.push_back(new_root_index);
    }

    return input;
}

} // namespace Mesh
} // namespace Sirikata
