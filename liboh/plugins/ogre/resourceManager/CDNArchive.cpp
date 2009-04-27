/*  Meru
 *  CDNArchive.cpp
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
#include "MeruDefs.hpp"
#include "CDNArchive.hpp"
#include <cassert>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include "ReplacingDataStream.hpp"

namespace Meru {

static boost::mutex CDNArchiveMutex;
static std::map<unsigned int, std::vector <Ogre::String> > CDNArchivePackages;
static std::map<Ogre::String, std::pair<ResourceBuffer, unsigned int> > CDNArchiveFileRefcount;
static std::vector <Ogre::String> CDNArchiveToBeDeleted;
static int sCurArchive = 0;
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

static void removeUndesirables()
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

static std::string MERU_URI_HASH_PREFIX("mhash:");

static String canonicalizeHash(const String&filename)
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

time_t CDNArchive::getModifiedTime(const Ogre::String&)
{
  time_t retval;
  memset(&retval, 0, sizeof(retval));
  return retval;
}

unsigned int CDNArchive::addArchive()
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  CDNArchivePackages[sCurArchive]=std::vector<Ogre::String>();
  removeUndesirables();
  return sCurArchive++;
}

unsigned int CDNArchive::addArchive(const Ogre::String&filename, const ResourceBuffer &rbuffer)
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  CDNArchivePackages[sCurArchive]=std::vector<Ogre::String>();
  removeUndesirables();
  addArchiveDataNoLock(sCurArchive, filename, rbuffer);
  return sCurArchive++;
}

void CDNArchive::addArchiveDataNoLock(unsigned int archiveName, const Ogre::String&filename, const ResourceBuffer &rbuffer)
{
  std::map<Ogre::String,std::pair<ResourceBuffer,unsigned int> >::iterator where=CDNArchiveFileRefcount.find(filename);
  if (where==CDNArchiveFileRefcount.end()) {
    SILOG(resource,debug,"File "<<filename<<" Added to CDNArchive");
    CDNArchiveFileRefcount[canonicalizeHash(filename)]=std::pair<ResourceBuffer,unsigned int>(rbuffer,1);
  }
  else {
    SILOG(resource,debug,"File "<<filename<<" already downloaded to CDNArchive, what a waste! incref to "<<where->second.second+1);
    ++where->second.second;
  }
  CDNArchivePackages[archiveName].push_back(filename);
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

void CDNArchive::addArchiveData(unsigned int archiveName, const Ogre::String&filename, const ResourceBuffer &rbuffer)
{
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  addArchiveDataNoLock(archiveName,canonicalMhashName(filename),rbuffer);
}

void CDNArchive::clearArchive(unsigned int which)
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

void CDNArchive::removeArchive(unsigned int which)
{
  clearArchive(which);

  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  std::map<unsigned int, std::vector<Ogre::String> >::iterator where=CDNArchivePackages.find(which);
  if (where!=CDNArchivePackages.end()) {
    CDNArchivePackages.erase(where);
  }
  removeUndesirables();
}

class CDNArchiveDataStream : public Ogre::MemoryDataStream
{
public:
  CDNArchiveDataStream(const Ogre::String &name, std::pair<ResourceBuffer,unsigned int> *input)
    : Ogre::MemoryDataStream((void*)input->first->data(),(size_t)input->first->length(),false),mBuffer(&input->first)
  {
      mName=name;
      input->second++;
  }

  virtual void close() {
    boost::mutex::scoped_lock lok(CDNArchiveMutex);
    std::map<Ogre::String,std::pair<ResourceBuffer,unsigned int> >::iterator where=CDNArchiveFileRefcount.find(getName());
    if (where!=CDNArchiveFileRefcount.end()) {
      if (where->second.second==0){
        SILOG(resource,error,"File "<<getName()<< " Not in CDNArchive Map already has refcount=0");
      }else {
        if (--where->second.second==0){
          CDNArchiveToBeDeleted.push_back(where->first);
        }
      }
    }else {
      SILOG(resource,error,"File "<<getName()<< " Not in CDNArchive Map to be removed upon close");
    }
  }
  ~CDNArchiveDataStream() {
  }

private:
  ResourceBuffer *mBuffer;
};

CDNArchive::CDNArchive(const Ogre::String& name, const Ogre::String& archType)
: Archive(name, archType)
{
    mNativeFileArchive=addArchive();
    for (int i=0;i<num_native_files;++i) {
        int size=native_files_size[i];
        DenseData*dd=new DenseData(Transfer::Range((Transfer::cache_usize_type)0,(Transfer::cache_usize_type)size,Transfer::LENGTH,true));
        memcpy(dd->writableData(),native_files_data[i],size);
        DenseDataPtr rbuffer(dd);
        addArchiveDataNoLock(mNativeFileArchive, native_files[i], rbuffer);
    }
}

CDNArchive::~CDNArchive() {
    removeArchive(mNativeFileArchive);
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
  boost::mutex::scoped_lock lok(CDNArchiveMutex);
  std::map<Ogre::String,std::pair<ResourceBuffer,unsigned int> >::iterator where=CDNArchiveFileRefcount.find(canonicalizeHash(filename));
  if (where != CDNArchiveFileRefcount.end()) {
    SILOG(resource,debug,"File "<<filename << " Opened");
    unsigned int hintlen=strlen(CDN_REPLACING_MATERIAL_STREAM_HINT);
    if (filename.length()>hintlen&&memcmp(filename.data(),CDN_REPLACING_MATERIAL_STREAM_HINT,hintlen)==0) {
      Ogre::DataStreamPtr inner (new CDNArchiveDataStream(filename.substr(hintlen),&where->second));
      Ogre::DataStreamPtr retval(new ReplacingDataStream(inner,filename.substr(hintlen),NULL));
      return retval;
    }
    else {
      Ogre::DataStreamPtr retval(new CDNArchiveDataStream(filename.find("mhash://") == 0 ? filename.substr(filename.length() - SHA256::hex_size) : filename, &where->second));
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
    boost::mutex::scoped_lock lok(CDNArchiveMutex);
    if (CDNArchiveFileRefcount.find(canonicalizeHash(filename))!=CDNArchiveFileRefcount.end()) {
      SILOG(resource,info,"File "<<filename << " Exists as "<<canonicalizeHash(filename));
        return true;
    }else {
        String Filename=filename;
      if (Filename.length()>5&&Filename.substr(0,5)=="%%_%%")
          Filename=Filename.substr(5);

      if (Filename.find("badfoood")!=0) {
          if (Filename.find("deadBeaf")!=0) {
              if (Filename.find("black.png")!=0) {
                  if (Filename.find("white.png")!=0) {
                      if (Filename.find("bad.mesh")!=0) {

                          SILOG(resource,info,"File "<<filename << " AKA "<<canonicalizeHash(filename)<<" Not existing in recently loaded cache");

                          return false;
                      }
                  }
              }
          }
      }
      return true;
    }
}

} // namespace Meru
