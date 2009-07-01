/*  Meru
 *  GraphicsResourceMaterial.cpp
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
#include "CDNArchive.hpp"
#include "GraphicsResourceManager.hpp"
#include "GraphicsResourceMaterial.hpp"
#include "ManualMaterialLoader.hpp"
#include "ResourceDependencyTask.hpp"
#include "ResourceLoadTask.hpp"
#include "ResourceLoadingQueue.hpp"
#include "ResourceUnloadTask.hpp"
#include "SequentialWorkQueue.hpp"
#include <OgreResourceBackgroundQueue.h>
#include <boost/regex.hpp>

namespace Meru {

class MaterialDependencyTask : public ResourceDependencyTask
{
public:
  MaterialDependencyTask(DependencyManager* mgr, WeakResourcePtr resource, const String& hash);
  virtual ~MaterialDependencyTask();

  virtual void operator()();
};

class MaterialLoadTask : public ResourceLoadTask
{
public:
  MaterialLoadTask(DependencyManager *mgr, SharedResourcePtr resource, const String &hash, unsigned int archiveName, Ogre::NameValuePairList *textureAliases, unsigned int epoch);

  virtual void doRun();

protected:
  const unsigned int mArchiveName;
  Ogre::NameValuePairList* mTextureAliases;
};

class MaterialUnloadTask : public ResourceUnloadTask
{
public:
  MaterialUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const String &hash, unsigned int archiveName, unsigned int epoch);

  virtual void doRun();

protected:
  const unsigned int mArchiveName;
};

GraphicsResourceMaterial::GraphicsResourceMaterial(const RemoteFileId &resourceID)
  : GraphicsResourceAsset(resourceID, GraphicsResource::MATERIAL),
  mArchiveName(CDNArchive::addArchive())
{

}

GraphicsResourceMaterial::~GraphicsResourceMaterial()
{
  if (mLoadState == LOAD_LOADED)
    doUnload();
}

void GraphicsResourceMaterial::resolveName(const URI& id, const URI& hash)
{
  mTextureAliases[id.toString()] = CDNArchive::canonicalMhashName(hash.toString());
}

ResourceDownloadTask* GraphicsResourceMaterial::createDownloadTask(DependencyManager *manager, ResourceRequestor *resourceRequestor)
{
  return new ResourceDownloadTask(manager, mResourceID, resourceRequestor);
}

ResourceDependencyTask* GraphicsResourceMaterial::createDependencyTask(DependencyManager *manager)
{
  return new MaterialDependencyTask(manager, getWeakPtr(), mResourceID.toString());
}

ResourceLoadTask* GraphicsResourceMaterial::createLoadTask(DependencyManager *manager)
{
  return new MaterialLoadTask(manager, getSharedPtr(), mResourceID.toString(), mArchiveName, &mTextureAliases, mLoadEpoch);
}

ResourceUnloadTask* GraphicsResourceMaterial::createUnloadTask(DependencyManager *manager)
{
  return new MaterialUnloadTask(manager, getWeakPtr(), mResourceID.toString(), mArchiveName, mLoadEpoch);
}

/***************************** MATERIAL DEPENDENCY TASK *************************/

MaterialDependencyTask::MaterialDependencyTask(DependencyManager *mgr, WeakResourcePtr resource, const String& hash)
  : ResourceDependencyTask(mgr, resource, hash)
{

}

MaterialDependencyTask::~MaterialDependencyTask()
{

}

// TODO: This code is borrowed from ReplacingDataStream.cpp (exactly identical except for some changes for
//       personal readability) - repeated code is not so great, maybe centralize this somewhere, later.
void find_lexeme (const MemoryBuffer &input,
                  MemoryBuffer::size_type &where_lexeme_start,
                  MemoryBuffer::size_type &return_lexeme_end)
{
  MemoryBuffer::const_iterator iter = input.begin() + where_lexeme_start;
  return_lexeme_end = input.size();

  while (where_lexeme_start != return_lexeme_end) {
    char c = *iter;
    if (!isspace(c)) {
      if ((c == '/') && (where_lexeme_start + 1 != return_lexeme_end) && (*(iter+1) == '/')) {
        //comment: find end of line
        while (where_lexeme_start++ != return_lexeme_end) {
          ++iter;
          if (*iter == '\n')
            break;
        }
      }
      else {
        break; //beginning our quest for the end token
      }
    }
    ++iter;
    ++where_lexeme_start;
  }
  return_lexeme_end = where_lexeme_start;
  bool comment_immunity=false;
  while (return_lexeme_end < input.size()) {
    char c = *iter;
    if (c=='\"') {
      comment_immunity=!comment_immunity;
      if (return_lexeme_end==where_lexeme_start)
        ++where_lexeme_start;
    }
    if ((isspace(c)) || (c == '{') || ((c == '/'&&comment_immunity==false) && (return_lexeme_end + 1 < input.size()) && (*(iter+1) == '/') && ((return_lexeme_end == where_lexeme_start) || (*(iter-1) != ':')))) {
      break;
    }
    ++iter;
    ++return_lexeme_end;
  }
  if (return_lexeme_end>where_lexeme_start&&*--iter=='\"') {
    --return_lexeme_end;
  }
}

