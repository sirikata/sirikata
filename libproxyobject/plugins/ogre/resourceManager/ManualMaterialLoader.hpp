/*  Meru
 *  ManualMaterialLoader.hpp
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
#ifndef _MANUAL_MATERIAL_LOADER_
#define _MANUAL_MATERIAL_LOADER_

#include "../meruCompat/MeruDefs.hpp"
#include <OgreResourceManager.h>
#include "../meruCompat/Event.hpp"

namespace Meru {
namespace EventTypes {
	extern const EventType MaterialLoaded;
}
class MaterialLoadedEvent:public Event {
public:
    MaterialLoadedEvent(const String&material_script_name):Event(EventID(EventTypes::MaterialLoaded,material_script_name)) {}
};
class LoadPreparedMaterial;
class MaterialScript : public Ogre::Resource
{
  friend class LoadPreparedMaterial;
  bool mTexturesInRam;
  const Ogre::NameValuePairList*mTextureAliases;
  std::vector<Ogre::String> provides;//materials and programs this provides
  std::vector<Ogre::String> depends_on;//materials and programs this depends on
  Ogre::DataStreamPtr mSourceStream;
  Ogre::DataStreamPtr mStreamToBeLoadedInForeground;
  //FIXME maybe textures and .cg files it depends on?
protected:
    // must implement these from the Ogre::Resource interface
    void parseScript();
    void prepareImpl();
    void unprepareImpl();
    void loadImpl();
    void postLoadImpl();
    void unloadImpl();
    size_t calculateSize()const;
public:
    bool texturesLoaded(){return mTexturesInRam;}
    MaterialScript(Ogre::ResourceManager *creator, const Ogre::String &name,
        Ogre::ResourceHandle handle, const Ogre::String &group, bool isManual = false,
        Ogre::ManualResourceLoader *loader = 0,const Ogre::NameValuePairList*createParams=NULL);
    void forceLoad(){
        postLoadImpl();
    }
    virtual ~MaterialScript();
};

class MaterialScriptPtr : public Ogre::SharedPtr<MaterialScript>
{
public:
    MaterialScriptPtr() : Ogre::SharedPtr<MaterialScript>() {}
    explicit MaterialScriptPtr(MaterialScript *rep) : Ogre::SharedPtr<MaterialScript>(rep) {}
    MaterialScriptPtr(const MaterialScriptPtr &r) : Ogre::SharedPtr<MaterialScript>(r) {}
    MaterialScriptPtr(const Ogre::ResourcePtr &r) : Ogre::SharedPtr<MaterialScript>()
    {
        // lock & copy other mutex pointer
        OGRE_LOCK_MUTEX(*r.OGRE_AUTO_MUTEX_NAME)
            OGRE_COPY_AUTO_SHARED_MUTEX(r.OGRE_AUTO_MUTEX_NAME)
            pRep = static_cast<MaterialScript*>(r.getPointer());
        pUseCount = r.useCountPointer();
        if (pUseCount)
        {
            ++(*pUseCount);
        }
    }

    /// Operator used to convert a ResourcePtr to a MaterialScriptPtr
    MaterialScriptPtr& operator=(const Ogre::ResourcePtr& r)
    {
        if (pRep == static_cast<MaterialScript*>(r.getPointer()))
            return *this;
        release();
        // lock & copy other mutex pointer
        OGRE_LOCK_MUTEX(*r.OGRE_AUTO_MUTEX_NAME)
            OGRE_COPY_AUTO_SHARED_MUTEX(r.OGRE_AUTO_MUTEX_NAME)
            pRep = static_cast<MaterialScript*>(r.getPointer());
        pUseCount = r.useCountPointer();
        if (pUseCount)
        {
            ++(*pUseCount);
        }
        return *this;
    }
};
class MaterialScriptManager : public Ogre::ResourceManager, public Ogre::Singleton<MaterialScriptManager>
{
protected:

    // must implement this from ResourceManager's interface
    Ogre::Resource *createImpl(const Ogre::String &name, Ogre::ResourceHandle handle,
        const Ogre::String &group, bool isManual, Ogre::ManualResourceLoader *loader,
        const Ogre::NameValuePairList *createParams);

public:

    MaterialScriptManager();
    virtual ~MaterialScriptManager();

//    virtual MaterialScriptPtr load(const Ogre::String &name, const Ogre::String &group);

    static MaterialScriptManager &getSingleton();
    static MaterialScriptManager *getSingletonPtr();
};
class ManualMaterialLoader : public Ogre::ManualResourceLoader
{
public:

  ManualMaterialLoader() {}
  virtual ~ManualMaterialLoader() {}

  virtual void loadResource(Ogre::Resource *resource);
};

} // namespace Meru

#endif //_MANUAL_MATERIAL_LOADER_
