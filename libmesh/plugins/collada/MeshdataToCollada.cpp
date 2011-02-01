#include "MeshdataToCollada.hpp"

#include "COLLADASWStreamWriter.h"

#include "COLLADASWAsset.h"
#include "COLLADASWScene.h"
#include "COLLADASWLibrary.h"
#include "COLLADASWLibraryMaterials.h"
#include "COLLADASWLibraryEffects.h"
#include "COLLADASWLibraryGeometries.h"
#include "COLLADASWLibraryLights.h"
#include "COLLADASWEffectProfile.h"
#include "COLLADASWSource.h"
#include "COLLADASWVertices.h"
#include "COLLADASWInputList.h"
#include "COLLADASWPrimitves.h"
#include "COLLADASWLibraryVisualScenes.h"
#include "COLLADASWNode.h"
#include "COLLADASWInstanceNode.h"
#include "COLLADASWInstanceGeometry.h"
#include "COLLADASWInstanceLight.h"
#include "COLLADASWBindMaterial.h"
#include "COLLADASWInstanceMaterial.h"
#include "COLLADASWLibraryImages.h"
#include "COLLADASWColorOrTexture.h"

#include <boost/lexical_cast.hpp>



namespace Sirikata {

using namespace Mesh;

  std::string removeNonAlphaNumeric(std::string str) {
    for (uint32 i = 0; i < str.size(); i++) {
      char ch = str[i];
      if ( !isalnum(ch)) {
        str[i]='_';
      }
    }

    return str;
  }


  void exportAsset(COLLADASW::StreamWriter*  streamWriter, const Meshdata& meshdata) {
    COLLADASW::Asset asset ( streamWriter );

    // Add contributor information
    // Set the author
    std::string userName = "MeruSpace_ObjectAggregator";

    asset.getContributor().mAuthor = std::string ( userName );

    asset.getContributor().mAuthoringTool = "OpenCOLLADA_StreamWriter";

    //TODO: do we only use meters as our units?
    asset.setUnit ( "meters", 1.0 );

    asset.setUpAxisType ( COLLADASW::Asset::UpAxisType(COLLADASW::Asset::Y_UP) );

    asset.add();

  }


  class MaterialExporter : public COLLADASW::LibraryMaterials {

  public:

    MaterialExporter(COLLADASW::StreamWriter*  streamWriter) :
      COLLADASW::LibraryMaterials ( streamWriter )
    {

    }

    void exportMaterial(const Meshdata& meshdata, std::map<std::string, int>& textureURIToEffectIndexMap, std::map<int, int>& materialRedirectionMap) {
      openLibrary();

      for (uint32 i=0; i < meshdata.materials.size(); i++) {
        //FIXME: this assumes all the texture URIs are the same in materials[i].textures
        const MaterialEffectInfo::Texture& texture = meshdata.materials[i].textures[0];
        if (textureURIToEffectIndexMap.find(texture.uri) != textureURIToEffectIndexMap.end() &&
            textureURIToEffectIndexMap[texture.uri] != i
           )
          {
            materialRedirectionMap[i] = textureURIToEffectIndexMap[texture.uri];
            continue;
          }

        if (texture.uri == "") {
          char colorEncodingStr[256];
          snprintf(colorEncodingStr, 256, "%f %f %f %f %d", texture.color.x, texture.color.y,
                   texture.color.z, texture.color.w, texture.affecting);
          String colorEncoding = colorEncodingStr;

          if (textureURIToEffectIndexMap.find(colorEncoding) != textureURIToEffectIndexMap.end() &&
              textureURIToEffectIndexMap[colorEncoding] != i
              )
            {
              materialRedirectionMap[i] = textureURIToEffectIndexMap[colorEncoding];
              continue;
            }
        }


        materialRedirectionMap[i] = i;
        char effectNameStr[256];
        snprintf(effectNameStr, 256, "material%d", i);
        std::string effectName = effectNameStr;
        std::string materialName = effectName + "ID";
        openMaterial(materialName, COLLADABU::Utils::checkNCName(materialName) );

        addInstanceEffect("#" + effectName + "-effect");

        closeMaterial();
      }

      closeLibrary();
    }
  };

