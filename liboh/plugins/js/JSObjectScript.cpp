/*  Sirikata
 *  JSObjectScript.cpp
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


#include <sirikata/oh/Platform.hpp>

#include <fstream>
#include "JSObjectScript.hpp"
#include "JSLogging.hpp"
#include "JSObjectScriptManager.hpp"

#include "JSSerializer.hpp"
#include <string>
#include "JSUtil.hpp"
#include "JSObjects/JSVec3.hpp"
#include "JSObjects/JSQuaternion.hpp"
#include "JSObjects/JSInvokableObject.hpp"

#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/util/Random.hpp>

#include <sirikata/core/odp/Defs.hpp>
#include <vector>
#include <set>
#include "JSObjects/JSFields.hpp"
#include "emerson/EmersonUtil.h"
#include "emerson/EmersonException.h"
#include "emerson/Util.h"
#include "JSSystemNames.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include "JSObjectStructs/JSTimerStruct.hpp"
#include "JSObjects/JSObjectsUtils.hpp"
#include "JSObjectStructs/JSUtilStruct.hpp"
#include <boost/lexical_cast.hpp>
#include "JSObjectStructs/JSCapabilitiesConsts.hpp"

#include <sirikata/core/util/Paths.hpp>

using namespace v8;
using namespace std;
namespace Sirikata {
namespace JS {

void printException(v8::TryCatch& try_catch, EmersonLineMap* lineMap);

namespace {

String exceptionAsString(v8::TryCatch& try_catch, EmersonLineMap* lineMap) {
    stringstream os;

    v8::HandleScope handle_scope;
    v8::String::Utf8Value exception(try_catch.Exception());
    String exception_string = FromV8String(exception);
    v8::Handle<v8::Message> message = try_catch.Message();

    // Print (filename):(line number): (message).
    if (!message.IsEmpty()) {
        v8::String::Utf8Value filename(message->GetScriptResourceName());
        String filename_string = FromV8String(filename);
        int linenum = message->GetLineNumber();
        if (lineMap != NULL) {
            EmersonLineMap::iterator iter = lineMap->find(linenum);
            if (iter != lineMap->end())
                linenum = iter->second;
        }
        os << filename_string << ':' << linenum << ": " << exception_string << "\n";
        // Print line of source code.
        v8::String::Utf8Value sourceline(message->GetSourceLine());
        String sourceline_string = FromV8String(sourceline);
        os << sourceline_string << "\n";
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn();
        for (int i = 0; i < start; i++)
            os << " ";
        int end = message->GetEndColumn();
        for (int i = start; i < end; i++)
            os << "^";
        os << "\n";
    }
    v8::String::Utf8Value stack_trace(try_catch.StackTrace());
    if (stack_trace.length() > 0) {
      String stack_trace_string = FromV8String(stack_trace);
      os << stack_trace_string << "\n";
    }
    return os.str();
}


/** Note that this return value isn't guaranteed to return anything. If an
 *  exception occurs, it will be left empty. This is due to a quirk in the way
 *  v8 manages exceptions: TryCatch objects only work if there is active
 *  Javascript code on the stack. Therefore, we return the exception object in
 *  exc if the caller has provided a pointer for it.
 */
v8::Handle<v8::Value> ProtectedJSCallbackFull(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> *target, v8::Handle<v8::Function> cb, int argc, v8::Handle<v8::Value> argv[], String* exc = NULL) {
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    Handle<Value> result;
    if (target != NULL && !(*target)->IsNull() && !(*target)->IsUndefined()) {
        result = cb->Call(*target, argc, argv);
    } else {
        result = cb->Call(ctx->Global(), argc, argv);
    }

    if (try_catch.HasCaught())
    {
        printException(try_catch);
        if (exc != NULL)
            *exc = exceptionAsString(try_catch, NULL);

        return v8::Handle<v8::Value>();
    }
    return result;
}




v8::Handle<v8::Value> ProtectedJSCallbackFull(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> *target, v8::Handle<v8::Function> cb, String* exc = NULL) {
    return ProtectedJSCallbackFull(ctx, target, cb, 0, NULL, exc);
}

}

void printException(v8::TryCatch& try_catch, EmersonLineMap* lineMap) {
    String eas = exceptionAsString(try_catch, lineMap);
    JSLOG(error, "Uncaught exception:\n" << eas);
}

void printException(v8::TryCatch& try_catch) {
    printException(try_catch, NULL);
}



JSObjectScript::EvalContext::EvalContext(JSContextStruct* jsctx)
 : currentScriptDir(),
   currentScriptBaseDir(),
   currentOutputStream(&std::cout),
   jscont(jsctx)
{}

JSObjectScript::EvalContext::EvalContext(const EvalContext& rhs)
 : currentScriptDir(rhs.currentScriptDir),
   currentScriptBaseDir(rhs.currentScriptBaseDir),
   currentOutputStream(rhs.currentOutputStream),
   jscont(rhs.jscont)
{}

