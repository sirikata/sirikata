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

class DeduplicationTest : public CxxTest::TestSuite
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

	void testFilterTriangles( void ) {
		//ply file with two distinct texturized triangles
		string triangles = getString("triangles");
		//before filtering
		MeshdataPtr mdp = loadMDP(triangles);
		
		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 4);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 4);
		for(int i = 0; i < mdp->geometry.size(); i++) {
			TS_ASSERT_EQUALS(mdp->geometry[i].positions.size(), 3);
			TS_ASSERT_EQUALS(mdp->geometry[i].skinControllers.size(), 0);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives.size(), 1);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives[0].indices.size(), 3);
		}
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 2);
		TS_ASSERT_EQUALS(mdp->materials[0].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());

		//after filtering
		mdp = loadFilteredMDP(triangles);
		
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

	void testFilterHex2s( void ) {
		//ply file with a two-sided hexagon
		string hex2s = getString("hex2s");
		//before filtering
		MeshdataPtr mdp = loadMDP(hex2s);

		//asserts
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp->getInstancedGeometryCount(), 2);
		TS_ASSERT_EQUALS(mdp->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp->getJointCount(), 0);
		TS_ASSERT_EQUALS(mdp->geometry.size(), 2);
		for(int i = 0; i < mdp->geometry.size(); i++) {
			TS_ASSERT_EQUALS(mdp->geometry[i].positions.size(), 6);
			TS_ASSERT_EQUALS(mdp->geometry[i].skinControllers.size(), 0);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives.size(), 1);
			TS_ASSERT_EQUALS(mdp->geometry[i].primitives[0].indices.size(), 12);
		}
		TS_ASSERT_EQUALS(mdp->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp->materials.size(), 2);
		for(int i = 0; i < mdp->materials.size(); i++) 
			TS_ASSERT_EQUALS(mdp->materials[i].textures.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes.size(), 1);
		TS_ASSERT_EQUALS(mdp->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp->globalTransform, Matrix4x4f::identity());

		//after filtering
		mdp = loadFilteredMDP(hex2s);

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
	
	MeshdataPtr loadMDP(string thing) {
		//loads the MeshdataPtr from the ply string
		Transfer::DenseData *dd = new Transfer::DenseData(thing);
		Transfer::DenseDataPtr data(dd);
		TS_ASSERT_EQUALS(msys->canLoad(data), true);

		Mesh::VisualPtr parsed = msys->load(data);
		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());
		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));
		return mdp;
	}

	MeshdataPtr loadFilteredMDP(string thing) {
		//loads the MeshdataPtr from the ply string and applies the deduplication filter
		std::vector<String> names_and_args;
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
