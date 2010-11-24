/*  Meru
 *  CDNArchive.cpp
 *
 *  Copyright (c) 2009, Stanford University
 *  All rights reserved. Go Bears! We beat you 34-28!
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
#include <cassert>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include "ReplacingDataStream.hpp"

namespace Sirikata {
namespace Graphics {

static const unsigned char white_png[] = /* 160 */
{0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44
,0x52,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x08,0x02,0x00,0x00,0x00,0xFD
,0xD4,0x9A,0x73,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0B,0x13
,0x00,0x00,0x0B,0x13,0x01,0x00,0x9A,0x9C,0x18,0x00,0x00,0x00,0x07,0x74,0x49
,0x4D,0x45,0x07,0xD7,0x09,0x0C,0x09,0x05,0x36,0x39,0x35,0x01,0xAD,0x00,0x00
,0x00,0x1D,0x74,0x45,0x58,0x74,0x43,0x6F,0x6D,0x6D,0x65,0x6E,0x74,0x00,0x43
,0x72,0x65,0x61,0x74,0x65,0x64,0x20,0x77,0x69,0x74,0x68,0x20,0x54,0x68,0x65
,0x20,0x47,0x49,0x4D,0x50,0xEF,0x64,0x25,0x6E,0x00,0x00,0x00,0x16,0x49,0x44
,0x41,0x54,0x08,0xD7,0x63,0xFC,0xFF,0xFF,0x3F,0x03,0x03,0x03,0x13,0x03,0x03
,0x03,0x03,0x03,0x03,0x00,0x24,0x06,0x03,0x01,0xBD,0x1E,0xE3,0xBA,0x00,0x00
,0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82};
static const unsigned char black_png[] =/* 158 */
{0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44
,0x52,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x08,0x02,0x00,0x00,0x00,0xFD
,0xD4,0x9A,0x73,0x00,0x00,0x00,0x01,0x73,0x52,0x47,0x42,0x00,0xAE,0xCE,0x1C
,0xE9,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0B,0x13,0x00,0x00
,0x0B,0x13,0x01,0x00,0x9A,0x9C,0x18,0x00,0x00,0x00,0x07,0x74,0x49,0x4D,0x45
,0x07,0xD8,0x04,0x10,0x09,0x01,0x05,0xF1,0x80,0x3B,0xFC,0x00,0x00,0x00,0x19
,0x74,0x45,0x58,0x74,0x43,0x6F,0x6D,0x6D,0x65,0x6E,0x74,0x00,0x43,0x72,0x65
,0x61,0x74,0x65,0x64,0x20,0x77,0x69,0x74,0x68,0x20,0x47,0x49,0x4D,0x50,0x57
,0x81,0x0E,0x17,0x00,0x00,0x00,0x0B,0x49,0x44,0x41,0x54,0x08,0xD7,0x63,0x60
,0x40,0x06,0x00,0x00,0x0E,0x00,0x01,0x31,0xE9,0xDD,0x15,0x00,0x00,0x00,0x00
,0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82};
static const unsigned char blackclear_png[] = /* 167 */
{0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44
,0x52,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x08,0x06,0x00,0x00,0x00,0x72
,0xB6,0x0D,0x24,0x00,0x00,0x00,0x06,0x62,0x4B,0x47,0x44,0x00,0xFF,0x00,0xFF
,0x00,0xFF,0xA0,0xBD,0xA7,0x93,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00
,0x00,0x0B,0x13,0x00,0x00,0x0B,0x13,0x01,0x00,0x9A,0x9C,0x18,0x00,0x00,0x00
,0x07,0x74,0x49,0x4D,0x45,0x07,0xD7,0x09,0x0C,0x09,0x1E,0x31,0x0E,0x67,0x5F
,0x94,0x00,0x00,0x00,0x1D,0x74,0x45,0x58,0x74,0x43,0x6F,0x6D,0x6D,0x65,0x6E
,0x74,0x00,0x43,0x72,0x65,0x61,0x74,0x65,0x64,0x20,0x77,0x69,0x74,0x68,0x20
,0x54,0x68,0x65,0x20,0x47,0x49,0x4D,0x50,0xEF,0x64,0x25,0x6E,0x00,0x00,0x00
,0x0B,0x49,0x44,0x41,0x54,0x08,0xD7,0x63,0x60,0x40,0x07,0x00,0x00,0x12,0x00
,0x01,0xEF,0x89,0x54,0xA4,0x00,0x00,0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42
,0x60,0x82};
static const unsigned char whiteclear_png[]=/* 190 */
{0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44
,0x52,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x08,0x06,0x00,0x00,0x00,0x72
,0xB6,0x0D,0x24,0x00,0x00,0x00,0x01,0x73,0x52,0x47,0x42,0x00,0xAE,0xCE,0x1C
,0xE9,0x00,0x00,0x00,0x06,0x62,0x4B,0x47,0x44,0x00,0xFF,0x00,0xFF,0x00,0xFF
,0xA0,0xBD,0xA7,0x93,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0B
,0x13,0x00,0x00,0x0B,0x13,0x01,0x00,0x9A,0x9C,0x18,0x00,0x00,0x00,0x07,0x74
,0x49,0x4D,0x45,0x07,0xD8,0x04,0x10,0x08,0x3B,0x34,0x84,0x35,0x8F,0x88,0x00
,0x00,0x00,0x1D,0x74,0x45,0x58,0x74,0x43,0x6F,0x6D,0x6D,0x65,0x6E,0x74,0x00
,0x43,0x72,0x65,0x61,0x74,0x65,0x64,0x20,0x77,0x69,0x74,0x68,0x20,0x54,0x68
,0x65,0x20,0x47,0x49,0x4D,0x50,0xEF,0x64,0x25,0x6E,0x00,0x00,0x00,0x15,0x49
,0x44,0x41,0x54,0x08,0xD7,0x63,0xFC,0xFF,0xFF,0x3F,0x03,0x03,0x03,0x03,0x03
,0x13,0x03,0x14,0x00,0x00,0x30,0x06,0x03,0x01,0x95,0x9B,0x05,0x15,0x00,0x00
,0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82};
static const unsigned char graytrans_png[]= /* 189 */
{0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44
,0x52,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x08,0x06,0x00,0x00,0x00,0x72
,0xB6,0x0D,0x24,0x00,0x00,0x00,0x01,0x73,0x52,0x47,0x42,0x00,0xAE,0xCE,0x1C
,0xE9,0x00,0x00,0x00,0x06,0x62,0x4B,0x47,0x44,0x00,0x00,0x00,0x00,0x00,0x00
,0xF9,0x43,0xBB,0x7F,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0B
,0x13,0x00,0x00,0x0B,0x13,0x01,0x00,0x9A,0x9C,0x18,0x00,0x00,0x00,0x07,0x74
,0x49,0x4D,0x45,0x07,0xD8,0x04,0x10,0x09,0x02,0x34,0x8B,0x73,0x68,0x05,0x00
,0x00,0x00,0x1D,0x74,0x45,0x58,0x74,0x43,0x6F,0x6D,0x6D,0x65,0x6E,0x74,0x00
,0x43,0x72,0x65,0x61,0x74,0x65,0x64,0x20,0x77,0x69,0x74,0x68,0x20,0x54,0x68
,0x65,0x20,0x47,0x49,0x4D,0x50,0xEF,0x64,0x25,0x6E,0x00,0x00,0x00,0x14,0x49
,0x44,0x41,0x54,0x08,0xD7,0x63,0xAC,0xAF,0xAF,0x6F,0x60,0x60,0x60,0x60,0x60
,0x62,0x80,0x02,0x00,0x1F,0x06,0x02,0x01,0xD4,0x71,0xFC,0xCA,0x00,0x00,0x00
,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82};
static const char *blackcube_bk_png=(char*)&black_png[0];
static const char *blackcube_dn_png=(char*)&black_png[0];
static const char *blackcube_up_png=(char*)&black_png[0];
static const char *blackcube_lf_png=(char*)&black_png[0];
static const char *blackcube_rt_png=(char*)&black_png[0];
static const char *blackcube_fr_png=(char*)&black_png[0];

