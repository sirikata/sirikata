// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "PlyModelSystem.hpp"
#include <sirikata/mesh/Meshdata.hpp>
#include <sstream>
#include <vector>

//primitive type (edge is rarely used)
enum {VERTEX, FACE, EDGE};
//property type
enum {X, Y, Z, RED, GREEN, BLUE, ALPHA, VI, TC, FILLER};
//texture type
enum {NONE, COLOR, TEXTURE};
const int BADCOLOR = 256;


namespace Sirikata {

PlyModelSystem::PlyModelSystem() {
}

PlyModelSystem::~PlyModelSystem () {
}

bool PlyModelSystem::canLoad(Transfer::DenseDataPtr data) {
    //Minimal check that only sees if "ply" and "ascii"
	//are somewhere near the start of the file, which is
	//a decent test for detecting ply files.
	if (!data) return false;

	//create string out of first 1K
	int32 sublen = std::min((int)data->length(), (int)1024);
    std::string subset((const char*)data->begin(), (std::size_t)sublen);

    if (subset.find("ply") != subset.npos
		&& subset.find("ascii") != subset.npos)
        return true;

    return false;
}

//should be moved into the filter plugin
bool comp(Vector3f point1, Vector3f point2) {
	if(point1.x == point2.x) {
		if(point1.y == point2.y) {
			return point1.z < point2.z;
		} else return point1.y < point2.y;
	} else return point1.x < point2.x;
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
	
	int state = FILLER;
	int vertexNum = 0, faceNum = 0, edgeNum = 0;
	std::vector<int> propV, propF, propE;
	
	bool loop = true;
	std::vector<String> file;
	while(ss && loop) {
		if(s == "TextureFile") {
			ss >> s;
			file.push_back(s);
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
			loop = false;
			double (*vert)[3] = new double[vertexNum][3]; //note: the type should be able to vary (but does it make a significant difference...?)
			double (*col)[4] = new double[vertexNum][4]; //color
			for(int i = 0; i < vertexNum; i++)
				for(int j = 0; j < 4; j++)
					col[i][j] = BADCOLOR;
			int (*face)[3] = new int[faceNum][3]; //faces can have more than 3 vertices, change later
			std::map<int, int> indexMap; //will map the original index to the new index
			std::vector<std::map<int, int> > reverseMap; //will map the new index to the original index (find more efficient way later)
			std::vector<int> counterIndex; //maybe reverseMap can replace counter
			for(int i = 0; i < vertexNum; i++)
				indexMap[i] = -1;
			double (*tc)[3] = new double[vertexNum][3]; //stores the texture coordinates in 0 and 1, and the texture file num in 2
			for(int i = 0; i < vertexNum; i++)
				for(int j = 0; j < 3; j++)
					tc[i][j] = -1;
			double temp; //note: the type should be able to vary (based on the previous input)
			int index;
			int numIndices;
			int numTexCoords;
			//obtain vertex data
			int lol = 0;
			for(int i = 0; i < vertexNum; i++) {
				for(int j = 0; j < propV.size(); j++) {
					ss >> temp;
					switch(propV[j]) {
						case X: vert[i][0] = temp; break;
						case Y: vert[i][1] = temp; break;
						case Z: vert[i][2] = temp; break;
						case RED: col[i][0] = temp; break;
						case GREEN: col[i][1] = temp; break;
						case BLUE: col[i][2] = temp;  break;
						case ALPHA: col[i][3] = temp; break;
					}
					//std::cout << vert[i][j] << ' ';
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
							for(int k = 0; k < numIndices; k++) {
								ss >> face[i][k];
							}
							break;
						case TC:
							ss >> numTexCoords;
							//(note this only applies if the filesize is 1, due to how annoying ply files are with only one texture)
							//reasoning here: if the tc hasn't been initilized (it's still -1) then we store the tex in it...
							//but if it already has a value in it, and it doesn't equal tex, then it's an invalid tc and we
							//store an invalid value - file.size();
							for(int k = 0; k < numTexCoords / 2; k++) {
								double temp1, temp2;
								temp1 = tc[face[i][k]][0];
								ss >> tc[face[i][k]][0];
								temp2 = tc[face[i][k]][1];
								ss >> tc[face[i][k]][1];
								
								if(file.size() == 1) {
									if((tc[face[i][k]][0] != temp1 && temp1 != -1) &&
										(tc[face[i][k]][1] != temp2 && temp2 != -1))
										tc[face[i][k]][2] = file.size();
									else
										tc[face[i][k]][2] = 0;
								}
								
							}
							if(file.size() > 1) {
								int numTex;
								ss >> numTex;
								for(int k = 0; k < numTexCoords / 2; k++) {
									tc[face[i][k]][2] = numTex;
									//we set each of the col values to -abs(texNum) so that it won't be mistaken as a normal colored point
									if(col[face[i][k]][0] == BADCOLOR) {
										col[face[i][k]][0] = -abs(numTex);
										col[face[i][k]][1] = -abs(numTex);
										col[face[i][k]][2] = -abs(numTex);
										col[face[i][k]][3] = -abs(numTex);
									}
								}
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
					Vector3f point = Vector3f(vert[i][0], vert[i][1], vert[i][2]);
					mdp->geometry[0].positions.push_back(point);
				}
			} else {
				bool quick = false;
				if(quick) { //millions and millions of times faster! it's beautiful!
					mdp->geometry.push_back(smg);
					gi.geometryIndex = 0;
					mdp->instances.push_back(gi);
					for(int i = 0; i < vertexNum; i++)
						mdp->geometry[0].positions.push_back(Vector3f(vert[i][0], vert[i][1], vert[i][2]));
					for(int i = 0; i < faceNum; i++)
						for(int j = 0; j < 3; j++)
							mdp->geometry[0].primitives[0].indices.push_back(face[i][j]);
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
						bool sameSMG = false;
						while(counterSMG < mdp->geometry.size()) {
							for(int j = 0; j < 3; j++) {
								//iterate through the indices
								for(int k = 0; k < mdp->geometry[counterSMG].primitives.size(); k++) {
									for(int l = 0; l < mdp->geometry[counterSMG].primitives[k].indices.size(); l++) {
										//we must check if the point is the same and also if the texture is the same
										//something is wrong here! reverseMap is creating new entries (and that shouldn't happen!)
										if(reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]] == face[i][j] &&
											((col[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][1] == col[face[i][j]][1] &&
											col[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][2] == col[face[i][j]][2] &&
											col[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][3] == col[face[i][j]][3]) ||
											(abs(tc[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][2]) > file.size() && 
											abs(tc[face[i][j]][2]) > file.size() && file.size() > 0))) {
										//if(vert[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][0] == vert[face[i][j]][0] && 
										//	vert[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][1] == vert[face[i][j]][1] && 
										//	vert[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][2] == vert[face[i][j]][2] &&
										//	((col[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][0] == col[face[i][j]][0] &&
										//	col[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][1] == col[face[i][j]][1] &&
										//	col[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][2] == col[face[i][j]][2] &&
										//	col[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][3] == col[face[i][j]][3]) ||
										//	(abs(tc[reverseMap[counterSMG][mdp->geometry[counterSMG].primitives[k].indices[l]]][2]) > file.size() && 
										//	abs(tc[face[i][j]][2]) > file.size() && file.size() > 0))) {
												sameSMG = true;
												bool isThere = false;
												for(int m = 0; m < hitSMG.size(); m++)
													if(hitSMG[m] == counterSMG) isThere = true; //is there a better way of doing this?
												if(!isThere) hitSMG.push_back(counterSMG);

												
										}
									}
								}
								
							}
							counterSMG++;
						}
						//add to the primitive if the point is the same
						if(sameSMG) {
							//if the point hit multiple SMG's, we will have to merge them before adding a point
							while(hitSMG.size() > 1) {
								//First, submeshgeometry
								for(int j = 0; j < mdp->geometry[hitSMG[1]].primitives[0].indices.size(); j++) {//we loop through the indices of the smg to be destroyed
									int ind = reverseMap[hitSMG[0]].size();										//index for increasing the reverseMap size...
									Vector3f point = Vector3f(vert[reverseMap[hitSMG[1]][mdp->geometry[hitSMG[1]].primitives[0].indices[j]]][0],				//here's a point from the smg to be transferred, at location j
										vert[reverseMap[hitSMG[1]][mdp->geometry[hitSMG[1]].primitives[0].indices[j]]][1],
										vert[reverseMap[hitSMG[1]][mdp->geometry[hitSMG[1]].primitives[0].indices[j]]][2]);
									bool addPoint = true;
									for(int k = 0; k < mdp->geometry[hitSMG[0]].positions.size() && addPoint; k++) {	//we loop through the other points in the smg to be stuffed
										if(point.x == mdp->geometry[hitSMG[0]].positions[k].x &&						//and try to see if there is a clone. if there is a clone,
											point.y == mdp->geometry[hitSMG[0]].positions[k].y &&						//we don't add a point? but if there is, we do add a point?
											point.z == mdp->geometry[hitSMG[0]].positions[k].z)
											addPoint = false;
									}
									if(addPoint) {
										mdp->geometry[hitSMG[0]].positions.push_back(point);					//since we need to add a point, we store the point in the smg...
										indexMap[reverseMap[hitSMG[1]][mdp->geometry[hitSMG[1]].primitives[0].indices[j]]] = ind;								//we map the old point to the new point...
										reverseMap[hitSMG[0]][ind] = reverseMap[hitSMG[1]][mdp->geometry[hitSMG[1]].primitives[0].indices[j]];					//and map the new point in reverse to the old!
									} 
									mdp->geometry[hitSMG[0]].primitives[0].indices.push_back(indexMap[reverseMap[hitSMG[1]][mdp->geometry[hitSMG[1]].primitives[0].indices[j]]]);
								}
								mdp->geometry.erase(mdp->geometry.begin() + hitSMG[1]);
								//then, geometryInstance
								mdp->instances.erase(mdp->instances.begin() + hitSMG[1]);
								for(int j = hitSMG[1]; j < mdp->instances.size(); j++)
									mdp->instances[j].geometryIndex = j;
								//also, reverseMap
								reverseMap.erase(reverseMap.begin() + hitSMG[1]);
								//finally, hitSMG
								hitSMG.erase(hitSMG.begin() + 1);
								for(int i = 1; i < hitSMG.size(); i++) hitSMG[i]--;//since we removed a hitSMG we have to lower the index of the other hits by one
							}
							for(int j = 0; j < 3; j++) {
								//we put the point in
								Vector3f point = Vector3f(vert[face[i][j]][0], vert[face[i][j]][1], vert[face[i][j]][2]);
								int ind = reverseMap[hitSMG[0]].size();

								if(indexMap[face[i][j]] == -1) {
									mdp->geometry[hitSMG[0]].positions.push_back(point);
									indexMap[face[i][j]] = ind;
									reverseMap[hitSMG[0]][ind] = face[i][j];
								}

								mdp->geometry[hitSMG[0]].primitives[0].indices.push_back(indexMap[face[i][j]]);
									
							}
						} else {
							//otherwise, we'll make a new submeshgeometry/geometry set
							mdp->geometry.push_back(smg);
							gi.geometryIndex = counterSMG;
							mdp->instances.push_back(gi);
							reverseMap.push_back(std::map<int, int>());
							for(int j = 0; j < 3; j++) {
								int ind = reverseMap[counterSMG].size();

								Vector3f point = Vector3f(vert[face[i][j]][0], vert[face[i][j]][1], vert[face[i][j]][2]);
								mdp->geometry[counterSMG].positions.push_back(point);

								indexMap[face[i][j]] = ind;
								reverseMap[counterSMG][ind] = face[i][j];

								mdp->geometry[counterSMG].primitives[0].indices.push_back(indexMap[face[i][j]]);
							}
						}
					}
				}
				
			}


			//int fileCounter = 0;
			int mapCounter = 0;
			for(int i = 0; i < mdp->geometry.size(); i++) {
				//one texture set for the geometry
				if(tc[0][0] > 0) {
					Mesh::SubMeshGeometry::TextureSet ts;
					ts.stride = 2;
					for(int j = 0; j < mdp->geometry[i].positions.size(); j++) {
						for(int k = 0; k < 2; k++)  {
							ts.uvs.push_back(tc[reverseMap[i][j]][k]);
						}
					}
					mdp->geometry[i].texUVs.push_back(ts);
				}
				Mesh::MaterialEffectInfo mei;
				//add material
				if(faceNum > 0) {
					Mesh::MaterialEffectInfo::Texture t;
					bool hasTexture = false;
					for(int j = 0; j < mdp->geometry[i].primitives[0].indices.size() && !hasTexture; j++) {
						if((abs(tc[reverseMap[i][mdp->geometry[i].primitives[0].indices[j]]][0]) > .001 &&
							abs(tc[reverseMap[i][mdp->geometry[i].primitives[0].indices[j]]][0]) < INT_MAX) ||
							(abs(tc[reverseMap[i][mdp->geometry[i].primitives[0].indices[j]]][1]) > .001 &&
							abs(tc[reverseMap[i][mdp->geometry[i].primitives[0].indices[j]]][1]) < INT_MAX))
							hasTexture = true;
					}
					//if the texCoords are all almost zero (or are infinite), then this is not a truly texturized geometry

					if(hasTexture && tc[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][2] >= 0
						&& tc[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][2] < file.size()) {						

						//we add the texture and typical values
						mei.shininess = -128;
						mei.reflectivity = -1;
						bool newFile = true;
						for(int l = 0; l < mdp->textures.size(); l++)
							if(mdp->textures[l] == file[tc[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][2]])
								newFile = false;
						if(newFile)
							mdp->textures.push_back(file[tc[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][2]]);
						t.uri = file[tc[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][2]];
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

					} else {
						//we add a color
						Vector4f c;
						if(mdp->geometry[i].primitives[0].primitiveType == Mesh::SubMeshGeometry::Primitive::TRIANGLES && 
							col[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][0] >= 0) { //we might need a better check here if we get certain files
							c = Vector4f(col[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][0] / 255.0,
								col[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][1] / 255.0,
								col[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][2] / 255.0,
								col[reverseMap[i][mdp->geometry[i].primitives[0].indices[0]]][3] / 255.0);
							t.color = c;
						}
						else t.color = Vector4f(1, 1, 1, 1); //default color is white
						t.affecting = t.DIFFUSE;

					}
					//we try to see if there is a clone of the material
					mei.textures.push_back(t);
					bool addMat = true;
					int matPos = -1;
					for(int j = 0; j < mdp->materials.size(); j++)
						if(mdp->materials[j] == mei) {
							addMat = false;
							matPos = j;
						}
					mdp->geometry[i].primitives[0].materialId = 1;
					//if there is not, we add it, otherwise we do not
					if(addMat) {		
						mdp->instances[i].materialBindingMap[mdp->geometry[i].primitives[0].materialId] = mapCounter;
						mapCounter++;
						mdp->materials.push_back(mei);
					} else
						mdp->instances[i].materialBindingMap[mdp->geometry[i].primitives[0].materialId] = matPos;

				} else mdp->materials.push_back(mei);
			}
			
			delete vert;
			delete col;
			delete face;
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