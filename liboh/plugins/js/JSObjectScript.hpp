/*  Sirikata
 *  JSObjectScript.hpp
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

#ifndef __SIRIKATA_JS_OBJECT_SCRIPT_HPP__
#define __SIRIKATA_JS_OBJECT_SCRIPT_HPP__



#include <string>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/proxyobject/SessionEventListener.hpp>

#include <boost/filesystem.hpp>

#include <v8.h>

#include "JSPattern.hpp"
#include "JSObjectStructs/JSEventHandlerStruct.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include "JSObjects/JSInvokableObject.hpp"
#include "JSVisibleStructMonitor.hpp"
#include "JSEntityCreateInfo.hpp"
#include <sirikata/oh/Storage.hpp>
#include <sirikata/oh/PersistedObjectSet.hpp>

#define EMERSON_RESOURCE_THRESHOLD 10000

namespace Sirikata {
namespace JS {


void printException(v8::TryCatch& try_catch);

class JSObjectScript : public ObjectScript
{

public:


    JSObjectScript(JSObjectScriptManager* jMan, OH::Storage* storage, OH::PersistedObjectSet* persisted_set, const UUID& internal_id);
    virtual ~JSObjectScript();

    v8::Handle<v8::Value> debug_fileWrite(const String& strToWrite,const String& filename);
    v8::Handle<v8::Value> debug_fileRead(const String& filename);


    v8::Handle<v8::Value> executeInSandbox(v8::Persistent<v8::Context> &contExecIn, v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv);


    //this function returns a context with
    v8::Local<v8::Object> createContext(JSPresenceStruct* presAssociatedWith,SpaceObjectReference* canMessage,bool sendEveryone, bool recvEveryone, bool proxQueries, bool canImport, bool canCreatePres,bool canCreateEnt,bool canEval, JSContextStruct*& internalContextField);

    void initialize(const String& args);

    /** Print the given string to the current output. */
    void print(const String& str);

    /** Eval a string, executing its contents in the root object's scope. */
    v8::Handle<v8::Value> eval(const String& contents, v8::ScriptOrigin* em_script_name,JSContextStruct* jscs);

    /** Import a file, executing its contents in contextCtx's root object's
     * scope. Pass in NULL to contextCtx to just execute in JSObjectScript's
     * mContext's root object's scope
     */
    v8::Handle<v8::Value> import(const String& filename, JSContextStruct* jscs);

    /** Require a file, executing its contents in the root object's scope iff it
     *  has not yet been imported.
     */
    v8::Handle<v8::Value> require(const String& filename,JSContextStruct* jscont);


    v8::Handle<v8::Value> storageBeginTransaction(JSContextStruct* jscont);
    v8::Handle<v8::Value> storageCommit(JSContextStruct* jscont, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageWrite(const OH::Storage::Key& key, const String& toWrite, v8::Handle<v8::Function> cb, JSContextStruct* jscont);
    v8::Handle<v8::Value> storageRead(const OH::Storage::Key& key, v8::Handle<v8::Function> cb, JSContextStruct* jscont);
    v8::Handle<v8::Value> storageErase(const OH::Storage::Key& key, v8::Handle<v8::Function> cb, JSContextStruct* jscont);

    v8::Handle<v8::Value> setRestoreScript(JSContextStruct* jscont, const String& script, v8::Handle<v8::Function> cb);


    /**
       We want to ensure that no sandboxes have while(1) loops.  Roughly, the
       way we do this is that our compiler inserts a lot of calls to
       checkResources.  If checkResources returns false, then we throw an
       uncatch-able error.

       Using a stupid implementation for checkResources.  Every time that we
       call eval, and are not currently executing code/handling an event, then
       we set mResourceCounter to zero.  Every interleaving call to
       checkResources increments mResourceCounter.  If mResourceCounter ever
       gets above a threshold value, the next checkResources call returns false,
       causing the script to throw an uncatchable error.

       @return {v8::Boolean} Returns true if still have adequate resources to
       continue computation.  Returns false otherwise.
     */
    v8::Handle<v8::Value> checkResources();


    Handle<v8::Context> context() { return mContext->mContext;}

    bool isRootContext(JSContextStruct* jscont);

    JSObjectScriptManager* manager() const { return mManager; }

    // is_emerson controls whether this is compiled as emerson or javascript code.
    v8::Handle<v8::Value> internalEval(v8::Persistent<v8::Context>ctx, const String& em_script_str, v8::ScriptOrigin* em_script_name, bool is_emerson);
    v8::Handle<v8::Function> functionValue(const String& em_script_str);

    // Print an exception "to" the script, i.e. using its system.print
    // method. This is useful for callbacks which are executed directly from C++
    // code but which should report errors to the user.
    void printExceptionToScript(JSContextStruct* ctx, const String& exc);

    // A generic interface for invoking callback methods, used by other classes
    // that have JSObjectScript* (e.g. Invokable). Probably needs a version for
    // contexts if the function was bound within a context
    v8::Handle<v8::Value> invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]);
    v8::Handle<v8::Value> invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]);
    v8::Handle<v8::Value> invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Function>& cb);

    JSContextStruct* rootContext() const { return mContext; }


