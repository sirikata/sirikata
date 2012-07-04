// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "PlyModelSystem.hpp"
#include <sirikata/mesh/Meshdata.hpp>
#include <sstream>
#include <vector>

namespace Sirikata {

enum {PLY, VERTEX, FACE, EDGE, X, Y, Z, RED, GREEN, BLUE, ALPHA, VI};



PlyModelSystem::PlyModelSystem() {
}

PlyModelSystem::~PlyModelSystem () {
}

bool PlyModelSystem::canLoad(Transfer::DenseDataPtr data) {
    //Minimal check that only sees if "ply" and "format"
	//are somewhere near the start of the file, which is
	//a decent test for detecting ply files.
	if (!data) return false;

	//create string out of first 1K
	int32 sublen = std::min((int)data->length(), (int)1024);
    std::string subset((const char*)data->begin(), (std::size_t)sublen);

    if (subset.find("ply") != subset.npos
		&& subset.find("format") != subset.npos)
        return true;

    return false;
}

Mesh::VisualPtr PlyModelSystem::load(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data) {
	if(!canLoad(data))
		return Mesh::VisualPtr();
	//data dump right here
	//std::cout << data->asString() << '\n';
	std::stringstream ss (std::stringstream::in | std::stringstream::out);
	ss << data->asString();
	String s;
	ss >> s;
	Mesh::MeshdataPtr mdp(new Mesh::Meshdata);
	
	int state = PLY;
	int vertexNum = 0, faceNum = 0, edgeNum = 0;
	std::vector<int> propV, propF, propE;
	
	while(ss) {
		if(s == "element") {
			ss >> s;
			if(s == "vertex") {
				state = VERTEX;
				ss >> vertexNum;
			}
			else if(s == "face") {
				state = FACE;
				ss >> faceNum;
			}
			else if(s == "edge") {
				state = EDGE;
				ss >> edgeNum;
			}
		}
		if(s == "property") {
			switch(state) {
				case VERTEX:
					ss >> s; //contains the type, I dunno how to deal with it right now...so i won't! :D
					ss >> s; //contains the name
					if(s == "x") propV.push_back(X);
					if(s == "y") propV.push_back(Y);
					if(s == "z") propV.push_back(Z);
					if(s == "red") propV.push_back(RED);
					if(s == "green") propV.push_back(GREEN);
					if(s == "blue") propV.push_back(BLUE);
					if(s == "alpha") propV.push_back(ALPHA);
					break;
				case FACE:
					ss >> s; //the only thing i see now is list... i havent seen any other cases
					if(s == "list") {
						ss >> s; //another type; it usually seems to be uchar
						ss >> s; //another type; it usually seems to be int
						ss >> s;
						if(s == "vertex_indices") {
							propF.push_back(VI);
						}

						//very sketchy, if there's ever the occasion where we're one off, we're doomed
						//also write notes like this, they help bunches
					}
					break;
				case EDGE:
					//later
					break;
			}
		}
		if(s == "end_header") {
			double (*valueV)[7] = new double[vertexNum][7]; //note: the type should be able to vary
			int (*valueF)[3] = new int[faceNum][3]; //faces can have more than 3 vertices, change later
			//std::vector<std::vector<int>> valueV(vertexNum, 0);
			//const int FNUM = vertexNum;
			//const int FSIZE = propV.size();
			//int valueF = new int[FNUM][FSIZE];
			//int valueE[edgeNum][propE.size()];
			double temp; //note: the type should be able to vary (based on the previous input
			int index;
			int numIndices;
			for(int i = 0; i < vertexNum; i++) {
				for(int j = 0; j < propV.size(); j++) {
					ss >> temp;
					valueV[i][j] = temp;
					//std::cout << valueV[i][j] << ' ';
				}
				//std::cout << '\n';
			}
			for(int i = 0; i < faceNum; i++) {
				ss >> numIndices; 
				for(int j = 0; j < numIndices; j++) {
					ss >> index;
					valueF[i][j] = index;
					//std::cout << valueF[i][j] << ' ';
				}
				//std::cout << '\n';
			}
			
			Mesh::SubMeshGeometry smg;
			//ignore colors for now
			for(int i = 0; i < vertexNum; i++) {
				Vector3f point = Vector3f(valueV[i][0], valueV[i][1], valueV[i][2]);
				smg.positions.push_back(point);
			}
			for(int i = 0; i < faceNum; i++) {
				Mesh::SubMeshGeometry::Primitive p;
				p.primitiveType = p.TRIANGLES;
				for(int j = 0; j < numIndices; j++)
					p.indices.push_back(valueF[i][j]);
				smg.primitives.push_back(p);
			}
			std::cout << "AROUND\n";
			mdp->geometry.push_back(smg);
			std::cout << "HERE\n";
		}

		ss >> s;
	}
	return mdp;
}

Mesh::VisualPtr PlyModelSystem::load(Transfer::DenseDataPtr data) {
	if(!canLoad(data))
		return Mesh::VisualPtr();
	//data dump right here
	//std::cout << data->asString() << '\n';
	std::stringstream ss (std::stringstream::in | std::stringstream::out);
	ss << data->asString();
	String s;
	ss >> s;
	Mesh::MeshdataPtr mdp(new Mesh::Meshdata);
	
	int state = PLY;
	int vertexNum = 0, faceNum = 0, edgeNum = 0;
	std::vector<int> propV, propF, propE;
	
	while(ss) {
		if(s == "element") {
			ss >> s;
			if(s == "vertex") {
				state = VERTEX;
				ss >> vertexNum;
			}
			else if(s == "face") {
				state = FACE;
				ss >> faceNum;
			}
			else if(s == "edge") {
				state = EDGE;
				ss >> edgeNum;
			}
		}
		if(s == "property") {
			switch(state) {
				case VERTEX:
					ss >> s; //contains the type, I dunno how to deal with it right now...so i won't! :D
					ss >> s; //contains the name
					if(s == "x") propV.push_back(X);
					if(s == "y") propV.push_back(Y);
					if(s == "z") propV.push_back(Z);
					if(s == "red") propV.push_back(RED);
					if(s == "green") propV.push_back(GREEN);
					if(s == "blue") propV.push_back(BLUE);
					if(s == "alpha") propV.push_back(ALPHA);
					break;
				case FACE:
					ss >> s; //the only thing i see now is list... i havent seen any other cases
					if(s == "list") {
						ss >> s; //another type; it usually seems to be uchar
						ss >> s; //another type; it usually seems to be int
						ss >> s;
						if(s == "vertex_indices") {
							propF.push_back(VI);
						}

						//very sketchy, if there's ever the occasion where we're one off, we're doomed
						//also write notes like this, they help bunches
					}
					break;
				case EDGE:
					//later
					break;
			}
		}
		if(s == "end_header") {
			double (*valueV)[7] = new double[vertexNum][7]; //note: the type should be able to vary
			int (*valueF)[3] = new int[faceNum][3]; //faces can have more than 3 vertices, change later
			//std::vector<std::vector<int>> valueV(vertexNum, 0);
			//const int FNUM = vertexNum;
			//const int FSIZE = propV.size();
			//int valueF = new int[FNUM][FSIZE];
			//int valueE[edgeNum][propE.size()];
			double temp; //note: the type should be able to vary (based on the previous input
			int index;
			int numIndices;
			for(int i = 0; i < vertexNum; i++) {
				for(int j = 0; j < propV.size(); j++) {
					ss >> temp;
					valueV[i][j] = temp;
					//std::cout << valueV[i][j] << ' ';
				}
				//std::cout << '\n';
			}
			for(int i = 0; i < faceNum; i++) {
				ss >> numIndices; 
				for(int j = 0; j < numIndices; j++) {
					ss >> index;
					valueF[i][j] = index;
					//std::cout << valueF[i][j] << ' ';
				}
				//std::cout << '\n';
			}
			
			Mesh::SubMeshGeometry smg;
			//ignore colors for now
			for(int i = 0; i < vertexNum; i++) {
				Vector3f point = Vector3f(valueV[i][0], valueV[i][1], valueV[i][2]);
				smg.positions.push_back(point);
			}
			for(int i = 0; i < faceNum; i++) {
				Mesh::SubMeshGeometry::Primitive p;
				p.primitiveType = p.TRIANGLES;
				for(int j = 0; j < numIndices; j++)
					p.indices.push_back(valueF[i][j]);
				smg.primitives.push_back(p);
			}
			mdp->geometry.push_back(smg);
		}

		ss >> s;
	}
	


	//general ply format:
	//ply										-oh my, it's ply
	//format ascii 1.0							-format is ascii, ignore 1.0
	//comment VCGLIP generated					-ignore comments, but this thing was generated by VCGLIP, we should hope minimal comments!
	//			now, the next few will have elements, each of which is followed by various properties
	//element vertex 192						-element: vertices, there are 192 of them (a single point)
	//property float x							-properties: x, y, z
	//property float y							-all vertices here will have these three properties 
	//property float z							-note: potentially more if one includes color, for example
	//element face 184							-element: faces, there are 184 of them (think primitive)
	//property list uchar int vertex_indices	-property: LIST of INTs, VERTEX_INDICES (contains UCHARs)
	//end_header								-oooh, almost forgot about this. NOW, it's the numbers.
	//-6061.9 -2620.84 565.545					-first vertex: note it has three values, x, y, and z
	//-6061.62 -2659.95 565.545					-second vertex
	//...										-and so on, until there are 192 vertices
	//3 0 1 2									-first face: note it consists of 3 indices: 0, 1, and 2
	//3 1 0 3									-second face
	//...										-and so on, until there are 184 faces
	//
	//it seems as though... we only care about elements, properties, and their following values
	//comments can be an issue if the person writes element or property there
	//in particular, the way this program is set up, we can't detect if something is part of a comment or not
	//
	//idea:
	//-move through the stringstream, nothing when we hit element
	//-we store stuff at that point until we reach property
	//-we store stuff at that point until we reach another property, then we move on
	//-two cases: we either hit another element or we reach numbers
	//	-if we reach another element, we may continue our above process
	//	-otherwise, since we conveniently stored the elements/properties in order, we can
	//	 store vertices and vertex_indices

	//Mesh::MeshdataPtr mdp(*md);

    return mdp;
}

bool PlyModelSystem::convertVisual(const Mesh::VisualPtr& visual, const String& format, std::ostream& vout) {
    NOT_IMPLEMENTED(mesh-ply);
    return false;
}

bool PlyModelSystem::convertVisual(const Mesh::VisualPtr& visual, const String& format, const String& filename) {
    NOT_IMPLEMENTED(mesh-ply);
    return false;
}
} // namespace Sirikata
