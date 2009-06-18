/*  Meru
 *  OgreMeshMaterialDependencyTool.cpp
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
#include <string.h>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <direct.h>
#endif

#include "UploadTool.hpp"
#include "ReplacingDataStream.hpp"

#include <OgreDataStream.h>
#include <OgreCommon.h>

#include <boost/thread/mutex.hpp>

#ifdef _WIN32
#include <io.h>
#define access _access
#define F_OK 0
#endif

namespace Meru {

using namespace ::Sirikata::Transfer;
using namespace ::Sirikata;

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
const char *const native_files[]={"white.png","black.png","whiteclear.png","blackclear.png","graytrans.png","blackcube_bk.png","blackcube_dn.png","blackcube_up.png","blackcube_lf.png","blackcube_rt.png","blackcube_fr.png","whitecube_bk.png","whitecube_dn.png","whitecube_up.png","whitecube_lf.png","whitecube_rt.png","whitecube_fr.png","UeberShader.hlsl"};
const char *const native_files_data[]={(char*)white_png,(char*)black_png,(char*)whiteclear_png , (char*)blackclear_png , (char*)graytrans_png , blackcube_bk_png , blackcube_dn_png , blackcube_up_png , blackcube_lf_png , blackcube_rt_png , blackcube_fr_png , whitecube_bk_png , whitecube_dn_png , whitecube_up_png , whitecube_lf_png , whitecube_rt_png , whitecube_fr_png, uebershader_hlsl };
const int native_files_size[]={sizeof(white_png),sizeof(black_png),sizeof(whiteclear_png) , sizeof(blackclear_png) , sizeof(graytrans_png) , sizeof(black_png), sizeof(black_png), sizeof(black_png), sizeof(black_png), sizeof(black_png), sizeof(black_png), sizeof(white_png), sizeof(white_png), sizeof(white_png), sizeof(white_png), sizeof(white_png), sizeof(white_png),sizeof(uebershader_hlsl) };
const int num_native_files=sizeof(native_files)/sizeof(native_files[0]);

class FingerprintArray {
    std::vector<Fingerprint> fprints;
public:
    FingerprintArray() {
        for (int i = 0; i < num_native_files; i++) {
            fprints.push_back(Fingerprint::computeDigest(native_files_data[i], native_files_size[i]));
        }
    }
    const Fingerprint &operator[] (size_t which) const {
        return fprints[which];
    }
};
static const FingerprintArray native_hashes;

bool isNativeFile(const Ogre::String&filename) {
    for (int i=0;i<num_native_files;++i) {
        if (filename==native_files[i]) return true;
    }
    return false;
}
const char* nativeData (const Ogre::String&file, int&len, Fingerprint *hash=NULL) {
    for (int i=0;i<num_native_files;++i) {
        if (file==native_files[i]) {
            len=native_files_size[i];
            if (hash) *hash = native_hashes[i];
            return native_files_data[i];
        }
    }
    return NULL;
}

DenseDataPtr nativeDataPtr(const Ogre::String&file, Fingerprint *hash=NULL) {
    int len;
    const char *data = nativeData(file, len, hash);
    if (!data || !len) {
        return DenseDataPtr();
    }
    return DenseDataPtr (new DenseData(std::string(data, len)));
}

Ogre::DataStream* nativeStream (const Ogre::String&file) {
    for (int i=0;i<num_native_files;++i) {
        if (file==native_files[i]) 
            return new Ogre::MemoryDataStream(native_files_data[i],native_files_size[i]);        
    }
    return NULL;
}

/**  
 * this is useful where, in materials, strings are often quoted to help the parser
 * \param s may be a string is surrounded by quotes: if so eliminate those
 * \returns stripped string
 */
String stripquotes(String s) {
    if (s.length()>1&&s[0]=='\"'&&s[s.length()-1]=='\"')
        return s.substr(1,s.length()-2);
    return s;
}
/**  
 * This fuction finds the name of a file in a directory
 * \param input is the full path of the file
 * \returns everything past the last slash if such a slash exists
 */
String stripslashes (DiskFile inputdp) {
  std::string input = inputdp.diskpath();
  std::string::size_type where=input.find_last_of("\\/");
  if (where==Ogre::String::npos) {
    return input;
  }else {
    return input.substr(where+1);
  }
}


struct ResourceFileUploadData : public ResourceFileUpload, public DependencyPair {
//    bool mProcessedDependencies;
//    ResourceFileUploadData() : mProcessedDependencies(false) {}
};

/* upload process:

-> specify mesh file. File is read into memory, and hash is computed.
   If HashToFilename map contains this file, its DenseData is used.
   File info is inserted into

 */





class ReplaceMaterialOptionsAndReturn :public ReplaceMaterialOptions {public:
    FileMap mFileMap;
    MaterialMap mMaterialMap;
    ReplaceMaterialOptionsAndReturn(const ReplaceMaterialOptions&opts):ReplaceMaterialOptions(opts) {}
};



/**
 * This function takes in 
 * \param usernameAndServer the input to serverHit, email address form of foo@bar.com
 * \param name the filename to be uploaded to the server
 * \param hash the hash of the file to be uploaded and set to the third level name
 * this function will need to be updated to work in conjunction with a file write
 * to upload the actual file to the server
 * \param opts will have the files written to it if those files have computed first level names already
 */

