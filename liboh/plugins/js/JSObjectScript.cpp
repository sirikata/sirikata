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

#include <sirikata/core/util/KnownServices.hpp>

#include <fstream>
#include "JSObjectScript.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSLogging.hpp"

#include "JSSerializer.hpp"
#include "JSObjectStructs/JSEventHandlerStruct.hpp"
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
#include "JS_JSMessage.pbj.hpp"
#include "emerson/EmersonUtil.h"
#include "emerson/EmersonException.h"
#include "lexWhenPred/LexWhenPredUtil.h"
#include "emerson/Util.h"
#include "JSSystemNames.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include "JSObjectStructs/JSTimerStruct.hpp"
#include "JSObjects/JSObjectsUtils.hpp"
#include "JSObjectStructs/JSUtilStruct.hpp"
#include <boost/lexical_cast.hpp>
#include "JSVisibleStructMonitor.hpp"



using namespace v8;
using namespace std;
namespace Sirikata {
namespace JS {




namespace {

String exceptionAsString(v8::TryCatch& try_catch) {
    stringstream os;

    v8::HandleScope handle_scope;
    v8::String::Utf8Value exception(try_catch.Exception());
    const char* exception_string = ToCString(exception);
    v8::Handle<v8::Message> message = try_catch.Message();

    // Print (filename):(line number): (message).
    if (!message.IsEmpty()) {
        v8::String::Utf8Value filename(message->GetScriptResourceName());
        const char* filename_string = ToCString(filename);
        int linenum = message->GetLineNumber();
        os << filename_string << ':' << linenum << ": " << exception_string << "\n";
        // Print line of source code.
        v8::String::Utf8Value sourceline(message->GetSourceLine());
        const char* sourceline_string = ToCString(sourceline);
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
      const char* stack_trace_string = ToCString(stack_trace);
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
        JSLOG(insane,"ProtectedJSCallback with target given.");
        result = cb->Call(*target, argc, argv);
    } else {
        JSLOG(insane,"ProtectedJSCallback without target given.");
        result = cb->Call(ctx->Global(), argc, argv);
    }

    if (try_catch.HasCaught())
    {
        
        printException(try_catch);
        if (exc != NULL)
            *exc = exceptionAsString(try_catch);


        return v8::Handle<v8::Value>();
    }


    return result;
}

v8::Handle<v8::Value> ProtectedJSCallbackFull(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> *target, v8::Handle<v8::Function> cb, String* exc = NULL) {
    return ProtectedJSCallbackFull(ctx, target, cb, 0, NULL, exc);
}

}

void printException(v8::TryCatch& try_catch) {
    String eas = exceptionAsString(try_catch);
    JSLOG(error, "Uncaught exception:\n" << eas);
}



JSObjectScript::EvalContext::EvalContext()
 : currentScriptDir(),
   currentScriptBaseDir(),
   currentOutputStream(&std::cout)
{}

JSObjectScript::EvalContext::EvalContext(const EvalContext& rhs)
 : currentScriptDir(rhs.currentScriptDir),
   currentScriptBaseDir(rhs.currentScriptBaseDir),
   currentOutputStream(rhs.currentOutputStream)
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



void JSObjectScript::initialize(const String& args)
{
    OptionValue* init_script;
    InitializeClassOptions(
        "jsobjectscript", this,
        // Default value allows us to use std libs in the build tree, starting
        // from build/cmake
        init_script = new OptionValue("init-script","",OptionValueType<String>(),"Script to run on startup."),
        NULL
    );

    OptionSet* options = OptionSet::getOptions("jsobjectscript", this);
    options->parse(args);

    // By default, our eval context has:
    // 1. Empty currentScriptDir, indicating it should only use explicitly
    //    specified search paths.
    mEvalContextStack.push(EvalContext());

    v8::HandleScope handle_scope;

    SpaceObjectReference sporef = SpaceObjectReference::null();
    mContext = new JSContextStruct(this,NULL,&sporef,true,true,true,true,true,true,true,mManager->mContextGlobalTemplate,contIDTracker);
    mContStructMap[contIDTracker] = mContext;
    ++contIDTracker;


    String script_contents = init_script->as<String>();
    if (script_contents.empty()) {
        JSLOG(detailed,"Importing default script.");
        import(mManager->defaultScript(),NULL);
        mContext->struct_setScript("system.require('" + mManager->defaultScript() + "');");
    }
    else {
        JSLOG(detailed,"Have an initial script to execute.  Executing.");
        EvalContext& ctx = mEvalContextStack.top();
        EvalContext new_ctx(ctx);
        v8::ScriptOrigin origin(v8::String::New("(original_import)"));
        protectedEval(script_contents, &origin, new_ctx,mContext);
        mContext->struct_setScript(script_contents);
    }
}


JSObjectScript::JSObjectScript(JSObjectScriptManager* jMan)
 : mResourceCounter(0),
   mNestedEvalCounter(0),
   contIDTracker(0),
   mManager(jMan)
{
}


v8::Handle<v8::Value> JSObjectScript::backendFlush(const String& seqKey, JSContextStruct* jscont)
{
    bool returner = mBackend.flush(seqKey);
    return v8::Boolean::New(returner);
}

v8::Handle<v8::Value> JSObjectScript::backendClearItem(const String& prepend, const String& itemName,JSContextStruct* jscont)
{
    bool returner = mBackend.clearItem(prepend,itemName);
    return v8::Boolean::New(returner);
}

v8::Handle<v8::Value> JSObjectScript::backendWrite(const String& seqKey, const String& id, const String& toWrite, JSContextStruct* jscont)
{
    bool returner = mBackend.write(seqKey,id,toWrite);
    return v8::Boolean::New(returner);
}


v8::Handle<v8::Value> JSObjectScript::backendClearEntry(const String& prepend, JSContextStruct* jscont)
{
    bool returner = mBackend.clearEntry(prepend);
    return v8::Boolean::New(returner);
}


v8::Handle<v8::Value> JSObjectScript::backendCreateEntry(const String& prepend, JSContextStruct* jscont)
{
    JSBackendInterface::JSBackendCreateCode jsbackCode = mBackend.createEntry(prepend);
    return v8::Number::New((int) jsbackCode);
}

v8::Handle<v8::Value> JSObjectScript::backendRead(const String& prepend, const String& id, JSContextStruct* jscont)
{
    String toReadTo;
    bool readSucceed = mBackend.read(prepend,id,toReadTo);

    if (!readSucceed)
        return v8::Null();

    return strToUint16Str(toReadTo);
}

v8::Handle<v8::Value> JSObjectScript::backendHaveEntry(const String& prepend, JSContextStruct* jscont)
{
    bool returner = mBackend.haveEntry(prepend);
    return v8::Boolean::New(returner);
}

v8::Handle<v8::Value> JSObjectScript::backendHaveUnflushedEvents(const String& prepend, JSContextStruct* jscont)
{
    bool returner = mBackend.haveUnflushedEvents(prepend);
    return v8::Boolean::New(returner);
}

v8::Handle<v8::Value> JSObjectScript::backendClearOutstanding(const String& prependToken, JSContextStruct* jscont)
{
    bool returner = mBackend.clearOutstanding(prependToken);
    return v8::Boolean::New(returner);
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



void JSObjectScript::printExceptionToScript(JSContextStruct* ctx, const String& exc) {
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

v8::Handle<v8::Value> JSObjectScript::invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]) {
    String exc;
    preEvalOps();
    v8::Handle<v8::Value> retval = ProtectedJSCallbackFull(ctx->mContext, target, cb, argc, argv, &exc);
    postEvalOps();
    if (retval.IsEmpty())
    {
        preEvalOps();
        printExceptionToScript(ctx, exc);
        postEvalOps();
    }
    return retval;
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

//Will compile and run code in the context ctx whose source is em_script_str.
v8::Handle<v8::Value>JSObjectScript::internalEval(v8::Persistent<v8::Context>ctx, const String& em_script_str, v8::ScriptOrigin* em_script_name, bool is_emerson)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    preEvalOps();
    
    // Special casing emerson compilation
    v8::Handle<v8::String> source;
#ifdef EMERSON_COMPILE
    if (is_emerson) {
        String em_script_str_new = em_script_str;

        if(em_script_str.empty())
        {
            postEvalOps();
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

            bool successfullyCompiled = emerson_compile(String(ToCString(parent_script_name)), em_script_str_new.c_str(), js_script_str,em_compile_err, handleEmersonRecognitionError);
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
            return v8::ThrowException( v8::Exception::SyntaxError(v8::String::New(e.toString().c_str())) );
        }
    }
    else
#endif
        //assume the input string to be a valid js rather than emerson
        source = v8::String::New(em_script_str.c_str(), em_script_str.size());

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
        return try_catch.ReThrow();
    }


