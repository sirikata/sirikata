
#include "MeshdataToCollada.hpp"

#include "COLLADASWStreamWriter.h"

#include "COLLADASWAsset.h"
#include "COLLADASWScene.h"
#include "COLLADASWLibrary.h"
#include "COLLADASWLibraryMaterials.h"
#include "COLLADASWLibraryEffects.h"
#include "COLLADASWLibraryGeometries.h"
#include "COLLADASWEffectProfile.h"
#include "COLLADASWSource.h"
#include "COLLADASWVertices.h"
#include "COLLADASWInputList.h"
#include "COLLADASWPrimitves.h"
#include "COLLADASWLibraryVisualScenes.h"
#include "COLLADASWNode.h"
#include "COLLADASWInstanceGeometry.h"
#include "COLLADASWBindMaterial.h"
#include "COLLADASWInstanceMaterial.h"
#include "COLLADASWLibraryImages.h"
#include "COLLADASWColorOrTexture.h"





namespace Sirikata {

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
              std::cout << colorEncoding << " : colorEncoding 1\n";
              if (  textureURIToEffectIndexMap.find(colorEncoding) != textureURIToEffectIndexMap.end()) {
                 continue;
              }
              std::cout << colorEncoding << " : colorEncoding 2\n";
              std::cout << texture.affecting << " : texture.affecting 2\n";
              
              
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

          colladaSource.setAccessorStride(2);

          colladaSource.getParameterNameList().push_back( "S" );
          colladaSource.getParameterNameList().push_back( "T" );

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

class VisualSceneExporter: public COLLADASW::LibraryVisualScenes {
public:
  VisualSceneExporter(COLLADASW::StreamWriter*  streamWriter) :
        COLLADASW::LibraryVisualScenes ( streamWriter )
    {
    }

  void exportVisualScene(COLLADASW::StreamWriter*  streamWriter, const Meshdata& meshdata, 
                         std::map<int,bool>& addedGeometriesList, std::map<int, int>& materialRedirectionMap) 
  {
    openLibrary();
    openVisualScene( "Space_Aggregated_Scene" );


    for(uint32 i = 0; i < meshdata.instances.size(); i++)
    {

      if (   addedGeometriesList.find(meshdata.instances[i].geometryIndex) == addedGeometriesList.end() 
             || addedGeometriesList[meshdata.instances[i].geometryIndex] == false)
      {
        continue;
      }

      char geometryNameStr[256];
      snprintf(geometryNameStr, 256, "mesh-geometry-%d", i );
      std::string geometryName = geometryNameStr;
      
      COLLADASW::Node colladaNode( streamWriter );

      colladaNode.setNodeId( "node-" + geometryName );

      colladaNode.setNodeName( COLLADASW::Utils::checkNCName( COLLADABU::NativeString("node-" + geometryName) ) );

      colladaNode.setType(COLLADASW::Node::NODE);

      colladaNode.start();

      COLLADASW::InstanceGeometry instanceGeometry ( streamWriter );

      char geometryUrlStr[256];
      snprintf(geometryUrlStr, 256, "mesh%d-geometry", meshdata.instances[i].geometryIndex);
      std::string geometryUrl = geometryUrlStr;
      instanceGeometry.setUrl ( "#" + geometryUrl );

      COLLADASW::BindMaterial& bindMaterial = instanceGeometry.getBindMaterial();
      for (std::map<SubMeshGeometry::Primitive::MaterialId,size_t>::const_iterator it =
             meshdata.instances[i].materialBindingMap.begin();
           it != meshdata.instances[i].materialBindingMap.end(); it++)
      {
        uint32 materialIdx = materialRedirectionMap[it->second];

        char effectNameStr[256];
        snprintf(effectNameStr, 256, "material%d", materialIdx);
        std::string effectName = effectNameStr;
        std::string materialName = effectName + "ID";


        COLLADASW::InstanceMaterial instanceMaterial( effectName, "#" + materialName );
        instanceMaterial.push_back(COLLADASW::BindVertexInput("TEX0", "TEXCOORD", 0)  );

        bindMaterial.getInstanceMaterialList().push_back( instanceMaterial );
      }

      instanceGeometry.add();

      Matrix4x4f mat = meshdata.instances[i].transform;

      double matrix[4][4] = { { mat(0,0), mat(0,1), mat(0,2), mat(0,3) },
                              { mat(1,0), mat(1,1), mat(1,2), mat(1,3)  },
                              { mat(2,0), mat(2,1), mat(2,2), mat(2,3)  },
                              { mat(3,0), mat(3,1), mat(3,2), mat(3,3)  }
                            };
      colladaNode.addMatrix( matrix );

      colladaNode.end();
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
    for (Meshdata::URIMap::const_iterator it = meshdata.textureMap.begin() ; it!= meshdata.textureMap.end(); it++) {
      texturesList[it->second] = 1;
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

    VisualSceneExporter visualSceneExporter(&streamWriter);
    visualSceneExporter.exportVisualScene(&streamWriter, meshdata, addedGeometriesList, materialRedirectionMap);

    exportScene(&streamWriter, meshdata);

    streamWriter.endDocument();

    return 0;
  }

}
