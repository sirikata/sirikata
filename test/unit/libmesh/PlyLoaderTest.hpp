/*  Sirikata Tests -- Sirikata Test Suite
 *  PlyLoaderTest.hpp
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
#include <sirikata/mesh/Filter.hpp>
#include <sirikata/mesh/CompositeFilter.hpp>

using namespace Sirikata;
using namespace std;
using namespace Mesh;

class PlyLoaderTest : public CxxTest::TestSuite
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
		if (!_initialized) {
            _initialized = 1;
            _pmgr.loadList("mesh-ply,common-filters");
        }
		//create ModelsSystem
		msys = ModelsSystemFactory::getSingleton ().getConstructor ( "mesh-ply" ) ( "" );
		assert(msys);

    }
    void tearDown( void )
    {
		delete msys;
		_pmgr.gc();
		_initialized = 0;
    }

	void testPlyLoaderSimple( void ) {
		//ply file with almost nothing
		string simple = getString("simple");

		Transfer::DenseData *dd = new Transfer::DenseData(simple);
		Transfer::DenseDataPtr data(dd);
		TS_ASSERT_EQUALS(msys->canLoad(data), true);

		Mesh::VisualPtr parsed = msys->load(data);
		TS_ASSERT_EQUALS(parsed, Mesh::VisualPtr());
		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		TS_ASSERT_EQUALS(mdp, MeshdataPtr());
    }

	void testPlyLoaderLine( void ) {
		//ply file with a line
		string line = getString("line");
		MeshdataPtr mdp = loadMDP(line);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
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
	
	void testPlyLoaderSquare( void ) {
		//ply file with a square (actually a rectangle)
		string square = getString("square");
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

	void testPlyLoaderSquare2s( void ) {
		//ply file with square with two sides, one of which is specially texturized
		string square2s = getString("square2s");
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
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 6)
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->materials.size(), 2);
		for(int i = 0; i < mdp->materials.size(); i++) 
			TS_ASSERT_EQUALS(mdp->materials[i].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}

	void testPlyLoaderCubes( void ) {
		//ply file with three textured cubes of different rotations
		string cubes = getString("cubes");
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

	void testPlyLoaderPrism( void ) {
		//ply file with partially textured hexagonal prism
		string prism = getString("prism");
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

	void testPlyLoaderCircleT( void ) {
		//ply file with one texturized circle
		string circleT = getString("circleT");
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

	void testPlyLoaderCircles( void ) {
		//ply file with two distinct circles
		string circles = getString("circles");
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

	void testPlyLoaderCylinders( void ) {
		//ply file with two cylinders
		string cylinders = getString("cylinders");
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

	void testPlyLoaderDrill( void ) {
		//ply file with drill
		//its results deviate significantly from those of the collada version...
		//but then again, the two formats are bettter for different things...
		//also, this loads *very* slowly, showing the inefficiency of the ply plugin
		string drill = getString("drill/reconstruction/drill_shaft_zip");
		MeshdataPtr mdp = loadMDP(drill);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 7);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 7);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 845);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 3831);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
	}

	void testPlyLoaderBunny( void ) {
		//ply file with bunny
		string bunny = getString("bunny/reconstruction/bun_zipper_res4");
		MeshdataPtr mdp = loadMDP(bunny);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].positions.size(), 453);
		TS_ASSERT_EQUALS(mdp->geometry[0].skinControllers.size(), 0);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 2844);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
	}

	void testPlyLoaderNull( void ) {
		//invalid ply file
		Transfer::DenseData *dd = new Transfer::DenseData("Hello world!");
		Transfer::DenseDataPtr data(dd);
		//the ply file shouldn't load
		TS_ASSERT_EQUALS(msys->canLoad(data), false);
		//so parsed should be null
		Mesh::VisualPtr parsed = msys->load(data);
		TS_ASSERT_EQUALS(parsed, Mesh::VisualPtr());
		//and so should mdp
		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		TS_ASSERT_EQUALS(mdp, MeshdataPtr());
	}

	string getString(string name) {
		string result;
		//obtains string of information from the ply file rather than
		//copying and directly placing the text in the file

		// For now only support in-tree execution
        boost::filesystem::path ply_data_dir = boost::filesystem::path(Path::Get(Path::DIR_EXE));
        // Windows exes are one level deeper due to Debug or RelWithDebInfo
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
        ply_data_dir = ply_data_dir / "..";
#endif
        ply_data_dir = ply_data_dir / "../../test/unit/libmesh/ply";

        ifstream fin ( (ply_data_dir / (name + ".ply")).string().c_str() );
        if (!fin) {
            TS_WARN("Unable to find ply data.");
            return "";
        }

		string temp;

		do {
			fin >> temp;
			result += temp + ' ';
		}while(fin);
		return result;
	}
	
	//MeshdataPtr loadMDP(string thing) {
	//	//loads the MeshdataPtr from the ply string
	//	Transfer::DenseData *dd = new Transfer::DenseData(thing);
	//	Transfer::DenseDataPtr data(dd);
	//	TS_ASSERT_EQUALS(msys->canLoad(data), true);

	//	Mesh::VisualPtr parsed = msys->load(data);
	//	TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());
	//	MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
	//	return mdp;
	//}

	MeshdataPtr loadMDP(string thing) {
		//loads the MeshdataPtr from the ply string and applies the deduplication filter
		std::vector<String> names_and_args;
		names_and_args.push_back("compute-normals"); names_and_args.push_back("");
		names_and_args.push_back("deduplication"); names_and_args.push_back("");
		Mesh::Filter* filter = new Mesh::CompositeFilter(names_and_args);

		Transfer::DenseData *dd = new Transfer::DenseData(thing);
		Transfer::DenseDataPtr data(dd);
		TS_ASSERT_EQUALS(msys->canLoad(data), true);

		Mesh::VisualPtr parsed = msys->load(data);
		Mesh::MutableFilterDataPtr input(new Mesh::FilterData);
		input->push_back(parsed);
		Mesh::FilterDataPtr output = filter->apply(input);
		parsed = output->get();
		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));

		return mdp;
	}
};
