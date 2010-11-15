
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

    asset.setUpAxisType ( COLLADASW::Asset::UpAxisType(/*meshdata.up_axis+1*/COLLADASW::Asset::Y_UP) );

    asset.add();

  }


  class MaterialExporter : public COLLADASW::LibraryMaterials {

  public:

    MaterialExporter(COLLADASW::StreamWriter*  streamWriter) :
      COLLADASW::LibraryMaterials ( streamWriter )
    {

    }

    void exportMaterial(const Meshdata& meshdata) {
      openLibrary();


      for (uint32 i=0; i < meshdata.materials.size(); i++) {
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

    void exportEffect(COLLADASW::StreamWriter*  streamWriter, const Meshdata& meshdata, std::map<String,int>& textureList) {
        openLibrary();

        for (uint32 i=0; i < meshdata.materials.size(); i++) {
          char effectNameStr[256];
          snprintf(effectNameStr, 256, "material%d", i);
          std::string effectName = effectNameStr;

          openEffect(effectName+"-effect");

          COLLADASW::EffectProfile effectProfile(streamWriter);
          effectProfile.setShininess(meshdata.materials[i].shininess);
          effectProfile.setReflectivity(meshdata.materials[i].reflectivity);
          effectProfile.setShaderType(COLLADASW::EffectProfile::PHONG);



          //dealing with texture.
          for (uint32 j=0; j<meshdata.materials[i].textures.size(); j++) {
            const MaterialEffectInfo::Texture& texture = meshdata.materials[i].textures[j];

            COLLADASW::ColorOrTexture colorOrTexture;

            if (texture.uri != "") {
              std::string nonAlphaNumericTextureURI = removeNonAlphaNumeric(texture.uri);

              COLLADASW::Texture colladaTexture = COLLADASW::Texture(nonAlphaNumericTextureURI);

              COLLADASW::Sampler colladaSampler(COLLADASW::Sampler::SAMPLER_TYPE_2D, std::string("sampler-")+
                                                nonAlphaNumericTextureURI,
                                                std::string("surface-")+ nonAlphaNumericTextureURI);

              int i = 0;

              for (std::map<String,int>::iterator it = textureList.begin(); it != textureList.end(); it++ ) {

                if (it->first == texture.uri) {
                  const int IMAGE_ID_LEN = 1024;
                  char imageID[IMAGE_ID_LEN];
                  snprintf(imageID, IMAGE_ID_LEN, "image_id_%d", i);
                  colladaSampler.setImageId( std::string(imageID) );
                  break;
                }

                i++;
              }

              colladaSampler.setMagFilter(COLLADASW::Sampler::SAMPLER_FILTER_LINEAR);
              colladaSampler.setMinFilter(COLLADASW::Sampler::SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR);

              colladaTexture.setSampler( colladaSampler  );
              colladaTexture.setTexcoord("TEX0");

              colorOrTexture = COLLADASW::ColorOrTexture(colladaTexture);


              //              std::cout << "TEXTURE.URI=" << texture.uri << "\n";
            }
            else {
              colorOrTexture = COLLADASW::ColorOrTexture( COLLADASW::Color(texture.color.x,
                                                                           texture.color.y,
                                                                           texture.color.z,
                                                                           texture.color.w));


              /*printf("COLOR: %f %f %f %f\n", texture.color.x,
                     texture.color.y,
                     texture.color.z,
                     texture.color.w);*/

            }

            switch(texture.affecting) {
              case MaterialEffectInfo::Texture::DIFFUSE:
                effectProfile.setDiffuse(colorOrTexture);

                break;
              case MaterialEffectInfo::Texture::SPECULAR:
                effectProfile.setSpecular(colorOrTexture);

                break;
            }

          }


          //
          addEffectProfile(effectProfile);

          closeEffect();
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

    void exportGeometry(COLLADASW::StreamWriter*  streamWriter, const Meshdata& meshdata) {
      openLibrary();


      for (uint32 i=0; i<meshdata.geometry.size(); i++) {
        char geometryNameStr[256];
        snprintf(geometryNameStr, 256, "mesh%d-geometry", i);
        std::string geometryName = geometryNameStr;

        openMesh(geometryName);

        //positions
        {
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
        {
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
        const GeometryInstance* geoInst = NULL;

        for (uint32 j=0; j<meshdata.instances.size(); j++) {
          if (meshdata.instances[j].geometryIndex == i) {
            geoInst = &(meshdata.instances[j]);
            break;
          }
        }


        for(uint32 j = 0; geoInst != NULL && j < meshdata.geometry[i].primitives.size(); ++j )
        {
          if (meshdata.geometry[i].primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;

          COLLADASW::Triangles triangles(streamWriter);
          triangles.setCount(meshdata.geometry[i].primitives[j].indices.size()/3);


          char materialName[256];
          GeometryInstance::MaterialBindingMap::const_iterator mbm_it = geoInst->materialBindingMap.find(meshdata.geometry[i].primitives[j].materialId);
          snprintf(materialName, 256, "material%d", mbm_it->second);
          triangles.setMaterial(std::string(materialName));

          int offset = 0;
          triangles.getInputList().push_back( COLLADASW::Input( COLLADASW::VERTEX, "#" + geometryName+"-vertex", offset++ ) );
          triangles.getInputList().push_back( COLLADASW::Input( COLLADASW::NORMAL, "#" + geometryName+"-normal", offset++ ) );
          triangles.getInputList().push_back( COLLADASW::Input( COLLADASW::TEXCOORD, "#" + geometryName+"-uv", offset++ ) );

          triangles.prepareToAppendValues();

          for( int k = 0; k < meshdata.geometry[i].primitives[j].indices.size(); k++ )
          {
            triangles.appendValues(meshdata.geometry[i].primitives[j].indices[k],
                                   0,
                                   meshdata.geometry[i].primitives[j].indices[k] );
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

  void exportVisualScene(COLLADASW::StreamWriter*  streamWriter, const Meshdata& meshdata) {
    openLibrary();
    openVisualScene( "Space_Aggregated_Scene" );


    for(uint32 i = 0; i < meshdata.instances.size(); i++)
    {
      char geometryNameStr[256];
      snprintf(geometryNameStr, 256, "mesh%d-geometry", i);
      std::string geometryName = geometryNameStr;

      COLLADASW::Node colladaNode( streamWriter );

      colladaNode.setNodeId( "node-" + geometryName );

      colladaNode.setNodeName( COLLADASW::Utils::checkNCName( COLLADABU::NativeString("node-" + geometryName) ) );

      colladaNode.setType(COLLADASW::Node::NODE);

      colladaNode.start();

      COLLADASW::InstanceGeometry instanceGeometry ( streamWriter );
      instanceGeometry.setUrl ( "#" + geometryName );

      COLLADASW::BindMaterial& bindMaterial = instanceGeometry.getBindMaterial();
      for (std::map<SubMeshGeometry::Primitive::MaterialId,size_t>::const_iterator it =
             meshdata.instances[i].materialBindingMap.begin();
           it != meshdata.instances[i].materialBindingMap.end(); it++)
      {

        uint32 materialIdx = it->second;

        char effectNameStr[256];
        snprintf(effectNameStr, 256, "material%d", materialIdx);
        std::string effectName = effectNameStr;
        std::string materialName = effectName + "ID";


        COLLADASW::InstanceMaterial instanceMaterial( effectName, "#" + materialName );
        instanceMaterial.push_back(COLLADASW::BindVertexInput("TEX0", "TEXCOORD", 0)  );

        bindMaterial.getInstanceMaterialList().push_back( instanceMaterial );
      }

      instanceGeometry.add();

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

    MaterialExporter materialExporter(&streamWriter);
    materialExporter.exportMaterial( meshdata);

    EffectExporter effectExporter(&streamWriter);
    effectExporter.exportEffect(&streamWriter, meshdata, texturesList);

    GeometryExporter geometryExporter(&streamWriter);
    geometryExporter.exportGeometry(&streamWriter, meshdata);

    VisualSceneExporter visualSceneExporter(&streamWriter);
    visualSceneExporter.exportVisualScene(&streamWriter, meshdata);

    exportScene(&streamWriter, meshdata);

    streamWriter.endDocument();

    return 0;
  }

}