static ResourceFileUploadData *getFileData(const DiskFile &name, ReplaceMaterialOptionsAndReturn&opts, URI *uri=NULL) {
    String filename = stripslashes(name);
    URI namedURI(opts.uploadNameContext,filename);
    bool disallowed_file=opts.disallowedThirdLevelFiles.find(filename)!=opts.disallowedThirdLevelFiles.end();

// Problem: in order to reference something by hash, you need to have loaded it into memory first.
// This means that you cannot compute URIs until you have found an ordering for dependencies.
// For material files, it is not possible to reference by hash because they can have self-referential URIs.

// I'm thinking only binary files with no dependencies should be allowed to be referenced by hash.
// In such cases, it can be possible to read the file into memory right here.

// Since there is not yet any sort of preference, hashURI is commented out for now.

//    URI hashURI(opts.uploadHashContext, hash.convertToHexString());
    if (!disallowed_file) {
        FileMap::iterator iter = opts.mFileMap.find(name);
        bool inserted = false;
        if (iter == opts.mFileMap.end()) {
            iter = opts.mFileMap.insert(FileMap::value_type(name, new ResourceFileUploadData)).first;
            inserted = true;
        }
        ResourceFileUploadData *fileinfo = ((*iter).second);
        if (inserted) {
            fileinfo->mSourceFilename = name;
            fileinfo->mReferencedByHash = false;
            if (filename.find(".mesh")==filename.length()-strlen(".mesh")) { //&&filename.find("models")!=String::npos) {
                fileinfo->mType = MESH;
            } else if (filename.find(".material")!=Ogre::String::npos||
                       filename.find(".script")!=Ogre::String::npos||
                       filename.find(".os")!=Ogre::String::npos||
                       filename.find(".program")!=Ogre::String::npos) {
                fileinfo->mType = MATERIAL;
            } else {
                fileinfo->mType = DATA;
            }
            if (isNativeFile(filename)) {
                fileinfo->mData = nativeDataPtr(filename, &fileinfo->mHash);
                fileinfo->mID = URI(opts.uploadHashContext,fileinfo->mHash.convertToHexString());
            } else {
                fileinfo->mID = namedURI;
            }
        }
        if (uri) {
            if (fileinfo) {
                *uri = fileinfo->mID;
            } else {
                *uri = namedURI;
            }
        }
        if (!isNativeFile(filename) && access(name.diskpath().c_str(),F_OK)) {
            std::cerr<<"File "<<name.diskpath()<<" does not exist!"<<std::endl;
            inserted = true;
        }
        return fileinfo;
    }
    if (uri) {
        *uri = namedURI;
    }
    return NULL;
};

static String getFileURI(const DiskFile &name, ReplaceMaterialOptionsAndReturn&opts) {
    URI namedURI;
    getFileData(name, opts, &namedURI);
    return namedURI.toString();
}

///Dummy name value pair list since the name/value pair is not used by the nonloading replacing data stream
static const Ogre::NameValuePairList nvpl;

/**
 * This function determines whether a first level name should be used for a given asset
 * \param texturename is the filename of the texture file, not including the full path
 * \param input is the entire script where this texture was discovered in a material script
 * \param pwhere is currently unused but contains the location in the input where the texture file itself was found
 * \param second_input is the location in the script after this texture file was discovered
 * \param opts gives additional information about file selection FIXME in the future this will contain information about which files were manually cancelled by the user
 * \returns whether the first level name should be used
 */
bool shouldForceFirstLevelName(const Ogre::String &textureName, const Ogre::String&input,   Ogre::String::size_type pwhere, Ogre::String::const_iterator second_input,ReplaceMaterialOptionsAndReturn& opts) {
    if (textureName=="white.png"||textureName=="black.png"||textureName=="blackclear.png"||textureName=="graytrans.png"||textureName=="whiteclear.png") {
        return true;
    }
    return false;
}

DiskFile fileRelativeTo(String filename, const String &path) {
    String ret(path);
    do {
        String::size_type pos = ret.rfind('/');
        if (pos != String::npos) {
            ret = ret.substr(0, pos);
        } else {
            break;
        }
        if (filename.length()>=3 && filename.substr(0,3)=="../") {
            filename = filename.substr(3);
            continue;
        } else {
            return DiskFile::makediskfile(ret + '/' + filename);
        }
    } while(true);
    return DiskFile::makediskfile(filename);
}

/**
 * This class records all the dependencies contained within a file using the ogre script regular expression
 * facility provided by the ReplacingDataStream
 */
