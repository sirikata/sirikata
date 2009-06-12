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
bool isNativeFile(const Ogre::String&file) {
    for (int i=0;i<num_native_files;++i) {
        if (file==native_files[i]) return true;
    }
    return false;
}
const char* nativeData (const Ogre::String&file, int&len) {
    for (int i=0;i<num_native_files;++i) {
        if (file==native_files[i]) {
            len=native_files_size[i];
            return native_files_data[i];
        }
    }
    return NULL;
    
}
Ogre::DataStream* nativeStream (const Ogre::String&file) {
    for (int i=0;i<num_native_files;++i) {
        if (file==native_files[i]) 
            return new Ogre::MemoryDataStream(native_files_data[i],native_files_size[i]);        
    }
    return NULL;
}

using namespace Meru;
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
Ogre::String stripslashes (Ogre::String input) {
  std::string::size_type where=input.find_last_of("\\/");
  if (where==Ogre::String::npos) {
    return input;
  }else {
    return input.substr(where+1);
  }
}

typedef  std::map<Ogre::String,Fingerprint,SpecialStringSort> FileMap;
typedef  std::map<Ogre::String,std::pair<Fingerprint,Ogre::String>,SpecialStringSort> MaterialMap;
typedef  std::map<String,std::vector<String> > ProviderMap;
typedef  std::map<Fingerprint,DependencyPair > DependencyMap;