static const char *whitecube_bk_png=(char*)&white_png[0];
static const char *whitecube_dn_png=(char*)&white_png[0];
static const char *whitecube_up_png=(char*)&white_png[0];
static const char *whitecube_lf_png=(char*)&white_png[0];
static const char *whitecube_rt_png=(char*)&white_png[0];
static const char *whitecube_fr_png=(char*)&white_png[0];
static const char uebershader_hlsl[]="This is a test of the emergency broadcast system\nStand clear!";
static const char *const native_files[]={"white.png","black.png","whiteclear.png","blackclear.png","graytrans.png","blackcube_bk.png","blackcube_dn.png","blackcube_up.png","blackcube_lf.png","blackcube_rt.png","blackcube_fr.png","whitecube_bk.png","whitecube_dn.png","whitecube_up.png","whitecube_lf.png","whitecube_rt.png","whitecube_fr.png"  };
static const char *const native_files_data[]={(char*)white_png,(char*)black_png,(char*)whiteclear_png , (char*)blackclear_png , (char*)graytrans_png , blackcube_bk_png , blackcube_dn_png , blackcube_up_png , blackcube_lf_png , blackcube_rt_png , blackcube_fr_png , whitecube_bk_png , whitecube_dn_png , whitecube_up_png , whitecube_lf_png , whitecube_rt_png , whitecube_fr_png, uebershader_hlsl };
static const int native_files_size[]={sizeof(white_png),sizeof(black_png),sizeof(whiteclear_png) , sizeof(blackclear_png) , sizeof(graytrans_png) , sizeof(black_png), sizeof(black_png), sizeof(black_png), sizeof(black_png), sizeof(black_png), sizeof(black_png), sizeof(white_png), sizeof(white_png), sizeof(white_png), sizeof(white_png), sizeof(white_png), sizeof(white_png) };
static const int num_native_files=sizeof(native_files)/sizeof(native_files[0]);