class RecordingDependencyDataStream:public ReplacingDataStream {
    ///This is essentially an additional return value--the first level textures contained within this file (the others are provides and depends_on)
    std::set<DiskFile> *firstLevelTextures;
    ///These are the options provided by the caller to the import process
    ReplaceMaterialOptionsAndReturn *opts;
public:
    std::vector<String> dependsOnMaterial;
/// The obvious constructor that fills in the data as required and passes in the noop name value pair list
    RecordingDependencyDataStream(Ogre::DataStreamPtr&input,Ogre::String destination, ReplaceMaterialOptionsAndReturn &opts): ReplacingDataStream(input,destination,&nvpl){
        this->opts=&opts;
  }
    /**
     * This function gets a callback from the ReplacingDataStream whenever a material name DEPENDED ON is encountered
     * it uses this callback to build up knowledge about the dependencies in a current resource script
     * \param input is passed from ReplacingDataStream as the entire data
     * \param where_lexeme_start is passed from ReplacingDataStream as where the current material name was discovered
     * \param return_lexeme_end is returned from this function after find_lexeme is run to tear out the material name
     * \param filename is the name of the current script which is not used in this context
     */
  virtual Ogre::String full_replace_lexeme(const Ogre::String &input, 
                                           Ogre::String::size_type where_lexeme_start,
                                           Ogre::String::size_type &return_lexeme_end,
                                           const Ogre::String &filename){
  String extension;
  if (input.length() > where_lexeme_start+8 && input.substr(where_lexeme_start,8)=="delegate") {
    extension = ".program";
  }
  find_lexeme(input,where_lexeme_start,return_lexeme_end);
  if (where_lexeme_start<return_lexeme_end) {
    Ogre::String provided=input.substr(where_lexeme_start,return_lexeme_end-where_lexeme_start);
    if (provided.find("*")==String::npos) {
        if (!extension.empty()) {
            depends_on.push_back(provided+extension);
        } else {
            dependsOnMaterial.push_back(provided+extension);
        }
    }
    return provided;
  }else {
    return Ogre::String();
  }
  }
    /**
     * This function gets a callback from the ReplacingDataStream whenever a resource file name is encountered
     * it uses this callback to build up knowledge about dependencies this file requires
     * \param retval allows text to be inserter at the location replace_reference to be discovered 
     * \param input is passed from ReplacingDataStream as the entire data
     * \param pwhere is the location where the texture was discovered and may be shifted to where the next input should be read if something is returned in retval
     * \param second_input is passed from ReplacingDataStream as where the current resource file name was discovered
     * \param filename is the name of the current script file which is not used in this context
     */
  virtual void replace_reference(Ogre::String&retval, 
                                 const Ogre::String&input,
                                 Ogre::String::size_type&pwhere,
                                 Ogre::String::const_iterator second_input,
                                 const Ogre::String&filename) {
      Ogre::String::size_type lexeme_start=second_input-input.begin(),return_lexeme_end;
      
      find_lexeme(input,lexeme_start,return_lexeme_end);
      if (lexeme_start<return_lexeme_end) {
          Ogre::String dep=input.substr(lexeme_start,return_lexeme_end-lexeme_start);
          depends_on.push_back(dep);
      }
  }
    /**
     * This function gets a callback from the ReplacingDataStream whenever a texture file name is encountered
     * it uses this callback to build up knowledge about dependencies this file requires
     * \param retval allows text to be inserter at the location replace_reference to be discovered 
     * \param input is passed from ReplacingDataStream as the entire data
     * \param pwhere is the location where the texture was discovered and may be shifted to where the next input should be read if something is returned in retval
     * \param second_input is passed from ReplacingDataStream as where the current texture file name was discovered
     * \param filename is the name of the current script file which is not used in this context
     */
  virtual void replace_texture_reference(Ogre::String&retval,
                                         const Ogre::String&input, 
                                         Ogre::String::size_type&pwhere,
                                         Ogre::String::const_iterator second_input,
                                         bool texture_instead_of_source,
                                         const Ogre::String&filename) {
      Ogre::String::size_type lexeme_start=second_input-input.begin(),return_lexeme_end;
      
      find_lexeme(input,lexeme_start,return_lexeme_end);
      if (lexeme_start<return_lexeme_end) {
          Ogre::String dep=input.substr(lexeme_start,return_lexeme_end-lexeme_start);
          depends_on.push_back("../textures/"+dep);
      }

  }  
  
    /**
     * This function gets a callback from the ReplacingDataStream whenever a material name PROVIDED is directly encountered
     * it uses this callback to build up knowledge about the dependencies in a current resource script
     * \param input is passed from ReplacingDataStream as the entire data
     * \param where_lexeme_start is passed from ReplacingDataStream as where the current material name was discovered
     * \param return_lexeme_end is returned from this function after find_lexeme is run to tear out the material name
     * \param filename is the name of the current script file which is not used in this context
     */
  virtual Ogre::String replace_lexeme(const Ogre::String &input, 
                                      Ogre::String::size_type where_lexeme_start,
                                      Ogre::String::size_type &return_lexeme_end,
                                      const Ogre::String &filename){
    find_lexeme(input,where_lexeme_start,return_lexeme_end);
    if (where_lexeme_start<return_lexeme_end) {
      Ogre::String depended=input.substr(where_lexeme_start,return_lexeme_end-where_lexeme_start);
      provides.push_back(depended);    
      return depended;
    }else {
      return Ogre::String();
    }
  }
};

String eliminateFilename(String filename, String provided) {
    String::size_type where=filename.find_last_of("/\\");
    String::size_type where_end=filename.find_last_of(".");
    if (where!=String::npos) {
        if (where_end!=String::npos) {
            filename=filename.substr(where+1,where_end-where-1);
        }else {
            filename=filename.substr(where+1);
        }
    }else {
        filename=filename.substr(0,where_end);
    }
    where=provided.find(filename);
    //printf ("Looking for %s in %s\n",filename.c_str(),provided.c_str());
    if (where!=String::npos&&filename.length()!=provided.length()) {

        provided=provided.substr(0,where)+provided.substr(where+filename.length());
    }
    return provided;
}
/**
 * This class replaces the script names, texture names and material names within a material script 
 * Script names need to be fixed up with first or third level names
 * Material names need to be qualified with fixed up script names
 * Texture names need to be fixed up with first or third level names
 * This class uses the regexp magic contained within the ReplacingDataStream class to perform
 */
