/*  Meru
 *  CDNArchive.hpp
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
#ifndef _CDN_ARCHIVE_HPP_
#define _CDN_ARCHIVE_HPP_

#include "MeruDefs.hpp"
#include <Ogre.h>
#include <vector>

namespace Meru {

/** A specialization of Ogre::Archive which gets its data through
 *  Meru's ResourceManager, and therefore indirectly via the web.
 *  See Ogre's documentation for details on the interface.
 */
class CDNArchive : public Ogre::Archive
{
  time_t getModifiedTime(const Ogre::String&);
  static void addArchiveDataNoLock(unsigned int archiveName, const Ogre::String &filename, const ResourceBuffer &rbuffer);
  unsigned int mNativeFileArchive;
public:

  /**
   * This function takes a mhash:// file descriptor and extracts the salient bits--- i.e. everything past the hint about where to get the hash based filename
   * i.e. mhash://meru/1bf00deadbeef turns into 1bf00deadbeef
   */
  static Ogre::String canonicalMhashName(const Ogre::String&filename);

  CDNArchive(const Ogre::String& name, const Ogre::String& archType);
  ~CDNArchive();

  /**
   * Adds a package to the CDNArchive that will stay open until all items are used
   * Must be called from main thread
   */
  static unsigned int addArchive();
  
  /**
   * Adds a package to the CDNArchive and one instance of filename/data
   */
  static unsigned int addArchive(const Ogre::String&filename, const ResourceBuffer &rbuffer);
  
  /**
   * Removes a package from CDNArchive that may be later opened by ogre
   * Must be called from main thread.
   */
  static void removeArchive(unsigned int name);

  /**
   * Removes all files from a CDNArchive.
   * Must be called from main thread.
   */
  static void clearArchive(unsigned int name);

  /** 
   * Add a specific data stream to a package that will be hung onto until removeArchive is called
   */
  static void addArchiveData(unsigned int archiveName, const Ogre::String &filename, const ResourceBuffer &rbuffer);
  bool isCaseSensitive() const;
  void load();
  void unload();
  Ogre::DataStreamPtr open(const Ogre::String& filename) const;
  Ogre::StringVectorPtr list(bool recursive = true, bool dirs = false);
  Ogre::FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false);
  Ogre::StringVectorPtr find(const Ogre::String& pattern, bool recursive = true, bool dirs = false);
  bool exists(const Ogre::String& filename);
  Ogre::FileInfoListPtr findFileInfo(const Ogre::String& pattern, bool recursive = true, bool dirs = false);
};
#define CDN_REPLACING_MATERIAL_STREAM_HINT "%%_%%"

} // namespace Meru

#endif //_CDN_ARCHIVE_HPP_

