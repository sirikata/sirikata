#include "JSCtx.hpp"


namespace Sirikata
{
namespace JS
{


JSCtx::JSCtx(
    Context* ctx,Network::IOStrandPtr oStrand,
    Network::IOStrandPtr vmStrand,v8::Isolate* is)
 : objStrand(oStrand),
   visManStrand(vmStrand),
   mainStrand(ctx->mainStrand),
   mIsolate(is),
   internalContext(ctx),
   isStopped(false),
   isInitialized(false),
   mCheck()
{
}

JSCtx::~JSCtx()
{
    mVisibleTemplate.Dispose();
    mPresenceTemplate.Dispose();
    mContextTemplate.Dispose();
    mUtilTemplate.Dispose();
    mInvokableObjectTemplate.Dispose();
    mSystemTemplate.Dispose();
    mTimerTemplate.Dispose();
    mContextGlobalTemplate.Dispose();

    // The manager tracks the templates so they can be reused by all the
    // individual scripts.
    mVec3Template.Dispose();
    mQuaternionTemplate.Dispose();
    mPatternTemplate.Dispose();

    if (mIsolate == v8::Isolate::GetCurrent())
        mIsolate->Exit();
    
    mIsolate->Dispose();
}

Sirikata::SerializationCheck* JSCtx::serializationCheck()
{
    return &mCheck;
}

bool JSCtx::initialized()
{
    return isInitialized;
}

void JSCtx::initialize()
{
    isInitialized = true;
}

bool JSCtx::stopped()
{
    return isStopped;
}

void JSCtx::stop()
{
    isStopped = true;
}

Network::IOService* JSCtx::getIOService()
{
    return internalContext->ioService;
}



} //end namespace JS
} //end namespace Sirikata