class DependencyReplacingDataStream:public ReplacingDataStream {
  /// A map of which material names are provided by which file (the filename of that file)
  MaterialMap *provides_to_name;
  /// A map of which dependencies are required by which files This is for when an olde school MshW file was required which would need all dependencies period
  FileMap *overarching_dependencies;
  ///The dependencies of my file (inherits from the overarching dependencies.
  DependencyPair*my_dependencies;
  ///The options provided by the caller to this entire replace_material script
  ReplaceMaterialOptionsAndReturn *username;
public:

///This function takes as input many of the maps required to figure out which material names belong in which 3rd or first level names for scripts in order to munge those names
    DependencyReplacingDataStream(Ogre::DataStreamPtr&input,const Ogre::String &destination,MaterialMap *provides_to_name,FileMap *overarching_dependencies,   DependencyPair *my_dependencies, ReplaceMaterialOptionsAndReturn&username): ReplacingDataStream(input,destination,&nvpl){
    this->provides_to_name=provides_to_name;
    this->overarching_dependencies=overarching_dependencies;
    this->my_dependencies=my_dependencies;
    this->username=&username;

  }
    /**
     * This function gets a callback from the ReplacingDataStream whenever a material name PROVIDED is directly encountered
     * It uses this callback to actually modify the script name to include the first or third level name for the given material
     * \param input is passed from ReplacingDataStream as the entire data
     * \param where_lexeme_start is passed from ReplacingDataStream as where the current material name was discovered
     * \param return_lexeme_end is returned from this function after find_lexeme is run to tear out the material name
     * \param filename is the name of the current script filename which is used to mangle names
     */

  virtual Ogre::String replace_lexeme(const Ogre::String &input, 
                                      Ogre::String::size_type where_lexeme_start,
                                      Ogre::String::size_type &return_lexeme_end,
                                      const Ogre::String &filename){
    find_lexeme(input,where_lexeme_start,return_lexeme_end);
    if (where_lexeme_start<return_lexeme_end) {
      Ogre::String provided=input.substr(where_lexeme_start,return_lexeme_end-where_lexeme_start);
      if (provided.find("*")==String::npos) {
          provides.push_back(provided);
      }
      return eliminateFilename(filename,provided);
    }else {
      return Ogre::String();
    }
  }
    /**
     * This is a helper function for replace_texture_reference or replace_reference. It does what those two functions decribe
     * It uses an additional bool which it then uses to determine whether it should ask if firstLevelNames should be forced
     * \param retval will return the name of the material as it is modified by firstOrThirdLevelName
     * \param input is passed from ReplacingDataStream as the entire data
     * \param pwhere is the location where the material was discovered and may be shifted to where the next input should be read if something is returned in retval
     * \param second_input is passed from ReplacingDataStream as where the current material file name was discovered
     * \param filename is the name of the current script filename which is used to replace referenced names
     * \param texture_reference indicates if this is a texture file being replaced
     */
    void replace_texture_or_other_reference(Ogre::String&retval, 
                                            const Ogre::String&input,
                                            Ogre::String::size_type&pwhere,
                                            Ogre::String::const_iterator second_input,
                                          const Ogre::String&filename, bool texture_reference) {
      Ogre::String::size_type lexeme_start=second_input-input.begin(),return_lexeme_end;
      
      find_lexeme(input,lexeme_start,return_lexeme_end);
      if (lexeme_start<return_lexeme_end) {
          Ogre::String dep=input.substr(lexeme_start,return_lexeme_end-lexeme_start);
          retval+=input.substr(pwhere,lexeme_start-pwhere);
          if (texture_reference) {
              dep = "../textures/"+dep;
          }
          retval += '\"'+getFileURI(fileRelativeTo(dep,filename),*username)+'\"';
          pwhere=return_lexeme_end;

      }

  }
    
