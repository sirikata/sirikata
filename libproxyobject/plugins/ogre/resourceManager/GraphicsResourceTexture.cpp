/*  Meru
 *  GraphicsResourceTexture.cpp
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
#include "GraphicsResourceTexture.hpp"
#include "ResourceDependencyTask.hpp"
#include "ResourceLoadTask.hpp"
#include "ResourceLoadingQueue.hpp"
#include "ResourceUnloadTask.hpp"
#include "../meruCompat/DependencyManager.hpp"
#include <boost/bind.hpp>
#include <OgreResourceBackgroundQueue.h>

namespace Meru {

class TextureDependencyTask : public ResourceDependencyTask
{
public:
  TextureDependencyTask(DependencyManager* mgr, WeakResourcePtr resource, const URI &uri);
  virtual ~TextureDependencyTask();

  virtual void operator()();
};

class TextureLoadTask : public ResourceLoadTask
{
public:
  TextureLoadTask(DependencyManager *mgr, SharedResourcePtr resource, const URI &uri, unsigned int epoch);

  virtual void doRun();

protected:
  unsigned int mArchiveName;
};

class TextureUnloadTask : public ResourceUnloadTask
{
public:
  TextureUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const URI &uri, unsigned int epoch);

  virtual void doRun();

protected:
  //bool mainThreadUnload(String name);
};


class TextureDownloadTask : public DependencyTask, public ResourceRequestor
{

  // simplified dds header struct with the minimum data needed
  enum {
    DDS_MAGIC = 0,
    DDS_SIZE = 1,
    DDS_FLAGS = 2,
    DDS_HEIGHT = 3,
    DDS_WIDTH = 4,
    DDS_PITCH = 5,
    DDS_DEPTH = 6,
    DDS_MIPMAPCOUNT = 7,
    DDS_PIXEL_SIZE = 19,
    DDS_PIXEL_FLAGS = 20,
    DDS_FOURCC = 21,
    DDS_RGB_BPP = 22,
    DDSHEADER_COUNT = 32,
    DDSHEADER_BYTESIZE = DDSHEADER_COUNT*4
  };
  /*
  struct DDSHEADER {
    char cMagic[4];
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    DWORD dwPitchOrLinearSize;
    DWORD dwDepth;
    DWORD dwMipMapCount;
    DWORD dwReserved1[11];
    DWORD dwSizeOfPixelFormat;
    DWORD dwFlagsOfPixelFormat;
    char cFourCharIdOfPixelFormat[4];
    char dataThatWeDoNotNeed[40];
  };
  */
  // The DDS format is stored in 32-bit little endian.
  static uint32 getDWORD(const unsigned char *data, int which) {
    return ((uint32)(data[which*4+0]))
      | ((uint32)(data[which*4+1]))<<8
      | ((uint32)(data[which*4+2]))<<16
      | ((uint32)(data[which*4+3]))<<24;
  }
  static void setDWORD(unsigned char *data, int which, uint32 value) {
    data[which*4+0] = (value)&255;
    data[which*4+1] = (value>>8)&255;
    data[which*4+2] = (value>>16)&255;
    data[which*4+3] = (value>>24)&255;
  }
  static bool checkFourCC(const unsigned char *data, int which,
                          const char *checkAgainst) {
    return !memcmp(data + which*4, checkAgainst, 4);
  }

    Sirikata::ProxyObjectPtr mProxy;
  ResourceDownloadTask *mHeaderTask;
  ResourceDownloadTask *mDataTask;
  ResourceRequestor* mOrigResourceRequestor;
  unsigned int mMaxDimension;

  size_t determineDownloadRange(unsigned char *header);
public:
    const URI &mURI;
    TextureDownloadTask(DependencyManager* mgr, const URI& uri, unsigned int maxDim, ResourceRequestor* resourceRequestor, Sirikata::ProxyObjectPtr proxy);
  virtual ~TextureDownloadTask();

  virtual void operator()();

  virtual void setResourceBuffer(const SparseData& buffer);
};


