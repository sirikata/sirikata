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

//using namespace Mesh;
class ColladaLoaderTest : public CxxTest::TestSuite
{
protected:
	int _initialized;
	String _plugin;
	PluginManager _pmgr;
	ModelsSystem *msys;
	string pikachu;
	string simple;

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
    }
    void testColladaLoader( void ) {

        // For now only support in-tree execution
        boost::filesystem::path collada_data_dir = boost::filesystem::path(Path::Get(Path::DIR_EXE));
        // Windows exes are one level deeper due to Debug or RelWithDebInfo
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
        collada_data_dir = collada_data_dir / "..";
#endif
        collada_data_dir = collada_data_dir / "../../test/unit/libmesh/collada";

        ifstream fin ( (collada_data_dir / "pikachu.dae").string().c_str() );
        // Warn and bail if we can't find the data
        if (!fin) {
            TS_WARN("Unable to find COLLADA data.");
            return;
        }

		//std::ifstream gill ((std::string)(boost::filesystem::path(Path::Get(Path::DIR_EXE)) / "../../../test/unit/libmesh/collada/pikachu.dae").string());
		string temp;

		do {
			fin >> temp;
			pikachu += temp + ' ';
		}while(fin && temp != "</COLLADA>");

        ifstream fin2 ( (collada_data_dir / "simple.dae").string().c_str() );
        if (!fin2) {
            TS_WARN("Unable to find COLLADA data.");
            return;
        }

		do {
			fin2 >> temp;
			simple += temp + ' ';
		}while(fin2 && temp != "</COLLADA>");

		//Test collada file: pikachu.dae
		Transfer::DenseData *dd = new Transfer::DenseData(pikachu);
		Transfer::DenseDataPtr data(dd);
		Mesh::VisualPtr parsed = msys->load(data);

		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());

		MeshdataPtr mdp(tr1::dynamic_pointer_cast<Meshdata>(parsed));
		TS_ASSERT_DIFFERS(mdp->getInstancedGeometryCount(), 0);
		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());

		//Minimal collada file: simple.dae
		Transfer::DenseData *dd2 = new Transfer::DenseData(simple);
		Transfer::DenseDataPtr data2(dd2);
		parsed = msys->load(data2);

		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());

		MeshdataPtr mdp2(tr1::dynamic_pointer_cast<Meshdata>(parsed));
		TS_ASSERT_EQUALS(mdp2->getInstancedGeometryCount(), 0);
		TS_ASSERT_EQUALS(mdp2->getInstancedLightCount(), 0);
		TS_ASSERT_EQUALS(mdp2->geometry.size(), 0);
		TS_ASSERT_EQUALS(mdp2->lights.size(), 0);
		TS_ASSERT_EQUALS(mdp2->textures.size(), 0);
		TS_ASSERT_EQUALS(mdp2->materials.size(), 0);
		TS_ASSERT_DIFFERS(mdp2, MeshdataPtr());
		TS_ASSERT_EQUALS(mdp2->nodes.size(), 2);
		TS_ASSERT_EQUALS(mdp2->nodes[0].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp2->nodes[1].transform, Matrix4x4f::identity());
		TS_ASSERT_EQUALS(mdp2->globalTransform, Matrix4x4f::identity());

    }
	//void testColladaLoaderNull( void ) {
	//	//note: the collada stuff can just be dumped......h...e...r...e
	//	Transfer::DenseData *dd = new Transfer::DenseData("Hello World!");
	//
	//	Transfer::DenseDataPtr data(dd);
	//
	//	Mesh::VisualPtr parsed = msys->load(data);
	//
	//	//the parsed should actually be null
	//	TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());
	//
	//	MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));

	//	TS_ASSERT_DIFFERS(mdp, MeshdataPtr());


	//}
};