    if (script.IsEmpty()) {
        v8::String::Utf8Value error(try_catch.Exception());
        std::string msg = std::string("Compile error: ") + std::string(*error);
        JSLOG(error, msg);
        postEvalOps();
        return v8::ThrowException( v8::Exception::Error(v8::String::New(msg.c_str())) );
    }


    TryCatch try_catch2;
    // Execute
    v8::Handle<v8::Value> result = script->Run();

    if (try_catch2.HasCaught()) {
        printException(try_catch2);
        postEvalOps();
        return try_catch2.ReThrow();
    }


    if (!result.IsEmpty() && !result->IsUndefined()) {
        v8::String::AsciiValue ascii(result);
        JSLOG(detailed, "Script result: " << *ascii);
    }

    postEvalOps();
    return result;
}




v8::Handle<v8::Value> JSObjectScript::protectedEval(const String& em_script_str, v8::ScriptOrigin* em_script_name, const EvalContext& new_ctx, JSContextStruct* jscs)
{
    ScopedEvalContext sec(this, new_ctx);
    if (jscs == NULL)
        return internalEval(mContext->mContext, em_script_str, em_script_name, true);

    return internalEval(jscs->mContext, em_script_str, em_script_name, true);
}


/*
  This function takes the string associated with cb, re-compiles it in the
  passed-in context, and then passes it back out.  (Note, enclose the function
  string in parentheses and semi-colon to get around v8 idiosyncracy associated with
  compiling anonymous functions.  Ie, if didn't add those in, it may not compile
  anonymous functions.)
 */
v8::Handle<v8::Value> JSObjectScript::compileFunctionInContext(v8::Persistent<v8::Context>ctx, v8::Handle<v8::Function>&cb)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

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
    v8::Handle<v8::Value> compileFuncResult = internalEval(ctx, v8Source, &cb_origin, false);

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

v8::Handle<v8::Value> JSObjectScript::checkResources()
{
    ++mResourceCounter;
    
    if (mResourceCounter > EMERSON_RESOURCE_THRESHOLD)
        return v8::Boolean::New(false);

    
    return v8::Boolean::New(true);
}


/*
  This function grabs the string associated with cb, and recompiles it in the
  context ctx.  It then calls the newly recompiled function from within ctx with
  args specified by argv and argc.
 */
v8::Handle<v8::Value> JSObjectScript::ProtectedJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[])
{

    v8::Handle<v8::Value> compiledFunc = compileFunctionInContext(ctx,cb);
    if (! compiledFunc->IsFunction())
    {
        JSLOG(error, "Callback did not compile to a function in ProtectedJSFunctionInContext.  Aborting execution.");
        //will be error message, or whatever else it compiled to.
        return compiledFunc;
    }

    v8::Handle<v8::Function> funcInCtx = v8::Handle<v8::Function>::Cast(compiledFunc);
    return executeJSFunctionInContext(ctx,funcInCtx,argc,target,argv);
}


