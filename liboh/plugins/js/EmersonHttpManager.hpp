

#ifndef __EMERSON_HTTP_MANAGER_HPP__
#define __EMERSON_HTTP_MANAGER_HPP__

#include <map>
#include <sirikata/core/transfer/HttpManager.hpp>
#include <v8.h>
#include <sirikata/core/util/SelfWeakPtr.hpp>

namespace Sirikata{
namespace JS{

class EmersonHttpManager;
class JSContextStruct;

typedef std::tr1::shared_ptr<EmersonHttpManager> EmersonHttpPtr;
typedef std::tr1::weak_ptr  <EmersonHttpManager> EmersonHttpWPtr;


class EmersonHttpManager : public SelfWeakPtr<EmersonHttpManager>
{
public:
    typedef uint32 EmersonHttpToken;
    
    EmersonHttpManager(Sirikata::Network::IOService* ioserve);
    ~EmersonHttpManager();

    typedef std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> HttpRespPtr;
    
    
    /*
      Whenever a context is destroyed, removes its entry from ctxTokeMap.  For
      all allbacks in tokeCBMap that are outstanding for this context, call
      dispose on their outstanding v8 callback functions, and sets their
      jscontext to null instead.
     */
    void deregisterContext(JSContextStruct* toDeregister);


    /**
       Each request sent out is associated with a unique EmersonHttpToken taken
       from currentToken (currentToken is incremented as a result of this
       call).

       If never sent a request from this from this context (ie,
       ctxTokeMap[jscont] is undefined), creates a new element in ctxTokeMap
       indexed by jscont.  

       Appends token associated with request to entry in ctxTokeMap[jscont].
       Also, creates a new entry in tokeCBMap that associates this request's
       token with jscont and cb.

       Returns the token associated with the request: in the future, may allow a
       user him/herself, to cancel an http request.
     */
    EmersonHttpToken makeRequest(Sirikata::Network::Address addr,     Transfer::HttpManager::HTTP_METHOD method, std::string req,v8::Persistent<v8::Function> cb, JSContextStruct* jscont);


private:

    typedef std::map<EmersonHttpToken, EmersonHttpToken> TokenMap;
    typedef TokenMap::iterator TokenMapIter;

    typedef std::pair<JSContextStruct* , v8::Persistent<v8::Function> > ContextCBPair;
    typedef std::map<EmersonHttpToken, ContextCBPair> TokenCBMap;
    typedef TokenCBMap::iterator TokenCBMapIter;

    typedef std::map<JSContextStruct*, TokenMap> ContextTokenMap;  
    typedef ContextTokenMap::iterator ContextTokenMapIter;

    //to ensure uniqueness, makeRequest should be the only function (outside of
    //constructor that reads or writes to this value).
    EmersonHttpToken currentToken;

    /**
       For each context, ctxTokeMap tracks all the outstanding http tokens for
       this existing context.  For its token map values, each token map key is
       identical to its index.  Use a map so that lookup is fast.
     */
    ContextTokenMap ctxTokeMap;

    /**
       For each token, can re-access its context struct and callback function
       from tokeCBMap.
     */
    TokenCBMap tokeCBMap;


    /**
       Whenever we issue an http request, we wrap this function as the
       httpcallback.  We map what emerson callback and context is associated
       with this http request with respToken, which can be used as an index to
       tokeCBMap.  If get an error, pass error to callback.  If JSContext is
       null (it had been cleared between the time we sent the query and received
       its response), do not evaluate callback.  If jscontext is suspended, do
       not evaluate callback and dispose of v8 callback.

       In all cases, remove entry from tokeCBMap and remove entry from value of
       ctxTokeMap.
     */
    void receiveHttpResponse(EmersonHttpToken respToken,
        HttpRespPtr hrp,
        Transfer::HttpManager::ERR_TYPE error,
        const boost::system::error_code& boost_error
    );
    void postReceiveResp(EmersonHttpToken respToken,HttpRespPtr hrp,Transfer::HttpManager::ERR_TYPE error,const boost::system::error_code& boost_error);


    void debugPrintTokenMap();
    void debugPrintContextMap();



    
    //If have outstanding requests, this pointer will have a pointer to self.
    //If don't, then it has an empty ptr.  Allows this to garbage collect properly
    EmersonHttpPtr managerLiveness;
    Sirikata::Network::IOService* mIO;
};

static EmersonHttpPtr nullEmersonHttpPtr;


} //namespace js
} //namespace sirikata

#endif