MemoryBuffer::const_iterator rfind(MemoryBuffer::const_iterator begin, MemoryBuffer::const_iterator end, char dat)
{
  while (end != begin) {
    if (dat == *--end)
      return end;
  }
  return begin;
}

bool next_eol(const MemoryBuffer& input, MemoryBuffer::size_type& where_lexeme_start);

void MaterialDependencyTask::operator()()
{
  SharedResourcePtr resourcePtr = mResource.lock();
  if (!resourcePtr) {
    finish(false);
    return;
  }

  GraphicsResourceManager *grm = GraphicsResourceManager::getSingletonPtr();

  // TODO: Fix this code so it doesn't make me want to blow my brains out
  boost::match_results<MemoryBuffer::const_iterator> what;
  boost::match_flag_type flags = boost::match_default;
  MemoryBuffer::size_type lexemeBeginIndex = 0;
  MemoryBuffer::size_type lexemeEndIndex = 0;
  // This regular expression pattern will help find dependencies in the material file.
  static const char *programRefAndTextureUnitPattern = "(?:[[.newline.]](?:(?:(?:/[^/\n])|[^/\n])*(?#The left takes care of commented out lines -- newline then slashes followed by not slashes or not slashes )(?:[[:space:]]|\n))?((?:delegate|vertex_program_ref|shadow_caster_vertex_program_ref|fragment_program_ref|shadow_caster_fragment_program_ref|shadow_receiver_fragment_program_ref|shadow_receiver_vertex_program_ref)(?#To the left specifies all the types of references available in ogre...they must be preceeded by a space to be sure that we dont get vertex_program_ref counted as well as shadow_receiver_vertex_program_ref)(?:[[:space:]]|\n)+))(?# space then the goods come next)|(?:[[.newline.]](?:(?:(?:/[^/\n])|[^/\n])*(?#Make sure no comments exist in the line, same as starting exp)(?:[[:space:]]|\n)+)?((?:(?:texture)|(?:anim_texture)|(?:cubic_texture))(?#to the left is the specification of any texture references that could occur...then a space, and the goods)(?:[[:space:]]|\n)+))|(?:[[.newline.]](?:(?:(?:/[^/\n])|[^/\n])*(?#make sure source line is not commented out)(?:[[:space:]]|\n)+)?(source(?:[[:space:]]|\n)+))(?#The goods--source followed by space)|(?:^[[:space:]]*(import(?:[[:space:]]|\n)+[^[.NUL.]]+?(?:[[:space:]]|\n)+from(?#before the _from_ comes a bunch of junk but then its from and the file we care about)[[:space:]]+))";
  static const boost::regex expression(programRefAndTextureUnitPattern);

  // Parse material to find dependencies.
  MemoryBuffer::const_iterator start, end;
  start = mBuffer.begin();
  end = mBuffer.end();

  while (boost::regex_search(start, end, what, expression, flags)) {
    start = what[0].second;
    if (what[0].matched) {
      // Dependency found - set lexeme indices.
      lexemeBeginIndex = what[0].second - mBuffer.begin();
      find_lexeme(mBuffer, lexemeBeginIndex, lexemeEndIndex);
    }
    if (what[1].matched) {
      // Material dependency has been found.
      // ignore protocol for now
      MemoryBuffer::const_iterator protocol = std::find(mBuffer.begin() + lexemeBeginIndex, mBuffer.begin() + lexemeEndIndex,':');
      if (protocol == mBuffer.begin() + lexemeEndIndex)
        protocol = mBuffer.begin() + lexemeBeginIndex;
      MemoryBuffer::const_iterator midpoint = rfind(protocol, mBuffer.begin() + lexemeEndIndex, ':');
      if (protocol != mBuffer.begin() + lexemeEndIndex && *protocol == ':')
        ++protocol; // : get past the colon
      if (protocol != mBuffer.begin() + lexemeEndIndex && *protocol == '/')
        ++protocol; // / slash
      if (protocol != mBuffer.begin() + lexemeEndIndex && *protocol == '/')
        ++protocol; // / slash

      /* LEGACY CODE... DELETE UNTIL ENDLEGACYCODE */
      if (midpoint < protocol) {
        midpoint = std::find(protocol, MemoryBuffer::const_iterator(mBuffer.begin() + lexemeEndIndex), ':'); //FIXME legacy code <-- delete this line when second layer worked out
        if (midpoint != mBuffer.begin()+lexemeEndIndex) {
          MemoryBuffer::const_iterator start_index=protocol;
          Ogre::String dependencyName (start_index,midpoint);
          if (dependencyName.size()) {
            SILOG(resource,debug,"Found LEGACY dependency "<<dependencyName);
            SharedResourcePtr hashResource = grm->getResourceAsset(URI(dependencyName), GraphicsResource::MATERIAL);
            resourcePtr->addDependency(hashResource);
          }
        }
      }
      /* ENDLEGACYCODE */
      else if (midpoint != mBuffer.begin()+lexemeEndIndex) {
        MemoryBuffer::const_iterator start_index = mBuffer.begin() + lexemeBeginIndex;
        Ogre::String dependencyName(start_index, midpoint);
        if (dependencyName.size()) {
          SILOG(resource,debug,"Found Normal dependency "<<dependencyName);
          SharedResourcePtr hashResource = grm->getResourceAsset(URI(dependencyName), GraphicsResource::MATERIAL);
          resourcePtr->addDependency(hashResource);
        }
      }
    }
    else if (what[2].matched || what[3].matched) {
      // Texture or program source dependency has been found.
      if (lexemeBeginIndex < lexemeEndIndex) {
        Ogre::String dependencyName(MemoryBuffer::const_iterator(mBuffer.begin() + lexemeBeginIndex), MemoryBuffer::const_iterator(mBuffer.begin() + lexemeEndIndex));
        if (dependencyName.size()/* && dependencyName.find("_noon") == std::string::npos*/) {
          // Add dependency
          if (what[2].matched) {
            SILOG(resource,debug,"Found texture/program dependency "<<dependencyName);
            {
              URI dependencyURI(dependencyName);
              if (!dependencyURI.proto().empty()) {
                SharedResourcePtr hashResource = grm->getResourceAsset(dependencyURI, GraphicsResource::TEXTURE);
                if (hashResource)
                  resourcePtr->addDependency(hashResource);
              }
            }

            static const int anim_texture_len=strlen("anim_texture");
            static const int cubic_texture_len=strlen("cubic_texture");
            if (what[2].second-what[2].first>anim_texture_len
              &&*what[2].first=='a'
              &&*(what[2].first+1)=='n'
              &&*(what[2].first+2)=='i'
              &&*(what[2].first+3)=='m'
              &&*(what[2].first+4)=='_'
              &&*(what[2].first+5)=='t'
              &&*(what[2].first+6)=='e'
              &&*(what[2].first+7)=='x'
              &&*(what[2].first+8)=='t'
              &&*(what[2].first+9)=='u'
              &&*(what[2].first+10)=='r'
              &&*(what[2].first+11)=='e'
              &&isspace(*(what[2].first+12))){
                while (!next_eol(mBuffer,lexemeEndIndex)){
                  lexemeBeginIndex=lexemeEndIndex;
                  find_lexeme(mBuffer, lexemeBeginIndex, lexemeEndIndex);
                  Ogre::String dependencyName(MemoryBuffer::const_iterator(mBuffer.begin() + lexemeBeginIndex), MemoryBuffer::const_iterator(mBuffer.begin() + lexemeEndIndex));
                  if (next_eol(mBuffer,lexemeEndIndex)){
                    break;//framerate is last, don't add to dependency
                  }
                  URI dependencyURI(dependencyName);
                  if (!dependencyURI.proto().empty()) {
                    SILOG(resource,debug,"Found ANIM texture dependency "<<dependencyName);
                    SharedResourcePtr hashResource = grm->getResourceAsset(dependencyURI, GraphicsResource::TEXTURE);
                    resourcePtr->addDependency(hashResource);
                  }
                }
            }else if (what[2].second-what[2].first>cubic_texture_len
              &&*what[2].first=='c'
              &&*(what[2].first+1)=='u'
              &&*(what[2].first+2)=='b'
              &&*(what[2].first+3)=='i'
              &&*(what[2].first+4)=='c'
              &&*(what[2].first+5)=='_'
              &&*(what[2].first+6)=='t'
              &&*(what[2].first+7)=='e'
              &&*(what[2].first+8)=='x'
              &&*(what[2].first+9)=='t'
              &&*(what[2].first+10)=='u'
              &&*(what[2].first+11)=='r'
              &&*(what[2].first+12)=='e'
              &&isspace(*(what[2].first+13))){
                if (!next_eol(mBuffer,lexemeEndIndex)){
                  for (int i=0;i<5;++i) {
                    lexemeBeginIndex=lexemeEndIndex;
                    find_lexeme(mBuffer, lexemeBeginIndex, lexemeEndIndex);
                    Ogre::String dependencyName(MemoryBuffer::const_iterator(mBuffer.begin() + lexemeBeginIndex), MemoryBuffer::const_iterator(mBuffer.begin() + lexemeEndIndex));
                    if (next_eol(mBuffer,lexemeEndIndex)){
                      break;
                    }
                    URI dependencyURI(dependencyName);
                    if (!dependencyURI.proto().empty()) {
                      SILOG(resource,debug,"Found CUBIC texture dependency "<<dependencyName);
                      SharedResourcePtr hashResource = grm->getResourceAsset(dependencyURI, GraphicsResource::TEXTURE);
                      resourcePtr->addDependency(hashResource);
                    }
                  }
                }
            }

          }
          if (what[3].matched) {
            SILOG(resource,debug,"Found what[3] dependency "<<dependencyName);
            SharedResourcePtr hashResource = grm->getResourceAsset(URI(dependencyName), GraphicsResource::SHADER);
            resourcePtr->addDependency(hashResource);
          }
        }
      }
    }
    else if (what[4].matched) {
      Ogre::String dependencyName (MemoryBuffer::const_iterator(mBuffer.begin() + lexemeBeginIndex), MemoryBuffer::const_iterator(mBuffer.begin() + lexemeEndIndex));
      if (dependencyName.size()) {
        // this is actually a material reference
        SILOG(resource,debug,"Found material dependency "<<dependencyName);
        SharedResourcePtr hashResource = grm->getResourceAsset(URI(dependencyName), GraphicsResource::MATERIAL);
        resourcePtr->addDependency(hashResource);
      }
    }
  }

  resourcePtr->parsed(true);

  finish(true);
}