JSObjectScript::EvalContext::EvalContext(const EvalContext& rhs, JSContextStruct* jsctx)
 : currentScriptDir(rhs.currentScriptDir),
   currentScriptBaseDir(rhs.currentScriptBaseDir),
   currentOutputStream(rhs.currentOutputStream),
   jscont(jsctx)
{}


boost::filesystem::path JSObjectScript::EvalContext::getFullRelativeScriptDir() const {
    using namespace boost::filesystem;

    path result;
    // Scan through all matching parts
    path::const_iterator script_it, base_it;
    for(script_it = currentScriptDir.begin(), base_it = currentScriptBaseDir.begin(); base_it != currentScriptBaseDir.end(); script_it++, base_it++) {
        assert(*script_it == *base_it);
    }
    // Get any leftover script parts
    while(script_it != currentScriptDir.end()) {
        result /= *script_it;
        script_it++;
    }
    return result;
}


JSObjectScript::ScopedEvalContext::ScopedEvalContext(JSObjectScript* _parent, const EvalContext& _ctx)
 : parent(_parent)
{
    parent->mEvalContextStack.push(_ctx);
}

JSObjectScript::ScopedEvalContext::~ScopedEvalContext() {
    assert(!parent->mEvalContextStack.empty());
    parent->mEvalContextStack.pop();
}



void JSObjectScript::initialize(const String& args, const String& script,int32 maxResThresh)
{
    maxResourceThresh =maxResThresh;

    InitializeClassOptions(
        "jsobjectscript", this,
        // Default value allows us to use std libs in the build tree, starting
        // from build/cmake
        NULL
    );

    OptionSet* options = OptionSet::getOptions("jsobjectscript", this);
    options->parse(args);


    v8::HandleScope handle_scope;

    SpaceObjectReference sporef = SpaceObjectReference::null();
    mContext = new JSContextStruct(this, NULL,sporef, Capabilities::getFullCapabilities(),mManager->mContextGlobalTemplate,contIDTracker,NULL);

    // By default, our eval context has:
    // 1. Empty currentScriptDir, indicating it should only use explicitly
    //    specified search paths.
    // 2. mContext as its JSContextStruct
    mEvalContextStack.push(EvalContext(mContext));

    mContStructMap[contIDTracker] = mContext;
    ++contIDTracker;

    v8::Context::Scope context_scope(mContext->mContext);
    if (!script.empty()) {
        JSLOG(detailed,"Have an initial script to execute.  Executing.");

        EvalContext& ctx = mEvalContextStack.top();
        EvalContext new_ctx(ctx,mContext);
        v8::ScriptOrigin origin(v8::String::New("(original_import)"));

        v8::Handle<v8::Value> result = protectedEval(script, &origin, new_ctx, mContext, true);
        if (!result.IsEmpty()) {
            v8::String::Utf8Value exception(result);
            String exception_string = FromV8String(exception);
            JSLOG(error,"Initial script threw an exception: " << exception_string);
        }
        mContext->struct_setScript(script);
    }
}


JSObjectScript::JSObjectScript(JSObjectScriptManager* jMan, OH::Storage* storage, OH::PersistedObjectSet* persisted_set, const UUID& internal_id, Network::IOStrand* objStrand)
 : mStrand(objStrand),
   mInternalID(internal_id),
   mResourceCounter(0),
   mNestedEvalCounter(0),
   contIDTracker(0),
   mManager(jMan),
   mStorage(storage),
   mPersistedObjectSet(persisted_set),
   stopCalled(false)
{
}


void JSObjectScript::start() {
}

void JSObjectScript::stop() {
    // If we're really a subclass, i.e. an EmersonScript, we'll have already
    // called letDie().
    if (Liveness::livenessAlive())
        Liveness::letDie();

    stopCalled = true;
    // This clear has to happen before ~JSObjectScript because it can call
    // virtual functions which need to be dispatched to subclasses
    // (i.e. EmersonScript).
    if (mContext != NULL)
    {
        v8::HandleScope handle_scope;
        mContext->clear();
        delete mContext;
        mContext = NULL;
    }
}

bool JSObjectScript::isStopped()
{
    return stopCalled;
}


void JSObjectScript::shimImportAndEvalScript(JSContextStruct* jscont, const String& toEval)
{
    if (mEvalContextStack.empty())
        mEvalContextStack.push(EvalContext(jscont));
    else
        mEvalContextStack.push(EvalContext(mEvalContextStack.top(),jscont));

    import("std/shim.em",false);
    v8::ScriptOrigin origin(v8::String::New("(reset_script)"));
    internalEval( toEval,&origin, true,true);

    mEvalContextStack.pop();
}


v8::Handle<v8::Value> JSObjectScript::storageBeginTransaction(JSContextStruct* jscont) {
    if (mStorage == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New("No persistent storage available.")) );
    mStorage->beginTransaction(mInternalID);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSObjectScript::storageCommit(JSContextStruct* jscont, v8::Handle<v8::Function> cb)
{
    if (mStorage == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New("No persistent storage available.")) );
    OH::Storage::CommitCallback wrapped_cb = 0;
    if (!cb.IsEmpty()) {
        wrapped_cb = std::tr1::bind(&JSObjectScript::storageCommitCallback, this, jscont, v8::Persistent<v8::Function>::New(cb), _1, _2);
    }
    mStorage->commitTransaction(mInternalID, wrapped_cb);
    return v8::Undefined();
}


