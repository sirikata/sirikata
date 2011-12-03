#include "JSCtx.hpp"


namespace Sirikata
{
namespace JS
{


JSCtx::JSCtx(Context* ctx,Network::IOStrand* oStrand,v8::Isolate* is)
 : Context("JS", ctx->ioService, ctx->mainStrand, NULL,Time::null()),
   objStrand(oStrand),
   mIsolate(is),
   isStopped(false),
   isInitialized(false),
   mCheck()
{
}

JSCtx::~JSCtx()
{
    /**
       lkjs;
       FIXME: who actually deletes the objStrand?
     */
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


} //end namespace JS
} //end namespace Sirikata
