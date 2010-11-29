/*  Meru
 *  CDNArchiveFactory.cpp
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

#include "CDNArchiveFactory.hpp"
#include "CDNArchive.hpp"

using namespace Ogre;

template<> Sirikata::Graphics::CDNArchiveFactory* Ogre::Singleton<Sirikata::Graphics::CDNArchiveFactory>::ms_Singleton = 0;

namespace Sirikata {
namespace Graphics {

using namespace Sirikata::Transfer;

CDNArchiveFactory::CDNArchiveFactory() {
	mCurArchive = 0;
}

CDNArchiveFactory::~CDNArchiveFactory() {
}

const Ogre::String& CDNArchiveFactory::getType() const {
	static const String name = "CDN";
	return name;
}

Ogre::Archive* CDNArchiveFactory::createInstance(const Ogre::String& name) {
	return new CDNArchive (this, name, getType());
}

void CDNArchiveFactory::destroyInstance(Ogre::Archive* archive) {
	delete archive;
}

unsigned int CDNArchiveFactory::addArchive()
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  CDNArchivePackages[mCurArchive]=std::vector<Ogre::String>();
  return mCurArchive++;
}

unsigned int CDNArchiveFactory::addArchive(const String& uri, const SparseData &rbuffer)
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  CDNArchivePackages[mCurArchive]=std::vector<Ogre::String>();
  addArchiveDataNoLock(mCurArchive, uri, rbuffer);
  return mCurArchive++;
}

void CDNArchiveFactory::addArchiveDataNoLock(unsigned int archiveName, const Ogre::String& uri, const SparseData &rbuffer)
{
    std::tr1::unordered_map<std::string,SparseData>::iterator where=CDNArchiveFiles.find(uri);
  if (where!=CDNArchiveFiles.end()) {
    SILOG(resource,error,"File "<<uri<<" Already exists in CDNArchive!!!");
  }
  SILOG(resource,debug,"File "<<uri<<" Adding to CDNArchive "<<(size_t)this);
  CDNArchiveFiles[uri]=rbuffer;

  CDNArchivePackages[archiveName].push_back(uri);
  SILOG(resource,debug,"File "<<uri<<" Added to CDNArchive "<<(size_t)this<< " success "<<(CDNArchiveFiles.find(uri)!=CDNArchiveFiles.end())<<" archive size "<<CDNArchiveFiles.size());
}

void CDNArchiveFactory::addArchiveData(unsigned int archiveName, const String &uri, const SparseData &rbuffer)
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  addArchiveDataNoLock(archiveName, uri,rbuffer);
}

void CDNArchiveFactory::clearArchive(unsigned int which)
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  std::map<unsigned int, std::vector<Ogre::String> >::iterator where=CDNArchivePackages.find(which);
  if (where!=CDNArchivePackages.end()) {
    for (std::vector<Ogre::String>::iterator i=where->second.begin(),ie=where->second.end();i!=ie;++i) {
      std::tr1::unordered_map<std::string,SparseData>::iterator where2=CDNArchiveFiles.find(*i);
      if (where2!=CDNArchiveFiles.end()) {
        // FIXME: clearArchive seems to get called too often, so texture files referenced in materials seem not to be found.
        //CDNArchiveFiles.erase(where2);
      }
    }
  }
}

void CDNArchiveFactory::removeArchive(unsigned int which)
{
  clearArchive(which);

  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  std::map<unsigned int, std::vector<Ogre::String> >::iterator where=CDNArchivePackages.find(which);
  if (where!=CDNArchivePackages.end()) {
    CDNArchivePackages.erase(where);
  }
}


} // namespace Graphics
} // namespace Sirikata