void JSObjectScript::storageCommitCallback(JSContextStruct* jscont, v8::Persistent<v8::Function> cb, bool success, OH::Storage::ReadSet* rs) {
    if (isStopped()) {
        JSLOG(warn, "Ignoring storage commit callback after shutdown request.");
        return;
    }

    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext->mContext);
    TryCatch try_catch;

    v8::Handle<v8::Boolean> js_success = v8::Boolean::New(success);
    v8::Handle<v8::Value> js_rs = v8::Undefined();
    if (rs && rs->size() > 0) {
        v8::Handle<v8::Object> js_rs_obj = v8::Object::New();
        for(OH::Storage::ReadSet::const_iterator it = rs->begin(); it != rs->end(); it++)
            js_rs_obj->Set(v8::String::New(it->first.c_str(), it->first.size()), strToUint16Str(it->second));
        js_rs = js_rs_obj;
        // We own the read set.
        delete rs;
    }

    int argc = 2;
    v8::Handle<v8::Value> argv[2] = { js_success, js_rs };
    invokeCallback(jscont, cb, argc, argv);
    postCallbackChecks();
}

void JSObjectScript::storageCountCallback(JSContextStruct* jscont, v8::Persistent<v8::Function> cb, bool success, int32 count) {
    if (isStopped()) {
        JSLOG(warn, "Ignoring storage commit callback after shutdown request.");
        return;
    }

    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext->mContext);
    TryCatch try_catch;

    v8::Handle<v8::Boolean> js_success = v8::Boolean::New(success);
    v8::Handle<v8::Integer> js_count = v8::Integer::New(count);

    int argc = 2;
    v8::Handle<v8::Value> argv[2] = { js_success, js_count };
    invokeCallback(jscont, cb, argc, argv);
    postCallbackChecks();
}


v8::Handle<v8::Value> JSObjectScript::storageErase(const OH::Storage::Key& key, v8::Handle<v8::Function> cb, JSContextStruct* jscont)
{
    if (mStorage == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New("No persistent storage available.")) );

    OH::Storage::CommitCallback wrapped_cb = 0;
    if (!cb.IsEmpty()) {
        wrapped_cb = std::tr1::bind(&JSObjectScript::storageCommitCallback, this, jscont, v8::Persistent<v8::Function>::New(cb), _1, _2);
    }

    bool returner = mStorage->erase(mInternalID, key, wrapped_cb);
    return v8::Boolean::New(returner);
}

v8::Handle<v8::Value> JSObjectScript::storageWrite(const OH::Storage::Key& key, const String& toWrite, v8::Handle<v8::Function> cb, JSContextStruct* jscont)
{
    if (mStorage == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New("No persistent storage available.")) );

    OH::Storage::CommitCallback wrapped_cb = 0;
    if (!cb.IsEmpty()) {
        wrapped_cb = std::tr1::bind(&JSObjectScript::storageCommitCallback, this, jscont, v8::Persistent<v8::Function>::New(cb), _1, _2);
    }

    bool returner = mStorage->write(mInternalID, key, toWrite, wrapped_cb);
    return v8::Boolean::New(returner);
}

v8::Handle<v8::Value> JSObjectScript::storageRead(const OH::Storage::Key& key, v8::Handle<v8::Function> cb, JSContextStruct* jscont)
{
    if (mStorage == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New("No persistent storage available.")) );

    OH::Storage::CommitCallback wrapped_cb = 0;
    if (!cb.IsEmpty()) {
        wrapped_cb = std::tr1::bind(&JSObjectScript::storageCommitCallback, this, jscont, v8::Persistent<v8::Function>::New(cb), _1, _2);
    }

    bool read_queue_success = mStorage->read(mInternalID, key, wrapped_cb);
    return v8::Boolean::New(read_queue_success);
}

v8::Handle<v8::Value> JSObjectScript::storageRangeRead(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb, JSContextStruct* jscont)
{
    if (mStorage == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New("No persistent storage available.")) );

    OH::Storage::CommitCallback wrapped_cb = 0;
    if (!cb.IsEmpty()) {
        wrapped_cb = std::tr1::bind(&JSObjectScript::storageCommitCallback, this, jscont, v8::Persistent<v8::Function>::New(cb), _1, _2);
    }

    bool returner = mStorage->rangeRead(mInternalID, start, finish, wrapped_cb);
    return v8::Boolean::New(returner);
}