    /**
     * This function gets a callback from the ReplacingDataStream whenever a script file name is encountered
     * it uses this callback to replace the script with that scripts's appropriate first or third level name
     * \param retval will return the name of the material as it is modified by firstOrThirdLevelName
     * \param input is passed from ReplacingDataStream as the entire data
     * \param pwhere is the location where the material was discovered and may be shifted to where the next input should be read if something is returned in retval
     * \param second_input is passed from ReplacingDataStream as where the current material file name was discovered
     * \param filename is the name of the current material which is not used in this context
     */
  virtual void replace_reference(Ogre::String&retval, 
                                 const Ogre::String&input,
                                 Ogre::String::size_type&pwhere,
                                 Ogre::String::const_iterator second_input,
                                 const Ogre::String&filename) {
      replace_texture_or_other_reference(retval,input,pwhere,second_input,filename,false);
  }
    /**
     * This function gets a callback from the ReplacingDataStream whenever a texture file name is encountered
     * it uses this callback to replace the texture with that texture's appropriate first or third level name
     * \param retval will return the name of the texture as it is modified by firstOrThirdLevelName
     * \param input is passed from ReplacingDataStream as the entire data
     * \param pwhere is the location where the texture was discovered and may be shifted to where the next input should be read if something is returned in retval
     * \param second_input is passed from ReplacingDataStream as where the current texture file name was discovered
     * \param filename is the name of the current material which is not used in this context
     */
  virtual void replace_texture_reference(Ogre::String&retval,
                                         const Ogre::String&input, 
                                         Ogre::String::size_type&pwhere,
                                         Ogre::String::const_iterator second_input,
                                         bool texture_instead_of_source,
                                         const Ogre::String&filename) {
      replace_texture_or_other_reference(retval,input,pwhere,second_input,filename,true);
  }  
    /**
     * This function gets a callback from the ReplacingDataStream whenever a material name DEPENDED ON is encountered
     * it uses this callback to replace that material name with a fully qualified material name of the form "<scriptname>:materialname"
     * An example input may be    
     * \param input is passed from ReplacingDataStream as the entire data
     * \param where_lexeme_start is passed from ReplacingDataStream as where the current material name was discovered
     * \param return_lexeme_end is returned from this function after find_lexeme is run to tear out the material name
     * \param filename is the name of the current material which is not used in this context
     * \returns the new name of the material For example if the input was Foo which lives in file meru://foo@bar/baz then the output would be "meru://foo@bar/baz:Foo"
     */
  virtual Ogre::String full_replace_lexeme(const Ogre::String &input, 
                                           Ogre::String::size_type where_lexeme_start,
                                           Ogre::String::size_type &return_lexeme_end,
                                           const Ogre::String &filename){
    find_lexeme(input,where_lexeme_start,return_lexeme_end);
    if (where_lexeme_start<return_lexeme_end) {
      Ogre::String depended=input.substr(where_lexeme_start,return_lexeme_end-where_lexeme_start);
      MaterialMap::const_iterator where=this->provides_to_name->find(depended);
      if (where!=provides_to_name->end()) {
          DiskFile diskfilename = DiskFile::makediskfile(filename);
          if (where->second!=diskfilename) {
          depends_on.push_back(depended);
          DependencyPair * other_dep=getFileData(where->second,*username);
          if (other_dep == NULL) {
          }
          my_dependencies->files.insert(other_dep->files.begin(),other_dep->files.end());
          my_dependencies->files.insert(where->second);
          return '\"'+getFileURI(where->second,*username)+':'+depended+'\"';
        }else {
            
           return eliminateFilename(filename,depended);
        }
      }else {
        return depended;
      }
    }else {
      return Ogre::String();
    }
  }
};


static std::string cutoff;

/**
 * This function opens a file from disk and returns the data within that file
 * it is currently a convenience method but later it will need to handle ogre resource streams
 * \param name is the filename
 * \param len returns the length of the file
 * \returns data malloced to size len filled with the file's contents
 */
DenseDataPtr getHashFileData(const DiskFile& name) {
  {
      bool isbinary=name.diskpath().find(".mesh")!=Ogre::String::npos;
    if (isbinary) {
        isbinary=true;
    }
    if (isNativeFile(stripslashes(name))) {
        DenseDataPtr data (nativeDataPtr(stripslashes(name)));
        if (data) {
            return data;
        }
    }
  }
  Ogre::String retval;
  std::ifstream handle;
  handle.open(name.diskpath().c_str(),std::ios::in|std::ios::binary);
  if (handle.good()) {
      Ogre::FileStreamDataStream fp(&handle,false);
      retval=fp.getAsString();
      fp.close();
      return DenseDataPtr(new DenseData(retval));
  } else {
      return DenseDataPtr();
  }
}
void processFileDependency(ResourceFileUploadData *file,FileMap &filemap, const MaterialMap &materialmap,ReplaceMaterialOptionsAndReturn &opts) {
  if (file->mData) {
      return; // Already processed.
  }
  file->mHash=Fingerprint::null();
  String filefirst = stripslashes(file->mSourceFilename);
  if (isNativeFile(filefirst)) {
      return; // don't care.
  }
  DenseDataPtr processed (getHashFileData(file->mSourceFilename));
  if (!processed) {
      return;
  }
  if (filefirst.find(".dds")!=Ogre::String::npos||filefirst.find(".gif")!=Ogre::String::npos||filefirst.find(".jpeg")!=Ogre::String::npos||filefirst.find(".png")!=Ogre::String::npos||filefirst.find(".tif")!=Ogre::String::npos||filefirst.find(".tga")!=Ogre::String::npos||filefirst.find(".jpg")!=Ogre::String::npos||filefirst.find(".vert")!=Ogre::String::npos||filefirst.find(".frag")!=Ogre::String::npos||filefirst.find(".hlsl")!=Ogre::String::npos||filefirst.find(".cg")!=Ogre::String::npos||filefirst.find(".glsl")!=Ogre::String::npos) {
      // These file formats cannot reference any other files, so do not alter them.
  }else {
    bool isbinary=filefirst.find(".mesh")!=Ogre::String::npos||filefirst.find(".skeleton")!=Ogre::String::npos;
    replaceAll(processed,filemap,materialmap,*file,opts,isbinary);//is binary
  }
  Fingerprint fileHash=Fingerprint::computeDigest(processed->data(), processed->length());
  file->mHash=fileHash;
  file->mData = processed;
}



/**
 * This function replaces one dependencies referenced within a file to that dependency's CDN name and in doing so begins to build a list of dependencies this file has for future use
 * This function must be called in dependency order per file (so the files with least dependencies get called first
 * This function then replaces all references to dependent files with the appropriate hash or mangling of those names
 * \param data is the content of the file (the entire file is stored in the data variable
 * \param src is the name as it would appear in this file, whether or not it is binary
 * \param dst is the name that src name should be replaced with if it is found in the file
 * \param filemap the map of other files that are available to be processed to their hash or high level names. This may modify that map
 * \param materialmap the set of other ogre materials (not scripts, materials within those scripts paired with those respective materials disk filename and third level names
 * \param overarching_dependencies a map from file names to those datafiles' dependencies (be they materials/scripts or texture/source files) This may modify that list due to recursive processing 
 * \param firstLevelTextures is a list of textures that must be first level due to their not being tileable (as set by the RecordingDependencyDataStream)
 * \param my_dependencies is the list of dependencies that this current file depends on...this is one of the return values of this function
 * \param username is the list of options that are passed in by the caller of this entire library
 * \param do_stripslashes is whether the \param src file being passed in may be referenced without a path (due to ogre naming conventions)
 * \param allow_binary is whether this particular file is binary so that dependencies are known to need to replace the length value before the data aspect
 */