/*
  This function takes the result of compileFunctionInContext, and executes it
  within context ctx.
 */
v8::Handle<v8::Value> JSObjectScript::executeJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Function> funcInCtx,int argc, v8::Handle<v8::Object>*target, v8::Handle<v8::Value> argv[])
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;
    preEvalOps();
    
    v8::Handle<v8::Value> result;
    bool targetGiven = false;
    if (target!=NULL)
    {
        if (((*target)->IsNull() || (*target)->IsUndefined()))
        {
            JSLOG(insane,"executeJSFunctionInContext with target given.");
            result = funcInCtx->Call(*target, argc, argv);
            targetGiven = true;
        }
    }

    if (!targetGiven)
    {
        JSLOG(insane,"executeJSFunctionInContext without target given.");
        result = funcInCtx->Call(ctx->Global(), argc, argv);
    }

    //check if any errors have occurred.
    if (try_catch.HasCaught()) {
        JSLOG(error,"Error in executeJSFunctionInContext");
        printException(try_catch);
        postEvalOps();
        return try_catch.Exception();
    }

    postEvalOps();
    return result;
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


/*
  executeInSandbox takes in a context, that you want to execute the function
  funcToCall in.  argv are the arguments to funcToCall from the current context,
  and are counted by argc.
 */