/* Verifies that a URI is in hashed form, and strips out all but the hash.
 * Also, removes quotes and a %%_%% prefix.
 * If the URI is not a hashed URI, then returns the input.
 */
std::string CDNArchive::canonicalizeHash(const String&filename_orig)
{
    std::string filename = filename_orig;
  if (filename.length()>strlen(CDN_REPLACING_MATERIAL_STREAM_HINT)&&memcmp(filename.data(),CDN_REPLACING_MATERIAL_STREAM_HINT,strlen(CDN_REPLACING_MATERIAL_STREAM_HINT))==0) {
    filename = filename.substr(strlen(CDN_REPLACING_MATERIAL_STREAM_HINT));
  }
  if (filename.length() > 2 && filename[0] == '\"' && filename[filename.length()-1] == '\"') {
    filename = filename.substr(1, filename.length()-2);
  }
  String::size_type lastslash = filename.rfind('/');
  if (lastslash != String::npos) {
    if (filename.length() == lastslash + 1 + SHA256::hex_size) {
      return filename.substr(lastslash + 1, SHA256::hex_size);
    }
  }
  return filename;
}
// Two functions that do almost the same thing and used interchangably in equivalent functions!
/*
String CDNArchive::canonicalizeHash(const String&filename)
{
  if (filename.length()>strlen(CDN_REPLACING_MATERIAL_STREAM_HINT)&&memcmp(filename.data(),CDN_REPLACING_MATERIAL_STREAM_HINT,strlen(CDN_REPLACING_MATERIAL_STREAM_HINT))==0) {
    return canonicalizeHash(filename.substr(strlen(CDN_REPLACING_MATERIAL_STREAM_HINT)));
  }
  if (filename.length()>SHA256::hex_size+2&&memcmp(filename.data()+1,MERU_URI_HASH_PREFIX.c_str(),MERU_URI_HASH_PREFIX.size())==0&&filename[filename.length()-1]=='\"'&&filename[0]=='\"'){
    return filename.substr(filename.length()-SHA256::hex_size-1,SHA256::hex_size);
  }
  if (filename.length()>SHA256::hex_size+2&&memcmp(filename.data(),MERU_URI_HASH_PREFIX.c_str(),MERU_URI_HASH_PREFIX.size())==0) {
    return filename.substr(filename.length()-SHA256::hex_size);
  }
  return filename;
}

Ogre::String CDNArchive::canonicalMhashName(const Ogre::String&filename)
{
  //return Fingerprint::convertFromHex(URI(filename).filename());
  if (filename.length()>MERU_URI_HASH_PREFIX.size()+2&&memcmp(MERU_URI_HASH_PREFIX.c_str(),filename.data()+1,MERU_URI_HASH_PREFIX.size())==0&&filename[0]=='\"'&&filename[filename.length()-1]=='\"'){
    Ogre::String::size_type endw=filename.rfind("/");
    if (endw!=Ogre::String::npos&&endw+1!=filename.length())
      return filename.substr(endw+1,filename.length()-endw-2);
  }
  if (filename.length()>MERU_URI_HASH_PREFIX.size()&&memcmp(MERU_URI_HASH_PREFIX.c_str(),filename.data(),MERU_URI_HASH_PREFIX.size())==0) {
    return filename.substr(filename.rfind("/")+1);
  }
  return filename;
}
*/