v8::Handle<v8::Value> JSObjectScript::storageRangeErase(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb, JSContextStruct* jscont)
{
    if (mStorage == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New("No persistent storage available.")) );

    OH::Storage::CommitCallback wrapped_cb = 0;
    if (!cb.IsEmpty()) {
        wrapped_cb = std::tr1::bind(&JSObjectScript::storageCommitCallback, this, jscont, v8::Persistent<v8::Function>::New(cb), _1, _2);
    }

    bool returner = mStorage->rangeErase(mInternalID, start, finish, wrapped_cb);
    return v8::Boolean::New(returner);
}

v8::Handle<v8::Value> JSObjectScript::storageCount(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb, JSContextStruct* jscont)
{
    if (mStorage == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New("No persistent storage available.")) );

    OH::Storage::CountCallback wrapped_cb = 0;
    if (!cb.IsEmpty()) {
        wrapped_cb = std::tr1::bind(&JSObjectScript::storageCountCallback, this, jscont, v8::Persistent<v8::Function>::New(cb), _1, _2);
    }

    bool returner = mStorage->count(mInternalID, start, finish, wrapped_cb);
    return v8::Boolean::New(returner);
}


void JSObjectScript::setRestoreScriptCallback(JSContextStruct* jscont, v8::Persistent<v8::Function> cb, bool success) {
    if (isStopped()) {
        JSLOG(warn, "Ignoring restore script callback after shutdown request.");
        return;
    }

    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext->mContext);
    TryCatch try_catch;

    v8::Handle<v8::Boolean> js_success = v8::Boolean::New(success);

    int argc = 1;
    v8::Handle<v8::Value> argv[1] = { js_success };
    invokeCallback(jscont, cb, argc, argv);
    postCallbackChecks();
}



//can instantly finish the clear operation in JSObjectScript because not in the
//midst of handling any events that might invalidate iterators.
void JSObjectScript::registerContextForClear(JSContextStruct* jscont)
{
    jscont->finishClear();
}

v8::Handle<v8::Value> JSObjectScript::setRestoreScript(JSContextStruct* jscont, const String& script, v8::Handle<v8::Function> cb) {
    if (mPersistedObjectSet == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New("No persistent storage available.")) );

    OH::PersistedObjectSet::RequestCallback wrapped_cb = 0;
    if (!cb.IsEmpty()) {
        wrapped_cb = std::tr1::bind(&JSObjectScript::setRestoreScriptCallback, this, jscont, v8::Persistent<v8::Function>::New(cb), _1);
    }

    // FIXME we should really tack on any additional parameters we
    // received initially here
    String script_type = "";
    if (!script.empty()) {
        script_type = "js";
    }

    // FIXME script_args
    mPersistedObjectSet->requestPersistedObject(mInternalID, script_type, "", script, wrapped_cb);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSObjectScript::debug_fileRead(const String& filename)
{
    std::ifstream fRead(filename.c_str(), std::ios::binary | std::ios::in);
    std::ifstream::pos_type begin, end;

    begin = fRead.tellg();
    fRead.seekg(0,std::ios::end);
    end   = fRead.tellg();
    fRead.seekg(0,std::ios::beg);

    std::ifstream::pos_type size = end-begin;
    char* readBuf = new char[size];
    fRead.read(readBuf,size);

    String interString(readBuf,size);

    v8::Handle<v8::Value> returner = strToUint16Str(interString);
    delete readBuf;
    return returner;
}


v8::Handle<v8::Value> JSObjectScript::debug_fileWrite(const String& strToWrite,const String& filename)
{
    std::ofstream fWriter (filename.c_str(),  std::ios::out | std::ios::binary);

    for (String::size_type s = 0; s < strToWrite.size(); ++s)
    {
        char toWrite = strToWrite[s];
        fWriter.write(&toWrite,sizeof(toWrite));
    }

    fWriter.flush();
    fWriter.close();

    return v8::Undefined();
}


void JSObjectScript::printExceptionToScript(const String& exc)
{
    if (mEvalContextStack.empty())
    {
        JSLOG(error, "Error when printing exception.  Eval context stack improperly maintained.  Aborting.");
        return;
    }

    JSContextStruct* ctx = mEvalContextStack.top().jscont;
    v8::Handle<v8::Object> sysobj = ctx->struct_getSystem();
    v8::Handle<v8::Value> printval = sysobj->Get(v8::String::New("print"));
    if (printval.IsEmpty() || !printval->IsFunction()) return;
    v8::Handle<v8::Function> printfunc = v8::Handle<v8::Function>::Cast(printval);

    int argc = 1;
    v8::Handle<v8::Value> argv[1] = { v8::String::New(exc.c_str(), exc.size()) };

    preEvalOps();
    ProtectedJSCallbackFull(ctx->mContext, &sysobj, printfunc, argc, argv, NULL);
    postEvalOps();
}


v8::Handle<v8::Value> JSObjectScript::invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]) {
    return invokeCallback(ctx, NULL, cb, argc, argv);
}

v8::Handle<v8::Value> JSObjectScript::invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Function>& cb) {
    return invokeCallback(ctx, NULL, cb, 0, NULL);
}


bool JSObjectScript::isRootContext(JSContextStruct* jscont)
{
    return (jscont == mContext);
}