/***************************** MATERIAL LOAD TASK *************************/

MaterialLoadTask::MaterialLoadTask(DependencyManager *mgr, SharedResourcePtr resourcePtr, const String& hash, unsigned int archiveName, Ogre::NameValuePairList* textureAliases, unsigned int epoch)
: ResourceLoadTask(mgr, resourcePtr, hash, epoch), mArchiveName(archiveName), mTextureAliases(textureAliases)
{
}

void MaterialLoadTask::doRun()
{
  CDNArchive::addArchiveData(mArchiveName, CDNArchive::canonicalMhashName(mHash), mBuffer);
  MaterialScriptManager::getSingleton().load(CDNArchive::canonicalMhashName(mHash), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
    false, 0, mTextureAliases);

  mResource->loaded(true, mEpoch);
}

/***************************** MATERIAL UNLOAD TASK *************************/

MaterialUnloadTask::MaterialUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const String &hash, unsigned int archiveName, unsigned int epoch)
: ResourceUnloadTask(mgr, resource, hash, epoch), mArchiveName(archiveName)
{

}
/*
bool MaterialUnloadTask::mainThreadUnload(String name)
{
  Ogre::MaterialManager::getSingleton().unload(name);
  operationCompleted(Ogre::BackgroundProcessTicket(), Ogre::BackgroundProcessResult());
  return false;
}
*/
void MaterialUnloadTask::doRun()
{
  /*I REALLY wish this were true*/
  //    SequentialWorkQueue::getSingleton().queueWork(std::tr1::bind(&MaterialUnloadTask::mainThreadUnload, this, mHash));

  MaterialScriptManager* materialManager = MaterialScriptManager::getSingletonPtr();
  materialManager->remove(mHash);

  Ogre::ResourcePtr materialResource = materialManager->getByName(mHash);
  assert(materialResource.isNull());

  CDNArchive::clearArchive(mArchiveName);

  SharedResourcePtr resource = mResource.lock();
  if (resource)
    resource->unloaded(true, mEpoch);
}

}
