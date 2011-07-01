

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
    ctxTokeMap[jscont][currentToken] = currentToken;

    //log that any http responses we get associated with current token should
    //call cb from jscont
    tokeCBMap[currentToken] = std::pair<JSContextStruct*, v8::Persistent<v8::Function> > (jscont,cb);

    if (managerLiveness == nullEmersonHttpPtr)
        managerLiveness = EmersonHttpPtr(this);

    std::ostringstream request_stream;
    request_stream.str("");
    request_stream << "GET " <<  "/" << " HTTP/1.1\r\n";
    request_stream << "Host: " << "www.google.com" << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";
//    request_stream << "Connection: keep-alive\r\n\r\n";

    // request_stream<<"GET / HTTP/1.1\r\n";
    // request_stream<<"Host: www.google.com\r\n";
    // request_stream<<"User-Agent: Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.9.2.18) Gecko/20110628 Ubuntu/10.04 (lucid) Firefox/3.6.18\r\n";
    // request_stream<<"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    // request_stream<<"Accept-Language: en-us,en;q=0.5\r\n";
    // request_stream<<"Accept-Encoding: gzip,deflate\r\n";
    // request_stream<<"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n";
    // request_stream<<"Keep-Alive: 115\r\n";
    // request_stream<<"Connection: keep-alive\r\n";
    // request_stream<<"Cache-Control: max-age=0\r\n";

    
    //issue query, and have response callback to receiveHttpResponse
//    Transfer::HttpManager::getSingleton().makeRequest(addr,method,req,
    Transfer::HttpManager::getSingleton().makeRequest(addr,method,request_stream.str(),
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

void EmersonHttpManager::postReceiveResp(EmersonHttpToken respToken,HttpRespPtr hrp,Transfer::HttpManager::ERR_TYPE error,const boost::system::error_code& boost_error)
{

    std::cout<<"\n\nInside of posting receive response\n\n";
    
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

    
}



} //end namespace JS
} //end namespace Sirikata