GraphicsResourceTexture::GraphicsResourceTexture(const URI &uri, Sirikata::ProxyObjectPtr proxy)
 : GraphicsResourceAsset(uri, GraphicsResource::TEXTURE, proxy)
{

}

GraphicsResourceTexture::~GraphicsResourceTexture()
{
  if (mLoadState == LOAD_LOADED)
    doUnload();
}

int GraphicsResourceTexture::maxDimension() const {
    return 512;
}

DependencyTask* GraphicsResourceTexture::createDownloadTask(DependencyManager *manager, ResourceRequestor *resourceRequestor)
{
    return new TextureDownloadTask(manager, mURI, maxDimension(), resourceRequestor, mProxy);
}

ResourceDependencyTask* GraphicsResourceTexture::createDependencyTask(DependencyManager *manager)
{
  return new TextureDependencyTask(manager, getWeakPtr(), mURI);
}

ResourceLoadTask* GraphicsResourceTexture::createLoadTask(DependencyManager *manager)
{
  return new TextureLoadTask(manager, getSharedPtr(), mURI, mLoadEpoch);
}

ResourceUnloadTask* GraphicsResourceTexture::createUnloadTask(DependencyManager *manager)
{
  return new TextureUnloadTask(manager, getWeakPtr(), mURI, mLoadEpoch);
}

/***************************** TEXTURE DEPENDENCY TASK *************************/

TextureDependencyTask::TextureDependencyTask(DependencyManager *mgr, WeakResourcePtr resource, const URI& uri)
  : ResourceDependencyTask(mgr, resource, uri)
{

}

TextureDependencyTask::~TextureDependencyTask()
{

}

void TextureDependencyTask::operator()()
{
  SharedResourcePtr resourcePtr = mResource.lock();
  if (!resourcePtr) {
    finish(false);
    return;
  }

  resourcePtr->setCost(mBuffer.size());
  resourcePtr->parsed(true);

  finish(true);
}

/***************************** TEXTURE LOAD TASK *************************/

TextureLoadTask::TextureLoadTask(DependencyManager *mgr, SharedResourcePtr resourcePtr, const URI &uri, unsigned int epoch)
: ResourceLoadTask(mgr, resourcePtr, uri, epoch)
{
}