bool replaceOne(DenseDataPtr &dataptr, DiskFile src, String strippedsrc, String dst,FileMap &filemap, const MaterialMap&materialmap,ReplaceMaterialOptionsAndReturn &opts,bool do_stripslashes, bool allow_binary) {
    Ogre::String::size_type where=0;
    bool retval = false;
    std::string data((const char *)dataptr->data(), (size_t)dataptr->length());
    std::string filename = stripslashes(src);
    if (strippedsrc.empty()) {
        strippedsrc=do_stripslashes?filename:src.diskpath();
    }

    /*if (src.find("OffsetMapping.cg")!=std::string::npos) {
      printf ("Searching for %s (%s)\n",src.c_str(),strippedsrc.c_str());
      }*/
    while ((where=data.find(strippedsrc,where))!=Ogre::String::npos) {
        /*if (src.find("OffsetMapping.cg")!=std::string::npos) {
          printf ("Found %s\n",src.c_str());
          }*/
        
        if (false) {
            printf("error: self referencial file %s\n",src.diskpath().c_str());
            break;
        }else {
            /*if (src.find("Ueber")!=String::npos) {
                printf ("COOLER %s\n",dst.c_str());
                }*/
            if (dst.length()==0) {
                if (filename.find(".dds")!=Ogre::String::npos||filename.find(".gif")!=Ogre::String::npos||filename.find(".jpeg")!=Ogre::String::npos||filename.find(".png")!=Ogre::String::npos||filename.find(".tif")!=Ogre::String::npos||filename.find(".tga")!=Ogre::String::npos||filename.find(".jpg")!=Ogre::String::npos||filename.find(".vert")!=Ogre::String::npos||filename.find(".frag")!=Ogre::String::npos||filename.find(".hlsl")!=Ogre::String::npos||filename.find(".cg")!=Ogre::String::npos||filename.find(".glsl")!=Ogre::String::npos) {
                    processFileDependency(filemap.find(src)->second,filemap,materialmap,opts);
                    dst=getFileURI(src,opts);
                }else{
                    Ogre::String summary =data.length()>200?data.substr(0,200):data;
                    String path = src.diskpath();
                    printf ("ERROR: File %s processed out of order with file beginning with %s\n",path.c_str(),summary.c_str());
                    
                }
            }
            if (allow_binary&&data.length()>where+strippedsrc.length()&&(data[where+strippedsrc.length()]=='\0'||data[where+strippedsrc.length()]=='\n')) {
                dst=stripquotes(dst);
                unsigned int oldlen=(unsigned char)data[where-1];
                oldlen*=256;
                oldlen+=(unsigned char)data[where-2];
                oldlen*=256;
                oldlen+=(unsigned char)data[where-3];
                oldlen*=256;
                oldlen+=(unsigned char)data[where-4];

                if (oldlen<data.size()*2&&dst.length()<(((size_t)1)<<31)&&strippedsrc.length()<(((size_t)1)<<31)) {
                    oldlen+=(unsigned int)dst.length();
                    oldlen-=(unsigned int)strippedsrc.length();
                    
                    data[where-4]=oldlen%256;
                    oldlen/=256;
                    data[where-3]=oldlen%256;
                    oldlen/=256;
                    data[where-2]=oldlen%256;
                    oldlen/=256;
                    data[where-1]=oldlen%256;
                    data=data.substr(0,where)+dst+data.substr(where+strippedsrc.length());        
                    where+=dst.length();
                    retval=true;
                }else {
                    where+=strippedsrc.length();
                }
            }else if ((data.length()<=strippedsrc.length()||isspace(data[where+strippedsrc.length()])||data[where+strippedsrc.length()]==':')) {
                data=data.substr(0,where)+(data[where+strippedsrc.length()]==':'?stripquotes(dst):dst)+data.substr(where+strippedsrc.length());
                where+=dst.length();
                retval=true;
                //something to do
            }else {
                /*std::string tmp=data.substr(where-(where>100?100:0),strippedsrc.length()+128);
                  printf ("error finding proper %s around %s\n",src.c_str(),tmp.c_str());*/
                where+=strippedsrc.length();
            }
        }
    }
    if (retval) {
        dataptr = DenseDataPtr(new DenseData(data));
    }
    return retval;
}
void replaceAll(DenseDataPtr &data, FileMap &filemap, const MaterialMap&materialmap, DependencyPair&my_dependencies, ReplaceMaterialOptionsAndReturn &opts,bool allow_binary) {
    for (FileMap::iterator i=filemap.begin(),ie=filemap.end();i!=ie;++i) {
        if (i->second->mType != MATERIAL) {
            if (replaceOne(data,i->first,std::string(),getFileURI(i->first, opts),filemap,materialmap,opts, true,allow_binary)) {
                my_dependencies.files.insert(i->first);
                DependencyPair *dp=i->second;
                my_dependencies.files.insert(dp->files.begin(),dp->files.end());
            }
        }
    }
    for (MaterialMap::const_iterator i=materialmap.begin(),ie=materialmap.end();i!=ie;++i) {
        if (replaceOne(data,i->second,i->first,'\"'+getFileURI(i->second,opts)+":"+i->first+'\"',filemap,materialmap,opts,false,allow_binary)) {
            my_dependencies.files.insert(i->second);
            DependencyPair *dp=getFileData(i->second,opts);
            my_dependencies.files.insert(dp->files.begin(),dp->files.end());
        }
    }
}

