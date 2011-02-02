/*  Sirikata
 *  Meshdata.cpp
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

#include <sirikata/mesh/Filter.hpp>

namespace Sirikata {
namespace Mesh {

SIRIKATA_MESH_EXPORT NodeIndex NullNodeIndex = -1;

Node::Node()
 : parent(NullNodeIndex),
   transform(Matrix4x4f::identity())
{
}

Node::Node(NodeIndex par, const Matrix4x4f& xform)
 : parent(par),
   transform(xform)
{
}

Node::Node(const Matrix4x4f& xform)
 : parent(NullNodeIndex),
   transform(xform)
{
}

Matrix4x4f Meshdata::getTransform(NodeIndex index) const {
    // Just trace up the tree, multiplying in transforms
    Matrix4x4f xform(Matrix4x4f::identity());

    while(index != NullNodeIndex) {
        xform = nodes[index].transform * xform;
        index = nodes[index].parent;
    }

    return globalTransform * xform;
}

Matrix4x4f Meshdata::getTransform(const GeometryInstance& geo) const {
    return getTransform(geo.parentNode);
}

Matrix4x4f Meshdata::getTransform(const LightInstance& light) const {
    return getTransform(light.parentNode);
}

Meshdata::GeometryInstanceIterator Meshdata::getGeometryInstanceIterator() {
    return GeometryInstanceIterator(this);
}

Meshdata::GeometryInstanceIterator::GeometryInstanceIterator(Meshdata* mesh)
 : mMesh(mesh),
   mRoot(-1)
{
}

bool Meshdata::GeometryInstanceIterator::next(uint32* geo_idx, Matrix4x4f* xform) {
    while(true) { // Outer loop keeps us moving until we hit something to return

        // First, if we emptied out, try to handle the next root.
        if (mStack.empty()) {
            mRoot++;
            if (mRoot >= mMesh->rootNodes.size()) return false;

            NodeState st;
            st.index = mMesh->rootNodes[mRoot];
            st.step = NodeState::Nodes;
            st.currentChild = -1;
            st.transform = mMesh->globalTransform;
            mStack.push(st);
        }

        NodeState& node = mStack.top();

        if (node.step == NodeState::Nodes) {
            node.currentChild++;
            if (node.currentChild >= mMesh->nodes[node.index].children.size()) {
                node.step = NodeState::InstanceNodes;
                node.currentChild = -1;
                continue;
            }

            NodeState st;
            st.index = mMesh->nodes[node.index].children[node.currentChild];
            st.step = NodeState::Nodes;
            st.currentChild = -1;
            st.transform = node.transform * mMesh->nodes[ st.index ].transform;
            mStack.push(st);
            continue;
        }

        if (node.step == NodeState::InstanceNodes) {
            node.currentChild++;
            if (node.currentChild >= mMesh->nodes[node.index].instanceChildren.size()) {
                node.step = NodeState::InstanceGeometries;
                node.currentChild = -1;
                continue;
            }

            NodeState st;
            st.index = mMesh->nodes[node.index].instanceChildren[node.currentChild];
            st.step = NodeState::Nodes;
            st.currentChild = -1;
            st.transform = node.transform * mMesh->nodes[ st.index ].transform;
            mStack.push(st);
            continue;
        }

        if (node.step == NodeState::InstanceGeometries) {
            // FIXME this step is inefficient because each node doesn't have a
            // list of instance geometries. Instead, we have to iterate over all
            // instance geometries for all nodes.
            node.currentChild++;
            if (node.currentChild >= mMesh->instances.size()) {
                node.step = NodeState::Done;
                continue;
            }
            // Need this check since we're using the global instances list
            if (node.index != mMesh->instances[node.currentChild].parentNode)
                continue;
            // Otherwise, just yield the information
            *geo_idx = node.currentChild;
            *xform = node.transform;
            return true;
        }

        if (node.step == NodeState::Done) {
            // We're finished with the node, just pop it so parent will continue
            // processing
            mStack.pop();
        }
    }
}


} // namespace Mesh
} // namespace Sirikata