v8::Handle<v8::Value>JSObjectScript::executeInSandbox(v8::Persistent<v8::Context> &contExecIn, v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv)
{
    JSLOG(insane, "executing script in alternate context");

    //entering new context associated with
    JSLOG(insane, "entering new context associated with JSContextStruct.");
    v8::Context::Scope context_scope(contExecIn);

    JSLOG(insane, "Evaluating function in context associated with JSContextStruct.");
    ProtectedJSFunctionInContext(contExecIn, NULL,funcToCall, argc, argv);

    JSLOG(insane, "execution in alternate context complete");
    return v8::Undefined();
}




void JSObjectScript::print(const String& str) {
    assert(!mEvalContextStack.empty());
    std::ostream* os = mEvalContextStack.top().currentOutputStream;
    assert(os != NULL);
    (*os) << str;
}






v8::Handle<v8::Value> JSObjectScript::eval(const String& contents, v8::ScriptOrigin* origin,JSContextStruct* jscs)
{
    EvalContext& ctx = mEvalContextStack.top();
    EvalContext new_ctx(ctx);

    if (jscs == NULL)
        return protectedEval(contents, origin, new_ctx, NULL);

    return protectedEval(contents, origin, new_ctx,jscs);
}


//takes in a name of a file to read from and execute all instructions within.
//also takes in a context to do so in.  If this context is null, just use
//mContext instead.
void JSObjectScript::resolveImport(const String& filename, boost::filesystem::path* full_file_out, boost::filesystem::path* base_path_out,JSContextStruct* contextCtx)
{
    v8::HandleScope handle_scope;

    if (contextCtx == NULL)
        contextCtx = mContext;

    v8::Context::Scope(contextCtx->mContext);

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

    std::list<String> search_paths = mManager->getOptions()->referenceOption("import-paths")->as< std::list<String> >();
    // Always search the current directory as a last resort
    search_paths.push_back(".");
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

v8::Handle<v8::Value> JSObjectScript::absoluteImport(const boost::filesystem::path& full_filename, const boost::filesystem::path& full_base_dir,JSContextStruct* jscont)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(jscont ? jscont->mContext : mContext->mContext);

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

    return  protectedEval(contents, &origin, new_ctx,jscont);
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

v8::Handle<v8::Value> JSObjectScript::import(const String& filename, JSContextStruct* jscont)
{

    JSLOG(detailed, "Importing: " << filename);
    std::string* fileToFind = extensionize(filename);
    if(fileToFind == NULL)
    {
      std::string errMsg = "Cannot import " + filename + ". Illegal file extension.";
      return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str()) ) );;
    }
    boost::filesystem::path full_filename, full_base;
    resolveImport(*fileToFind, &full_filename, &full_base,jscont);
    delete fileToFind;
    // If we still haven't filled this in, we just can't find the file.
    if (full_filename.empty())
    {
        std::string errorMessage("Couldn't find file for import named");
        errorMessage+=filename;
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())) );
    }
    return absoluteImport(full_filename, full_base,jscont);
}