JSObjectScript::~JSObjectScript()
{
}



namespace {

class EmersonParserException {
public:
    EmersonParserException(uint32 li, uint32 pos, const String& msg)
     : line(li), position(pos), message(msg)
    {}

    uint32 line;
    uint32 position;
    String message;

    String toString() const {
        std::stringstream err_msg;
        err_msg << "SyntaxError at line " << line << " position " << position << ":\n";
        // FIXME it would be nice to give them the same ^ pointer we do for V8 errors.
        err_msg << message;
        return err_msg.str();
    }
};

void handleEmersonRecognitionError(struct ANTLR3_BASE_RECOGNIZER_struct* recognizer, pANTLR3_UINT8* tokenNames) {
    pANTLR3_EXCEPTION exception = recognizer->state->exception;
    throw EmersonParserException(exception->line, exception->charPositionInLine, (const char*)exception->message);
}
}

v8::Handle<v8::Value> JSObjectScript::emersonCompileString(const String& toCompile)
{
    HandleScope handle_scope;
    String em_script_str = toCompile;
    EmersonLineMap lineMap;

    if(em_script_str.size() > 0 &&em_script_str.at(em_script_str.size() -1) != '\n')
        em_script_str.push_back('\n');

    emerson_init();

    try {
        int em_compile_err = 0;

        String js_script_str;
        bool successfullyCompiled = emerson_compile(String("eval statement"), em_script_str.c_str(),
            js_script_str, em_compile_err, handleEmersonRecognitionError,
            &lineMap);

        if (successfullyCompiled)
        {
            JSLOG(insane, " Compiled JS script = \n" <<js_script_str);
            return v8::String::New(js_script_str.c_str());
        }
    }
    catch(EmersonParserException e)
    {
        v8::Handle<v8::Value> err = v8::Exception::SyntaxError(v8::String::New(e.toString().c_str()));
        return v8::ThrowException(err);
    }

    JSLOG(error, "Got a compiler error in internalEval");
    return v8::String::New("");
}


v8::Handle<v8::Context> JSObjectScript::getCurrentV8Context()
{
    if (mEvalContextStack.empty())
    {
        JSLOG(error, "Error: context stack is empty.  Returning context associated with root instead.");
        return mContext->mContext;
    }

    return mEvalContextStack.top().jscont->mContext;
}



v8::Handle<v8::Value> JSObjectScript::internalEval(const String& em_script_str, v8::ScriptOrigin* em_script_name, bool is_emerson, bool return_exc)
{
    v8::HandleScope handle_scope;
    //reads context value from the top of the context stack.
    v8::Context::Scope context_scope(getCurrentV8Context());
    EmersonLineMap lineMap;

    TryCatch try_catch;
    preEvalOps();

    // Special casing emerson compilation
    v8::Handle<v8::String> source;
#ifdef EMERSON_COMPILE
    if (is_emerson)
    {
        String em_script_str_new = em_script_str;

        if(em_script_str.empty())
        {
            postEvalOps();
            if (return_exc)
                return v8::Handle<v8::Value>(); // No exception
            else
                return v8::Undefined();
        }

        if(em_script_str.at(em_script_str.size() -1) != '\n')
        {
            em_script_str_new.push_back('\n');
        }

        emerson_init();

        JSLOG(insane, " Input Emerson script = \n" <<em_script_str_new);

        try {
            int em_compile_err = 0;
            v8::String::Utf8Value parent_script_name(em_script_name->ResourceName());

            String js_script_str;
            bool successfullyCompiled = emerson_compile(FromV8String(parent_script_name), em_script_str_new.c_str(),
                                                        js_script_str, em_compile_err, handleEmersonRecognitionError,
                                                        &lineMap);


            if (successfullyCompiled)
            {
                JSLOG(insane, " Compiled JS script = \n" <<js_script_str);
                source = v8::String::New(js_script_str.c_str());
            }
            else
            {
                source = v8::String::New("");
                JSLOG(error, "Got a compiler error in internalEval");
            }
        }
        catch(EmersonParserException e) {
            postEvalOps();
            v8::Handle<v8::Value> err = v8::Exception::SyntaxError(v8::String::New(e.toString().c_str()));
            if (return_exc)
                return handle_scope.Close(err);
            else
                return v8::ThrowException(err);
        }
    }
    else
#endif
    {
        //assume the input string to be a valid js rather than emerson
        source = v8::String::New(em_script_str.c_str(), em_script_str.size());
    }
    // Compile
    //note, because using compile command, will run in the mContext context
    v8::Handle<v8::Script> script = v8::Script::Compile(source, em_script_name);
    if (try_catch.HasCaught()) {
        v8::String::Utf8Value error(try_catch.Exception());
        String uncaught( *error);
        uncaught = "Uncaught exception " + uncaught + "\nwhen trying to run script: "+ em_script_str;
        JSLOG(error, uncaught);
        printException(try_catch);
        postEvalOps();
        if (return_exc)
            return handle_scope.Close(try_catch.Exception());
        else
            return try_catch.ReThrow();
    }


    if (script.IsEmpty()) {
        v8::String::Utf8Value error(try_catch.Exception());
        std::string msg = std::string("Compile error: ") + std::string(*error);
        JSLOG(error, msg);
        postEvalOps();
        v8::Handle<v8::Value> err = v8::Exception::Error(v8::String::New(msg.c_str()));
        if (return_exc)
            return handle_scope.Close(err);
        else
            return v8::ThrowException(err);
    }

    // Execute
    v8::Handle<v8::Value> result = script->Run();
    if (try_catch.HasCaught()) {
        printException(try_catch, &lineMap);
        postEvalOps();
        if (return_exc)
            return handle_scope.Close(try_catch.Exception());
        else
            return try_catch.ReThrow();
    }


    if (!result.IsEmpty() && !result->IsUndefined()) {
        v8::String::Utf8Value text_val(result);
        JSLOG(detailed, "Script result: " << FromV8String(text_val));
    }

    postEvalOps();
    if (return_exc)
        return v8::Handle<v8::Value>();
    else
        return result;
}