time_t CDNArchive::getModifiedTime(const Ogre::String&)
{
  time_t retval;
  memset(&retval, 0, sizeof(retval));
  return retval;
}

class CDNArchiveDataStream : public Ogre::DataStream
{
public:
	CDNArchiveDataStream(CDNArchiveFactory *owner, const Ogre::String &name, const Transfer::SparseData &input)
		: Ogre::DataStream()
	{
		mOwner=owner;
		mName=name;
		mData=input;
		mSize=mData.size();
		mIter=mData.begin();
	}
	virtual size_t read(void* buffer, size_t length) {
		Transfer::SparseData::value_type *data = mIter.dataAt();
		if (!data) {
			return 0;
		}
		size_t buflength = mIter.lengthAt();
		if (buflength < length) {
			length = buflength;
		}
		std::memcpy(buffer, data, length);
		mIter += length;
		return length;
	}
	virtual void skip(long int relative) {
		mIter += relative;
	}
	virtual void seek(size_t absolute) {
		mIter = mData.begin() + absolute;
	}
	virtual size_t tell() const {
		return mIter - mData.begin();
	}
	virtual bool eof() const {
		return mIter == mData.end();
	}

	virtual void close() {
	}

	~CDNArchiveDataStream() {
	}

private:
	Transfer::SparseData mData;
	Transfer::SparseData::const_iterator mIter;
	CDNArchiveFactory *mOwner;
};


CDNArchive::CDNArchive(CDNArchiveFactory *owner, const Ogre::String& name, const Ogre::String& archType)
: Archive(name, archType)
{
	mOwner = owner;
    mNativeFileArchive=mOwner->addArchive();
    for (int i=0;i<num_native_files;++i) {
        int size=native_files_size[i];
        Transfer::DenseData*dd=new Transfer::DenseData(Transfer::Range((Transfer::cache_usize_type)0,(Transfer::cache_usize_type)size,Transfer::LENGTH,true));
        memcpy(dd->writableData(),native_files_data[i],size);
        Transfer::DenseDataPtr rbuffer(dd);
        mOwner->addArchiveDataNoLock(mNativeFileArchive, native_files[i], Transfer::SparseData(rbuffer));
    }
}

CDNArchive::~CDNArchive() {
    mOwner->removeArchive(mNativeFileArchive);
}

bool CDNArchive::isCaseSensitive() const {
	return true;
}

void CDNArchive::load() {
}

void CDNArchive::unload() {
}

char * findMem(char *data, size_t howmuch, const char * target, size_t targlen)
{
    for (size_t i=0;i+targlen<=howmuch;++i) {
        if (memcmp(data+i,target,targlen)==0) return data+i;
    }
    return NULL;
}