protected:

    // Object host internal identifier for the object associated with
    // this script. We copy this information here because this base
    // class is used for emheadless, which can't get the identifier
    // from the HostedObjectPtr.
    UUID mInternalID;

    /**
       @see checkResources
     */
    int32 mResourceCounter;
    /**
       We want to keep track of whether we're in
       an eval loop or not.  If we are, then mNestedEvalCounter keeps track of
       how many levels of evals we're in.  Ie, if we call eval and from inside
       that eval, call eval again, then we're mNestedEvalCounter should be 2.
       If we are outside of all evals, then mNestedEvalCounter should be zero.
    */
    int32 mNestedEvalCounter;

    /**
       Each time before we call an eval function into v8, we first call
       preEvalOps.  preEvalOps sets up mResourceCounter and mNestedEvalCounter
       before calling eval/run/call.  (It increments mNestedEvalCounter and if
       this eval isn't called from inside another eval, then we set
       mResourceCounter to zero.)
     */
    void preEvalOps();

    /**
       Decrements mNestedEvalCounter.  Used for bookkeeping so that we can keep
       track of what level of eval nesting we're in.
     */
    void postEvalOps();



    // Each context has an id that is assigned from this variable.
    uint32 contIDTracker;
    std::map<uint32,JSContextStruct*> mContStructMap;


    // EvalContext tracks the current state w.r.t. eval-related statements which
    // may change in response to user actions (changing directory) or due to the
    // way the system defines actions (e.g. import searches the current script's
    // directory before trying other import paths).
    struct EvalContext {
        EvalContext();
        EvalContext(const EvalContext& rhs);

        // Current directory the script being evaluated was in,
        // e.g. ../../liboh/plugins/js/scripts/std/movement
        boost::filesystem::path currentScriptDir;
        // Current base import-path directory the script was found in,
        // e.g. for the above it might look like
        // ../../liboh/plugins/js/scripts/.
        // This is used to provide nice relative paths in exceptions.
        boost::filesystem::path currentScriptBaseDir;

        // Gets the full, but relative, path for the script. In the
        // above example this would be std/movement because the
        // currentScriptBaseDir is stripped off to leave just the
        // relative part.
        boost::filesystem::path getFullRelativeScriptDir() const;

        std::ostream* currentOutputStream;
    };
    // This is a helper which adds an EvalContext to the stack and ensures that
    // when it goes out of scope it is removed. This will almost always be the
    // right way to add and remove an EvalContext from the stack, ensure
    // multiple exit paths from a method don't cause the stack to become
    // incorrect.
    struct ScopedEvalContext {
        ScopedEvalContext(JSObjectScript* _parent, const EvalContext& _ctx);
        ~ScopedEvalContext();

        JSObjectScript* parent;
    };
    friend class ScopedEvalContext;

    std::stack<EvalContext> mEvalContextStack;

    //indexed by which context/sandbox you're in.
    typedef     std::map<uint32,std::set<String>  > ImportedFileMap;
    typedef ImportedFileMap::iterator ImportedFileMapIter;
    ImportedFileMap mImportedFiles;


    // add an extenstion to the filename that you get for import/require
    std::string* extensionize(std::string);

    // Resolve a relative path for import to an absolute
    // path. "Returns" the full path of the file as well as the import
    // base path.
    void resolveImport(const String& filename, boost::filesystem::path* full_file_out, boost::filesystem::path* base_path_out, JSContextStruct* jscs);
    // Perform an import on the absolute path filename. This performs no
    // resolution and *always* performs the import, even if the file has already
    // been imported.
    v8::Handle<v8::Value> absoluteImport(const boost::filesystem::path& full_filename, const boost::filesystem::path& full_base_dir,JSContextStruct* jscs);


    JSContextStruct* mContext;


    v8::Handle<v8::Value> protectedEval(const String& em_script_str, v8::ScriptOrigin* em_script_name, const EvalContext& new_ctx, JSContextStruct* jscs);


    v8::Handle<v8::Value> ProtectedJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]);
    v8::Handle<v8::Value> executeJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Function> funcInCtx,int argc, v8::Handle<v8::Object>*target, v8::Handle<v8::Value> argv[]);
    v8::Handle<v8::Value> compileFunctionInContext(v8::Persistent<v8::Context>ctx, v8::Handle<v8::Function>&cb);



    void  printStackFrame(std::stringstream&, v8::Local<v8::StackFrame>);

    typedef std::map<uint32, SuspendableVec> ContIDToSuspMap;
    ContIDToSuspMap toFixup;

    JSObjectScriptManager* mManager;
    OH::Storage* mStorage;
    OH::PersistedObjectSet* mPersistedObjectSet;


    void storageCommitCallback(JSContextStruct* jscont, v8::Persistent<v8::Function> cb, bool success, OH::Storage::ReadSet* rs);

    void setRestoreScriptCallback(JSContextStruct* jscont, v8::Persistent<v8::Function> cb, bool success);
};

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_HPP_
