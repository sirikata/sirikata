/*  Meru
 *  CDNArchiveFactory.hpp
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
#ifndef _CDN_ARCHIVE_FACTORY_HPP_
#define _CDN_ARCHIVE_FACTORY_HPP_

#include "../OgreHeaders.hpp"
#include <Ogre.h>
#include <OgreArchiveFactory.h>
#include <OgreSingleton.h>
#include <sirikata/core/transfer/TransferData.hpp>

namespace Sirikata {
namespace Graphics {

/** Archive factory for URLArchives, a specialization of Ogre::ArchiveFactory.
 *  See Ogre's documentation for interface details and documentation.
 */
class CDNArchiveFactory : public Ogre::ArchiveFactory, public Ogre::Singleton<CDNArchiveFactory> {

  friend class CDNArchive;

  void addArchiveDataNoLock(unsigned int archiveName, const Ogre::String &filename, const Transfer::SparseData &rbuffer);

  boost::mutex CDNArchiveMutex;
  std::map<unsigned int, std::vector <Ogre::String> > CDNArchivePackages;
  std::tr1::unordered_map<std::string, Transfer::SparseData > CDNArchiveFiles;
  int mCurArchive;

public:

	CDNArchiveFactory();
	~CDNArchiveFactory();
	const Ogre::String& getType() const;
	Ogre::Archive* createInstance(const Ogre::String& name);
	void destroyInstance(Ogre::Archive* archive);

  /**
   * Add a specific data stream to a package that will be hung onto until removeArchive is called
   */
  void addArchiveData(unsigned int archiveName, const String &uri, const Transfer::SparseData &rbuffer);

  /**
   * Adds a package to the CDNArchive that will stay open until all items are used
   * Must be called from main thread
   */
  unsigned int addArchive();

  /**
   * Adds a package to the CDNArchive and one instance of filename/data
   */
  unsigned int addArchive(const String& uri, const Transfer::SparseData &rbuffer);

  /**
   * Removes a package from CDNArchive that may be later opened by ogre
   * Must be called from main thread.
   */
  void removeArchive(unsigned int name);

  /**
   * Removes all files from a CDNArchive.
   * Must be called from main thread.
   */
  void clearArchive(unsigned int name);

};

} // namespace Graphics
} // namespace Sirikata

#endif //_URL_ARCHIVE_FACTORY_HPP_