Ogre::DataStreamPtr CDNArchive::open(const Ogre::String& filename) const
{
  boost::mutex::scoped_lock lok(mOwner->CDNArchiveMutex);
  std::string canonicalName = canonicalizeHash(filename);
  std::tr1::unordered_map<std::string,Transfer::SparseData>::iterator where =
      mOwner->CDNArchiveFiles.find(canonicalName);
  if (where == mOwner->CDNArchiveFiles.end()) {
      where =
          mOwner->CDNArchiveFiles.find(filename);
  }
  if (where != mOwner->CDNArchiveFiles.end()) {
    SILOG(resource,debug,"File "<<filename << " Opened");
    unsigned int hintlen=strlen(CDN_REPLACING_MATERIAL_STREAM_HINT);
    if (filename.length()>hintlen&&memcmp(filename.data(),CDN_REPLACING_MATERIAL_STREAM_HINT,hintlen)==0) {
      Ogre::DataStreamPtr inner (new CDNArchiveDataStream(mOwner, filename.substr(hintlen),where->second));
      Ogre::DataStreamPtr retval(new ReplacingDataStream(inner,filename.substr(hintlen),NULL));
      return retval;
    }
    else {
      Ogre::DataStreamPtr retval(new CDNArchiveDataStream(mOwner, filename.find("mhash://") == 0
          ? filename.substr(filename.length() - SHA256::hex_size)
          : filename, where->second));
      return retval;
    }
  }
  else {
    SILOG(resource,debug,"File " << filename << " Not available in recently loaded cache");
    return Ogre::DataStreamPtr();
  }
}

Ogre::StringVectorPtr CDNArchive::list(bool recursive, bool dirs) {
	// We don't have a listing of files because a) its remote
	// and b) we have a flat namespace, so a listing is infeasible.
	// If we wanted to for some reason we could list local files,
	// but nothing should rely on that behavior.
	Ogre::StringVectorPtr ret = Ogre::StringVectorPtr(new Ogre::StringVector());
	return ret;
}

Ogre::FileInfoListPtr CDNArchive::listFileInfo(bool recursive, bool dirs) {
	// We don't have a listing of files because a) its remote
	// and b) we have a flat namespace, so a listing is infeasible.
	// If we wanted to for some reason we could list local files,
	// but nothing should rely on that behavior.
	return Ogre::FileInfoListPtr(new Ogre::FileInfoList());
}

Ogre::StringVectorPtr CDNArchive::find(const Ogre::String& pattern, bool recursive, bool dirs) {
	// We don't have a listing of files because a) its remote
	// and b) we have a flat namespace, so a listing is infeasible.
	// If we wanted to for some reason we could list local files,
	// but nothing should rely on that behavior.
	Ogre::StringVectorPtr ret = Ogre::StringVectorPtr(new Ogre::StringVector());
	return ret;
}

Ogre::FileInfoListPtr CDNArchive::findFileInfo(const Ogre::String& pattern, bool recursive, bool dirs) {
	// We don't have a listing of files because a) its remote
	// and b) we have a flat namespace, so a listing is infeasible.
	// If we wanted to for some reason we could list local files,
	// but nothing should rely on that behavior.
	Ogre::FileInfoListPtr ret = Ogre::FileInfoListPtr(new Ogre::FileInfoList());
	return ret;
}

bool CDNArchive::exists(const Ogre::String& filename) {
    boost::mutex::scoped_lock lok(mOwner->CDNArchiveMutex);
    std::string canonicalName = canonicalizeHash(filename);
    if (mOwner->CDNArchiveFiles.find(canonicalName)!=mOwner->CDNArchiveFiles.end()) {
        SILOG(resource,info,"File "<<filename << " Exists as "<<canonicalName);
        return true;
    }else if (mOwner->CDNArchiveFiles.find(filename)!=mOwner->CDNArchiveFiles.end()) {
        SILOG(resource,info,"File "<<filename << " Exists as "<<canonicalName);
        return true;
    }else {
      String Filename=filename;
      if (Filename.length()>5&&Filename.substr(0,5)==CDN_REPLACING_MATERIAL_STREAM_HINT)
          Filename=Filename.substr(5);

      if (Filename.find("badfoood")!=0) {
          if (Filename.find("deadBeaf")!=0) {
              if (Filename.find("black.png")!=0) {
                  if (Filename.find("white.png")!=0) {
                      if (Filename.find("bad.mesh")!=0) {

                          SILOG(resource,error,"File "<<filename << " AKA "<<canonicalName<<" Not existing in recently loaded cache for "<<(size_t)mOwner<<" because "<<(mOwner->CDNArchiveFiles.find(canonicalName)!=mOwner->CDNArchiveFiles.end())<<" archive size "<<mOwner->CDNArchiveFiles.size());

                          return false;
                      }
                  }
              }
          }
      }
      return true;
    }
}

} // namespace Graphics
} // namespace Sirikata