  class EffectExporter : public COLLADASW::LibraryEffects {
    public:
      EffectExporter(COLLADASW::StreamWriter*  streamWriter) :
        COLLADASW::LibraryEffects ( streamWriter )
      {
      }

    void exportEffect(COLLADASW::StreamWriter*  streamWriter, const Meshdata& meshdata, std::map<String,int>& textureList,
                      std::map<std::string, int>& textureURIToEffectIndexMap)
    {
        openLibrary();

        for (uint32 i=0; i < meshdata.materials.size(); i++) {
          COLLADASW::EffectProfile effectProfile(streamWriter);

          bool effectProfileEmpty = true;
          //dealing with texture.
          for (uint32 j=0; j<meshdata.materials[i].textures.size(); j++) {
            const MaterialEffectInfo::Texture& texture = meshdata.materials[i].textures[j];

            COLLADASW::ColorOrTexture colorOrTexture;
            String colorEncoding = "";

            if (texture.uri != "") {
               if (  textureURIToEffectIndexMap.find(texture.uri) != textureURIToEffectIndexMap.end()) {
                 continue;
               }

              std::string nonAlphaNumericTextureURI = removeNonAlphaNumeric(texture.uri);

              COLLADASW::Texture colladaTexture = COLLADASW::Texture(nonAlphaNumericTextureURI);

              COLLADASW::Sampler colladaSampler(COLLADASW::Sampler::SAMPLER_TYPE_2D,
                                                std::string("sampler-")+ nonAlphaNumericTextureURI,
                                                std::string("surface-")+ nonAlphaNumericTextureURI);

              int k = 0;

              for (std::map<String,int>::iterator it = textureList.begin(); it != textureList.end(); it++ ) {
                if (it->first == texture.uri) {
                  const int IMAGE_ID_LEN = 1024;
                  char imageID[IMAGE_ID_LEN];
                  snprintf(imageID, IMAGE_ID_LEN, "image_id_%d", k);
                  colladaSampler.setImageId( std::string(imageID) );
                  break;
                }

                k++;
              }

              colladaSampler.setMagFilter(COLLADASW::Sampler::SAMPLER_FILTER_LINEAR);
              colladaSampler.setMinFilter(COLLADASW::Sampler::SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR);

              colladaTexture.setSampler( colladaSampler  );
              colladaTexture.setTexcoord("TEX0");

              colorOrTexture = COLLADASW::ColorOrTexture(colladaTexture);
            }
            else {
              char colorEncodingStr[256];
              snprintf(colorEncodingStr, 256, "%f %f %f %f %d", texture.color.x, texture.color.y,
                       texture.color.z, texture.color.w, texture.affecting);
              colorEncoding = colorEncodingStr;

              if (  textureURIToEffectIndexMap.find(colorEncoding) != textureURIToEffectIndexMap.end()) {
                 continue;
              }


              colorOrTexture = COLLADASW::ColorOrTexture( COLLADASW::Color(texture.color.x, texture.color.y,
                                                                           texture.color.z, texture.color.w));
            }



            switch(texture.affecting) {
              case MaterialEffectInfo::Texture::DIFFUSE:
                effectProfile.setDiffuse(colorOrTexture);
                effectProfileEmpty = false;
                break;
              case MaterialEffectInfo::Texture::SPECULAR:
                effectProfile.setSpecular(colorOrTexture);
                effectProfileEmpty = false;
                break;
            }

            if (texture.uri != "") {
              textureURIToEffectIndexMap[texture.uri] = i;
            }
            else {
              textureURIToEffectIndexMap[colorEncoding] = i;
            }
          }

          if (!effectProfileEmpty) {
            char effectNameStr[256];
            snprintf(effectNameStr, 256, "material%d", i);
            std::string effectName = effectNameStr;

            openEffect(effectName+"-effect");

            effectProfile.setShininess(meshdata.materials[i].shininess);
            effectProfile.setReflectivity(meshdata.materials[i].reflectivity);
            effectProfile.setShaderType(COLLADASW::EffectProfile::PHONG);

            //
            addEffectProfile(effectProfile);

            closeEffect();
          }
          else {

          }

        }

        closeLibrary();
      }
  };

