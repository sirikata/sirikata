/*  Sirikata Tests -- Sirikata Test Suite
 *  MeshDataTest.hpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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
#include <cxxtest/TestSuite.h>
#include <sirikata/mesh/Meshdata.hpp>

using namespace Sirikata;
using namespace Sirikata::Mesh;
class MeshDataTest : public CxxTest::TestSuite
{
	typedef Meshdata::GeometryInstanceIterator GeometryInstanceIterator;
	typedef Meshdata::LightInstanceIterator LightInstanceIterator;
public:
    void setUp( void )
    {
    }
    void tearDown( void )
    {
    }
	void testGeometryInstanceIterator( void ) {
		//test if GeometryInstanceIterator works over multiple nodes
		Meshdata md;
		
		//nodes
		Node ned;
		ned.parent = NullNodeIndex;
		ned.transform = Matrix4x4f::translate(Vector3f(0,3,0));
		ned.children.push_back(1);
		ned.children.push_back(2);
		md.nodes.push_back(ned);
		//we have to recognize that there is a root node, otherwise we may encounter errors
		md.rootNodes.push_back(0);

		Node nod;
		nod.parent = 0;
		nod.transform = Matrix4x4f::translate(Vector3f(3,0,0));
		md.nodes.push_back(nod);
		
		Node nud;
		nud.parent = 0;
		nud.transform = Matrix4x4f::translate(Vector3f(0,0,3));
		md.nodes.push_back(nud);

		//geometry instances
		GeometryInstance gi;
		gi.geometryIndex = 0;
		gi.parentNode = 1;
		md.instances.push_back(gi);

		GeometryInstance gj;
		gj.geometryIndex = 1;
		gj.parentNode = 1;
		md.instances.push_back(gj);

		GeometryInstance gk;
		gk.geometryIndex = 0;
		gk.parentNode = 2;
		md.instances.push_back(gk);

		GeometryInstance gl;
		gl.geometryIndex = 1;
		gl.parentNode = 2;
		md.instances.push_back(gl);

		TS_ASSERT_EQUALS(md.getInstancedGeometryCount(), 4);
		TS_ASSERT_EQUALS(md.getInstancedLightCount(), 0);

		//SubMeshGeometry
		SubMeshGeometry smg1;
		SubMeshGeometry smg2;
		md.geometry.push_back(smg1);
		md.geometry.push_back(smg2);

		//iterator to go through each geometry instance
		GeometryInstanceIterator gijoe = md.getGeometryInstanceIterator();
		uint32 ui;
		Matrix4x4f m;
		
		bool success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);
		
		TS_ASSERT_EQUALS(ui, 0);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(3,3,0)));

		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);
		
		TS_ASSERT_EQUALS(ui, 1);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(3,3,0)));

		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);
		
		TS_ASSERT_EQUALS(ui, 2);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(0,3,3)));
		
		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);

		TS_ASSERT_EQUALS(ui, 3);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(0,3,3)));

		//only four GeometryInstances
		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, false);

    }
	void testGeometryInstanceIteratorInstanceChildren( void ) {
		//test if GeometryInstanceIterator works with instance children
		Meshdata md;
		
		//nodes
		Node ned;
		ned.parent = NullNodeIndex;
		ned.transform = Matrix4x4f::translate(Vector3f(0,3,0));

		ned.children.push_back(1);
		//instanceChildren instead of just children
		ned.instanceChildren.push_back(1);
		ned.instanceChildren.push_back(1);
		md.nodes.push_back(ned);
		md.rootNodes.push_back(0);

		Node nod;
		nod.transform = Matrix4x4f::translate(Vector3f(3,0,0));
		md.nodes.push_back(nod);
	
		//geometry instances
		GeometryInstance gi;
		gi.geometryIndex = 0;
		gi.parentNode = 1;
		md.instances.push_back(gi);

		GeometryInstance gj;
		gj.geometryIndex = 1;
		gj.parentNode = 1;
		md.instances.push_back(gj);

		TS_ASSERT_EQUALS(md.getInstancedGeometryCount(), 6);
		TS_ASSERT_EQUALS(md.getInstancedLightCount(), 0);

		//GeometryInstanceIterator
		GeometryInstanceIterator gijoe = md.getGeometryInstanceIterator();
		uint32 ui;
		Matrix4x4f m;
		
		//successes
		bool success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);
		
		TS_ASSERT_EQUALS(ui, 0);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(3,3,0)));

		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);
		
		TS_ASSERT_EQUALS(ui, 1);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(3,3,0)));

		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);
		
		TS_ASSERT_EQUALS(ui, 0);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(3,3,0)));
		
		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);

		TS_ASSERT_EQUALS(ui, 1);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(3,3,0)));

		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);

		TS_ASSERT_EQUALS(ui, 0);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(3,3,0)));

		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);

		TS_ASSERT_EQUALS(ui, 1);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(3,3,0)));

		//there should be no more
		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, false);

		
	}
	void testSubMeshGeometry( void ) {
		//test a SubMeshGeometry and some Primitives
		Meshdata md;
		Node ned;
		ned.parent = NullNodeIndex;
		ned.transform = Matrix4x4f::translate(Vector3f(0,10,0));
		md.nodes.push_back(ned);
		md.rootNodes.push_back(0);

		GeometryInstance gi;
		gi.geometryIndex = 0;
		gi.parentNode = 0;
		
		//SubMeshGeometry
		SubMeshGeometry smg1;
		//a SubMeshGeometry must have its bounds initilaized
		smg1.recomputeBounds();
		TS_ASSERT_EQUALS(smg1.radius, 0);
		
		//Adding primitives
		SubMeshGeometry::Primitive p1;
		p1.primitiveType = p1.TRIANGLES;
		for(int i = 0; i < 3; i++) p1.indices.push_back(i);

		SubMeshGeometry::Primitive p2;
		p2.primitiveType = p2.TRIANGLES;
		for(int i = 1; i < 4; i++) p2.indices.push_back(i);
		
		//first, adding points
		Vector3f point[4] = {Vector3f(1,2,3), Vector3f(2,3,1), Vector3f(3,1,2), Vector3f(4,6,6)};
		smg1.positions.push_back(point[0]);
		smg1.positions.push_back(point[1]);
		smg1.positions.push_back(point[2]);
		smg1.positions.push_back(point[3]);
		smg1.primitives.push_back(p1);
		smg1.primitives.push_back(p2);
		//recomputeBounds after adding primitives
		smg1.recomputeBounds();

		TS_ASSERT_EQUALS(smg1.primitives.size(),2);
		TS_ASSERT_EQUALS(smg1.aabb.center(),Vector3f(2.5, 3.5, 3.5));
		TS_ASSERT_EQUALS(smg1.aabb.contains(Vector3f(0,0,0)), false);
		TS_ASSERT_EQUALS(smg1.aabb.contains(Vector3f(0,0,0), 1), true);
		TS_ASSERT_EQUALS(smg1.aabb.degenerate(), false);
		TS_ASSERT_EQUALS(smg1.aabb.diag(), Vector3f(3,5,5));

		md.instances.push_back(gi);
		md.geometry.push_back(smg1);

		//GeometryInstanceIterator
		GeometryInstanceIterator gijoe = md.getGeometryInstanceIterator();
		uint32 ui;
		Matrix4x4f m;
		
		bool success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);
		
		TS_ASSERT_EQUALS(ui, 0);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(0,10,0)));
		
		//computeTransformedBounds after a transformation
		md.instances[ui].computeTransformedBounds(md, m,
			&md.geometry[md.instances[ui].geometryIndex].aabb,
			&md.geometry[md.instances[ui].geometryIndex].radius);
		TS_ASSERT_EQUALS(md.geometry[md.instances[ui].geometryIndex].aabb.center(),Vector3f(2.5,13.5,3.5));
		TS_ASSERT_EQUALS(md.geometry[md.instances[ui].geometryIndex].aabb.diag(), Vector3f(3,5,5));
		
	}
	void testLightInstanceIterator( void ) {
		//test LightInstanceIterator functionality
		Meshdata md;
		//one node
		Node ned;
		ned.parent = NullNodeIndex;
		ned.transform = Matrix4x4f::translate(Vector3f(1,2,3));
		md.nodes.push_back(ned);
		md.rootNodes.push_back(0);

		//lightinstance instead of geometryinstance
		LightInstance li;
		li.lightIndex = 0;
		li.parentNode = 0;
		md.lightInstances.push_back(li);
		
		TS_ASSERT_EQUALS(md.getInstancedGeometryCount(), 0);
		TS_ASSERT_EQUALS(md.getInstancedLightCount(), 1);

		//LightInstanceIterator
		LightInstanceIterator lithium = md.getLightInstanceIterator();
		uint32 u;
		Matrix4x4f m;
		
		bool success = lithium.next(&u, &m);
        TS_ASSERT_EQUALS(success, true);
		TS_ASSERT_EQUALS(u, 0);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(1,2,3)));
	}
	void testMeshDataGlobalTransform( void ) {
		//simple test to see if globalTransform is set as identity when
		//initialized
		Meshdata md;
		TS_ASSERT_EQUALS(md.globalTransform, Matrix4x4f::identity());
	}
	void testMeshdataSanity( void ) {
		//test a simple Meshdata
		Meshdata md;

		//one node
		Node ned;
		ned.parent = NullNodeIndex;
		ned.transform = Matrix4x4f::translate(Vector3f(0,3,0));
		md.nodes.push_back(ned);
		md.rootNodes.push_back(0);

		//geometry instance
		GeometryInstance gi;
		gi.geometryIndex = 0;
		gi.parentNode = 0;
		md.instances.push_back(gi);

		//iterator
		GeometryInstanceIterator gijoe = md.getGeometryInstanceIterator();
		uint32 ui;
		Matrix4x4f m;
		
		//success
		bool success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);
		
		TS_ASSERT_EQUALS(ui, 0);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(0,3,0)));

		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, false);
	}
};