namespace Meru {
class ReplaceMaterialOptionsAndReturn :public ReplaceMaterialOptions {public:
    std::map<Fingerprint,size_t> hashToFirstLevelName; ///< Map from hash to position in files vector.
    std::vector <ResourceFileUpload> uploadedFiles;
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

static void writeThirdLevelName(String name, const Fingerprint &fprint,ReplaceMaterialOptionsAndReturn&opts) {
    if (fprint != Fingerprint::null()){
        std::map<Fingerprint,size_t>::iterator where;
        if ((where=opts.hashToFirstLevelName.find(fprint))!=opts.hashToFirstLevelName.end()) {
            opts.uploadedFiles.push_back(ResourceFileUpload());
            opts.uploadedFiles.back().mHash=fprint;
            opts.uploadedFiles.back().mID=URI(opts.uploadNameContext, stripslashes(name));
            opts.uploadedFiles.back().mData=opts.uploadedFiles[where->second].mData;
        }else {
			std::string hashstr (fprint.convertToHexString());
            fprintf (stderr,"ERROR: name %s found to have hash %s that is not in database\n",name.c_str(),hashstr.c_str());
        }
    }
}

// computeDigest, convertToHexString

/**
 * This function takes in some parameters and decides what sort of name the file should have based on its merits
 * \param oldName indicates the name that was on the filesystem of the file to be uploaded
 * \param opts contains information about which groups should be user or FIXME in the future that certain files have been cancel/allowed by the upload Navi
 * \param hash contains the hash of the file in case a first level name is decided
 * \param addQuotes indicates whether the function should add quotes to the return value for use in a material
 * \param forceFirstLevel indicates whether the file should be absolutely the first level no matter the options (except the future options where user is god over even that 
 * \returns fully qualified name of the object
 */
static String firstOrThirdLevelName(String oldName, ReplaceMaterialOptionsAndReturn& opts, const Fingerprint &hash, bool forceFirstLevel) {
/*

  String username=opts.username;
    if (oldName.find(".dds")!=String::npos&&opts.meruBaseTileable) {
        username=opts.group_username;
    }
*/
    bool core_system_file=false;
    if (oldName.find(".material")!=String::npos||oldName.find(".program")!=String::npos||oldName.find(".hlsl")!=String::npos||oldName.find(".cg")!=String::npos||oldName.find(".glsl")!=String::npos||oldName.find(".vert")!=String::npos||oldName.find(".frag")!=String::npos) {
        core_system_file=true;
    } 
    URI tentativeThirdLevelName(opts.uploadNameContext,stripslashes(oldName));
    bool disallowed_file=opts.disallowedThirdLevelFiles.find(tentativeThirdLevelName.toString())!=opts.disallowedThirdLevelFiles.end();
    if (disallowed_file&&!core_system_file) {
        ///root username always gets uploaded no matter what
        forceFirstLevel=true;
    }
    bool forceThirdLevel=oldName.find(".skeleton")==String::npos&&oldName.find("VPhlsl")==String::npos&&oldName.find("VPglsl")==String::npos&&oldName.find("VPcg")==String::npos&&oldName.find("VPcgGL")==String::npos&&oldName.find("FPhlsl")==String::npos&&oldName.find("FPglsl")==String::npos&&oldName.find("FPcg")==String::npos&&oldName.find("FPcgGL")==String::npos&&forceFirstLevel==false;


    URI retval(opts.uploadHashContext, hash.convertToHexString());
    if (opts.forceFirstLevelNames==false&&(forceThirdLevel||opts.forceThirdLevelNames)) {
        if (!disallowed_file) {// don't want to request an upload of a different core system file if the user does not know the root password
            writeThirdLevelName(stripslashes(oldName),hash,opts);
        }
        retval= tentativeThirdLevelName;
    }
    return retval.toString();
    
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
    if (textureName=="blackcube.png"||textureName=="whitecube.png"||textureName.find(".vert")!=String::npos||textureName.find(".frag")!=String::npos||textureName.find(".hlsl")!=String::npos||textureName.find(".glsl")!=String::npos||textureName.find(".cg")!=String::npos)
        return false;
    if (opts.namedTileable==false)
        return true;
    const static Ogre::String texcoordset="tex_coord_set";
    Ogre::String::size_type where=input.find(texcoordset,second_input-input.begin());
    if (where!=Ogre::String::npos) {
        where+=texcoordset.length();
        while (where<input.length()&&isspace(input[where])){
            ++where;
        }
        while (where<input.length()&&!isspace(input[where])){
            if (input[where]!='0')
                return false;
            where++;
        }
        return true;
    }
    return false;
}
};
/**
 * This class records all the dependencies contained within a file using the ogre script regular expression
 * facility provided by the ReplacingDataStream
 */
class RecordingDependencyDataStream:public ReplacingDataStream {
    ///This is essentially an additional return value--the first level textures contained within this file (the others are provides and depends_on)
    std::set<Ogre::String> *firstLevelTextures;
    ///These are the options provided by the caller to the import process
    ReplaceMaterialOptionsAndReturn *opts;
public:
/// The obvious constructor that fills in the data as required and passes in the noop name value pair list
    RecordingDependencyDataStream(Ogre::DataStreamPtr&input,const Ogre::String &destination, std::set<Ogre::String>&firstLevelTextures, ReplaceMaterialOptionsAndReturn &opts): ReplacingDataStream(input,destination,&nvpl){
        this->firstLevelTextures=&firstLevelTextures;
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
  find_lexeme(input,where_lexeme_start,return_lexeme_end);
  if (where_lexeme_start<return_lexeme_end) {
    Ogre::String provided=input.substr(where_lexeme_start,return_lexeme_end-where_lexeme_start);
    if (provided.find("*")==String::npos) {
        depends_on.push_back(provided);
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
          if (shouldForceFirstLevelName(dep,input,pwhere,second_input,*this->opts)) {
              firstLevelTextures->insert(dep);              
          }else if (firstLevelTextures->find(dep)!=firstLevelTextures->end()){
              firstLevelTextures->erase(firstLevelTextures->find(dep));
              printf("WARNING: tilable texture %s specified in the 0th coordinate set\n",dep.c_str());
          }
          depends_on.push_back(dep);
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
  /// A map of which material names are provided by which file (the filename of that file and that files hash)
  MaterialMap *provides_to_name;
  /// A map of which dependencies are required by which files This is for when an olde school MshW file was required which would need all dependencies period
  DependencyMap *overarching_dependencies;
  ///The dependencies of my file (inherits from the overarching dependencies.
  DependencyPair*my_dependencies;
  ///The options provided by the caller to this entire replace_material script
  ReplaceMaterialOptionsAndReturn *username;
public:

///This function takes as input many of the maps required to figure out which material names belong in which 3rd or first level names for scripts in order to munge those names
    DependencyReplacingDataStream(Ogre::DataStreamPtr&input,const Ogre::String &destination,MaterialMap *provides_to_name,DependencyMap *overarching_dependencies,   DependencyPair *my_dependencies, bool thirdLevelNaming, ReplaceMaterialOptionsAndReturn&username): ReplacingDataStream(input,destination,&nvpl){
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
          MaterialMap::iterator where=this->provides_to_name->find(dep);
          Fingerprint hash_value = Fingerprint::null();
          if (where==this->provides_to_name->end()|| where->second.first == Fingerprint::null()) {
              //hash_value=processFileDependency(where,*provides_to_name,std::map<Ogre::String,Ogre::String,SpecialStringSort>(),*overarching_dependencies, username);
          }else{
              hash_value=where->second.first;
          }

          retval+=input.substr(pwhere,lexeme_start-pwhere);
          bool shouldForceFirst=false;
          if (texture_reference) {
              if (shouldForceFirstLevelName(dep,input,pwhere,second_input,*username)) {
                  shouldForceFirst=true;
              }
          }
          retval += (hash_value==Fingerprint::null())?dep:firstOrThirdLevelName(dep,*username,hash_value,shouldForceFirst);
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
        
        if (where->second.second!=filename) {
          depends_on.push_back(depended);
          DependencyPair * other_dep=&(*overarching_dependencies)[where->second.first];
          my_dependencies->materials.insert(other_dep->materials.begin(),other_dep->materials.end());
          my_dependencies->files.insert(other_dep->files.begin(),other_dep->files.end());
          my_dependencies->materials.insert(where->second.first);
          return '\"'+firstOrThirdLevelName(where->second.second,*username,where->second.first,false)+':'+depended+'\"';
        }else {
            
           return eliminateFilename(filename,depended);
        }
      }else if (depended.find_first_of('*')==String::npos) {
        printf ("Cannot locate provider of %s\n",depended.c_str());
        return depended;
      }else {
        return depended;
      }
    }else {
      return Ogre::String();
    }
  }
};

void hashfwrite(const Fingerprint&digest, const char * data, size_t len, ReplaceMaterialOptionsAndReturn &opts) {
    opts.hashToFirstLevelName[digest]=opts.uploadedFiles.size();
    opts.uploadedFiles.push_back(ResourceFileUpload());
    opts.uploadedFiles.back().mID=URI(opts.uploadHashContext, digest.convertToHexString());
    opts.uploadedFiles.back().mHash=digest;
    MutableDenseDataPtr dat (new DenseData(Range(0,len,LENGTH,true)));
    if (len)
        memcpy(dat->writableData(),data,len);
    opts.uploadedFiles.back().mData=dat;
}
static std::string cutoff;

/**
 * This function opens a file from disk and returns the data within that file
 * it is currently a convenience method but later it will need to handle ogre resource streams
 * \param name is the filename
 * \param len returns the length of the file
 * \returns data malloced to size len filled with the file's contents
 */
Ogre::String getHashFileData(const Ogre::String& name) {
  {
    bool isbinary=name.find(".mesh")!=Ogre::String::npos;
    if (isbinary) {
        isbinary=true;
    }
    int length=0;
    char * data=const_cast<char*>(nativeData(name,length));
    if (data) {
      size_t len=(size_t)length;
      return String(data,len);
    }
  }
  Ogre::String retval;
  std::ifstream handle;
  handle.open(name.c_str(),std::ios::in|std::ios::binary);
  {
      Ogre::FileStreamDataStream fp(&handle,false);
      retval=fp.getAsString();
      fp.close();
  }
  return retval;
}
namespace Meru {
Fingerprint processFileDependency(FileMap::iterator file,FileMap &filemap, const MaterialMap &materialmap,DependencyMap&overarching_dependencies, const std::set<Ogre::String>&firstLevelTextures, ReplaceMaterialOptionsAndReturn &opts) {
  file->second=Fingerprint::null();
  size_t len=0;
  Ogre::String processed = getHashFileData(file->first);
  bool ismesh=file->first.find(".mesh")!=Ogre::String::npos;
  DependencyPair mydeps;
  if (file->first.find(".dds")!=Ogre::String::npos||file->first.find(".gif")!=Ogre::String::npos||file->first.find(".jpeg")!=Ogre::String::npos||file->first.find(".png")!=Ogre::String::npos||file->first.find(".tif")!=Ogre::String::npos||file->first.find(".tga")!=Ogre::String::npos||file->first.find(".jpg")!=Ogre::String::npos||file->first.find(".vert")!=Ogre::String::npos||file->first.find(".frag")!=Ogre::String::npos||file->first.find(".hlsl")!=Ogre::String::npos||file->first.find(".cg")!=Ogre::String::npos||file->first.find(".glsl")!=Ogre::String::npos) {
  }else {
    bool isbinary=ismesh||file->first.find(".skeleton")!=Ogre::String::npos;
    replaceAll(processed,filemap,materialmap,overarching_dependencies,firstLevelTextures,mydeps,opts,isbinary);//is binary
  }
  Fingerprint fileHash=Fingerprint::computeDigest(processed);
  file->second=fileHash;
  hashfwrite(fileHash,processed.data(),processed.length(),opts);
  
  overarching_dependencies[fileHash]=mydeps;
  return fileHash;
}

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
bool replaceOne(Ogre::String &data, Ogre::String src, Ogre::String dst,FileMap &filemap, const MaterialMap&materialmap, DependencyMap&overarching_dependencies,const std::set<Ogre::String>&firstLevelTextures,ReplaceMaterialOptionsAndReturn &opts,bool do_stripslashes, bool allow_binary) {
    Ogre::String::size_type where=0;
    bool retval=false;
    std::string strippedsrc=do_stripslashes?stripslashes(src):src;
    /*if (src.find("OffsetMapping.cg")!=std::string::npos) {
      printf ("Searching for %s (%s)\n",src.c_str(),strippedsrc.c_str());
      }*/
    while ((where=data.find(strippedsrc,where))!=Ogre::String::npos) {
        /*if (src.find("OffsetMapping.cg")!=std::string::npos) {
          printf ("Found %s\n",src.c_str());
          }*/
        
        if (false) {
            printf("error: self referencial file %s\n",src.c_str());
            break;
        }else {
            /*if (src.find("Ueber")!=String::npos) {
                printf ("COOLER %s\n",dst.c_str());
                }*/
            if (dst.length()==0) {
                if (src.find(".dds")!=Ogre::String::npos||src.find(".gif")!=Ogre::String::npos||src.find(".jpeg")!=Ogre::String::npos||src.find(".png")!=Ogre::String::npos||src.find(".tif")!=Ogre::String::npos||src.find(".tga")!=Ogre::String::npos||src.find(".jpg")!=Ogre::String::npos||src.find(".vert")!=Ogre::String::npos||src.find(".frag")!=Ogre::String::npos||src.find(".hlsl")!=Ogre::String::npos||src.find(".cg")!=Ogre::String::npos||src.find(".glsl")!=Ogre::String::npos) {
                    dst=firstOrThirdLevelName(src,opts,processFileDependency(filemap.find(src),filemap,materialmap,overarching_dependencies, firstLevelTextures, opts),firstLevelTextures.find(stripslashes(src))!=firstLevelTextures.end());
                }else{
                    Ogre::String summary =data.length()>200?data.substr(0,200):data;
                    printf ("ERROR: File %s processed out of order with file beginning with %s\n",src.c_str(),summary.c_str());
                    
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
    return retval;
}
namespace Meru {
void replaceAll(Ogre::String &data, FileMap &filemap, const MaterialMap&materialmap,DependencyMap &overarching_dependencies, const std::set<Ogre::String>&firstLevelTextures, DependencyPair&my_dependencies, ReplaceMaterialOptionsAndReturn &opts,bool allow_binary) {
    for (FileMap::iterator i=filemap.begin(),ie=filemap.end();i!=ie;++i) {
        if (replaceOne(data,i->first,firstOrThirdLevelName(i->first,opts,i->second,firstLevelTextures.find(stripslashes(i->first))!=firstLevelTextures.end()),filemap,materialmap,overarching_dependencies,firstLevelTextures,opts,true,allow_binary)) {
            my_dependencies.files.insert(i->second);
            DependencyPair *dp=&overarching_dependencies[i->second];
            my_dependencies.materials.insert(dp->materials.begin(),dp->materials.end());
            my_dependencies.files.insert(dp->files.begin(),dp->files.end());
        }
    }
    for (MaterialMap::const_iterator i=materialmap.begin(),ie=materialmap.end();i!=ie;++i) {
        // ??? // if (allow_binary&&i->first.find("Colu")!=String::npos) { printf ("WOOT"); }
            
        if (replaceOne(data,i->first,'\"'+firstOrThirdLevelName(i->second.second,opts,i->second.first,false)+":"+i->first+'\"',filemap,materialmap,overarching_dependencies,firstLevelTextures,opts,false,allow_binary)) {
            my_dependencies.materials.insert(i->second.first);
            DependencyPair *dp=&overarching_dependencies[i->second.first];
            my_dependencies.materials.insert(dp->materials.begin(),dp->materials.end());
            my_dependencies.files.insert(dp->files.begin(),dp->files.end());
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

namespace Meru {

static bool isMHashScheme(const URI &uri, ReplaceMaterialOptionsAndReturn &opts) {
    return (uri.context() == opts.uploadHashContext);
}

// FIXME: I don't get the point of this function
static bool find_duplicate(const URI &hashuri,ReplaceMaterialOptionsAndReturn &opts, const std::multimap<Fingerprint,URI>&existing) {
    Fingerprint hash (Fingerprint::convertFromHex(hashuri.filename()));
    for (std::multimap<Fingerprint,URI>::const_iterator where=existing.find(hash);
         where!=existing.end()&&where->first==hash;
         ++where) {
        if (isMHashScheme(where->second,opts)) 
            return true;
    }
    return false;
}
std::vector<ResourceFileUpload> ProcessOgreMeshMaterialDependencies(const std::vector<Ogre::String> &filenames,const ReplaceMaterialOptions&options) {
  ReplaceMaterialOptionsAndReturn opts(options);  
  opts.disallowedThirdLevelFiles.insert("meru:///UeberShader.hlsl");//this file should be updated manually by devs, not by artists
  DependencyMap overarching_dependencies;
  FileMap oldname_to_newname;
  MaterialMap provides_to_argnum;
  ProviderMap argnum_to_provides;
  ProviderMap argnum_to_depends;
  std::map<int,Ogre::String> dependency_order;
  int round=0;
  std::set<Ogre::String> firstLevelTextures;
  for (int i=0;i<num_native_files;++i) {  
      oldname_to_newname[native_files[i]]=Fingerprint::null();
  }
  for (size_t i=0;i<filenames.size();++i) {
    oldname_to_newname[filenames[i]]=Fingerprint::null();
    if (filenames[i].find(".material")!=Ogre::String::npos||
        filenames[i].find(".script")!=Ogre::String::npos||
        filenames[i].find(".os")!=Ogre::String::npos||
        filenames[i].find(".program")!=Ogre::String::npos) {
      std::ifstream fhandle;
      fhandle.open(filenames[i].c_str(),std::ios::in|std::ios::binary);
      RecordingDependencyDataStream *tmprds;
      std::vector<Ogre::String> provides,depends_on;
      {
          Ogre::DataStreamPtr input(new Ogre::FileStreamDataStream(&fhandle,false));
          Ogre::DataStreamPtr rds (tmprds=new RecordingDependencyDataStream (input,filenames[i],firstLevelTextures,opts));//HELP FIXME ogre crashes ... need to get rid of inner bloc too
      

          tmprds->preprocessData(provides,depends_on);
          argnum_to_provides[filenames[i]]=provides;
          for (std::vector<Ogre::String>::iterator iter=provides.begin(),itere=provides.end();iter!=itere;++iter) {
              MaterialMap::iterator dupe;
              if ((dupe=provides_to_argnum.find(*iter))!=provides_to_argnum.end()&&dupe->second.first!=Fingerprint::null()) {
				  std::string s (dupe->second.first.convertToHexString());
                  printf ("Duplicate Material provided %s by both %s and %s\n",
                          iter->c_str(),
                          s.c_str(),
                          filenames[i].c_str());
               }else {
                  //printf ("%s provided by %s\n",iter->c_str(),filenames[i]);
                  provides_to_argnum[*iter]=std::pair<Fingerprint,Ogre::String>(Fingerprint::null(),filenames[i]);
               }
          }
      }
      {
        bool depends_on_nothing=true;
        for (size_t j=0;j<depends_on.size();++j) {
          if (provides_to_argnum[depends_on[j]].second!=filenames[i]) {
            depends_on_nothing=false;
            //printf ("%s depends on %s\n",filenames[i].c_str(),depends_on[j].c_str());
            argnum_to_depends[filenames[i]].push_back(depends_on[j]);
          }
        }
        if (depends_on_nothing) {
          dependency_order[round++]=filenames[i];
        }
      }
    }
  }
  
  ProviderMap::iterator dep;
  size_t prevsize;
  do {
    prevsize=argnum_to_depends.size();
    for(dep=argnum_to_depends.begin();dep!=argnum_to_depends.end();) {
      std::vector<Ogre::String>::iterator iter=dep->second.begin(),itere=dep->second.end();
      for (;iter!=itere;++iter) {
          if (argnum_to_depends.find(*iter)!=argnum_to_depends.end()||argnum_to_depends.find(provides_to_argnum[*iter].second)!=argnum_to_depends.end()){
              //can't use me
              //printf ("FOUND %s\n",iter->c_str());
              break;
          }
      }
      if (iter==itere) {
        //can use me...no dependencies
        dependency_order[round++]=dep->first;                
        Ogre::String target;
        ++dep;
        if (dep!=argnum_to_depends.end()) {
          target=dep->first;
          --dep;
          argnum_to_depends.erase(dep);
          dep=argnum_to_depends.find(target);
        }else {
          --dep;
          argnum_to_depends.erase(dep);
          break;
        }
      }else {
        ++dep;
      }
      
    }    
  }while (argnum_to_depends.size()!=prevsize);
  if (argnum_to_depends.size()) {
    fprintf (stderr,"circular dependency. Not processing files:\n");
    dep=argnum_to_depends.begin();
    for(;dep!=argnum_to_depends.end();++dep) {
      printf ("%s ",dep->first.c_str());
    }
  }

  for (std::map<int,Ogre::String>::iterator dep=dependency_order.begin();dep!=dependency_order.end();++dep) {
      printf ("PLAN %s\n",dep->second.c_str());
      }
  for (std::map<int,Ogre::String>::iterator dep=dependency_order.begin();dep!=dependency_order.end();++dep) {
    std::ifstream fhandle;  
    {
        Ogre::DataStreamPtr input(isNativeFile(dep->second)?nativeStream(dep->second):(fhandle.open(dep->second.c_str(),std::ios::in|std::ios::binary),new Ogre::FileStreamDataStream(&fhandle,false)));
        DependencyPair mydeps;
    
        Ogre::DataStreamPtr rds (new DependencyReplacingDataStream (input,dep->second,&provides_to_argnum,&overarching_dependencies,&mydeps,false,opts));
        Ogre::String data=rds->getAsString();

        //printf ("Processing %s\n",dep->second.c_str());
        //fflush(stdout);
        replaceAll(data,oldname_to_newname,MaterialMap(),overarching_dependencies,firstLevelTextures,mydeps,opts, false);
        Fingerprint fileHash=Fingerprint::computeDigest(data);
    
        overarching_dependencies[fileHash]=mydeps;
        hashfwrite(fileHash,data.data(),data.length(),opts);
        std::vector<Ogre::String>::iterator i=argnum_to_provides[dep->second].begin(),ie=argnum_to_provides[dep->second].end();
        for (;i!=ie;++i) {
            provides_to_argnum[*i].first=fileHash;
        }
        oldname_to_newname[dep->second]=fileHash;
      } 
  }
  for (FileMap::iterator i=oldname_to_newname.begin(),ie=oldname_to_newname.end();i!=ie;++i) {
	  if (i->second==Fingerprint::null())
        processFileDependency(i,oldname_to_newname,provides_to_argnum,overarching_dependencies,firstLevelTextures,opts);
  }
  String mesh_old_name;
  Fingerprint mesh_new_hash;
  String mesh_new_name;
  std::set<Fingerprint> mesh_hashes;
  std::set<Fingerprint> skeleton_hashes;
  for (FileMap::const_iterator iter=oldname_to_newname.begin(),iterend=oldname_to_newname.end();iter!=iterend;++iter) {
      firstOrThirdLevelName(iter->first,opts,iter->second,firstLevelTextures.find(stripslashes(iter->first))!=firstLevelTextures.end());
      if (iter->first.find(".mesh")!=String::npos&&iter->first.find(".mesh")+strlen(".mesh")==iter->first.length()&&iter->first.find("models")!=String::npos) {
          mesh_hashes.insert(iter->second);
          mesh_old_name=iter->first;
          mesh_new_name=iter->second.convertToHexString();
          mesh_new_hash=iter->second;
      }
      if (iter->first.find(".skeleton")+strlen(".skeleton")==iter->first.length()&&iter->first.find(".skeleton")!=String::npos) {
          skeleton_hashes.insert(iter->second);
      }
  }
  size_t where=0;
  for (ptrdiff_t i=opts.uploadedFiles.size();i>0;--i) {

      if (opts.disallowedThirdLevelFiles.find(opts.uploadedFiles[i-1].mID.toString())!=opts.disallowedThirdLevelFiles.end()) {
          opts.uploadedFiles.erase(opts.uploadedFiles.begin()+i-1);
      }
      if (opts.uploadedFiles[i-1].mHash == mesh_new_hash) {
          where=i-1;
          String::size_type named_ending=opts.uploadedFiles[i-1].mID.toString().find(".mesh");
          if (named_ending!=String::npos&&named_ending+strlen(".mesh")==opts.uploadedFiles[i-1].mID.toString().length()) {
              break;//found the real deal
          }
      }
      String::size_type whereMeshName=opts.uploadedFiles[i-1].mID.toString().find(mesh_new_name);
      if (whereMeshName!=String::npos&&whereMeshName+mesh_new_name.length()==opts.uploadedFiles[i-1].mID.toString().length()) {
          where=i-1;
      }
  }
  
  if (opts.uploadedFiles.size()&&where) {
      ResourceFileUpload tmp=opts.uploadedFiles[0];
      opts.uploadedFiles[0]=opts.uploadedFiles[where];
      opts.uploadedFiles[where]=tmp;
  }
  std::map<URI,ResourceFileUpload> existingFiles;
  std::multimap<Fingerprint,URI> reverseExistingFiles;//a map from any shasum to its file
  for (size_t i=0;i<opts.uploadedFiles.size();++i) {
      std::map<URI,ResourceFileUpload>::iterator where=existingFiles.find(opts.uploadedFiles[i].mID);
      if (where==existingFiles.end()){
          existingFiles.insert(std::pair<URI,ResourceFileUpload>(opts.uploadedFiles[i].mID,opts.uploadedFiles[i]));
          reverseExistingFiles.insert(std::pair<Fingerprint,URI>(opts.uploadedFiles[i].mHash,opts.uploadedFiles[i].mID));
          //retval.push_back(opts.uploadedFiles[i]);
      }else if (where->second.mHash!=opts.uploadedFiles[i].mHash) {
          SILOG(ogre,error,"File "<<opts.uploadedFiles[i].mID.toString() << " has Hash mismatch: "<<opts.uploadedFiles[i].mHash.convertToHexString() << " and "<<where->second.mHash.convertToHexString());
          //retval.push_back(opts.uploadedFiles[i]);          
      }
  }
  std::vector<ResourceFileUpload> retval;
  std::set <URI> material_files;
  std::vector<ResourceFileUpload> ordered_material_files;
  for (std::map<int,Ogre::String>::iterator dep=dependency_order.begin();dep!=dependency_order.end();++dep) {
      const Fingerprint &material_hash=oldname_to_newname[dep->second];
      for (std::multimap<Fingerprint,URI>::iterator where=reverseExistingFiles.find(material_hash);
           where!=reverseExistingFiles.end()&&where->first==material_hash;
           ++where) {
              std::map<URI,ResourceFileUpload>::iterator whereFileUpload=existingFiles.find(where->second);
              if (whereFileUpload!=existingFiles.end()) {
                  material_files.insert(where->second);
                  ordered_material_files.push_back(whereFileUpload->second);
              }else {
                  SILOG(ogre,fatal, "File in reverse existing file map not in forward map");
              }
      }
  }
  std::vector<ResourceFileUpload> ordered_named_textures;
  std::vector<ResourceFileUpload> ordered_unnamed_textures;
  std::vector<ResourceFileUpload> ordered_skeletons;
  std::vector<ResourceFileUpload> ordered_meshes;
  for (std::map<URI,ResourceFileUpload>::iterator iter=existingFiles.begin();iter!=existingFiles.end();++iter) {
      if (material_files.find(iter->first)==material_files.end()) {
          if (mesh_hashes.find(iter->second.mHash)!=mesh_hashes.end()) {
              if (isMHashScheme(iter->first,opts)==false||!find_duplicate(iter->first,opts,reverseExistingFiles)) {
                  ordered_meshes.push_back(iter->second);
                  if (iter->second.mID==opts.uploadedFiles[0].mID) {
                      ResourceFileUpload tmp=ordered_meshes[0];
                      opts.uploadedFiles[0]=iter->second;
                      opts.uploadedFiles.back()=tmp;                  
                  }
              }
          } else if (skeleton_hashes.find(iter->second.mHash)!=skeleton_hashes.end()) {
              if (isMHashScheme(iter->first,opts)==false||!find_duplicate(iter->first,opts,reverseExistingFiles)) {
                  ordered_skeletons.push_back(iter->second);              
              }
          }else {
              if (isMHashScheme(iter->first,opts)){
                  if (!find_duplicate(iter->first,opts,reverseExistingFiles)) {
                      ordered_unnamed_textures.push_back(iter->second);
                  }
              }else {
                  ordered_named_textures.push_back(iter->second);
              }
              //it's a texture, f00
          }
      }
  }
  retval.insert(retval.end(),ordered_named_textures.begin(),ordered_named_textures.end());
  retval.insert(retval.end(),ordered_unnamed_textures.begin(),ordered_unnamed_textures.end());
  retval.insert(retval.end(),ordered_material_files.begin(),ordered_material_files.end());
  retval.insert(retval.end(),ordered_skeletons.begin(),ordered_skeletons.end());
  std::reverse(ordered_meshes.begin(),ordered_meshes.end());
  retval.insert(retval.end(),ordered_meshes.begin(),ordered_meshes.end());
  std::reverse(retval.begin(),retval.end());
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
    Transfer::UploadEventPtr uploadev (std::tr1::dynamic_pointer_cast<UploadEvent>(ev));
    if (!uploadev) {
        return EventResponse::nop();
    }
    bool del;
    {
        boost::unique_lock<boost::mutex> (mLock);
        ResourceUploadStatus st;
        st = uploadev->getStatus();
        stat->mStatusMap.insert(ResourceStatusMap::value_type(current, st));
        if (0 == --stat->mNumberRemaining) {
            stat->mCallback(stat->mStatusMap);
            del = true;
        }
    }
    if (del) {
        delete stat;
    }
    return EventResponse::nop();
}

void UploadFilesAndConfirmReplacement(TransferManager*tm, 
                                      const std::vector<ResourceFileUpload> &filesToUpload,
                                      const URIContext &hashContext,
                                      const std::tr1::function<void(ResourceStatusMap const &)> &callback) {
    UploadStatus *status = new UploadStatus(callback, filesToUpload.size());

    for (size_t i = 0; i < filesToUpload.size(); ++i) {
        const ResourceFileUpload &current = filesToUpload[i];

        if (current.mID.context() == hashContext) {
            tm->uploadByHash(Transfer::RemoteFileId(current.mHash, current.mID),
                       current.mData,
                       std::tr1::bind(&UploadFinished, status, current, _1));
        } else {
            tm->upload(current.mID,
                       Transfer::RemoteFileId(current.mHash, hashContext),
                       current.mData,
                       std::tr1::bind(&UploadFinished, status, current, _1));
        }
    }
}

}
