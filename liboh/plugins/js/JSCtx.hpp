#ifndef __SIRIKATA_JSCTX_HPP__
#define __SIRIKATA_JSCTX_HPP__

#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/util/SerializationCheck.hpp>
#include <v8.h>


namespace Sirikata
{
namespace JS
{

/**
   Note: trace, epoch, and simlen
 */

class JSCtx : public Context
{
public:    
    JSCtx(Context* ctx,Network::IOStrand* oStrand,v8::Isolate* is);
    ~JSCtx();
    
    Network::IOStrand* objStrand;

    v8::Isolate* mIsolate;
    bool stopped();
    void stop();
    void initialize();
    bool initialized();

    Sirikata::SerializationCheck* serializationCheck();
    
    
private:
    bool isStopped;
    bool isInitialized;
    Sirikata::SerializationCheck mCheck;
};

}//end namespace Sirikata
}//end namespace JS


#endif