v8::Handle<v8::Value> JSObjectScript::protectedEval(const String& em_script_str, v8::ScriptOrigin* em_script_name, const EvalContext& new_ctx, bool return_exc, bool isJS)
{
    ScopedEvalContext sec(this, new_ctx);
    return internalEval(em_script_str, em_script_name, !isJS, return_exc);
}



/*
  This function takes the string associated with cb, re-compiles it in the
  passed-in context, and then passes it back out.  (Note, enclose the function
  string in parentheses and semi-colon to get around v8 idiosyncracy associated with
  compiling anonymous functions.  Ie, if didn't add those in, it may not compile
  anonymous functions.)
 */
v8::Handle<v8::Value> JSObjectScript::compileFunctionInContext(v8::Handle<v8::Function>&cb)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(getCurrentV8Context());

    TryCatch try_catch;

    String errorMessage= "Cannot interpret callback function as string while executing in context.  ";
    String v8Source;
    bool stringDecodeSuccessful = decodeString(cb->ToString(),v8Source,errorMessage);
    if (! stringDecodeSuccessful)
    {
        JSLOG(error, errorMessage);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );
    }

    v8Source = "(" + v8Source;
    v8Source += ");";

    ScriptOrigin cb_origin = cb->GetScriptOrigin();
    v8::Handle<v8::Value> compileFuncResult = internalEval(v8Source, &cb_origin, false);


    if (! compileFuncResult->IsFunction())
    {
        String errorMessage = "Uncaught exception: function passed in did not compile to a function";
        JSLOG(error, errorMessage);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );
    }

    //check if any errors occurred during compile.
    if (try_catch.HasCaught()) {
        JSLOG(error,"Error in compileFunctionInContext");
        printException(try_catch);
        return try_catch.Exception();
    }


    JSLOG(insane, "Successfully compiled function in context.  Passing back function object.");
    return compileFuncResult;
}


//returns true if haven't surpassed resource threshold
bool JSObjectScript::checkResourcesCPP()
{
    ++mResourceCounter;
    return (mResourceCounter < maxResourceThresh);
}

v8::Handle<v8::Value> JSObjectScript::checkResources()
{
    return v8::Boolean::New(checkResourcesCPP());
}


void JSObjectScript::preEvalOps()
{
    if (mNestedEvalCounter < 0)
        JSLOG(error, "Error in preEvalOps.  Should never indicate that we have executed negative evals");

    if (mNestedEvalCounter == 0)
        mResourceCounter =0;

    ++mNestedEvalCounter;
}

void JSObjectScript::postEvalOps()
{
    --mNestedEvalCounter;
}


v8::Handle<v8::Value> JSObjectScript::invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[])
{
    if (mEvalContextStack.empty())
        mEvalContextStack.push(EvalContext(ctx));
    else
        mEvalContextStack.push(EvalContext(mEvalContextStack.top(),ctx));

    ScopedEvalContext sec (this,EvalContext(ctx));
    String exc;
    preEvalOps();
    v8::Handle<v8::Value> retval = ProtectedJSCallbackFull(ctx->mContext, target, cb, argc, argv, &exc);
    postEvalOps();
    if (retval.IsEmpty())
    {
        preEvalOps();
        printExceptionToScript(exc);
        postEvalOps();
    }

    mEvalContextStack.pop();
    return retval;
}


bool JSObjectScript::checkCurCtxtHasCapability(JSPresenceStruct* jspres, Capabilities::Caps whatCap)
{
    if (mEvalContextStack.empty())
        return false;


    //only way that this context won't have capability is if jspres is context's
    //associated presence.  If it is, check that context has capability.
    JSContextStruct* jscont = mEvalContextStack.top().jscont;
    if (Capabilities::givesCap(jscont->getCapNum(), whatCap,
            jscont->getAssociatedPresenceStruct(), jspres))
    {
        return true;
    }

    return false;
}