void TextureLoadTask::doRun()
{
    mArchiveName = CDNArchiveFactory::getSingleton().addArchive(mURI.toString(), mBuffer);
  Ogre::TextureManager::getSingleton().load(mURI.toString(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
  CDNArchiveFactory::getSingleton().removeArchive(mArchiveName);
  mResource->loaded(true, mEpoch);
}

/***************************** TEXTURE UNLOAD TASK *************************/

TextureUnloadTask::TextureUnloadTask(DependencyManager *mgr, WeakResourcePtr resource, const URI &uri, unsigned int epoch)
: ResourceUnloadTask(mgr, resource, uri, epoch)
{

}
/*
bool TextureUnloadTask::mainThreadUnload(String name)
{
  Ogre::TextureManager::getSingleton().unload(name);
  operationCompleted(Ogre::BackgroundProcessTicket(), Ogre::BackgroundProcessResult());
  return false;
}
*/
void TextureUnloadTask::doRun()
{
  /*I REALLY wish this were true*/
  //    SequentialWorkQueue::getSingleton().queueWork(std::tr1::bind(&TextureUnloadTask::mainThreadUnload, this, mHash));

  Ogre::TextureManager* textureManager = Ogre::TextureManager::getSingletonPtr();
  textureManager->remove(mURI.toString());

  Ogre::ResourcePtr textureResource = textureManager->getByName(mURI.toString());
  assert(textureResource.isNull());

  SharedResourcePtr resource = mResource.lock();
  if (resource)
    resource->unloaded(true, mEpoch);
}

/***************************** TEXTURE DOWNLOAD TASK *************************/

TextureDownloadTask::TextureDownloadTask(DependencyManager* mgr, const URI& uri, unsigned int maxDim, ResourceRequestor* resourceRequestor, Sirikata::ProxyObjectPtr proxy)
 : DependencyTask(mgr->getQueue()),
   mProxy(proxy),
   mURI(uri)
{
    mHeaderTask = new ResourceDownloadTask(mgr, uri, this, mProxy->priority, NULL);
    mHeaderTask->addDepender(this);
    mDataTask = new ResourceDownloadTask(mgr, uri, this, mProxy->priority, NULL);
    mDataTask->addDepender(this);

	mHeaderTask->setRange(Transfer::Range(0, DDSHEADER_BYTESIZE, Transfer::BOUNDS));
    mHeaderTask->go();

    mMaxDimension = maxDim;
    mOrigResourceRequestor = resourceRequestor;
}

TextureDownloadTask::~TextureDownloadTask() {
}

enum {DDPF_RGB = 0x00000040};

size_t TextureDownloadTask::determineDownloadRange(unsigned char *header) {
    if (!checkFourCC(header, DDS_MAGIC, "DDS ")) {
        // attempt a normal download.
        SILOG(resource,insane,"File "<<mURI.toString()<<" not a dds: "<< (char)header[0]<<(char)header[1]<<(char)header[2]<<(char)header[3]<<(char)header[4]);
        return 0;
    }
    if (getDWORD(header, DDS_SIZE) + 4 > DDSHEADER_BYTESIZE) {
        // Don't have complete header. Might be a DX10 DDS file.
        return 0;
    }
    // read the header
    int DXT_ver = 0;
    if (checkFourCC(header, DDS_FOURCC, "DXT1")) { DXT_ver = 1; }
    else if (checkFourCC(header, DDS_FOURCC, "DXT2")) { DXT_ver = 2; }
    else if (checkFourCC(header, DDS_FOURCC, "DXT3")) { DXT_ver = 3; }
    else if (checkFourCC(header, DDS_FOURCC, "DXT4")) { DXT_ver = 4; }
    else if (checkFourCC(header, DDS_FOURCC, "DXT5")) { DXT_ver = 5; }
    else if (!(getDWORD(header, DDS_FLAGS) & DDPF_RGB)) {
        // unknown type: attempt a normal download.
        return 0;
    }
    uint32 width = getDWORD(header, DDS_WIDTH);
    uint32 height = getDWORD(header, DDS_HEIGHT);
    uint32 rgb_pitch = getDWORD(header, DDS_PITCH);
    uint32 rgb_bpp = getDWORD(header, DDS_RGB_BPP);
    uint32 num_mipmaps = getDWORD(header, DDS_MIPMAPCOUNT);
    uint32 dxt_bytes_per_block = (DXT_ver==1) ? 8 : 16;
    if (DXT_ver) {
        if (rgb_pitch != dxt_bytes_per_block * ((width + 3)/4) * ((height + 3)/4)) {
            SILOG(resource,error,"LinearSize in RGB DDS file "<<rgb_pitch<<" does not match: width "<<width<<" bytes/blk "<<dxt_bytes_per_block<<" height "+height);
        }
    } else {
        if (rgb_pitch != 4 * ((width * rgb_bpp + 24) / 32)) {
            SILOG(resource,error,"Pitch in RGB DDS file "<<rgb_pitch<<" does not match: width "<<width<<" bpp "<<rgb_bpp);
        }
    }
    size_t data_to_skip = 0;
    // http://doc.51windows.net/Directx9_SDK/?url=/directx9_sdk/graphics/reference/DDSFileReference/ddsfileformat.htm
    while (num_mipmaps > 1) {
        if (width < 1) { width = 1; }
        if (height < 1) { height = 1; }
        rgb_pitch = 4 * ((width * rgb_bpp + 24) / 32);
        size_t num_bytes;
        if (DXT_ver) {
            num_bytes = dxt_bytes_per_block * ((width + 3)/4) * ((height + 3)/4);
        } else {
            num_bytes = height * rgb_pitch; // DWORD aligned.
        }
        if (width <= mMaxDimension && height <= mMaxDimension) {
            setDWORD(header, DDS_WIDTH, width < 1 ? 1 : width);
            setDWORD(header, DDS_HEIGHT, height < 1 ? 1 : height);
            if (DXT_ver) {
                // PITCH set to bytes per "scan line" whatever that means.
                setDWORD(header, DDS_PITCH, num_bytes);
            } else {
                setDWORD(header, DDS_PITCH, rgb_pitch);
            }
            setDWORD(header, DDS_MIPMAPCOUNT, num_mipmaps);
            return data_to_skip;
        }
        data_to_skip += num_bytes;
        --num_mipmaps;
        width /= 2;
        height /= 2;
    }
    // File has no mipmaps, or is broken, or...
    return 0;
}

void TextureDownloadTask::operator()() {
    finish(true);
}

void TextureDownloadTask::setResourceBuffer(const SparseData &sdata)
{
    if (mHeaderTask) {
        mHeaderTask = NULL;
        mDataTask->mergeData(sdata);
        if (sdata.contains(Transfer::Range(0, DDSHEADER_BYTESIZE, Transfer::BOUNDS))) {
            unsigned char header [DDSHEADER_BYTESIZE];
            std::copy (sdata.begin(), sdata.begin()+DDSHEADER_BYTESIZE, header);
            size_t dataToSkip = determineDownloadRange(header);
            if (dataToSkip > 0) {
                Transfer::Range range(DDSHEADER_BYTESIZE + dataToSkip, true);
                mDataTask->setRange(range);
            } else {
                SILOG(resource, insane, "Texture without mipmaps "<<mURI.toString());
            }
        } else {
            SILOG(resource, insane, "Texture does not contain header "<<mURI.toString());
        }
        mDataTask->go();
        return;
    }
    /* Check size because a 1x1 png file takes less space than the DDS header.
     * Parse the header in its entirety one more time,
     * since an evil server could send malformed data the second time.
     * In addition, some servers may not support partial GET requests,
     * but we still want to pick a lower mipmap.
     */
    Transfer::SparseData finalData = sdata;
    if (sdata.contains(Transfer::Range(0, DDSHEADER_BYTESIZE, Transfer::BOUNDS))) {
        unsigned char header [DDSHEADER_BYTESIZE];
        // Get header array.
        std::copy (sdata.begin(), sdata.begin()+DDSHEADER_BYTESIZE, header);
        size_t dataToSkip = determineDownloadRange(header);
        if (dataToSkip > 0) {
            // Would like to skip some mipmaps.
            Transfer::Range sourceRange(dataToSkip+DDSHEADER_BYTESIZE, true);
            if (!sdata.contains(sourceRange)) {
                // The DDS file is missing data at the mipmap we want.
                std::ostringstream os;
                Transfer::Range::printRangeList<Transfer::DenseDataList>(os, sdata, sourceRange);
                SILOG(resource, error, "Partial DDS bad sparse data "
                      <<" want "<<(dataToSkip+DDSHEADER_BYTESIZE)
                      <<" have "<<os.str());
                return; // We have a corrupt texture--not what we asked for!
            }
            Transfer::SparseData::const_iterator iter = sdata.begin();
            iter += dataToSkip+DDSHEADER_BYTESIZE;
            // Allocate some new data (Would be nice to be able to point at the old data...)
            Transfer::Range range(0, DDSHEADER_BYTESIZE + (sdata.end()-iter),
                                  Transfer::BOUNDS, true);
            Transfer::MutableDenseDataPtr fixedData (new Transfer::DenseData(range));
            // Copy header from source to destination.
            std::memcpy(fixedData->writableData(), header, DDSHEADER_BYTESIZE);
            // Copy starting from mipmap all the way to the end from source to destination.
            std::copy(iter, sdata.end(), fixedData->writableData()+DDSHEADER_BYTESIZE);
            mOrigResourceRequestor->setResourceBuffer(SparseData(fixedData));
            return;
        }
    }
    mOrigResourceRequestor->setResourceBuffer(sdata);
}

}
