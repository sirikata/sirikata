
#include "JSFileBackend.hpp"
#include <map>
#include <fstream>

namespace Sirikata{
namespace JS{



JSFileBackend::JSFileBackend()
{
}

JSFileBackend::~JSFileBackend()
{
}


/**
   @param {string} prepend.  The name of a folder to create in our file system.
   @return {uuid} Token used to identify this folder.  It is passed back to
   whatever calls createEntry.

   During writes and reads, scripter must pass token back to the FileBackend to
   be able to write to files in this folder.
 */
UUID JSFileBackend::createEntry(const String & prepend)
{
    boost::filesystem::create_directory(boost::filesystem::path(prepend));
    UUID returner = UUID::random();
    idsToFolderNames[returner] = prepend;
    return returner;
}


/**
   Queues writes to the file named idToWriteTo that is in the folder
   corresponding to the String mapped to in the map idsToFolderNames.  Writes
   will not be committed until flush command.  Note, if issue this command
   multiple times with the same seqKey and idToWriteTo, will only process the
   last when calling flush.

   @param {uuid} seqKey.  Used to identify folder to write the file into.
   (Created by call to createEntry.)
   @param{String} idToWriteTo.  The filename to write to in the folder.
   @param{String} strToWrite.  What should actually be written into that file
   pointed at in idToWriteTo.

   @return {bool} returns true if write is queued (ie if have foldername
   corresponding to seqKey).  Otherwise, returns false
   
 */
bool JSFileBackend::write(const UUID& seqKey, const String& idToWriteTo, const String& strToWrite)
{
    std::map<UUID,String>::iterator seqFinder = idsToFolderNames.find(seqKey);
    if (seqFinder == idsToFolderNames.end())
        return false;

    outstandingWrites[seqKey][idToWriteTo] = strToWrite;
    return true;
}


/**
   Flushes all outstanding writes contained in outstandingWrites, and resets
   outstandingWrites to be empty.

   @return {bool} returns true if the seqKey matches a valid folder to write
   to.  Returns false otherwise.
 */
bool JSFileBackend::flush(const UUID& seqKey)
{
    std::map<UUID,String>::iterator seqFinder = idsToFolderNames.find(seqKey);
    if (seqFinder == idsToFolderNames.end())
        return false;

    String folderName = seqFinder->second;
    std::map<String,String> toWrite = outstandingWrites[seqKey];

    for (std::map<String,String>::iterator iter = toWrite.begin(); iter != toWrite.end(); ++iter)
    {
        String fileToWrite = folderName + '/' + iter->first;
        std::ofstream fWriter (fileToWrite.c_str(),  std::ios::out | std::ios::binary);
        String strToWrite = iter->second;
        
        for (String::size_type s = 0; s < strToWrite.size(); ++s)
        {
            char toWrite = strToWrite[s];
            fWriter.write(&toWrite,sizeof(toWrite));
        }

        fWriter.flush();
        fWriter.close();
    }

    outstandingWrites.clear();
    return true;
}



bool JSFileBackend::clearEntry (const String& prepend)
{
    //remove the folder from maps
    for(std::map<UUID,String>::iterator iter= idsToFolderNames.begin(); iter != idsToFolderNames.end(); ++iter)
    {
        if (iter->second == prepend)
        {
            UUID toRemove = iter->first;
            idsToFolderNames.erase(iter);

            //clear outstanding writes
            std::map<UUID,std::map<String,String> >::iterator outstandingIter = outstandingWrites.find(toRemove);
            if (outstandingIter != outstandingWrites.end())
                outstandingWrites.erase(outstandingIter);

            break;
        }
    }        

    
    boost::filesystem::path path (prepend);
    if (! boost::filesystem::exists(path))
        return false;

    boost::filesystem::remove(path);
    return true;
}

bool JSFileBackend::read(const String& prepend, const String& idToReadFrom, String& toReadTo)
{
    boost::filesystem::create_directory(boost::filesystem::path(prepend));
    boost::filesystem::path path (prepend);
    boost::filesystem::path suffix (idToReadFrom);
    path = path / suffix;
    
    if (! boost::filesystem::exists(path))
        return false;

    String fileToRead = path.string();
    
    std::ifstream fRead(fileToRead.c_str(), std::ios::binary | std::ios::in);
    std::ifstream::pos_type begin, end;

    begin = fRead.tellg();
    fRead.seekg(0,std::ios::end);
    end   = fRead.tellg();
    fRead.seekg(0,std::ios::beg);

    std::ifstream::pos_type size = end-begin;
    char* readBuf = new char[size];
    fRead.read(readBuf,size);

    String interString(readBuf,size);
    delete readBuf;
    toReadTo = interString;
    return true;
}


}//end namespace JS
}//end namespace Sirikata