  class GeometryExporter : public COLLADASW::LibraryGeometries {
    public:
    GeometryExporter(COLLADASW::StreamWriter*  streamWriter) :
        COLLADASW::LibraryGeometries ( streamWriter )
    {
    }

    void exportGeometry(COLLADASW::StreamWriter*  streamWriter, const Meshdata& meshdata,
                        std::map<int,bool>& addedGeometriesList, std::map<int, int>& materialRedirectionMap)
    {
      openLibrary();

      for (uint32 i=0; i<meshdata.geometry.size(); i++) {
        char geometryNameStr[256];
        snprintf(geometryNameStr, 256, "mesh%d-geometry", i);
        std::string geometryName = geometryNameStr;

        const GeometryInstance* geoInst = NULL;

        for (uint32 j=0; j<meshdata.instances.size(); j++) {
          if (meshdata.instances[j].geometryIndex == i) {
            geoInst = &(meshdata.instances[j]);
            break;
          }
        }

        bool hasTriangles = false;
        for(uint32 j = 0; geoInst != NULL && meshdata.geometry[i].positions.size() > 0  && j < meshdata.geometry[i].primitives.size(); ++j )
        {
          if (meshdata.geometry[i].primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;

          if (meshdata.geometry[i].primitives[j].indices.size() > 0) {
            hasTriangles = true;
            break;
          }
        }

        if (!hasTriangles) {
          std::cout << "Skipping this one in generation: " << geometryName   << "\n";
          continue;
        }

        addedGeometriesList[i] = true;

        openMesh(geometryName);

        //positions
        if (meshdata.geometry[i].positions.size() > 0) {
          COLLADASW::FloatSource colladaSource (streamWriter);
          colladaSource.setId(geometryName + "-position");
          colladaSource.setArrayId(geometryName + "-position-array");

          colladaSource.setAccessorStride(3);

          colladaSource.getParameterNameList().push_back( "X" );
          colladaSource.getParameterNameList().push_back( "Y" );
          colladaSource.getParameterNameList().push_back( "Z" );

          colladaSource.setAccessorCount(meshdata.geometry[i].positions.size());

          colladaSource.prepareToAppendValues();


          for (uint32 j = 0; j < meshdata.geometry[i].positions.size(); j++) {
            colladaSource.appendValues(meshdata.geometry[i].positions[j].x,
                                       meshdata.geometry[i].positions[j].y,
                                       meshdata.geometry[i].positions[j].z);
          }

          colladaSource.finish();
        }

        //normals
        if (meshdata.geometry[i].normals.size() > 0) {
          COLLADASW::FloatSource colladaSource (streamWriter);
          colladaSource.setId(geometryName + "-normal");
          colladaSource.setArrayId(geometryName + "-normal-array");

          colladaSource.setAccessorStride(3);

          colladaSource.getParameterNameList().push_back( "X" );
          colladaSource.getParameterNameList().push_back( "Y" );
          colladaSource.getParameterNameList().push_back( "Z" );

          colladaSource.setAccessorCount(meshdata.geometry[i].normals.size());

          colladaSource.prepareToAppendValues();


          for (uint32 j = 0; j < meshdata.geometry[i].normals.size(); j++) {
            colladaSource.appendValues(meshdata.geometry[i].normals[j].x,
                                       meshdata.geometry[i].normals[j].y,
                                       meshdata.geometry[i].normals[j].z);
          }

          colladaSource.finish();
        }

        //uvs
        for (uint32 j = 0; j < meshdata.geometry[i].texUVs.size(); j++)
        {
          if (meshdata.geometry[i].texUVs[j].uvs.size() == 0) continue;

          COLLADASW::FloatSource colladaSource (streamWriter);
          colladaSource.setId(geometryName + "-uv");
          colladaSource.setArrayId(geometryName + "-uv-array");

          int32 stride = meshdata.geometry[i].texUVs[j].stride;
          colladaSource.setAccessorStride(stride);
          colladaSource.getParameterNameList().push_back( "S" );
          if (stride >= 2) colladaSource.getParameterNameList().push_back( "T" );
          if (stride >= 3) colladaSource.getParameterNameList().push_back( "P" );

          colladaSource.setAccessorCount(meshdata.geometry[i].texUVs[j].uvs.size()/2);
          colladaSource.prepareToAppendValues();


          for (uint32 k = 0; k+1 < meshdata.geometry[i].texUVs[j].uvs.size(); k+=2) {
            colladaSource.appendValues(meshdata.geometry[i].texUVs[j].uvs[k],
                                     meshdata.geometry[i].texUVs[j].uvs[k+1]
                                     );
          }

          colladaSource.finish();
        }

        //vertices
        {
          COLLADASW::Vertices vertices( streamWriter );
          vertices.setId( geometryName + "-vertex" );
          char name[256];
          sprintf(name, "#mesh%d-geometry-position", i);
          vertices.getInputList().push_back( COLLADASW::Input( COLLADASW::POSITION,
                                                               COLLADABU::URI(name) ) );
          vertices.add();
        }

        //triangles
        for(uint32 j = 0; geoInst != NULL && j < meshdata.geometry[i].primitives.size(); ++j )
        {
          if (meshdata.geometry[i].primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;
          if (meshdata.geometry[i].primitives[j].indices.size() == 0) continue;


          COLLADASW::Triangles triangles(streamWriter);
          if (meshdata.geometry[i].texUVs.size() > 0) {
              triangles.setCount(meshdata.geometry[i].primitives[j].indices.size()/3);
          } else {
              triangles.setCount(meshdata.geometry[i].primitives[j].indices.size()/2);
          }


          char materialName[256];
          GeometryInstance::MaterialBindingMap::const_iterator mbm_it = geoInst->materialBindingMap.find(meshdata.geometry[i].primitives[j].materialId);
          snprintf(materialName, 256, "material%d", materialRedirectionMap[mbm_it->second]);
          triangles.setMaterial(std::string(materialName));

          int offset = 0;
          triangles.getInputList().push_back( COLLADASW::Input( COLLADASW::VERTEX, "#" + geometryName+"-vertex", offset++ ) );
          triangles.getInputList().push_back( COLLADASW::Input( COLLADASW::NORMAL, "#" + geometryName+"-normal", offset++ ) );
          if (meshdata.geometry[i].texUVs.size() > 0) {
              triangles.getInputList().push_back( COLLADASW::Input( COLLADASW::TEXCOORD, "#" + geometryName+"-uv", offset++ ) );
          }

          triangles.prepareToAppendValues();

          for( int k = 0; k < meshdata.geometry[i].primitives[j].indices.size(); k++ )
          {
              if (meshdata.geometry[i].texUVs.size() > 0) {
                triangles.appendValues(meshdata.geometry[i].primitives[j].indices[k],
                                       meshdata.geometry[i].primitives[j].indices[k],
                                       meshdata.geometry[i].primitives[j].indices[k] );
              } else {
                  triangles.appendValues(meshdata.geometry[i].primitives[j].indices[k],
                                         meshdata.geometry[i].primitives[j].indices[k] );
              }
          }


          triangles.finish();
        }

        closeMesh();
        closeGeometry();
      }



      closeLibrary();
    }
  };


class LightExporter : public COLLADASW::LibraryLights {
public:
    LightExporter(COLLADASW::StreamWriter*  streamWriter) :
     COLLADASW::LibraryLights ( streamWriter )
    {
    }

    void exportLights(COLLADASW::StreamWriter* streamWriter, const Meshdata& meshdata,
        std::map<int,bool>& addedLightsList)
    {
        if (meshdata.lights.empty()) return;

        openLibrary();

        for (uint32 light_idx = 0; light_idx < meshdata.lights.size(); light_idx++) {
            const LightInfo& light_info = meshdata.lights[light_idx];

            // Ambient lights are encoded as a special case of point lights
            bool is_ambient = (light_info.mType == LightInfo::POINT && light_info.mDiffuseColor == Vector3f(0,0,0) && light_info.mSpecularColor == Vector3f(0,0,0));

            COLLADASW::Color col;
            if (is_ambient)
                col = COLLADASW::Color(light_info.mAmbientColor.x, light_info.mAmbientColor.y, light_info.mAmbientColor.z, 1.0);
            else
                col = COLLADASW::Color(light_info.mDiffuseColor.x, light_info.mDiffuseColor.y, light_info.mDiffuseColor.z, 1.0);

            COLLADASW::Light* light = NULL;
            String light_name = "light" + boost::lexical_cast<String>(light_idx) + "-light";
            switch(light_info.mType) {
              case LightInfo::POINT:
                if (is_ambient) {
                    light = new COLLADASW::AmbientLight(streamWriter, light_name);
                }
                else {
                    light = new COLLADASW::PointLight(streamWriter, light_name);
                }
                break;
              case LightInfo::SPOTLIGHT:
                light = new COLLADASW::SpotLight(streamWriter, light_name);
                break;
              case LightInfo::DIRECTIONAL:
                light = new COLLADASW::DirectionalLight(streamWriter, light_name);
                break;
            }

            light->setConstantAttenuation(light_info.mConstantFalloff);
            light->setLinearAttenuation(light_info.mLinearFalloff);
            light->setQuadraticAttenuation(light_info.mQuadraticFalloff);

            addLight(*light);
            delete light;

            addedLightsList[light_idx] = true;
        }

        closeLibrary();
    }
};

namespace {
void convertMatrixForCollada(const Matrix4x4f& mat, double output[4][4]) {
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            output[i][j] = mat(i,j);
}

// State for traversing a node tree
struct NodeState {
    NodeState(NodeIndex n) : node(n), currentChild(-1), colladaNode(NULL) {}

    NodeIndex node;
    int32 currentChild;
    COLLADASW::Node* colladaNode;
};
}

class VisualSceneExporter: public COLLADASW::LibraryVisualScenes {
public:
  VisualSceneExporter(COLLADASW::StreamWriter*  streamWriter) :
        COLLADASW::LibraryVisualScenes ( streamWriter )
    {
    }

  void exportVisualScene(COLLADASW::StreamWriter*  streamWriter, const Meshdata& meshdata,
      std::map<int,bool>& addedGeometriesList, std::map<int, int>& materialRedirectionMap,
      std::map<int,bool>& addedLightsList
  )
  {
      // Generate a reverse index for instance lights, instance geometries and
      // joints so we can determine which instances need to be inserted for each node.
      typedef std::multimap<NodeIndex, uint32> InstanceNodeIndex;
      InstanceNodeIndex nodeGeoInstances;
      InstanceNodeIndex nodeLightInstances;
      InstanceNodeIndex nodeJoints;

      for(uint32 geo_it = 0; geo_it < meshdata.instances.size(); geo_it++)
          nodeGeoInstances.insert( std::make_pair(meshdata.instances[geo_it].parentNode, geo_it) );
      for(uint32 light_it = 0; light_it < meshdata.lightInstances.size(); light_it++)
          nodeLightInstances.insert( std::make_pair(meshdata.lightInstances[light_it].parentNode, light_it) );
      for(uint32 joint_it = 0; joint_it < meshdata.joints.size(); joint_it++)
          nodeJoints.insert( std::make_pair(meshdata.joints[joint_it], joint_it) );

      openLibrary();
      openVisualScene( "Space_Aggregated_Scene" );

      // We need to emit:
      //  1. The node hierarchy (this is the main part of the traversal).
      //  2. Instance ndoes
      //  3. Geometry instances
      //  4. Light instances

      // We need one outer node to hold all the others to manage our
      // global transform. Note that, in order to avoid inflating the
      // file unnecessarily, we avoid adding this if the transform is
      // just the identity.
      COLLADASW::Node* globalNode = NULL;
      if (meshdata.globalTransform != Matrix4x4f::identity()) {
          globalNode = new COLLADASW::Node( streamWriter );
          String node_name = "globalTransformNode";
          globalNode->setNodeId(node_name);
          globalNode->setNodeName( COLLADASW::Utils::checkNCName( COLLADABU::NativeString(node_name) ) );
          globalNode->setType(COLLADASW::Node::NODE);

          globalNode->start();
          double mat[4][4];
          convertMatrixForCollada(meshdata.globalTransform, mat);
          globalNode->addMatrix(mat);
      }

      std::stack<NodeState> node_stack;
      for(uint32 root_i = 0; root_i < meshdata.rootNodes.size(); root_i++) {
          node_stack.push( NodeState(meshdata.rootNodes[root_i]) );

          while(!node_stack.empty()) {
              NodeState& current = node_stack.top();

              // First, if this is the first time we're seeing this node, do
              // node setup
              if (current.currentChild == -1) {
                  current.colladaNode = new COLLADASW::Node( streamWriter );
                  String node_name = "node-" + boost::lexical_cast<String>(current.node);
                  current.colladaNode->setNodeId(node_name);
                  current.colladaNode->setNodeName( COLLADASW::Utils::checkNCName( COLLADABU::NativeString(node_name) ) );
                  bool is_joint = (nodeJoints.find(current.node) != nodeJoints.end());
                  current.colladaNode->setType(is_joint ? COLLADASW::Node::JOINT : COLLADASW::Node::NODE);

                  current.colladaNode->start();
                  double mat[4][4];
                  convertMatrixForCollada(meshdata.nodes[current.node].transform, mat);
                  current.colladaNode->addMatrix(mat);

                  current.currentChild++;
              }

              // If we've finished handling all children, handle non-node
              // children, i.e. instance nodes, instance lights, instance geometries
              if (current.currentChild >= meshdata.nodes[current.node].children.size()) {
                  // InstanceNodes
                  for(NodeIndexList::const_iterator inst_it = meshdata.nodes[current.node].instanceChildren.begin();
                      inst_it != meshdata.nodes[current.node].instanceChildren.end();
                      inst_it++)
                  {
                      String inst_node_url = "#node-" + boost::lexical_cast<String>(current.node);
                      COLLADASW::InstanceNode instanceNode(streamWriter, inst_node_url);
                      instanceNode.add();
                  }

                  // Geometry instances
                  InstanceNodeIndex::iterator geo_it = nodeGeoInstances.find(current.node);
                  while(geo_it != nodeGeoInstances.end() && geo_it->first == current.node) {
                      uint32 instanced_geo = geo_it->second;
                      const GeometryInstance& geo_inst = meshdata.instances[instanced_geo];
                      if (addedGeometriesList.find(geo_inst.geometryIndex) == addedGeometriesList.end() ||
                          addedGeometriesList[geo_inst.geometryIndex] == false)
                      {
                          geo_it++;
                          continue;
                      }

                      COLLADASW::InstanceGeometry instanceGeometry(streamWriter);
                      instanceGeometry.setUrl ( "#mesh" + boost::lexical_cast<String>(geo_inst.geometryIndex) + "-geometry");

                      COLLADASW::BindMaterial& bindMaterial = instanceGeometry.getBindMaterial();
                      for (GeometryInstance::MaterialBindingMap::const_iterator mat_it =
                               geo_inst.materialBindingMap.begin();
                           mat_it != geo_inst.materialBindingMap.end(); mat_it++)
                      {
                          uint32 materialIdx = materialRedirectionMap[mat_it->second];

                          String effectName ="material" + boost::lexical_cast<String>(materialIdx);
                          String materialName = effectName + "ID";
                          COLLADASW::InstanceMaterial instanceMaterial( effectName, "#" + materialName );
                          // FIXME multiple textures?
                          instanceMaterial.push_back(COLLADASW::BindVertexInput("TEX0", "TEXCOORD", 0)  );
                          bindMaterial.getInstanceMaterialList().push_back( instanceMaterial );
                      }

                      instanceGeometry.add();

                      geo_it++;
                  }

                  // Light instances
                  InstanceNodeIndex::iterator light_it = nodeLightInstances.find(current.node);
                  while(light_it != nodeLightInstances.end() && light_it->first == current.node) {
                      uint32 instanced_light = light_it->second;
                      const LightInstance& light_inst = meshdata.lightInstances[instanced_light];
                      if (addedLightsList.find(light_inst.lightIndex) == addedLightsList.end() ||
                          addedLightsList[light_inst.lightIndex] == false)
                      {
                          light_it++;
                          continue;
                      }

                      COLLADASW::InstanceLight instanceLight(
                          streamWriter,
                          COLLADABU::URI("#light" + boost::lexical_cast<String>(light_inst.lightIndex) + "-light")
                      );
                      instanceLight.add();

                      light_it++;
                  }

                  // And end the node
                  current.colladaNode->end();
                  delete current.colladaNode;
                  node_stack.pop();

                  continue;
              }

              // Otherwise, we're processing another child
              int32 child_idx = current.currentChild;
              current.currentChild++;
              node_stack.push( NodeState( meshdata.nodes[current.node].children[child_idx] ) );
          }
      }

      if (globalNode != NULL) {
          globalNode->end();
          delete globalNode;
          globalNode = NULL;
      }

    closeVisualScene();
    closeLibrary();
  }
};

  class ImageExporter : public COLLADASW::LibraryImages {

  public:

    ImageExporter(COLLADASW::StreamWriter*  streamWriter) :
      COLLADASW::LibraryImages ( streamWriter )
    {

    }

    void exportImages(std::map<String,int>& textureList) {

      if (textureList.size() == 0) return;

      openLibrary();

      int i = 0;

      for (std::map<String,int>::iterator it = textureList.begin(); it != textureList.end(); it++ ) {
        const int IMAGE_ID_LEN = 1024;
        char imageID[IMAGE_ID_LEN];
        snprintf(imageID, IMAGE_ID_LEN, "image_id_%d", i);
        addImage( COLLADASW::Image( it->first, std::string(imageID)  )  );
        i++;
      }

      closeLibrary();
    }
  };


  void exportScene(COLLADASW::StreamWriter*  streamWriter, const Meshdata& meshdata) {
      COLLADASW::Scene scene ( streamWriter, COLLADASW::URI ( "#Space_Aggregated_Scene" ) );
      scene.add();
  }

  int meshdataToCollada(const Meshdata& meshdata, const std::string& fileName) {
    COLLADASW::StreamWriter  streamWriter( COLLADASW::NativeString (fileName), false );

    streamWriter.startDocument();

    exportAsset(&streamWriter, meshdata);

    std::map<String,int> texturesList;
    for (TextureList::const_iterator it = meshdata.textures.begin() ; it!= meshdata.textures.end(); it++) {
      texturesList[*it] = 1;
    }

    ImageExporter imageExporter(&streamWriter);
    imageExporter.exportImages( texturesList);


    std::map<std::string, int> textureURIToEffectIndexMap;
    EffectExporter effectExporter(&streamWriter);
    effectExporter.exportEffect(&streamWriter, meshdata, texturesList, textureURIToEffectIndexMap);


    std::map<int, int> materialRedirectionMap;
    MaterialExporter materialExporter(&streamWriter);
    materialExporter.exportMaterial( meshdata, textureURIToEffectIndexMap, materialRedirectionMap);


    std::map<int,bool> addedGeometriesList;
    GeometryExporter geometryExporter(&streamWriter);
    geometryExporter.exportGeometry(&streamWriter, meshdata, addedGeometriesList, materialRedirectionMap);

    std::map<int,bool> addedLightsList;
    LightExporter lightExporter(&streamWriter);
    lightExporter.exportLights(&streamWriter, meshdata, addedLightsList);

    VisualSceneExporter visualSceneExporter(&streamWriter);
    visualSceneExporter.exportVisualScene(&streamWriter, meshdata, addedGeometriesList, materialRedirectionMap, addedLightsList);

    exportScene(&streamWriter, meshdata);

    streamWriter.endDocument();

    return 0;
  }

}


