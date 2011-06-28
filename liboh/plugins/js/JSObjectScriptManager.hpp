/*  Sirikata
 *  JSObjectScriptManager.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_JS_OBJECT_SCRIPT_MANAGER_HPP_
#define _SIRIKATA_JS_OBJECT_SCRIPT_MANAGER_HPP_


#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>
#include <sirikata/mesh/ModelsSystem.hpp>
#include <sirikata/mesh/Filter.hpp>
#include <sirikata/mesh/Meshdata.hpp>

#include <v8.h>



namespace Sirikata {
namespace JS {

class JSObjectScript;

class JSObjectScriptManager : public ObjectScriptManager {
public:
    static ObjectScriptManager* createObjectScriptManager(ObjectHostContext* ctx, const Sirikata::String& arguments);

    JSObjectScriptManager(ObjectHostContext* ctx, const Sirikata::String& arguments);
    virtual ~JSObjectScriptManager();

    virtual ObjectScript* createObjectScript(HostedObjectPtr ho, const String& args, const String& script);
    virtual void destroyObjectScript(ObjectScript* toDestroy);

    JSObjectScript* createHeadless(const String& args, const String& script,int32 maxres);

    OptionSet* getOptions() const { return mOptions; }


    v8::Persistent<v8::ObjectTemplate> mHandlerTemplate;
    v8::Persistent<v8::FunctionTemplate> mVisibleTemplate;

    v8::Persistent<v8::FunctionTemplate> mPresenceTemplate;
    v8::Persistent<v8::ObjectTemplate>   mContextTemplate;
    v8::Persistent<v8::ObjectTemplate>   mUtilTemplate;
    v8::Persistent<v8::ObjectTemplate>   mInvokableObjectTemplate;
    v8::Persistent<v8::ObjectTemplate>   mSystemTemplate;
    v8::Persistent<v8::ObjectTemplate>   mTimerTemplate;
    v8::Persistent<v8::ObjectTemplate>   mContextGlobalTemplate;

    // Mesh loading functions
    typedef std::tr1::function<void(Mesh::MeshdataPtr)> MeshLoadCallback;
    void loadMesh(const Transfer::URI& uri, MeshLoadCallback cb);

private:
    ObjectHostContext* mContext;

    void createVisibleTemplate();
    void createHandlerTemplate();
    void createPresenceTemplate();
    void createContextTemplate();
    void createUtilTemplate();
    void createJSInvokableObjectTemplate();
    void createSystemTemplate();
    void createTimerTemplate();
    void createContextGlobalTemplate();


    void createTemplates();

    // The manager tracks the templates so they can be reused by all the
    // individual scripts.
    v8::Persistent<v8::FunctionTemplate> mVec3Template;
    v8::Persistent<v8::FunctionTemplate> mQuaternionTemplate;
    v8::Persistent<v8::FunctionTemplate> mPatternTemplate;

    OptionSet* mOptions;

    // The manager also maintains mesh data. We store it here so it is easily
    // shared by all the scripts, particularly important because mesh data is so
    // costly memory-wise.
    // FIXME this should be more complicated -- perhaps tracking shared_ptr's for awhile
    // or just maintaining an LRU.
    typedef std::tr1::unordered_map<Transfer::URI, Mesh::MeshdataWPtr, Transfer::URI::Hasher> MeshCache;
    MeshCache mMeshCache;
    // Some additional information is needed to keep track of in-progress
    // downloads. Note that these are only ever modified in the main thread
    typedef std::tr1::unordered_map<Transfer::URI, Transfer::ResourceDownloadTaskPtr, Transfer::URI::Hasher> MeshDownloads;
    MeshDownloads mMeshDownloads;
    typedef std::vector<MeshLoadCallback> MeshLoadCallbackList;
    typedef std::tr1::unordered_map<Transfer::URI, MeshLoadCallbackList, Transfer::URI::Hasher> WaitingMeshCallbacks;
    WaitingMeshCallbacks mMeshCallbacks;

    Transfer::TransferPoolPtr mTransferPool;
    // FIXME because we don't have proper multithreaded support in cppoh, we
    // need to allocate our own thread dedicated to parsing
    Network::IOService* mParsingIOService;
    Network::IOWork* mParsingWork;
    Thread* mParsingThread;

    ModelsSystem* mModelParser;
    Mesh::Filter* mModelFilter;

    void meshDownloaded(Transfer::ChunkRequestPtr request, Transfer::DenseDataPtr data);
    void parseMeshWork(const Transfer::URI& uri, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data);
    void meshParsed();
    void finishMeshDownload(const Transfer::URI& uri, Mesh::MeshdataPtr mesh);

};

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_MANAGER_HPP_
