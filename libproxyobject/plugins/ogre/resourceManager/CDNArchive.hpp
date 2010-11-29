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

#include <sirikata/proxyobject/Platform.hpp>
#include <Ogre.h>
#include <vector>
#include "CDNArchiveFactory.hpp"

namespace Sirikata {
namespace Graphics {

class CDNArchiveFactory;

/** A specialization of Ogre::Archive which gets its data through
 *  Meru's ResourceManager, and therefore indirectly via the web.
 *  See Ogre's documentation for details on the interface.
 */
class CDNArchive : public Ogre::Archive
{
  time_t getModifiedTime(const Ogre::String&);
  unsigned int mNativeFileArchive;
  CDNArchiveFactory *mOwner;
public:
  /**
   * This function takes a hash-based URI or filename and extracts the salient bits---
   * i.e. everything past the hint about where to get the hash based filename
   * i.e. mhash://meru/1bf00deadbeef turns into 1bf00deadbeef
   * This function also strips quotes as well as a preceeding %%_%% (CDN_REPLACING_MATERIAL_STREAM_HINT)
   */
  static String canonicalizeHash(const String&filename);

  CDNArchive(CDNArchiveFactory *owner, const Ogre::String& name, const Ogre::String& archType);
  ~CDNArchive();

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

} // namespace Graphics
} // namespace Sirikata

#define CDN_REPLACING_MATERIAL_STREAM_HINT "%%_%%"

#endif //_CDN_ARCHIVE_HPP_