static bool isMHashScheme(const URI &uri, ReplaceMaterialOptionsAndReturn &opts) {
    return (uri.context() == opts.uploadHashContext);
}


//////////////// This function is really long and convoluted?!?! I wish there were comments here!

std::vector<ResourceFileUpload> ProcessOgreMeshMaterialDependencies(const std::vector<DiskFile> &origfilenames,const ReplaceMaterialOptions&options) {
  std::vector<DiskFile> filenames = origfilenames;
  ReplaceMaterialOptionsAndReturn opts(options);  
  opts.disallowedThirdLevelFiles.insert("meru:///UeberShader.hlsl");//this file should be updated manually by devs, not by artists

/////////////// Not sure what this code does at all

  String mesh_old_name;
  Fingerprint mesh_new_hash;
  String mesh_new_name;
  std::set<Fingerprint> mesh_hashes;
  std::set<Fingerprint> skeleton_hashes;

/////// End look for THE mesh.

  {
  std::set<DiskFile> knownFilenames(filenames.begin(),filenames.end());
  for (size_t i=0;i<filenames.size();++i) {
    ResourceFileUploadData *deps = getFileData(filenames[i], opts);
    if (deps->mType == MESH) {
        String path = filenames[i].diskpath();
        String::size_type pos = path.rfind('/');
        if (pos != String::npos) {
            path = path.substr(0, pos);
            String strippedname = stripslashes(filenames[i]);
            String::size_type dot = strippedname.rfind('.');
            if (dot != String::npos) {
                strippedname = strippedname.substr(0, dot) + ".os";
                pos = path.rfind('/');
                if (pos != String::npos) {
                    path = path.substr(0, pos);
                    path += "/";
                } else {
                    path = "../";
                }
                path += "materials/scripts/"+strippedname;
                filenames.push_back(DiskFile::makediskfile(path));
            }
        }
    }
    if (deps->mType == MATERIAL) {
      std::ifstream fhandle;
      fhandle.open(filenames[i].diskpath().c_str(),std::ios::in|std::ios::binary);
      if (!fhandle.good()) {
          fprintf(stderr, "Error: Material File %s does not exist!\n", filenames[i].diskpath().c_str());
          filenames.erase(filenames.begin()+i);
          --i;
          continue;
      }
      std::vector<Ogre::String> provides,depends_on;
      {
          RecordingDependencyDataStream *tmprds; // goes out of scope.
          Ogre::DataStreamPtr input(new Ogre::FileStreamDataStream(&fhandle,false));
          Ogre::DataStreamPtr rds (tmprds=new RecordingDependencyDataStream (input,filenames[i].diskpath(),opts));
          //HELP FIXME ogre crashes ... need to get rid of inner bloc too.
          tmprds->preprocessData(provides,depends_on);
          for (std::vector<Ogre::String>::iterator iter=provides.begin(),itere=provides.end();iter!=itere;++iter) {
              MaterialMap::iterator dupe;
              if ((dupe=opts.mMaterialMap.find(*iter))!=opts.mMaterialMap.end()&&getFileData(dupe->second,opts)->mData) {
                  std::string s (getFileData(dupe->second,opts)->mID.toString());
                  fprintf (stderr, "Duplicate Material provided %s by both %s and %s\n",
                          iter->c_str(),
                          s.c_str(),
                          filenames[i].diskpath().c_str());
               }else {
                  //printf ("%s provided by %s\n",iter->c_str(),filenames[i]);
                  opts.mMaterialMap[*iter]=filenames[i];
               }
          }
/*
          for (size_t j=0;j<tmprds->dependsOnMaterial.size();++j) {
              if (opts.mMaterialMap[tmprds->dependsOnMaterial[j]]!=filenames[i]) {
              }
          }
*/
      }
      {
        bool depends_on_nothing=true;
        for (size_t j=0;j<depends_on.size();++j) {
            depends_on_nothing=false;
            //printf ("%s depends on %s\n",filenames[i].c_str(),depends_on[j].c_str());
            DiskFile dep (fileRelativeTo(depends_on[j], filenames[i].diskpath()));
            deps->files.insert(dep);
            if (knownFilenames.insert(dep).second == true) {
                std::cout << "Adding dependency "<<dep.diskpath()<<" (first referenced from "<<filenames[i].diskpath()<<")"<<std::endl;
              filenames.push_back(dep);
            }
        }
      }
    }
  }
  }



  for (size_t i=0;i<filenames.size();++i) {
    ResourceFileUploadData *deps = getFileData(filenames[i], opts);
    if (deps->mType == MATERIAL) {
        std::ifstream fhandle;
        DiskFile depsecond (filenames[i]);
        {
            Ogre::DataStreamPtr input;
            if (isNativeFile(stripslashes(depsecond))) {
                input = Ogre::DataStreamPtr(nativeStream(stripslashes(depsecond)));
            } else {
                fhandle.open(depsecond.diskpath().c_str(),std::ios::in|std::ios::binary);
                if (fhandle.good()) {
                    input = Ogre::DataStreamPtr(new Ogre::FileStreamDataStream(&fhandle,false));
                } else {
                    fprintf(stderr, "Error: File %s does not exist!\n", filenames[i].diskpath().c_str());
                    filenames.erase(filenames.begin()+i);
                    --i;
                }
            }
            ResourceFileUploadData *mydeps = getFileData(depsecond, opts);
            
            Ogre::DataStreamPtr rds (new DependencyReplacingDataStream (input,depsecond.diskpath(),&opts.mMaterialMap,&opts.mFileMap,mydeps,opts));
            DenseDataPtr data (new DenseData(rds->getAsString()));
            
            //printf ("Processing %s\n",dep->second.c_str());
            //fflush(stdout);
            //replaceAll(data,opts.mFileMap,MaterialMap(),*mydeps,opts, false);
            Fingerprint fileHash=Fingerprint::computeDigest(data->data(), data->length());
            
            mydeps->mHash = fileHash;
            mydeps->mData = data;
        }
    }
  }
  for (FileMap::iterator i=opts.mFileMap.begin(),ie=opts.mFileMap.end();i!=ie;++i) {
      if (!i->second->mData) {
          processFileDependency(i->second,opts.mFileMap,opts.mMaterialMap,opts);
      }
  }


#if 0


  std::map<URI,ResourceFileUpload> existingFiles;
  std::multimap<Fingerprint,URI> reverseExistingFiles;//a map from any shasum to its file.  No, really??? Actually, it's a *MULTIMAP*, so file should be plural.
#endif

// Delete 150 additional lines of completely useless code. That feels so much better.


  std::vector<ResourceFileUpload> retval;
  retval.reserve(opts.mFileMap.size());
  for (FileMap::const_iterator iter = opts.mFileMap.begin(); iter != opts.mFileMap.end(); ++iter) {
      if (iter->second->mData) {
          retval.push_back(*(iter->second));
      } else {
          // File did not exist.
      }
      delete iter->second;
  }

  return retval;
}

