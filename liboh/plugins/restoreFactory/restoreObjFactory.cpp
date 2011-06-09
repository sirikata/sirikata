/*  Sirikata
 *  CSVObjectFactory.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "restoreObjFactory.hpp"
#include <list>
#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <vector>


namespace Sirikata {

RestoreObjFactory::RestoreObjFactory(ObjectHostContext* ctx, ObjectHost* oh, const SpaceID& space, const String& filename, int32 max_objects, int32 connect_rate)
 : mContext(ctx),
   mOH(oh),
   mFilename(filename),
   mMaxObjects(max_objects),
   mConnectRate(connect_rate)
{
}


void RestoreObjFactory::generate()
{
    std::ifstream fp(mFilename.c_str());
    if (!fp) return;

    String fullFile = "";
    // For each line
    while(fp)
    {
        String line;
        std::getline(fp, line);
        // First char is # and not the first non whitespace char
	// then this is a comment
        if((line.length() > 0) && (line.at(0) == '#'))
            continue;

        fullFile += ("\n" +line);
    }

    parseEntArgs(fullFile);
    
    fp.close();
    execEnts();
    return;
}


void RestoreObjFactory::parseEntArgs(String toParse)
{
    CSVObjectFactory::StringList allParts = CSVObjectFactory::sepCommas(toParse);

    for (CSVObjectFactory::StringList::iterator iter = allParts.begin(); iter != allParts.end(); ++iter)
    {
        if ((iter->size() != 0) && (mIncompleteEnts.size() < mMaxObjects))
            mIncompleteEnts.push(*iter);
    }
}


void RestoreObjFactory::execEnts()
{
    if (mContext->stopped())
    {
        std::cout<<"\n\nContext stopped.  Will not get anywhere\n\n";
        return;
    }

    String scriptType = "js";
    for(int32 i = 0; i < mConnectRate && !mIncompleteEnts.empty(); i++)
    {
        String argToExec= mIncompleteEnts.front();
        mIncompleteEnts.pop();

        HostedObjectPtr ent;
        ent = mOH->createObject(UUID::null(), &scriptType, &argToExec);
    }

    if (!mIncompleteEnts.empty())
    {
        mContext->mainStrand->post(
            Duration::seconds(1.f),
            std::tr1::bind(&RestoreObjFactory::execEnts, this)
        );
    }
}


}//end namespace sirikata
