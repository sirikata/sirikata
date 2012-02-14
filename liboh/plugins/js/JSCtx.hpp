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

class JSCtx 
{
public:    
    JSCtx(
        Context* ctx,Network::IOStrandPtr oStrand,
        Network::IOStrandPtr vmStrand,v8::Isolate* is);
    
    ~JSCtx();
    
    Network::IOStrandPtr objStrand;
    Network::IOStrandPtr visManStrand;
    Network::IOStrand* mainStrand;
    
    v8::Isolate* mIsolate;
    bool stopped();
    void stop();
    void initialize();
    bool initialized();

    Sirikata::SerializationCheck* serializationCheck();
    Network::IOService* getIOService();
    
    v8::Persistent<v8::FunctionTemplate> mVisibleTemplate;
    v8::Persistent<v8::FunctionTemplate> mPresenceTemplate;
    v8::Persistent<v8::ObjectTemplate>   mContextTemplate;
    v8::Persistent<v8::ObjectTemplate>   mUtilTemplate;
    v8::Persistent<v8::ObjectTemplate>   mInvokableObjectTemplate;
    v8::Persistent<v8::ObjectTemplate>   mSystemTemplate;
    v8::Persistent<v8::ObjectTemplate>   mTimerTemplate;
    v8::Persistent<v8::ObjectTemplate>   mContextGlobalTemplate;

    // The manager tracks the templates so they can be reused by all the
    // individual scripts.
    v8::Persistent<v8::FunctionTemplate> mVec3Template;
    v8::Persistent<v8::FunctionTemplate> mQuaternionTemplate;
    v8::Persistent<v8::FunctionTemplate> mPatternTemplate;
    
    
private:
    Context* internalContext;
    bool isStopped;
    bool isInitialized;
    Sirikata::SerializationCheck mCheck;
};


}//end namespace Sirikata
}//end namespace JS


#endif