struct UploadStatus {
    std::tr1::function<void(ResourceStatusMap const &)> mCallback;
    ResourceStatusMap mStatusMap;
    int mNumberRemaining;
    boost::mutex mLock;
    UploadStatus (const std::tr1::function<void(ResourceStatusMap const &)> &cb, int count)
        : mCallback(cb), mNumberRemaining(count) {
    }
};

EventResponse UploadFinished(UploadStatus *stat, const ResourceFileUpload &current, EventPtr ev) {
    Transfer::UploadEventPtr uploadev (std::tr1::static_pointer_cast<UploadEvent>(ev));
    if (!uploadev) {
        return EventResponse::nop();
    }
    bool del = false;
    {
        boost::unique_lock<boost::mutex> mylock (stat->mLock);
        ResourceUploadStatus st;
        st = uploadev->getStatus();
        if (stat->mStatusMap.insert(ResourceStatusMap::value_type(current, st)).second == true) {
            if (0 == --stat->mNumberRemaining) {
                stat->mCallback(stat->mStatusMap);
                del = true;
            }
        }
    }
    if (del) {
        delete stat;
    }
    return EventResponse::del();
}

void UploadFilesAndConfirmReplacement(::Sirikata::Transfer::TransferManager*tm, 
                                      const std::vector<ResourceFileUpload> &filesToUpload,
                                      const ::Sirikata::Transfer::URIContext &hashContext,
                                      const std::tr1::function<void(ResourceStatusMap const &)> &callback) {
    UploadStatus *status = new UploadStatus(callback, filesToUpload.size());
    for (size_t i = 0; i < filesToUpload.size(); ++i) {
        const ResourceFileUpload &current = filesToUpload[i];
        std::cout << "Uploading "<<stripslashes(current.mSourceFilename)<<" to URI " << current.mID<<". Hash = "<<current.mHash<<"; Size = "<<current.mData->length()<<std::endl;
        if (current.mID.context() == hashContext) {
            tm->uploadByHash(Transfer::RemoteFileId(current.mHash, current.mID),
                       current.mData,
                       std::tr1::bind(&UploadFinished, status, current, _1),false);
        } else {
            tm->upload(current.mID,
                       Transfer::RemoteFileId(current.mHash, hashContext),
                       current.mData,
                       std::tr1::bind(&UploadFinished, status, current, _1),false);
        }
    }
}

}

#ifdef STANDALONE
int main (int argc, char ** argv) {
  ReplaceMaterialOptions opts;
  for (int i=0;i<argc;++i) {
    char match=0;
    int j;
    if (strncmp(argv[i],"-u",2)==0&&!match) {
        opts.username = argv[i]+2;
        match=1;
    }
    else if (strncmp(argv[i],"-d",2)==0&&!match) {
      cutoff=argv[i]+2;
      match=1;
    }else if (strcmp(argv[i],"-1")==0&&!match) {
        opts.forceFirstLevelNames=true;
        match=1;
    }else if (strcmp(argv[i],"-3")==0&&!match) {
        opts.forceThirdLevelNames=true;
        match=1;
    }
    if (match) {
      for (j=i+1;j<argc;++j) argv[j-1]=argv[j];
      argc--;
      i--;
    }
  }
  std::vector <Ogre::String> filenames;
  for (int i=1;i<argc;++i) {
      filenames.push_back(argv[i]);
  }
  ProcessOgreMeshMaterialDependencies(filenames,opts);
  return 0;
}
#endif