/*
  executeInSandbox takes in a context, that you want to execute the function
  funcToCall in.  argv are the arguments to funcToCall from the current context,
  and are counted by argc.
 */
v8::Handle<v8::Value>JSObjectScript::executeInSandbox(JSContextStruct* jscont, v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv)
{
    ScopedEvalContext scopedContext(this,EvalContext(jscont));

    JSLOG(insane, "executing script in alternate context");
    v8::Handle<v8::Value> compiledFunc = compileFunctionInContext(funcToCall);
    if (! compiledFunc->IsFunction())
        V8_EXCEPTION_CSTR("Error when calling execute from sandbox.  Could not re-compile function object.");


    v8::Handle<v8::Function> funcInCtx = v8::Handle<v8::Function>::Cast(compiledFunc);
    preEvalOps();
    ProtectedJSCallbackFull(jscont->mContext, NULL,funcInCtx, 0, NULL);
    postEvalOps();
    return v8::Undefined();
}




void JSObjectScript::print(const String& str) {
    assert(!mEvalContextStack.empty());
    std::ostream* os = mEvalContextStack.top().currentOutputStream;
    assert(os != NULL);
    (*os) << str;
}





//takes in a name of a file to read from and execute all instructions within.
//also takes in a context to do so in.  If this context is null, just use
//mContext instead.
void JSObjectScript::resolveImport(const String& filename, boost::filesystem::path* full_file_out, boost::filesystem::path* base_path_out)
{
    using namespace boost::filesystem;

    // Search through the import paths to find the file to import, searching the
    // current directory first if it is non-empty.
    path filename_as_path(filename);
    assert(!mEvalContextStack.empty());
    EvalContext& ctx = mEvalContextStack.top();
    if (!ctx.currentScriptDir.empty()) {
        path fq = ctx.currentScriptDir / filename_as_path;
        try {
            if (boost::filesystem::exists(fq)) {
                *full_file_out = fq;
                *base_path_out = ctx.currentScriptBaseDir;
                return;
            }
        } catch (boost::filesystem::filesystem_error) {
            // Ignore, this just means we don't have access to some directory so
            // we can't check for its existence.
        }
    }

    std::list<String> default_search_paths = mManager->getOptions()->referenceOption("default-import-paths")->as< std::list<String> >();
    std::list<String> additional_search_paths = mManager->getOptions()->referenceOption("import-paths")->as< std::list<String> >();
    std::list<String> search_paths;
    search_paths.splice(search_paths.end(), default_search_paths);
    search_paths.splice(search_paths.end(), additional_search_paths);
    // Replace special tags with their values
    for(std::list<String>::iterator search_it = search_paths.begin(); search_it != search_paths.end(); search_it++)
        *search_it = Path::SubstitutePlaceholders(*search_it);
    for (std::list<String>::iterator pit = search_paths.begin(); pit != search_paths.end(); pit++) {
        path base_path(*pit);
        path fq = base_path / filename_as_path;
        try {
            if (boost::filesystem::exists(fq)) {
                *full_file_out = fq;
                *base_path_out = base_path;
                return;
            }
        } catch (boost::filesystem::filesystem_error) {
            // Ignore, this just means we don't have access to some directory so
            // we can't check for its existence.
        }
    }

    *full_file_out = path();
    *base_path_out = path();

    return;
}


v8::Handle<v8::Value> JSObjectScript::absoluteImport(const boost::filesystem::path& full_filename, const boost::filesystem::path& full_base_dir,bool isJS)
{
    if (mEvalContextStack.empty())
    {
        JSLOG(error, "Error in absolute import.  Not within a context to import from.  Aborting call");
        return v8::Undefined();
    }

    JSContextStruct* jscont = mEvalContextStack.top().jscont;

    //to prevent infinite cycles
    if (!checkResourcesCPP())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Detected a potential infinite loop in imports.  Aborting.")));


    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(jscont->mContext);

    JSLOG(detailed, " Performing import on absolute path: " << full_filename.string());

    // Now try to read in and run the file.
    FILE * pFile;
    long lSize;
    char * buffer;
    long result;

    pFile = fopen (full_filename.string().c_str(), "rb" );
    if (pFile == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Couldn't open file for import.")) );

    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);

    std::string contents(lSize, '\0');

    result = fread (&(contents[0]), 1, lSize, pFile);
    fclose (pFile);

    if (result != lSize)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Failure reading file for import.")) );

    EvalContext& ctx = mEvalContextStack.top();
    EvalContext new_ctx(ctx);

    new_ctx.currentScriptDir = full_filename.parent_path();
    new_ctx.currentScriptBaseDir = full_base_dir;
    ScriptOrigin origin( v8::String::New( (new_ctx.getFullRelativeScriptDir() / full_filename.filename()).string().c_str() ) );

    mImportedFiles[jscont->getContextID()].insert( full_filename.string() );

    v8::Handle<v8::Value> returner = protectedEval(contents, &origin, new_ctx,false,isJS);
    return  handle_scope.Close(returner);
}

