/*  Sirikata
 *  CassandraObjectFactory.cpp
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

#include "CassandraObjectFactory.hpp"
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/cassandra/Cassandra.hpp>

#define CF_NAME "objects" // Column Family Name

namespace Sirikata {

CassandraObjectFactory::CassandraObjectFactory(ObjectHostContext* ctx, ObjectHost* oh, const SpaceID& space, const String& host, int port, const String& oh_id)
 : mContext(ctx),
   mOH(oh),
   mSpace(space),
   mDBHost(host),
   mDBPort(port),
   mOHostID(oh_id),
   mConnectRate(1)
{
}

void CassandraObjectFactory::generate() {
    CassandraDBPtr db = Cassandra::getSingleton().open(mDBHost, mDBPort);

    std::vector<SuperColumn> superColumns;
    try{
        SliceRange range;
        range.count=1000000;
        superColumns = db->db()->getSuperColumns(mOHostID, CF_NAME, range);
    }
    catch(...){
        std::cout <<"Exception Caught when get object lists"<<std::endl;
        return;
    }

    for (std::vector<SuperColumn>::iterator it= superColumns.begin(); it != superColumns.end(); ++it) {
        String object_str((*it).name);
        String script_args((*it).columns[0].value);
        String script_contents((*it).columns[1].value);
        String script_type((*it).columns[2].value);

        if (!script_type.empty()) {
            ObjectInfo info;
            info.id = UUID(object_str, UUID::HexString());
            info.scriptType = script_type;
            info.scriptArgs = script_args;
            info.scriptContents = script_contents;
            mIncompleteObjects.push(info);
        }
    }

    connectObjects();
}

void CassandraObjectFactory::connectObjects() {
    if (mContext->stopped())
        return;

    for(int32 i = 0; i < mConnectRate && !mIncompleteObjects.empty(); i++) {
        ObjectInfo info = mIncompleteObjects.front();
        mIncompleteObjects.pop();

        HostedObjectPtr obj = mOH->createObject(
            info.id, info.scriptType, info.scriptArgs, info.scriptContents
        );
    }

    if (!mIncompleteObjects.empty())
        mContext->mainStrand->post(
            Duration::seconds(1.f),
            std::tr1::bind(&CassandraObjectFactory::connectObjects, this)
        );
}

} // namespace Sirikata
