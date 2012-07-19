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
			std::map<int, int> indexMap; //will map the original index to the new index
			std::vector<std::map<int, int> > reverseMap; //will map the new index to the original index (find more efficient way later)
			std::vector<int> counterIndex; //maybe reverseMap can replace counter
			for(int i = 0; i < vertexNum; i++)
				indexMap[i] = -1;
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
				for(int j = 0; j < propF.size(); j++) {
					switch(propF[j]) {
						case VI:
							ss >> numIndices;
							//int (*temp) = new int[numIndices];
							for(int j = 0; j < numIndices; j++) {
								ss >> index;
								valueF[i][j] = index;
							}
							break;
						case TC:
							ss >> numTexCoords;
							for(int j = 0; j < numTexCoords / 2; j++) {
								ss >> tc[valueF[i][j]][0];
								ss >> tc[valueF[i][j]][1];
							}
							break;
					}
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

			//add a geometryinstance for every smg
			Mesh::GeometryInstance gi;
			gi.geometryIndex = 0;
			gi.parentNode = 0;
			
			//color: currently just takes the average of the colors and sets
			//the color of the figure to this average color
			int sumRed = 0, sumGreen = 0, sumBlue = 0, sumAlpha = 0;


			//no faces means that there could be just a line
			if(faceNum == 0) {
				mdp->geometry.push_back(smg);
				mdp->instances.push_back(gi);
				for(int i = 0; i < 2; i++) {
					mdp->geometry[0].primitives[0].indices.push_back(i);
					Vector3f point = Vector3f(valueV[i][0], valueV[i][1], valueV[i][2]);
					mdp->geometry[0].positions.push_back(point);
				}
			} else {
				//for each next set of indices, we first see
				//if a previous primitive has either shared an 
				//index or shared a point. If yes, we may add
				//to the same primitive, or we may make a new
				//submeshgeometry.
				//LATER: primitives should be differentiated 
				//based on material/texture.
				for(int i = 0; i < faceNum; i++) {
					int counterSMG = 0;
					std::vector<int> hitSMG;
					int markSMG = -1;
					bool sameSMG = false;
					while(counterSMG < mdp->geometry.size()) {
						for(int j = 0; j < 3; j++) {
							//iterate through the indices
							for(int k = 0; k < mdp->geometry[counterSMG].primitives.size(); k++) {
								for(int l = 0; l < mdp->geometry[counterSMG].primitives[k].indices.size(); l++) {
									if(reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]] == valueF[i][j])
										sameSMG = true;
									if(valueV[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][0] == valueV[valueF[i][j]][0] && 
										valueV[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][1] == valueV[valueF[i][j]][1] && 
										valueV[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][2] == valueV[valueF[i][j]][2]) {
											sameSMG = true;
											bool isThere = false;
											for(int m = 0; m < hitSMG.size(); m++)
												if(hitSMG[m] == counterSMG) isThere = true;
											if(!isThere) hitSMG.push_back(counterSMG);
											if(markSMG == -1) markSMG = counterSMG;
									}
								}
							}
							
						}
						counterSMG++;
					}
					//add to the primitive if the index or the point is the same
					if(sameSMG) {
						//if the point hit multiple SMG's, we will have to merge them before adding a point
						while(hitSMG.size() > 1) {
							//First, submeshgeometry
							for(int j = 0; j < mdp->geometry[hitSMG[1]].primitives[0].indices.size(); j++) {//we loop through the indices of the smg to be destroyed
								int ind = reverseMap[markSMG].size();										//prepare index for boosting the reverseMap size...
								Vector3f point = Vector3f(valueV[reverseMap[hitSMG[1]][mdp->geometry[hitSMG[1]].primitives[0].indices[j]]][0],				//here's a point from the smg to be transferred, at location j
									valueV[reverseMap[hitSMG[1]][mdp->geometry[hitSMG[1]].primitives[0].indices[j]]][1],
									valueV[reverseMap[hitSMG[1]][mdp->geometry[hitSMG[1]].primitives[0].indices[j]]][2]);
								bool addPoint = true;
								int pos;
								for(pos = 0; pos < mdp->geometry[markSMG].positions.size() && addPoint; pos++) {//we loop through the other points in the smg to be stuffed
									if(point.x == mdp->geometry[markSMG].positions[pos].x &&					//and try to see if there is a clone. if there is a clone,
										point.y == mdp->geometry[markSMG].positions[pos].y &&					//we don't add a point? but if there is, we do add a point?
										point.z == mdp->geometry[markSMG].positions[pos].z)
										addPoint = false;
								}
								if(addPoint) {
									mdp->geometry[markSMG].positions.push_back(point);						//since we need to add a point, we store the point in the smg...
									indexMap[reverseMap[hitSMG[1]][j]] = ind;								//we map the old point to the new point...
									reverseMap[markSMG][ind] = reverseMap[hitSMG[1]][j];					//and map the new point in reverse to the old!
								} else {
									indexMap[reverseMap[hitSMG[1]][j]] = pos - 1;							//here, a clone already exists... so we map the old point to the clone (pos - 1 due to for loop)
								}

								mdp->geometry[markSMG].primitives[0].indices.push_back(indexMap[reverseMap[hitSMG[1]][j]]);
							}
							mdp->geometry.erase(mdp->geometry.begin() + hitSMG[1]);
							//then, geometryInstance
							mdp->instances.erase(mdp->instances.begin() + hitSMG[1]);
							for(int j = hitSMG[1]; j < mdp->instances.size(); j++)
								mdp->instances[j].geometryIndex = j;
							//also, reverseMap
							reverseMap.erase(reverseMap.begin() + hitSMG[1]);
							hitSMG.erase(hitSMG.begin() + hitSMG[1]);
						}
						for(int j = 0; j < 3; j++) {
							int ind = reverseMap[markSMG].size();
							if(indexMap[valueF[i][j]] == -1) {
								Vector3f point = Vector3f(valueV[valueF[i][j]][0], valueV[valueF[i][j]][1], valueV[valueF[i][j]][2]);
								mdp->geometry[markSMG].positions.push_back(point);

								indexMap[valueF[i][j]] = ind;
								reverseMap[markSMG][ind] = valueF[i][j];
							}
							mdp->geometry[markSMG].primitives[0].indices.push_back(indexMap[valueF[i][j]]);
							
						}
					} else {
						//otherwise, we'll make a new submeshgeometry/geometry set
						mdp->geometry.push_back(smg);
						gi.geometryIndex = counterSMG;
						mdp->instances.push_back(gi);
						reverseMap.push_back(std::map<int, int>());
						for(int j = 0; j < 3; j++) {
							int ind = reverseMap[counterSMG].size();

							Vector3f point = Vector3f(valueV[valueF[i][j]][0], valueV[valueF[i][j]][1], valueV[valueF[i][j]][2]);
							mdp->geometry[counterSMG].positions.push_back(point);

							indexMap[valueF[i][j]] = ind;
							reverseMap[counterSMG][ind] = valueF[i][j];

							mdp->geometry[counterSMG].primitives[0].indices.push_back(indexMap[valueF[i][j]]);
						}
					}
				}

			}

			for(int i = 0; i < vertexNum; i++) {
				if(valueV[i][3] >= 0) sumRed += valueV[i][3];
				if(valueV[i][4] >= 0) sumGreen += valueV[i][4];
				if(valueV[i][5] >= 0) sumBlue += valueV[i][5];
				if(valueV[i][6] >= 0) sumAlpha += valueV[i][6];
			}

			//add material
			Mesh::MaterialEffectInfo mei;
			Mesh::MaterialEffectInfo::Texture t;
			if(!file.empty()) {
				//TextureSet 
				Mesh::SubMeshGeometry::TextureSet ts;
				ts.stride = 2;
				for(int i = 0; i < vertexNum; i++) {
					for(int j = 0; j < 2; j++)  {
						ts.uvs.push_back(tc[i][j]);
						//std::cout << tc[i][j];
					}
				}
				mdp->geometry[0].texUVs.push_back(ts);

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
				
				mdp->geometry[0].primitives[0].materialId = 1;
				mdp->instances[0].materialBindingMap.insert(std::pair<int, int> (1, 0));

			} else {
				Vector4f c;
				if(vertexNum > 0) c = Vector4f(sumRed / vertexNum / 255.0, sumGreen / vertexNum / 255.0, sumBlue / vertexNum / 255.0, sumAlpha / vertexNum / 255.0);
				if(hasColor) {
					t.color = c;
					t.affecting = t.DIFFUSE;
				}
			}
			if(faceNum > 0) mei.textures.push_back(t);
			mdp->materials.push_back(mei);
	
			delete valueV;
			delete valueF;
			delete tc;
		}

		ss >> s;
	}
    return mdp;
}

Mesh::VisualPtr PlyModelSystem::load(Transfer::DenseDataPtr data) {
	Transfer::RemoteFileMetadata rfm(Transfer::Fingerprint(), Transfer::URI(), 0, Transfer::ChunkList(), Transfer::FileHeaders());
	return load(rfm, Transfer::Fingerprint(), data);
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