std::string* JSObjectScript::extensionize(const String filename)
{
    std::string* fileToFind = new std::string(filename);
    int index = filename.find(".");
    if(index == -1)
    {
      *fileToFind = *fileToFind + ".em";
    }
    else
    {
      if(filename.substr(index) != ".em")
      {
        // found a file with different extenstion
        return NULL;
      }
    }
    return fileToFind;
}

v8::Handle<v8::Value> JSObjectScript::evalInGlobal(const String& contents, v8::ScriptOrigin* origin,JSContextStruct* jscs)
{
    EvalContext& ctx = mEvalContextStack.top();
    EvalContext new_ctx(ctx);

    if (jscs == NULL)
        return protectedEval(contents, origin, new_ctx, NULL);

    return protectedEval(contents, origin, new_ctx,jscs);
}


v8::Handle<v8::Value> JSObjectScript::import(const String& filename,  bool isJS)
{
    JSLOG(detailed, "Importing: " << filename);
    v8::HandleScope handle_scope;

    std::string* fileToFind= NULL;

    if (! isJS)
        fileToFind =  extensionize(filename);
    else
        fileToFind = new String(filename);

    if(fileToFind == NULL)
    {
      std::string errMsg = "Cannot import " + filename + ". Illegal file extension.";
      return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str()) ) );;
    }
    boost::filesystem::path full_filename, full_base;
    resolveImport(*fileToFind, &full_filename, &full_base);
    delete fileToFind;
    // If we still haven't filled this in, we just can't find the file.
    if (full_filename.empty())
    {
        std::string errorMessage("Couldn't find file for import named ");
        errorMessage+=filename;
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())) );
    }

    return handle_scope.Close(absoluteImport(full_filename, full_base,isJS));
}


v8::Handle<v8::Value> JSObjectScript::require(const String& filename,bool isJS)
{

    if (mEvalContextStack.empty())
    {
        JSLOG(error, "Error in require.  Not within a context to require from.  Aborting call");
        return v8::Undefined();
    }

    JSContextStruct* jscont = mEvalContextStack.top().jscont;

    JSLOG(detailed, "Requiring: " << filename);
    HandleScope handle_scope;
    std::string* fileToFind= NULL;
    if (! isJS)
        fileToFind =  extensionize(filename);
    else
        fileToFind = new String(filename);

    if(fileToFind == NULL)
    {
      std::string errMsg = "Cannot import " + filename + ". Illegal file extension.";
      return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str()) ) );;
    }

    boost::filesystem::path full_filename, full_base;
    resolveImport(*fileToFind, &full_filename, &full_base);
    delete fileToFind;
    // If we still haven't filled this in, we just can't find the file.
    if (full_filename.empty())
    {
        std::string errorMessage("Couldn't find file for require named ");
        errorMessage += filename;
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())) );
    }

    ImportedFileMapIter iter = mImportedFiles.find(jscont->getContextID());
    if (iter != mImportedFiles.end())
    {
        if (iter->second.find(full_filename.string()) != iter->second.end())
        {
        JSLOG(detailed, " Skipping already imported file: " << filename);
        return v8::Undefined();
        }
    }

    return handle_scope.Close(absoluteImport(full_filename, full_base,isJS));
}

v8::Local<v8::Object> JSObjectScript::createContext(JSPresenceStruct* jspres,const SpaceObjectReference& canSendTo,uint32 capNum, JSContextStruct*& internalContextField, JSContextStruct* creator)
{
    v8::HandleScope handle_scope;

    v8::Local<v8::Object> returner =mManager->mContextTemplate->NewInstance();
    internalContextField = new JSContextStruct(this,jspres,canSendTo,capNum,mManager->mContextGlobalTemplate,contIDTracker,creator);
    mContStructMap[contIDTracker] = internalContextField;
    ++contIDTracker;

    returner->SetInternalField(CONTEXT_FIELD_CONTEXT_STRUCT, External::New(internalContextField));
    returner->SetInternalField(TYPEID_FIELD,External::New(new String(CONTEXT_TYPEID_STRING)));

    return handle_scope.Close(returner);
}


//lkjs
//Careful during serialization.  Need to add to context stack before deserializing.
v8::Local<v8::Function> JSObjectScript::functionValue(const String& js_script_str)
{
  v8::HandleScope handle_scope;

  static int32 counter;
  std::stringstream sstream;
  sstream <<  " __emerson_deserialized_function_" << counter << "__ = " << js_script_str << ";";


  const std::string new_code = sstream.str();
  counter++;

  v8::ScriptOrigin origin(v8::String::New("(deserialized)"));
  v8::Local<v8::Value> v = v8::Local<v8::Value>::New(internalEval(new_code, &origin, false));
  if (!v->IsFunction())
      v = v8::Local<v8::Value>::New(internalEval("function(){}", &origin, false));

  v8::Local<v8::Function> f = v8::Local<v8::Function>::Cast(v);
  return handle_scope.Close(f);
}





} // namespace JS
} // namespace Sirikata
