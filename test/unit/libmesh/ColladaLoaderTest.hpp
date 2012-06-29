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
using namespace std;
using namespace Mesh;

class ColladaLoaderTest : public CxxTest::TestSuite
{
protected:
	int _initialized;
	String _plugin;
	PluginManager _pmgr;
	ModelsSystem *msys;
	string simple;
	string line;
	string square;
	string cube;
	string triangles;

public:

    void setUp( void )
    {
		
		
		_plugin = "colladamodels";
		if (!_initialized) {
            _initialized = 1;
            _pmgr.load(_plugin);
        }
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
        getString("simple", simple);

		Transfer::DenseData *dd = new Transfer::DenseData(simple);
		Transfer::DenseDataPtr data(dd);
		Mesh::VisualPtr parsed = msys->load(data);

		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());

		MeshdataPtr mdp(tr1::dynamic_pointer_cast<Meshdata>(parsed));

		//asserts
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 0);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 0);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 0);
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
    }
	void testColladaLoaderLine( void ) {
		getString("line", line);

		Transfer::DenseData *dd = new Transfer::DenseData(line);
	
		Transfer::DenseDataPtr data(dd);
	
		Mesh::VisualPtr parsed = msys->load(data);
	
		//the parsed should actually be null
		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());

		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		
		//asserts
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 2);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}
	void testColladaLoaderSquare( void ) {
		getString("square", square);

		Transfer::DenseData *dd = new Transfer::DenseData(square);
	
		Transfer::DenseDataPtr data(dd);

		TS_ASSERT_EQUALS(msys->canLoad(data), true);
		Mesh::VisualPtr parsed = msys->load(data);
	
		//the parsed should actually be null
		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());

		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		
		//asserts
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 6);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}
	void testColladaLoaderCube( void ) {
		getString("cube", cube);

		Transfer::DenseData *dd = new Transfer::DenseData(cube);
	
		Transfer::DenseDataPtr data(dd);
		
		TS_ASSERT_EQUALS(msys->canLoad(data), true);
		Mesh::VisualPtr parsed = msys->load(data);
	
		//the parsed should actually be null
		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());

		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		
		//asserts
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 1);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 36);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}
	void testColladaLoaderTriangles( void ) {
		getString("triangles", triangles);

		Transfer::DenseData *dd = new Transfer::DenseData(triangles);
	
		Transfer::DenseDataPtr data(dd);
		
		TS_ASSERT_EQUALS(msys->canLoad(data), true);
		Mesh::VisualPtr parsed = msys->load(data);
	
		//the parsed should actually be null
		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());

		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		
		//asserts
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 2);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 2);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[1].primitives.size(), 1);
		TS_ASSERT_EQUALS(mdp->geometry[0].primitives[0].indices.size(), 3);
		TS_ASSERT_EQUALS(mdp->geometry[1].primitives[0].indices.size(), 3);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 1);
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());
	}
	void testColladaLoaderNull( void ) {
		Transfer::DenseData *dd = new Transfer::DenseData("This should not load");
	
		Transfer::DenseDataPtr data(dd);
		
		TS_ASSERT_EQUALS(msys->canLoad(data), false);
		//even though canLoad is false, the thing loads anyways
		Mesh::VisualPtr parsed = msys->load(data);
	
		//the parsed should actually be null
		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());

		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		
		//asserts
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 0);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 0);
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 0);
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->nodes.size(), 0);

	}
	//obtains string of information from the collada file rather than copying
	//and directly placing the text in the file
	void getString(string name, string& thing) {


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
            return;
        }

		string temp;

		do {
			fin >> temp;
			thing += temp + ' ';
		}while(fin && temp != "</COLLADA>");
	}
};