v8::Handle<v8::Value> JSObjectScript::require(const String& filename,JSContextStruct* jscont)
{
    JSLOG(detailed, "Requiring: " << filename);
    std::string* fileToFind = extensionize(filename);
    if(fileToFind == NULL)
    {
      std::string errMsg = "Cannot import " + filename + ". Illegal file extension.";
      return v8::ThrowException( v8::Exception::Error(v8::String::New(errMsg.c_str()) ) );;
    }

    boost::filesystem::path full_filename, full_base;
    resolveImport(*fileToFind, &full_filename, &full_base,jscont);
    delete fileToFind;
    // If we still haven't filled this in, we just can't find the file.
    if (full_filename.empty())
    {
        std::string errorMessage("Couldn't find file for require named");
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

    return absoluteImport(full_filename, full_base,jscont);
}



//presAssociatedWith: who the messages that this context's system sends will
//be from
//canMessage: who you can always send messages to.
//sendEveryone creates system that can send messages to everyone besides just
//who created you.
//recvEveryone means that you can receive messages from everyone besides just
//who created you.
//proxQueries means that you can issue proximity queries yourself, and latch on
//callbacks for them.
//canImport means that you can import files/libraries into your code.
//canCreatePres is whether have capability to create presences
//canCreateEnt is whether have capability to create entities
//last field returns the created context struct by reference
v8::Local<v8::Object> JSObjectScript::createContext(JSPresenceStruct* presAssociatedWith,SpaceObjectReference* canMessage,bool sendEveryone, bool recvEveryone, bool proxQueries, bool canImport, bool canCreatePres,bool canCreateEnt,bool canEval,JSContextStruct*& internalContextField)
{
    v8::HandleScope handle_scope;

    v8::Local<v8::Object> returner =mManager->mContextTemplate->NewInstance();
    internalContextField = new JSContextStruct(this,presAssociatedWith,canMessage,sendEveryone,recvEveryone,proxQueries, canImport,canCreatePres,canCreateEnt,canEval,mManager->mContextGlobalTemplate, contIDTracker);
    mContStructMap[contIDTracker] = internalContextField;
    ++contIDTracker;


    returner->SetInternalField(CONTEXT_FIELD_CONTEXT_STRUCT, External::New(internalContextField));
    returner->SetInternalField(TYPEID_FIELD,External::New(new String(CONTEXT_TYPEID_STRING)));

    return returner;
}



v8::Handle<v8::Function> JSObjectScript::functionValue(const String& js_script_str)
{
  v8::HandleScope handle_scope;

  static int32_t counter;
  std::stringstream sstream;
  sstream <<  " __emerson_deserialized_function_" << counter << "__ = " << js_script_str << ";";

  //const std::string new_code = std::string("(function () { return ") + js_script_str + "}());";
  // The function name is not required. It is being put in because emerson is not compiling "( function() {} )"; correctly
  const std::string new_code = sstream.str();
  counter++;

  v8::ScriptOrigin origin(v8::String::New("(deserialized)"));
  v8::Local<v8::Value> v = v8::Local<v8::Value>::New(internalEval(mContext->mContext, new_code, &origin, false));
  v8::Local<v8::Function> f = v8::Local<v8::Function>::Cast(v);
  v8::Persistent<v8::Function> pf = v8::Persistent<v8::Function>::New(f);
  return pf;
}





} // namespace JS
} // namespace Sirikata
