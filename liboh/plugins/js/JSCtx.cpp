#include "JSCtx.hpp"


namespace Sirikata
{
namespace JS
{


JSCtx::JSCtx(Context* ctx,Network::IOStrand* oStrand)
 : Context("JS", ctx->ioService, ctx->mainStrand, NULL,Time::null()),
   objStrand(oStrand),
   isStopped(false)
{
}

JSCtx::~JSCtx()
{
    #ifdef BFTM_DEBUG
    lkjs;  Who actually deletes the objStrand?;
    #endif
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
