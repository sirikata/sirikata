/*  Meru
 *  OgreMeshMaterialDependencyTool.hpp
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

#include <transfer/URI.hpp>
#include <transfer/TransferManager.hpp>
#include <transfer/TransferData.hpp>

namespace Meru {

using ::Sirikata::Transfer::URIContext;
using ::Sirikata::Transfer::URI;
using ::Sirikata::Transfer::Fingerprint;
using ::Sirikata::Transfer::DenseData;
using ::Sirikata::Transfer::DenseDataPtr;
using ::Sirikata::Transfer::TransferManager;
using ::Sirikata::Transfer::UploadEvent;
using ::Sirikata::Transfer::UploadEventPtr;
//typedef TransferManager::Status ResourceUploadStatus;
typedef int ResourceUploadStatus;

enum FileType { MESH, MATERIAL, DATA, NUMTYPES };

class DiskFile {
	String str;
public:
	const String &diskpath() const { return str; }
	String &diskpath() { return str; }
	static DiskFile makediskfile(const String &mypath) {
		DiskFile ret;
		ret.str = mypath;
		return ret;
	}
	bool operator==(const DiskFile &other) const {
		return str == other.str;
	}
	bool operator!=(const DiskFile &other) const {
		return str != other.str;
	}
	bool operator<(const DiskFile &other) const {
		return str < other.str;
	}
};

class ResourceFileUpload {
public:
    FileType mType;
    URI mID;
	bool mReferencedByHash;
    DiskFile mSourceFilename;
    ///Hash optional
    Fingerprint mHash;
    DenseDataPtr mData;
    ResourceFileUpload():mHash(Fingerprint::null()) {
        
    }
    static ResourceFileUpload comparator(const URI &id) {
        ResourceFileUpload retval;
        retval.mID=id;
        return retval;
    }
    bool operator <(const ResourceFileUpload&i)const {
        return mID<i.mID;
    }
    bool operator ==(const ResourceFileUpload&i) const {
        return mID==i.mID;
    }
};
 
/** 
 * This class allows a client to pass options it wishes a material export to obey
 */
class ReplaceMaterialOptions {
public:
    ///Should tileable textures try to use group central acct
//    bool meruBaseTileable;
    ///Should tileable textures be named at all, or just addressed by hash
    bool namedTileable;
    ///Should All references be by content or hash
    bool forceFirstLevelNames;
    ///Should all references be by filename
    bool forceThirdLevelNames;
//    String human_readable_name;
    std::set<String> disallowedThirdLevelFiles;

    URIContext uploadHashContext; ///< URI of the form "mhash:///"
    URIContext uploadNameContext; ///< URI of the form "meru:///"

    ReplaceMaterialOptions() {
        namedTileable=true;
        forceThirdLevelNames=false;
        forceFirstLevelNames=false;
/*
        meruBaseTileable=true;
*/
    }
};

///A class that holds the ReplaceMaterialOptions and various temporary values to calculate returned files from
class ReplaceMaterialOptionsAndReturn;



class ResourceFileUploadData;
//maps indexed by filename
// filemap goes filename -> URI.
typedef  std::map<DiskFile, ResourceFileUploadData*> FileMap;


/**
 * This helper class wraps
 * all the dependencies a file could have in terms of 
 * materials it depends on
 * and resource files that it uses
 */
class DependencyPair{
public:
    ///The Ogre scripts and texture/source files on which this script file depends
  std::set<DiskFile> files;

};

// Map from Material Names to the file that Provides them.
// .first are provided by .second
typedef std::map<String, DiskFile> MaterialMap;

/**
 * A special string sorting class that takes length into account first
 */
class SpecialStringSort {
public:
  bool operator() (const std::string &a, const std::string&b) const {
    if (a.length()!=b.length()) return a.length()>b.length();
    return a<b;
  }
};

