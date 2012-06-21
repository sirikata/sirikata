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
		//Meshdata
		Meshdata md;
		md.globalTransform = Matrix4x4f::identity();
		
		//first node
		Node ned;
		ned.parent = NullNodeIndex;
		ned.transform = Matrix4x4f::translate(Vector3f(0,3,0));
		ned.children.push_back(1);
		ned.children.push_back(2);
		md.nodes.push_back(ned);
	
		//we have to recognize that there is a root node
		md.rootNodes.push_back(0);
		//more nodes
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

		//iterator to go through each of the geometry instances
		GeometryInstanceIterator gijoe = md.getGeometryInstanceIterator();
		uint32 ui;
		Matrix4x4f m;
		
		//now for success
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

		//there should be no more
		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, false);

    }
	void testGeometryInstanceIteratorExtras( void ) {
		//Meshdata
		Meshdata md;
		md.globalTransform = Matrix4x4f::identity();
		
		//first node
		Node ned;
		ned.parent = NullNodeIndex;
		ned.transform = Matrix4x4f::translate(Vector3f(0,3,0));
		//note: putting instanceChildren instead of children will work too
		ned.children.push_back(1);
		ned.instanceChildren.push_back(2);
		md.nodes.push_back(ned);
	
		//we have to recognize that there is a root node
		md.rootNodes.push_back(0);
		//more nodes
		Node nod;
		nod.parent = 0;
		nod.transform = Matrix4x4f::translate(Vector3f(3,0,0));
		md.nodes.push_back(nod);
		
		Node nud;
		nud.parent = 0;
		nud.transform = Matrix4x4f::translate(Vector3f(0,0,3));
		md.nodes.push_back(nud);

		//sub mesh geometry
		SubMeshGeometry smg1;
		SubMeshGeometry smg2;
		md.geometry.push_back(smg1);
		md.geometry.push_back(smg2);
	
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

		//iterator to go through each of the geometry instances
		GeometryInstanceIterator gijoe = md.getGeometryInstanceIterator();
		uint32 ui;
		Matrix4x4f m;
		
		//now for success
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

		//there should be no more
		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, false);
	}

	void testLightInstanceIterator( void ) {
		Meshdata md;
		//Meshdata, with LightInstance, needs identity matrix
		md.globalTransform = Matrix4x4f::identity();
		
		//one node
		Node ned;
		ned.parent = NullNodeIndex;
		md.nodes.push_back(ned);
		ned.transform = Matrix4x4f::translate(Vector3f(1,2,3));

		LightInfo info;
		md.lights.push_back(info);

		//as with GeometryInstanceIterator
		md.rootNodes.push_back(0);

		//lightinstance instead of geometryinstance
		LightInstance li;
		li.lightIndex = 0;
		li.parentNode = 0;
		md.lightInstances.push_back(li);
		
		//LightInstanceIterator
		LightInstanceIterator lithium = md.getLightInstanceIterator();
		uint32 u;
		Matrix4x4f m;
		
		bool success = lithium.next(&u, &m);
        TS_ASSERT_EQUALS(success, true);
		TS_ASSERT_EQUALS(u, 0);
		//note: value was expected - Matrix4x4f::translate(Vectort3f(0,0,0))
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(0,0,0)));
		
	}
	
	void xtestLightInstanceIterator2( void ) {
		Meshdata md;
		//Meshdata, with LightInstance, needs identity matrix
		md.globalTransform = Matrix4x4f::identity();
		
		//one node
		Node ned;
		ned.parent = NullNodeIndex;
		ned.children.push_back(1);
		md.nodes.push_back(ned);
		//even with a transform nothing happens
		ned.transform = Matrix4x4f::translate(Vector3f(1,2,3));
		
		Node nod;
		nod.parent = 0;
		md.nodes.push_back(ned);
		nod.transform = Matrix4x4f::translate(Vector3f(1,2,3));

		LightInfo info;
		md.lights.push_back(info);

		//as with GeometryInstanceIterator
		md.rootNodes.push_back(0);

		//lightinstance instead of geometryinstance
		LightInstance li;
		li.lightIndex = 0;
		li.parentNode = 0;
		md.lightInstances.push_back(li);
		
		//LightInstanceIterator
		LightInstanceIterator lithium = md.getLightInstanceIterator();
		uint32 u;
		Matrix4x4f m;
		
		bool success = lithium.next(&u, &m);
        TS_ASSERT_EQUALS(success, true);
		TS_ASSERT_EQUALS(u, 0);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(0,0,0)));
	}
	void testMeshDataGlobalTransform( void ) {
		Meshdata md;
		TS_ASSERT_EQUALS(md.globalTransform, Matrix4x4f::identity());
	}
	void testMeshdataSanity( void ) {
		//Meshdata
		Meshdata md;
		//we have to have identity matrix otherwise we are multiplying by zero matrix
		md.globalTransform = Matrix4x4f::identity();
		
		
		//only node
		Node ned;
		ned.parent = NullNodeIndex;
		ned.transform = Matrix4x4f::translate(Vector3f(0,3,0));
		md.nodes.push_back(ned);
	
		//we have to recognize that there is a root node
		md.rootNodes.push_back(0);
	
		//sub mesh geometry
		//SubMeshGeometry smg1;
		//md.geometry.push_back(smg1);
	
		//geometry instances
		GeometryInstance gi;
		gi.geometryIndex = 0;
		gi.parentNode = 0;
		//note to self: everything must connect back to md
		md.instances.push_back(gi);

		//iterator to go through each of the geometry instances
		GeometryInstanceIterator gijoe = md.getGeometryInstanceIterator();
		uint32 ui;
		Matrix4x4f m;
		
		//now for success
		bool success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, true);
		
		TS_ASSERT_EQUALS(ui, 0);
		TS_ASSERT_EQUALS(m, Matrix4x4f::translate(Vector3f(0,3,0)));

		success = gijoe.next(&ui, &m);
        TS_ASSERT_EQUALS(success, false);
		
		
	}
};
