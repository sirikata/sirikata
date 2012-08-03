/*  Sirikata Tests -- Sirikata Test Suite
 *  ColladaLoaderTest.hpp
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
#include <sirikata/mesh/ModelsSystemFactory.hpp>
#include <fstream>
#include <sirikata/core/util/Paths.hpp>
#include <boost/filesystem.hpp>

using namespace Sirikata;
using namespace Mesh;

class ColladaLoaderTest : public CxxTest::TestSuite
{
protected:
	int _initialized;
	String _plugin;
	PluginManager _pmgr;
	ModelsSystem *msys;

public:

    void setUp( void )
    {
		//initialize plugin
		_plugin = "colladamodels";
		if (!_initialized) {
            _initialized = 1;
            _pmgr.load(_plugin);
        }
		//create ModelsSystem
		msys = ModelsSystemFactory::getSingleton ().getConstructor ( "colladamodels" ) ( "" );
		assert(msys);
    }
    void tearDown( void )
    {
		delete msys;
		_pmgr.gc();
		_initialized = 0;
    }

    void testColladaLoaderSimple( void ) {
		//collada file with almost nothing
		String simple = getString("simple");
		MeshdataPtr mdp = loadMDP(simple);

		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr()); //something small is causing them to be slightly different
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 0);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 0);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 0);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 0);
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
    }

	void testColladaLoaderLine( void ) {
		//collada file with only a single line
		String line = getString("line");
		MeshdataPtr mdp = loadMDP(line);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 2);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 2);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderSquare( void ) {
		//collada file with square (actually rectangle)
		String square = getString("square");
		MeshdataPtr mdp = loadMDP(square);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 4);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 6);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderSquare2s( void ) {
		//collada file with square with two sides, one of which is specially texturized
		String square2s = getString("square2s");
		MeshdataPtr mdp = loadMDP(square2s);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 8);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 2);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 6);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials.size(), 2);
		for(int i = 0; i < mdp->materials.size(); i++) 
			TS_ASSERT_EQUALS(mdp->materials[i].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderHex2s( void ) {
		//collada file with two-sided hexagon
		String hex2s = getString("hex2s");
		MeshdataPtr mdp = loadMDP(hex2s);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 12);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 2);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 12);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 2);
		for(int i = 0; i < mdp->materials.size(); i++) 
			TS_ASSERT_EQUALS(mdp->materials[i].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderCube( void ) {
		//collada file with cube (actually a rectangular prism)
		String cube = getString("cube");
		MeshdataPtr mdp = loadMDP(cube);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 24);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 36);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderCubes( void ) {
		//collada file with three textured cubes of different rotations
		String cubes = getString("cubes");
		MeshdataPtr mdp = loadMDP(cubes);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 3);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 3);
		for(int i = 0; i < mdp->geometry.size(); i++) {
			TS_ASSERT_EQUALS(mdp->geometry[i].positions.size(), 24);
			TS_ASSERT_EQUALS(mdp->geometry[i].skinControllers.size(), 0);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives.size(), 1);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives[0].indices.size(), 36);
		}
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 2);
		TS_ASSERT_EQUALS(mdp->materials.size(), 3);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderPrism( void ) {
		//collada file with partially textured hexagonal prism
		String prism = getString("prism");
		MeshdataPtr mdp = loadMDP(prism);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 3);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 3);
		for(int i = 0; i < mdp->geometry.size(); i++) {
			TS_ASSERT_EQUALS(mdp->geometry[i].skinControllers.size(), 0);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives.size(), 1);
		}
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 2);
		TS_ASSERT_EQUALS(mdp->materials.size(), 3);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderTriangles( void ) {
		//collada file with two distinct texturized triangles
		String triangles = getString("triangles");
		MeshdataPtr mdp = loadMDP(triangles);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 2);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 2);
		for(int i = 0; i < mdp->geometry.size(); i++) {
			TS_ASSERT_EQUALS(mdp->geometry[i].positions.size(), 6);
			TS_ASSERT_EQUALS(mdp->geometry[i].skinControllers.size(), 0);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives.size(), 2);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives[0].indices.size(), 3);
		}
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 2);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderTriangles3d( void ) {
		//collada file with two connected disjoint triangles in 3d
		String triangles3d = getString("triangles3d");
		MeshdataPtr mdp = loadMDP(triangles3d);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 6);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 6);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderCircle( void ) {
		//collada file with one circle
		String circle = getString("circle");
		MeshdataPtr mdp = loadMDP(circle);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 24);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 66);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderCircleT( void ) {
		//collada file with one texturized circle
		String circleT = getString("circleT");
		MeshdataPtr mdp = loadMDP(circleT);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 48);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 132);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderCircles( void ) {
		//collada file with two distinct circles
		String circles = getString("circles");
		MeshdataPtr mdp = loadMDP(circles);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 2);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 2);
		for(int i = 0; i < mdp->geometry.size(); i++) {
			TS_ASSERT_EQUALS(mdp->geometry[i].positions.size(), 24);
			TS_ASSERT_EQUALS(mdp->geometry[i].skinControllers.size(), 0);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives.size(), 1);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives[0].indices.size(), 66);
		}
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderCylinders( void ) {
		//collada file with two distinct cylinders
		String cylinders = getString("cylinders");
		MeshdataPtr mdp = loadMDP(cylinders);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 2);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 2);
		for(int i = 0; i < mdp->geometry.size(); i++) {
			TS_ASSERT_EQUALS(mdp->geometry[i].positions.size(), 96);
			TS_ASSERT_EQUALS(mdp->geometry[i].skinControllers.size(), 0);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives.size(), 1);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives[0].indices.size(), 276);
		}
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testColladaLoaderPikachu( void ) {
		//collada file with pikachu
		String pikachu = getString("pikachu/pikachu");
		MeshdataPtr mdp = loadMDP(pikachu);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 7);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 7);
		for(int i = 0; i < mdp->geometry.size(); i++)
			TS_ASSERT_EQUALS(mdp->geometry[i].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 3);
		for(int i = 0; i < mdp->nodes.size(); i++)
			TS_ASSERT_EQUALS(mdp->nodes[i].transform, Matrix4x4f::identity());
	}

	void testColladaLoaderDrill( void ) {
		//collada file with drill
		String drill = getString("drill");
		MeshdataPtr mdp = loadMDP(drill);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 3864);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 3864);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 0);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
	}

	void testColladaLoaderBunny( void ) {
		//collada file with stanford bunny
		String bunny = getString("bunny");
		MeshdataPtr mdp = loadMDP(bunny);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 11553);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 11553);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 0);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
	}

	void testColladaLoaderNull( void ) {
		Transfer::DenseData *dd = new Transfer::DenseData("Hello world!");
		Transfer::DenseDataPtr data(dd);
		//the collada file shouldn't load
		TS_ASSERT_EQUALS(msys->canLoad(data), false);
		//so parsed should be null
		Mesh::VisualPtr parsed = msys->load(data);
		TS_ASSERT_EQUALS(parsed, Mesh::VisualPtr());
		//and so should mdp
		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		TS_ASSERT_EQUALS(mdp, MeshdataPtr());
	}
	
	
	String getString(String name) {
		String result;
		//obtains string of information from the collada file rather than
		//copying and directly placing the text in the file

		// For now only support in-tree execution
        boost::filesystem::path collada_data_dir = boost::filesystem::path(Path::Get(Path::DIR_EXE));
        // Windows exes are one level deeper due to Debug or RelWithDebInfo
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
        collada_data_dir = collada_data_dir / "..";
#endif
        collada_data_dir = collada_data_dir / "../../test/unit/libmesh/collada";

        ifstream fin ( (collada_data_dir / (name + ".dae")).string().c_str() );
        if (!fin) {
            TS_WARN("Unable to find COLLADA data.");
            return "";
        }

		String temp;

		do {
			fin >> temp;
			result += temp + ' ';
		}while(fin && temp != "</COLLADA>");
		return result;
	}
	
	MeshdataPtr loadMDP(String thing) {
		//loads the MeshdataPtr from the collada string
		Transfer::DenseData *dd = new Transfer::DenseData(thing);
		Transfer::DenseDataPtr data(dd);
		TS_ASSERT_EQUALS(msys->canLoad(data), true);

		Mesh::VisualPtr parsed = msys->load(data);
		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());
		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		return mdp;
	}
};
