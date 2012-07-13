// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "PlyModelSystem.hpp"
#include <sirikata/mesh/Meshdata.hpp>
#include <sstream>
#include <vector>

namespace Sirikata {

//too many states, it should be cut down later
enum {PLY, VERTEX, FACE, EDGE, X, Y, Z, RED, GREEN, BLUE, ALPHA, VI, TC, FILLER};


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

	//stringstream to read data
	std::stringstream ss (std::stringstream::in | std::stringstream::out);
	ss << data->asString();
	String s;
	ss >> s;

	Mesh::MeshdataPtr mdp(new Mesh::Meshdata);
	
	int state = PLY;
	int vertexNum = 0, faceNum = 0, edgeNum = 0;
	std::vector<int> propV, propF, propE;
	
	bool loop = true;
	String file;
	while(ss && loop) {
		if(s == "TextureFile") {
			ss >> file;
		}
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
					ss >> s; //contains the type, generally it is a float
					ss >> s; //contains the name
					if(s == "x") propV.push_back(X);
					else if(s == "y") propV.push_back(Y);
					else if(s == "z") propV.push_back(Z);
					else if(s == "red") propV.push_back(RED);
					else if(s == "green") propV.push_back(GREEN);
					else if(s == "blue") propV.push_back(BLUE);
					else if(s == "alpha") propV.push_back(ALPHA);
					else propV.push_back(FILLER);
					//doing this allows us to collect vertex information
					//even if the data is out of order, for example if
					//colors somehow appeared before points
					break;
				case FACE:
					ss >> s; //For now, I've only seen "list"
					if(s == "list") {
						ss >> s; //another type; it usually seems to be uchar
						ss >> s; //another type; it usually seems to be int
						ss >> s;
						if(s == "vertex_indices") propF.push_back(VI);
						if(s == "texcoord") propF.push_back(TC);

						//there could be the possibility that the program is one string ahead
						//or behind, causing errors
					}
					break;
				case EDGE:
					//later
					break;
			}
		}
		if(s == "end_header") {
			if(vertexNum == 0) return Mesh::VisualPtr(); //no vertices means an empty ply file
			bool hasColor = false;
			loop = false;
			double (*valueV)[7] = new double[vertexNum][7]; //note: the type should be able to vary (but does it make a significant difference...?)
			int (*valueF)[3] = new int[faceNum][3]; //faces can have more than 3 vertices, change later
			int (*valueE) = new int[2];
			double (*tc)[2] = new double[vertexNum][2]; //stores the texture coordinates
			//it's separate from the vertex data because it was stuck with the FACE data for some reason...
			double temp; //note: the type should be able to vary (based on the previous input)
			int index;
			int numIndices;
			int numTexCoords;
			//obtain vertex data
			for(int i = 0; i < vertexNum; i++) {
				for(int j = 0; j < propV.size(); j++) {
					ss >> temp;
					switch(propV[j]) {
						case X: valueV[i][0] = temp; break;
						case Y: valueV[i][1] = temp; break;
						case Z: valueV[i][2] = temp; break;
						case RED: valueV[i][3] = temp; hasColor = true; break;
						case GREEN: valueV[i][4] = temp; hasColor = true; break;
						case BLUE: valueV[i][5] = temp; hasColor = true; break;
						case ALPHA: valueV[i][6] = temp; hasColor = true; break;
					}
					//std::cout << valueV[i][j] << ' ';
				}
				//std::cout << '\n';
			}
			//obtain face data
			for(int i = 0; i < faceNum; i++) {
				ss >> numIndices;
				//int (*temp) = new int[numIndices];
				for(int j = 0; j < numIndices; j++) {
					ss >> index;
					valueF[i][j] = index;
				}
				if(propF.size() > 1) {
					ss >> numTexCoords; //doesn't do anything right now
					for(int j = 0; j < numIndices; j++) {
						ss >> tc[valueF[i][j]][0];
						ss >> tc[valueF[i][j]][1];
					}
				}
			}
			//if there are no faces, then add a line (an edge, here)
			if(faceNum == 0) {
				for(int i = 0; i < 2; i++) {
					ss >> index;
					valueE[i] = index;
				}
			}
			//one node
			Mesh::Node nod;
			nod.parent = Mesh::NullNodeIndex;
			mdp->nodes.push_back(nod);
			mdp->rootNodes.push_back(0);
			
			Mesh::SubMeshGeometry smg;
			//make a primitive
			Mesh::SubMeshGeometry::Primitive p;
			if(vertexNum >= 3) p.primitiveType = Mesh::SubMeshGeometry::Primitive::TRIANGLES;
			else p.primitiveType = Mesh::SubMeshGeometry::Primitive::LINES;
			smg.primitives.push_back(p);
			mdp->geometry.push_back(smg);
			//add a geometryinstance for every smg
			Mesh::GeometryInstance gi;
			gi.geometryIndex = 0;
			gi.parentNode = 0;
			mdp->instances.push_back(gi);
			
			//no faces means that there could be just a line
			if(faceNum == 0)
				for(int i = 0; i < 2; i++)
					mdp->geometry[0].primitives[0].indices.push_back(valueE[i]);
			else {
				//we have one set of indices to start off...
				for(int j = 0; j < 3; j++) {
					mdp->geometry[0].primitives[0].indices.push_back(valueF[0][j]);
				}
				//then for each next set of indices, we first see
				//if a previous primitive has either shared an 
				//index or shared a point. If yes, we may add to
				//the same primitive, or we may make a new
				//submeshgeometry.
				//LATER: primitives should be differentiated 
				//based on material/texture.
				for(int i = 1; i < faceNum; i++) {
					int counterSMG = 0;
					bool samePrim = false;
					while(!samePrim && counterSMG < mdp->geometry.size()) {
						for(int j = 0; j < 3 && !samePrim; j++) {
							//iterate through the indices
							//counterPrim = 0;
							for(std::vector<Mesh::SubMeshGeometry::Primitive>::iterator it = mdp->geometry[counterSMG].primitives.begin();
								it < mdp->geometry[counterSMG].primitives.end() && !samePrim;
								it++) {
								for(std::vector<unsigned short>::iterator iter = it->indices.begin();
									iter < it->indices.end() && !samePrim;
									iter++) {
									if(*iter == valueF[i][j])
										samePrim = true;
									if(valueV[*iter][0] == valueV[valueF[i][j]][0] && 
										valueV[*iter][1] == valueV[valueF[i][j]][1] &&
										valueV[*iter][2] == valueV[valueF[i][j]][2]) {
										samePrim = true;
										//hitSMG = counterSMG;
									}
								}
								//if(!samePrim) counterPrim++;
							}
							
							
						}
						if(!samePrim) counterSMG++;
					}
					//add to the primitive if the index or the point is the same
					if(samePrim) {
						for(int j = 0; j < 3; j++) 
							mdp->geometry[counterSMG].primitives[0].indices.push_back(valueF[i][j]);
					} else {
						//otherwise, we'll make a new submeshgeometry/geometry set
						mdp->geometry.push_back(smg);
						gi.geometryIndex = counterSMG;
						mdp->instances.push_back(gi);
						for(int j = 0; j < 3; j++)
							mdp->geometry[counterSMG].primitives[0].indices.push_back(valueF[i][j]);
					}
				}



			}
			//note: sometimes the primitives are not sorted the way we want.
			//thus, there may be the case where two submeshgeometries should
			//be merged but aren't because the connecting point is not
			//recorded early enough.
			//we could try to merge them again... but that may take a while.

			//TextureSet created
			Mesh::SubMeshGeometry::TextureSet ts;
			ts.stride = 2;
			for(int i = 0; i < vertexNum; i++) {
				for(int j = 0; j < 2; j++)  {
					ts.uvs.push_back(tc[i][j]);
					//std::cout << tc[i][j];
				}
			}
			mdp->geometry[0].texUVs.push_back(ts);

			//color: currently just takes the average of the colors and sets
			//the color of the figure to this average color
			int sumRed = 0, sumGreen = 0, sumBlue = 0, sumAlpha = 0;
			for(int i = 0; i < vertexNum; i++) {
				Vector3f point = Vector3f(valueV[i][0], valueV[i][1], valueV[i][2]);
				if(valueV[i][3] >= 0) sumRed += valueV[i][3];
				if(valueV[i][4] >= 0) sumGreen += valueV[i][4];
				if(valueV[i][5] >= 0) sumBlue += valueV[i][5];
				if(valueV[i][6] >= 0) sumAlpha += valueV[i][6];
				mdp->geometry[0].positions.push_back(point);
			}
			
			//we will have to do this in the particular order of the geometries
			//textures seem to be out of my reach (D:) for now...

			//add material
			Mesh::MaterialEffectInfo mei;
			Mesh::MaterialEffectInfo::Texture t;
			if(!file.empty()) {
				//the textured material path!
				mei.shininess = -128;
				mei.reflectivity = -1;
				mdp->textures.push_back(file);
				t.uri = file;
				t.texCoord = 0;
				t.affecting = t.DIFFUSE;
				t.samplerType = t.SAMPLER_TYPE_2D;
				t.minFilter = t.SAMPLER_FILTER_LINEAR;
				t.magFilter = t.SAMPLER_FILTER_LINEAR;
				t.wrapS = t.WRAP_MODE_WRAP;
				t.wrapT = t.WRAP_MODE_WRAP;
				t.wrapU = t.WRAP_MODE_WRAP;
				t.maxMipLevel = 255;
				t.mipBias = 0;

				mdp->uri = metadata.getURI().toString();
				
				//mdp->geometry[0].textures
				mdp->geometry[0].primitives[0].materialId = 1;
				mdp->instances[0].materialBindingMap.insert(std::pair<int, int> (1, 0));

			} else {
				Vector4f c;
				if(vertexNum > 0) c = Vector4f(sumRed / vertexNum / 255.0, sumGreen / vertexNum / 255.0, sumBlue / vertexNum / 255.0, sumAlpha / vertexNum / 255.0)
				if(hasColor) {
					t.color = c;
					t.affecting = t.DIFFUSE;
				}
			}
			if(faceNum > 0) mei.textures.push_back(t);
			mdp->materials.push_back(mei);
	
			delete valueV;
			delete valueF;
			delete valueE;
			delete tc;
		}

		ss >> s;
	}

    return mdp;
}

Mesh::VisualPtr PlyModelSystem::load(Transfer::DenseDataPtr data) {
	return Mesh::VisualPtr();
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
