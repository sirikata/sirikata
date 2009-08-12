/*  Meru
 *  ManualMaterialLoader.cpp
 *
 *  Copyright (c) 2009, Stanford University
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
#include "ManualMaterialLoader.hpp"

#include <OgreHighLevelGpuProgramManager.h>
#include <OgreGpuProgramManager.h>
#include <OgreScriptCompiler.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreFrameListener.h>
#include <OgreRoot.h>

#include "ReplacingDataStream.hpp"
#include "EventSource.hpp"
#include "util/Time.hpp"
#include "transfer/URI.hpp"
#include "util/Logging.hpp"
#include "SequentialWorkQueue.hpp"

template<> Meru::MaterialScriptManager *Ogre::Singleton<Meru::MaterialScriptManager>::ms_Singleton = 0;

namespace Meru {

OptionValue*OPTION_THREADED_OGRE_COMPILER = new OptionValue("threaded-ogre-compiler","true",OptionValueType<bool>(),"Is the Ogre material compiler thread safe");
OptionValue*OPTION_THREADED_OGRE_GLSL_COMPILER = new OptionValue("threaded-ogre-glsl-compiler","false",OptionValueType<bool>(),"Is the Ogre material GLSL compiler thread safe");

InitializeGlobalOptions manualmaterialloaderopts("ogregraphics",
    OPTION_THREADED_OGRE_COMPILER,
    OPTION_THREADED_OGRE_GLSL_COMPILER,
    NULL);

using Ogre::MaterialManager;
MaterialScript::MaterialScript(Ogre::ResourceManager* creator, const Ogre::String &name,
    Ogre::ResourceHandle handle, const Ogre::String &group, bool isManual,
    Ogre::ManualResourceLoader *loader
    ,const Ogre::NameValuePairList*createParams) :
Ogre::Resource(creator, name, handle, group, isManual, loader)
{
    /* If you were storing a pointer to an object, then you would set that pointer to NULL here.
    */

    /* For consistency with StringInterface, but we don't add any parameters here
    That's because the Resource implementation of StringInterface is to
    list all the options that need to be set before loading, of which
    we have none as such. Full details can be set through scripts.
    */
    createParamDictionary("MaterialScript");
    mTextureAliases=createParams;
    mTexturesInRam=false;
}

MaterialScript::~MaterialScript()
{
    unload();
}

// farm out to MaterialScriptSerializer
void MaterialScript::parseScript()
{
    mStreamToBeLoadedInForeground->seek(0);
    Ogre::ScriptCompilerManager::getSingleton().parseScript(mStreamToBeLoadedInForeground,mGroup);
    mStreamToBeLoadedInForeground->seek(0);
    mStreamToBeLoadedInForeground->close();
    bool prepare_material=mSourceStream->getName().find(".material")==String::npos||mSourceStream->getName().find("meru:///")!=0;//make sure it's a .material owned by r00t
    for (std::vector<Ogre::String>::iterator iter=provides.begin(),itere=provides.end();iter!=itere;++iter) {
        Ogre::ResourcePtr resource=MaterialManager::getSingleton().getByName(*iter);

        if (!resource.isNull()) {
            Ogre::MaterialPtr mat=resource;
            if (1/*debug mode*/) {
                Ogre::Technique*tech=mat->createTechnique();
                tech->setSchemeName("untextured");
                Ogre::Pass*pass=tech->createPass();
                Ogre::TextureUnitState *tus=pass->createTextureUnitState("white.png",0);
            }
            if (prepare_material)
                mat->prepare();
/* calling getTexturePtr on which to call prepare on texture pointers ends up being a bad idea
            Ogre::Material::TechniqueIterator i = static_cast<Ogre::Material*>(&*resource)->getSupportedTechniqueIterator();
            while (i.hasMoreElements()) {
                Ogre::Technique *t = i.getNext();
                Ogre::Technique::PassIterator j = t->getPassIterator();
                while (j.hasMoreElements()) {
                    Ogre::Pass *p = j.getNext();
                    Ogre::Pass::TextureUnitStateIterator k =
                        p->getTextureUnitStateIterator();
                    while (k.hasMoreElements()) {
                        Ogre::TextureUnitState *tus = k.getNext();
                        size_t nf=tus->getNumFrames();
                        for (size_t whichFrame=0;whichFrame<nf;++whichFrame)
                            tus->_getTexturePtr(whichFrame)->prepare();
                    }
                }
            }
*/
        }

        resource=Ogre::HighLevelGpuProgramManager::getSingleton().getByName(*iter);
        if (!resource.isNull()) {
            resource->prepare();
        }
        resource=Ogre::GpuProgramManager::getSingleton().getByName(*iter);
        if (!resource.isNull()) {
            resource->prepare();
        }
    }
}




