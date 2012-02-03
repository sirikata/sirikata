// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "EmersonHttpManager.hpp"
#include "JSLogging.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include <sirikata/core/service/Context.hpp>

namespace Sirikata {
namespace JS {

EmersonHttpManager::EmersonHttpManager(JSCtx* ctx)
 : SelfWeakPtr<EmersonHttpManager>(),
   currentToken(0),
   mContext(ctx)
{
    managerLiveness = nullEmersonHttpPtr;
}

EmersonHttpManager::~EmersonHttpManager()
{}

EmersonHttpManager::EmersonHttpToken EmersonHttpManager::makeRequest(Sirikata::Network::Address addr, Transfer::HttpManager::HTTP_METHOD method, std::string req,v8::Persistent<v8::Function> cb, JSContextStruct* jscont)
{
    ++currentToken;

    //Log that jscont has issued a request associated with token current token.
    ctxTokeMap[jscont][currentToken] = currentToken;

    //log that any http responses we get associated with current token should
    //call cb from jscont
    tokeCBMap[currentToken] = std::pair<JSContextStruct*, v8::Persistent<v8::Function> > (jscont,cb);

    if (managerLiveness == nullEmersonHttpPtr)
        managerLiveness = getSharedPtr();

    //issue query, and have response callback to receiveHttpResponse
    Transfer::HttpManager::getSingleton().makeRequest(addr,method,req,true,
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
    if (mContext->stopped()) {
        JSLOG(warn, "Received HTTP response after shutdown request, ignoring...");
        return;
    }

    //require to check if initialized because http code is on separate strand
    //from main or object.  During initialization (including initial shim/file
    //import), on main strand.  If that code made an http request (processed on
    //http strand) that finished before initial import was over, should not post
    //it back to the object strand.  Should instead re-post and try again.
    while(!mContext->initialized())
    {    }

    mContext->objStrand->post(
        std::tr1::bind(&EmersonHttpManager::postReceiveResp, this, respToken,hrp, error, boost_error),
        "EmersonHttpManager::postReceiveResp"
    );
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


//should be called from within mStrand
void EmersonHttpManager::postReceiveResp(EmersonHttpToken respToken,HttpRespPtr hrp,Transfer::HttpManager::ERR_TYPE error,const boost::system::error_code& boost_error)
{

    //first lookup token in outstanding token map to find corresponding context
    //and callback
    TokenCBMapIter tokeFindIt = tokeCBMap.find(respToken);
    if (tokeFindIt == tokeCBMap.end())
    {
        JSLOG(error, "Error in EmersonHttpManager.  Received an http response for a token I had not assigned: "<<respToken);
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
        if (error == Transfer::HttpManager::BOOST_ERROR)
        {
            JSLOG(detailed,"emerson http callback was unsuccessful");
            jscont->httpFail(cb,"UNKNOWN_ERROR");
        }
        else if (error ==Transfer::HttpManager::REQUEST_PARSING_FAILED)
        {
            JSLOG(info,"Parsing request header failed");
            jscont->httpFail(cb,"REQUEST_PARSING_ERROR");
        }
        else if(error == Transfer::HttpManager::RESPONSE_PARSING_FAILED)
        {
            JSLOG(info,"Parsing response header failed");
            jscont->httpFail(cb,"RESPONSE_PARSING_ERROR");
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
