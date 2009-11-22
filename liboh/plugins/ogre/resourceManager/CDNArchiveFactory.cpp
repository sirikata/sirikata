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
#include "precomp.hpp"
#include "MeruDefs.hpp"
#include "CDNArchiveFactory.hpp"
#include "CDNArchive.hpp"

using namespace Ogre;

namespace Meru {

template<> CDNArchiveFactory* Ogre::Singleton<CDNArchiveFactory>::ms_Singleton = 0;

CDNArchiveFactory::CDNArchiveFactory() {
	mCurArchive = 0;
}

CDNArchiveFactory::~CDNArchiveFactory() {
}

void CDNArchiveFactory::removeUndesirables()
{
  while (!CDNArchiveToBeDeleted.empty()) {
    std::map<Ogre::String, std::pair<ResourceBuffer,unsigned int> >::iterator tbd=CDNArchiveFileRefcount.find(CDNArchiveToBeDeleted.back());
    if (tbd!=CDNArchiveFileRefcount.end()) {
      if (tbd->second.second==0) {
        CDNArchiveFileRefcount.erase(tbd);
      }
    }
    CDNArchiveToBeDeleted.pop_back();
  }
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
  removeUndesirables();
  return mCurArchive++;
}

unsigned int CDNArchiveFactory::addArchive(const Ogre::String&filename, const ResourceBuffer &rbuffer)
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  CDNArchivePackages[mCurArchive]=std::vector<Ogre::String>();
  removeUndesirables();
  addArchiveDataNoLock(mCurArchive, filename, rbuffer);
  return mCurArchive++;
}

void CDNArchiveFactory::addArchiveDataNoLock(unsigned int archiveName, const Ogre::String&filename, const ResourceBuffer &rbuffer)
{
  std::map<Ogre::String,std::pair<ResourceBuffer,unsigned int> >::iterator where=CDNArchiveFileRefcount.find(filename);
  if (where==CDNArchiveFileRefcount.end()) {
    SILOG(resource,debug,"File "<<filename<<" Added to CDNArchive");
    CDNArchiveFileRefcount[CDNArchive::canonicalizeHash(filename)]=std::pair<ResourceBuffer,unsigned int>(rbuffer,1);
  }
  else {
    SILOG(resource,debug,"File "<<filename<<" already downloaded to CDNArchive, what a waste! incref to "<<where->second.second+1);
    ++where->second.second;
  }
  CDNArchivePackages[archiveName].push_back(filename);
}

void CDNArchiveFactory::addArchiveData(unsigned int archiveName, const Ogre::String&filename, const ResourceBuffer &rbuffer)
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  addArchiveDataNoLock(archiveName,CDNArchive::canonicalMhashName(filename),rbuffer);
}

void CDNArchiveFactory::clearArchive(unsigned int which)
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  std::map<unsigned int, std::vector<Ogre::String> >::iterator where=CDNArchivePackages.find(which);
  if (where!=CDNArchivePackages.end()) {
    for (std::vector<Ogre::String>::iterator i=where->second.begin(),ie=where->second.end();i!=ie;++i) {
      std::map<Ogre::String,std::pair<ResourceBuffer,unsigned int> >::iterator where2=CDNArchiveFileRefcount.find(*i);
      if (where2!=CDNArchiveFileRefcount.end()) {
        if (where2->second.second==0||--where2->second.second==0) {
          CDNArchiveFileRefcount.erase(where2);
          SILOG(resource,debug,"File "<<*i<<" Removed from CDNArchive");
        }else {
          SILOG(resource,debug,"File "<<*i<<" Decref'd from CDNArchive to "<<where2->second.second);
        }
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
  removeUndesirables();
}

void CDNArchiveFactory::decref(const Ogre::String &name)
{
    boost::mutex::scoped_lock lok(CDNArchiveMutex);
    std::map<Ogre::String,std::pair<ResourceBuffer,unsigned int> >::iterator where =
        CDNArchiveFileRefcount.find(name);
    if (where!=CDNArchiveFileRefcount.end()) {
      if (where->second.second==0){
        SILOG(resource,error,"File "<<name<< " Not in CDNArchive Map already has refcount=0");
      }else {
        if (--where->second.second==0){
          CDNArchiveToBeDeleted.push_back(where->first);
        }
      }
    }else {
      SILOG(resource,error,"File "<<name<< " Not in CDNArchive Map to be removed upon close");
    }
}


} // namespace Meru

