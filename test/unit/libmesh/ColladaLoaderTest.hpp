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
		
		ifstream fin ("../../../test/unit/libmesh/collada/pikachu.dae");
		//std::ifstream gill ((std::string)(boost::filesystem::path(Path::Get(Path::DIR_EXE)) / "../../../test/unit/libmesh/collada/pikachu.dae").string());
		string temp;

		do {
			fin >> temp;
			pikachu += temp + ' ';
		}while(temp != "</COLLADA>");

		//Transfer::RemoteFileMetadata metadata;
		Meshdata md;

		//node
		Node ned;
		ned.parent = NullNodeIndex;
		ned.transform = Matrix4x4f::translate(Vector3f(0,-3,0));
		md.nodes.push_back(ned);
		md.rootNodes.push_back(0);

		//geometry instance
		GeometryInstance gi;
		gi.geometryIndex = 0;
		gi.parentNode = 0;
		md.instances.push_back(gi);

		//submeshgeometry
		SubMeshGeometry smg;
		SubMeshGeometry::Primitive p1;
		p1.primitiveType = p1.TRIANGLES;
		for(int i = 0; i < 3; i++) p1.indices.push_back(i);

		//adding points
		Vector3f point[3] = {Vector3f(7,2,6), Vector3f(9,5,1), Vector3f(8,4,3)};
		for(int i = 0; i < 3; i++) smg.positions.push_back(point[i]);
		smg.primitives.push_back(p1);
		smg.recomputeBounds();
		md.geometry.push_back(smg);
		
		//note: the collada stuff can just be dumped......h...e...r...e
		Transfer::DenseData *dd = new Transfer::DenseData(pikachu);
		
		Transfer::DenseDataPtr data(dd);
		
		Mesh::VisualPtr parsed = msys->load(data);
		
		//the parsed should actually be null
		TS_ASSERT_DIFFERS(parsed, Mesh::VisualPtr());
		
		MeshdataPtr mdp(std::tr1::dynamic_pointer_cast<Meshdata>(parsed));

		TS_ASSERT_DIFFERS(mdp->getInstancedGeometryCount(), 0);

		TS_ASSERT_DIFFERS(mdp, MeshdataPtr());
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