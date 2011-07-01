

#include "EmersonHttpManager.hpp"
#include "JSLogging.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"

namespace Sirikata{
namespace JS{



EmersonHttpManager::EmersonHttpManager(Sirikata::Network::IOService* ioserve)
 : currentToken(0),
   mIO(ioserve)
{
    managerLiveness = nullEmersonHttpPtr;
}

EmersonHttpManager::~EmersonHttpManager()
{}

EmersonHttpManager::EmersonHttpToken EmersonHttpManager::makeRequest(Sirikata::Network::Address addr,     Transfer::HttpManager::HTTP_METHOD method, std::string req,v8::Persistent<v8::Function> cb, JSContextStruct* jscont)
{
    ++currentToken;

    //Log that jscont has issued a request associated with token current token.
    // if ((ctxTokeMap.find(jscont) == ctxTokeMap.end()) || (ctxTokeMap.find(jscont)->second.empty()))
    // {
    //     TokenMap toLoad;
    //     toLoad[currentToken] = currentToken;
    //     ctxTokeMap[jscont]   = toLoad;
    // }
    // else
    // {
    //     ctxTokeMap[jscont][currentToken] = currentToken;
    
    ctxTokeMap[jscont][currentToken] = currentToken;

    //log that any http responses we get associated with current token should
    //call cb from jscont
    tokeCBMap[currentToken] = std::pair<JSContextStruct*, v8::Persistent<v8::Function> > (jscont,cb);

    if (managerLiveness == nullEmersonHttpPtr)
        managerLiveness = EmersonHttpPtr(this);

    //issue query, and have response callback to receiveHttpResponse
    Transfer::HttpManager::getSingleton().makeRequest(addr,method,req,
        std::tr1::bind(&EmersonHttpManager::receiveHttpResponse, this, currentToken, _1,_2,_3));

    return currentToken;
}


void EmersonHttpManager::deregisterContext(JSContextStruct* toDeregister)
{

    //get list of all tokens that were associated with the context we're
    //de-registering
    ContextTokenMapIter findCtxTokensIter = ctxTokeMap.find(toDeregister);
    if (findCtxTokensIter == ctxTokeMap.end())
    {
        JSLOG(detailed,"No outstanding http requests for context that is de-registering.");
        return;
    }
    
    //contains all tokens assocaited with this context
    TokenMap tknMap= findCtxTokensIter->second;

    //erase entry for context in ctxTokeMap
    ctxTokeMap.erase(findCtxTokensIter);


    //use tokens in tknMap as keys to run through tokeCBMap to:
    // 1: dispose of persistent v8 callback in value pair
    // 2: set JSContextStruct in value pair to null.
    //NOTE: the httpcallback function should check to not dispatch callback on
    // null jscontextstruct.
    for (TokenMapIter tknMapIt = tknMap.begin(); tknMapIt != tknMap.end(); ++tknMapIt)
    {
        TokenCBMapIter findCBIter = tokeCBMap.find(tknMapIt->first);
        if (findCBIter == tokeCBMap.end())
        {
            JSLOG(error, "Error in emerson http manager.  Should not have received a request to remove a token that does not exist");
            continue;
        }

        //dispose of the persistent v8 function and set jscontextstruct for cb
        //to null
        findCBIter->second.first = NULL;
        findCBIter->second.second.Dispose();
    }
}


void EmersonHttpManager::receiveHttpResponse(EmersonHttpToken respToken,HttpRespPtr hrp,Transfer::HttpManager::ERR_TYPE error,const boost::system::error_code& boost_error)
{
    std::cout<<"\n\nGot a response to message in receiveHttpResponse\n";
    mIO->post(std::tr1::bind(&EmersonHttpManager::postReceiveResp, this, respToken,hrp, error, boost_error));
}

void EmersonHttpManager::debugPrintContextMap()
{
    std::cout<<"\nPrinting context map\n";
    for (ContextTokenMapIter ctxIter = ctxTokeMap.begin(); ctxIter != ctxTokeMap.end(); ++ ctxIter)
    {
        std::cout<<"Context: "<<ctxIter->first<<"\n";
        for (TokenCBMapIter tokeCBIter = tokeCBMap.begin(); tokeCBIter!=tokeCBMap.end(); ++tokeCBIter)
            std::cout<<tokeCBIter->first<<"\n";

        std::cout<<"\n\n";
    }
}

void EmersonHttpManager::debugPrintTokenMap()
{
    std::cout<<"\nPrinting token map\n";
    for (TokenCBMapIter tknIter = tokeCBMap.begin(); tknIter != tokeCBMap.end(); ++ tknIter)
    {
        std::cout<<"Token: "<<tknIter->first<<"\n";
        std::cout<<"Context: "<<tknIter->second.first<<"\n\n";
    }
    std::cout<<"\n\n";
}

void EmersonHttpManager::postReceiveResp(EmersonHttpToken respToken,HttpRespPtr hrp,Transfer::HttpManager::ERR_TYPE error,const boost::system::error_code& boost_error)
{

    std::cout<<"\n\nInside of posting receive response\n\n";

    debugPrintContextMap();
    debugPrintTokenMap();
    
    //first lookup token in outstanding token map to find corresponding context
    //and callback
    TokenCBMapIter tokeFindIt = tokeCBMap.find(respToken);

    
    if (tokeFindIt == tokeCBMap.end())
    {
        JSLOG(error, "Error in EmersonHttpManager.  Received an http response for a token I had not assigned.");
        return;
    }


    //check if jscontext is null (context had been cleared before response got
    //back).  If it has, only need to remove the JSContextStruct/v8 cb pair
    //corresponding to this token in the token map.
    JSContextStruct* jscont         = tokeFindIt->second.first;
    v8::Persistent<v8::Function> cb = tokeFindIt->second.second;
    if (tokeFindIt->second.first == NULL)
    {
        tokeCBMap.erase(tokeFindIt);

        if (tokeCBMap.empty())
            managerLiveness = nullEmersonHttpPtr;
        return;
    }

    //remove the token from the list of tokens associated with jscont
    ContextTokenMapIter findTokeCtxIter = ctxTokeMap.find(jscont);
    if (findTokeCtxIter == ctxTokeMap.end())
        JSLOG(error,"Error in EmersonHttpManager.  Received a callback for a context that is unlisted in manager.");
    else
    {
        TokenMapIter eraseTokenIter = findTokeCtxIter->second.find(respToken);
        if ( eraseTokenIter == findTokeCtxIter->second.end())
            JSLOG(error,"Error in EmersonHttpManager.  Received a token that was unlisted for this context.");
        else
            findTokeCtxIter->second.erase(eraseTokenIter);
    }
        
    
    //cannot execute callback if suspended.  And never will.
    if (! jscont->getIsSuspended())
    {
        //must call callback
        if ((error == Transfer::HttpManager::BOOST_ERROR) || (error ==Transfer::HttpManager::REQUEST_PARSING_FAILED) || (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED))
        {
            JSLOG(detailed,"emerson http callback was unsuccessful");
            jscont->httpFail(cb);
        }
        else
        {
            JSLOG(detailed,"emerson http callback successful");
            jscont->httpSuccess(cb,hrp);
        }
    }

    //dispose of persistent v8 callback
    cb.Dispose();
    tokeCBMap.erase(tokeFindIt);

    if (tokeCBMap.empty())
        managerLiveness = nullEmersonHttpPtr;


    std::cout<<"\n\n\nDEBUG: LAST PRINT\n\n";
    debugPrintContextMap();
    debugPrintTokenMap();
}



} //end namespace JS
} //end namespace Sirikata
