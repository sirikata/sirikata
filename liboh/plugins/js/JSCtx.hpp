#ifndef __SIRIKATA_JSCTX_HPP__
#define __SIRIKATA_JSCTX_HPP__

#include <sirikata/core/service/Context.hpp>

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
    JSCtx(Context* ctx,Network::IOStrand* oStrand);
    ~JSCtx();
    
    Network::IOStrand* objStrand;

    bool stopped();
    void stop();
    
private:
    bool isStopped;
    
};

}//end namespace Sirikata
}//end namespace JS


#endif