/**
 * This internal class processes dependencies of a single file from an iterator into the map of of files to be committed
 * This internal class will take all files in the map and replace any references to them with the appropriate hash or 3rd level name
 * \param file iterator into the map of files to have its dependencies calculated (will modify the iterator)
 * \param filemap the map of other files that are available to be processed to their hash or high level names
 * \param materialmap the set of other ogre materials (not scripts, materials within those scripts paired with those respective materials disk filename and third level names
 * \param overarching_dependencies a map from file names to those datafiles' dependencies (be they materials/scripts or texture/source files) This may modify that list due to recursive processing
 * \param firstLevelTextures is a list of textures that must be first level due to their not being tileable (as set by the RecordingDependencyDataStream)
 * \param opts is the overarching options for the entire export
 * \returns the filename of the processed material
 */
Fingerprint processFileDependency(std::map<String,Fingerprint,SpecialStringSort>::iterator file,std::map<String,Fingerprint,SpecialStringSort> &filemap, const std::map<String,std::pair<Fingerprint,String>,SpecialStringSort> &materialmap,std::map<Fingerprint,DependencyPair>&overarching_dependencies, const std::set<String>&firstLevelTextures, ReplaceMaterialOptionsAndReturn &opts);

/**
 * This function replaces all dependencies referenced within a file to their CDN names and returns a list of dependencies this file has for future use
 * This function must be called in dependency order per file (so the files with least dependencies get called first
 * This function then replaces all references to dependent files with the appropriate hash or mangling of those names
 * \param data is the content of the file (the entire file is stored in the data variable
 * \param filemap the map of other files that are available to be processed to their hash or high level names. This may modify that map
 * \param materialmap the set of other ogre materials (not scripts, materials within those scripts paired with those respective materials disk filename and third level names
 * \param overarching_dependencies a map from file names to those datafiles' dependencies (be they materials/scripts or texture/source files) This may modify that list due to recursive processing 
 * \param firstLevelTextures is a list of textures that must be first level due to their not being tileable (as set by the RecordingDependencyDataStream)
 * \param my_dependencies is the list of dependencies that this current file depends on...this is one of the return values of this function
 * \param OptionsAndOutput is the list of options that are passed in by the caller of this entire library. This tool will return all materials to be written in the OptionsAndOutput varialbe
 * \param allow_binary is whether this particular file is binary so that dependencies are known to need to replace the length value before the data aspect
 */
void replaceAll(DenseDataPtr &data, FileMap &filemap, const MaterialMap&materialmap, DependencyPair&my_dependencies, ReplaceMaterialOptionsAndReturn &opts,bool allow_binary);
/**
 * This function takes in 
 * \param filenames the list of file names on the hard disk to be uploaded
 * \param opts a set of options (what the username is, etc)
 * \returns a list of files to be uploaded, including their hashes and filenames and data contents These files will have been properly modified to include direct references to
 * hashes of other files passed in as well as 3rd level names where appropriate.  Users can pass in lists of exclusions in the ReplaceMaterialOptions
 */
std::vector<ResourceFileUpload> ProcessOgreMeshMaterialDependencies(const std::vector<DiskFile> &filenames,const ReplaceMaterialOptions&options);


/**
 * Upload a set of files to
 * \param rm , the resource manager
 * \param filesToUpload is vector of filesToUpload --all of these files should be uploaded
 * \param username_to_resource_upload_choices is a map of previously chosen responses to uploads from usernames
 * \callback is the function to call when its over...it takes a map mapping resource uploads to whether they succeeded so that a second try can be establised, the result of new resource_upload_choices and a set of usernames to logout when this is all over 
 * \param actuallyUpload dictates if the file should actually be uploaded

   FIXME, Reimplement using Sirikata::Transfer::TransferManager::upload().

 */
class ResourceManager;
typedef std::map<ResourceFileUpload,ResourceUploadStatus> ResourceStatusMap;
void UploadFilesAndConfirmReplacement(::Sirikata::Transfer::TransferManager*tm, 
                                      const std::vector<ResourceFileUpload> &filesToUpload, 
                                      const URIContext &hashContext,
                                      const std::tr1::function<void(ResourceStatusMap const &)> &callback);



}