void MaterialScript::unprepareImpl()
{
    SILOG(resource,warn,"UnprepareImpl called for Material "<<getName()<<" not sure what to do here");
/*
    for (std::vector<Ogre::String>::iterator iter=provides.begin(),itere=provides.end();iter!=itere;++iter) {
        Ogre::ResourcePtr resource=MaterialManager::getSingleton().getByName(*iter);
        if (!resource.isNull()) {
            resource->unprepareImpl();
        }
        resource=Ogre::HighLevelGpuProgramManager::getSingleton().getByName(*iter);
        if (!resource.isNull()) {
            resource->unprepareImpl();
        }
        resource=Ogre::GpuProgramManager::getSingleton().getByName(*iter);
        if (!resource.isNull()) {
            resource->unprepareImpl();
        }
    }
*/
}
namespace EventTypes {
const EventType MaterialLoaded = "MaterialLoaded";
}

class LoadPreparedMaterial : public Sirikata::Task::WorkItem {
    Ogre::String mMaterialScriptName;
    std::vector<Ogre::String> mMaterialNames;
    Ogre::TextureUnitState *mTus;
    size_t mWhichTextureFrame;
public:
    LoadPreparedMaterial(const std::vector<Ogre::String> &materialNames,const Ogre::String &materialScriptName):mMaterialScriptName(materialScriptName) {
        if (mMaterialScriptName.find(".material")==String::npos) {
            mMaterialNames=materialNames;///do not prepare the 'middleman' materials just meant to be inherited
        }

        mTus=NULL;
        mWhichTextureFrame=0;

    }
    void operator()() {
        AutoPtr deleteme(this);
/*    int err=glGetError();
    if (err!=0) {
        fprintf(stderr,"HELP BEFOREJOB %d\n",err);
        }*/
        while (mMaterialNames.size()) {
            bool ready_to_process=(mTus==NULL);
            Ogre::ResourcePtr resource=MaterialManager::getSingleton().getByName(mMaterialNames.back());
            if (!resource.isNull()) {
                Ogre::Material::TechniqueIterator i = static_cast<Ogre::Material*>(&*resource)->getSupportedTechniqueIterator();
                while (i.hasMoreElements()) {
                    Ogre::Technique *t = i.getNext();
                    Ogre::Technique::PassIterator j = t->getPassIterator();
                    while (j.hasMoreElements()) {
                        Ogre::Pass *p = j.getNext();
                        Ogre::Pass::TextureUnitStateIterator k =
                            p->getTextureUnitStateIterator();
                        while (k.hasMoreElements()) {
                            Ogre::TextureUnitState *tus = k.getNext();
                            if (mTus==tus) {
                                ready_to_process=true;
                            } else if (ready_to_process) {
                                size_t nf=tus->getNumFrames();
                                while(mWhichTextureFrame<nf) {
                                    Ogre::TexturePtr tp=tus->_getTexturePtr(mWhichTextureFrame);
                                    if (tp.isNull()==false&&tp->isLoading()==false&&tp->isLoaded()==false) {
                                        assert(tp->isPrepared()==true);
                                        tus->_getTexturePtr(mWhichTextureFrame)->load();
                                        //fixme decide a time total among all LoadPreparedMaterials
                                    }
                                    mWhichTextureFrame++;
                                    return;
                                }
                                mWhichTextureFrame=0;
                                mTus=tus;
                            }
                        }
                    }
                }
            }else {
                Ogre::ResourcePtr resource=Ogre::HighLevelGpuProgramManager::getSingleton().getByName(mMaterialNames.back());

                if (!resource.isNull()) {
                    resource->load();
                }else {
                    resource=Ogre::GpuProgramManager::getSingleton().getByName(mMaterialNames.back());
                    resource->load();
                }
            }
            mMaterialNames.pop_back();
            mTus=NULL;
            mWhichTextureFrame=0;
            return;
        }
        if (mMaterialNames.empty()) {
            MaterialScriptPtr finishMe=MaterialScriptManager::getSingleton().getByName(mMaterialScriptName);
            finishMe->mTexturesInRam=true;
            EventSource::getSingleton().fire(EventPtr(new MaterialLoadedEvent(mMaterialScriptName)));

            return;
        }
    }
};

