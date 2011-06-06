
#include "JSFileBackend.hpp"
#include <map>
#include <fstream>

namespace Sirikata{
namespace JS{

//Events

//Write data contained in file
void FileBackendWriteItem::processEvent()
{
    boost::filesystem::path path (folderName);
    boost::filesystem::path suffix (fileName);
    path = path / suffix;

    String fileToWrite = path.string();
    std::ofstream fWriter (fileToWrite.c_str(),  std::ios::out | std::ios::binary);
        
    for (String::size_type s = 0; s < toWrite.size(); ++s)
    {
        char toWriteChar = toWrite[s];
        fWriter.write(&toWriteChar,sizeof(toWriteChar));
    }

    fWriter.flush();
    fWriter.close();
}

//Deletes item if it exists
void FileBackendClearItem::processEvent()
{
    boost::filesystem::path path (folderName);
    boost::filesystem::path suffix (fileName);
    path = path / suffix;

     if (! boost::filesystem::exists(path))
         return;

     boost::filesystem::remove(path);
 }





JSFileBackend::JSFileBackend()
{
}


/**
   Frees memory associated with all outstanding events that have not been
   flushed.  DOES NOT FLUSH EVENTS.
 */
JSFileBackend::~JSFileBackend()
{
    std::vector<String> allEntries;
    for (OutEventsIter iter = unflushedEvents.begin(); iter != unflushedEvents.end(); ++iter)
        allEntries.push_back(iter->first);

    for (std::vector<String>::iterator strIter = allEntries.begin(); strIter != allEntries.end(); ++strIter)
        clearOutstanding(*strIter);
}



JSBackendInterface::JSBackendCreateCode JSFileBackend::createEntry(const String & prepend)
{
    if (haveUnflushedEvents(prepend))
        return JSBackendInterface::BACKEND_CREATE_FAIL_HAVE_UNFLUSHED;
    
    if (haveEntry(prepend))
        return JSBackendInterface::BACKEND_CREATE_FAIL_EXISTS;

    boost::filesystem::create_directory(boost::filesystem::path(prepend));

    return JSBackendInterface::BACKEND_CREATE_SUCCESS;
}



bool JSFileBackend::haveEntry(const String& prepend)
{
    return boost::filesystem::exists(boost::filesystem::path(prepend));
}


bool JSFileBackend::haveUnflushedEvents(const String& prepend)
{
    OutEventsIter iter = unflushedEvents.find(prepend);
    return (iter !=  unflushedEvents.end());
}
    

bool JSFileBackend::clearItem(const String& prependToken,const String& itemID)
{
    if(! haveUnflushedEvents(prependToken))
        if (! haveEntry(prependToken))
            return false;

    FileBackendClearItem* fbci = new FileBackendClearItem(prependToken, itemID);
    unflushedEvents[prependToken].push_back(fbci);
    return true;
}


bool JSFileBackend::write(const String & prependToken, const String& idToWriteTo, const String& strToWrite)
{
    if(! haveUnflushedEvents(prependToken))
        if (! haveEntry(prependToken))
            return false;

    FileBackendWriteItem* fbw = new FileBackendWriteItem(prependToken,idToWriteTo,strToWrite);
    unflushedEvents[prependToken].push_back(fbw);
    return true;
}


bool JSFileBackend::flush(const String& prependToken)
{
    if(! haveUnflushedEvents(prependToken))
        if (! haveEntry(prependToken))
            return false;

    std::vector<FileBackendEvent*> fbeVec = unflushedEvents[prependToken];
    for (FileEventVecIter iter = fbeVec.begin(); iter != fbeVec.end(); ++iter)
        (*iter)->processEvent();

    clearOutstanding(prependToken);

    return true;
}


bool JSFileBackend::clearOutstanding(const String& prependToken)
{
    if(! haveUnflushedEvents(prependToken))
        return false;
    
    OutEventsIter unflushedIter =  unflushedEvents.find(prependToken);
    
    FileEventVec fev = unflushedIter->second;
    for (FileEventVecIter fevIter = fev.begin(); fevIter != fev.end(); ++fevIter)
    {
        FileBackendEvent* fbe = *fevIter;
        delete fbe;
        *fevIter = NULL;
    }

    unflushedEvents.erase(unflushedIter);
    
    return true;
}


bool JSFileBackend::clearEntry (const String& prependToken)
{
    //remove the folder from maps
    bool outstandingCleared = clearOutstanding(prependToken);
    
    boost::filesystem::path path (prependToken);
    if (! boost::filesystem::exists(path))
        return outstandingCleared || false;

    
    boost::filesystem::remove_all(path);
    return true;
}


bool JSFileBackend::read(const String& prepend, const String& idToReadFrom, String& toReadTo)
{
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