void MaterialScript::prepareImpl(){
    bool threadsafe_ogre_compiler=OPTION_THREADED_OGRE_COMPILER->as<bool>();
    bool threadsafe_ogre_glsl_compiler=OPTION_THREADED_OGRE_GLSL_COMPILER->as<bool>();
    Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(mName, mGroup, true, this);
    mSourceStream=stream;
    Ogre::DataStream *ogredatastream=NULL;
    if ((mName.find("badfoood")!=0)&&(mName.find("deadBeaf")!=0)&& (mName.find("black.png")!=0)&& (mName.find("white.png")!=0)&&(mName.find("bad.mesh")!=0)) {
        ReplacingDataStream*datastream=new ReplacingDataStream(stream,mName,mTextureAliases);
        datastream->preprocessData(this->provides,this->depends_on);
        ogredatastream=datastream;
    }
    Ogre::DataStreamPtr ds(ogredatastream);
    if (ogredatastream==NULL)
        ds=stream;
    mStreamToBeLoadedInForeground=ds;
    unsigned char search[1024]={0};
    size_t len;
    memset(search,0,6);
    bool hasGLSL=false;
    while (threadsafe_ogre_glsl_compiler==false&&hasGLSL==false&&!mStreamToBeLoadedInForeground->eof()) {
        len=mStreamToBeLoadedInForeground->read(search+6,1018);
        int endNdx = (len >= 12) ? 1018 : len + 6;
        for (int i = 0; i < endNdx; ++i) {
            if (isspace(search[i])&&((search[i+1]=='g'&&search[i+2]=='l'&&search[i+3]=='s'&&search[i+4]=='l')||(0&&search[i+1]=='c'&&search[i+2]=='g'))) {
                hasGLSL=true;
                break;
            }
        }
        memcpy(search,search+1018,6);
    }
    if (threadsafe_ogre_compiler&&(threadsafe_ogre_glsl_compiler||!hasGLSL)) {
        SILOG(resource,debug,"Parsing "<<mName<<" Material in background thread");
        parseScript();
        Ogre::DataStreamPtr nildatastrm;
        mStreamToBeLoadedInForeground=nildatastrm;
        mSourceStream=nildatastrm;
    }
}
void MaterialScript::loadImpl(){
/*    int err=glGetError();
    if (err!=0) {
        fprintf(stderr,"HELP BEFORE %d\n",err);
        }*/
    if (!mStreamToBeLoadedInForeground.isNull()) {
        SILOG(resource,debug,"Parsing "<<mName<<" Material in foreground thread due to glsl contents");
        parseScript();
    }
    Ogre::DataStreamPtr nildatastrm;
    mStreamToBeLoadedInForeground=nildatastrm;
    mSourceStream=nildatastrm;
/*    err=glGetError();
    if (err!=0) {
        fprintf(stderr,"HELP AFTER %d\n",err);
        }*/
}
void MaterialScript::postLoadImpl()
{
    std::vector<Ogre::String> provided_materials;
    for (std::vector<Ogre::String>::iterator iter=provides.begin(),itere=provides.end();iter!=itere;++iter) {
        //if (MaterialManager::getSingleton().resourceExists(*iter)) {
        provided_materials.push_back(*iter);
        //}
    }
    SequentialWorkQueue::getSingleton().enqueue(new LoadPreparedMaterial(provided_materials,getName()));

}


void MaterialScript::unloadImpl()
{
    /* If you were storing a pointer to an object, then you would check the pointer here,
    and if it is not NULL, you would destruct the object and set its pointer to NULL again.
    */
  for (std::vector<Ogre::String>::iterator i=provides.begin(),ie=provides.end();i!=ie;++i) {
    Ogre::ResourcePtr resource=MaterialManager::getSingleton().getByName(*i);
    if (!resource.isNull()) {
      MaterialManager::getSingleton().remove(resource);
    }
    resource=Ogre::HighLevelGpuProgramManager::getSingleton().getByName(*i);
    if (!resource.isNull()) {
      Ogre::HighLevelGpuProgramManager::getSingleton().remove(resource);
    }
    resource=Ogre::GpuProgramManager::getSingleton().getByName(*i);
    if (!resource.isNull()) {
      Ogre::GpuProgramManager::getSingleton().remove(resource);
    }
  }
  this->provides.clear();
  this->depends_on.clear();
}

MaterialScriptManager *MaterialScriptManager::getSingletonPtr()
{
    return ms_Singleton;
}

MaterialScriptManager &MaterialScriptManager::getSingleton()
{
    assert(ms_Singleton);
    return(*ms_Singleton);
}

MaterialScriptManager::MaterialScriptManager()
{
    mResourceType = "MaterialScript";

    // low, because it will likely reference other resources
    mLoadOrder = 30.0f;

    // this is how we register the ResourceManager with OGRE
    Ogre::ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
}

MaterialScriptManager::~MaterialScriptManager()
{
    // and this is how we unregister it
    Ogre::ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
}
size_t MaterialScript::calculateSize() const{
  size_t s=sizeof(void*)*4;
  for (std::vector<Ogre::String>::const_iterator i=provides.begin(),ie=provides.end();i!=ie;++i) {
    s+=sizeof(Ogre::String)+(*i).length();
  }
  for (std::vector<Ogre::String>::const_iterator i=depends_on.begin(),ie=depends_on.end();i!=ie;++i) {
    s+=sizeof(Ogre::String)+(*i).length();
  }
  return s;
}
/*
MaterialScriptPtr MaterialScriptManager::load(const Ogre::String &name, const Ogre::String &group)
{
    MaterialScriptPtr textf = getByName(name);

    if (textf.isNull())
        textf = create(name, group);

    textf->load();
    return textf;
    }*/

Ogre::Resource *MaterialScriptManager::createImpl(const Ogre::String &name, Ogre::ResourceHandle handle,
                                            const Ogre::String &group, bool isManual, Ogre::ManualResourceLoader *loader,
                                            const Ogre::NameValuePairList *createParams)
{
    return new MaterialScript(this, name, handle, group, isManual, loader,createParams);
}



void ManualMaterialLoader:: loadResource(Ogre::Resource *resource)   {
  assert("loadResource not actually implemented yet "&&0);
}


} // namespace Meru
