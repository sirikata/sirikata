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

#include "CSVObjectFactory.hpp"
#include <sirikata/oh/HostedObject.hpp>

namespace Sirikata {

CSVObjectFactory::CSVObjectFactory(ObjectHostContext* ctx, ObjectHost* oh, const SpaceID& space, const String& filename)
 : mContext(ctx),
   mOH(oh),
   mSpace(space),
   mFilename(filename)
{
}

template<typename T>
T safeLexicalCast(const String& orig) {
    if (orig.empty())
        return (T)0;
    return boost::lexical_cast<T>(orig);
}

void CSVObjectFactory::generate() {
    typedef std::vector<String> StringList;

    std::ifstream fp(mFilename.c_str());
    if (!fp) return;

    bool is_first = true;
    int objtype_idx = -1;
    int pos_idx = -1;
    int orient_idx = -1;
    int vel_idx = -1;
    int mesh_idx = -1;
    // For each line
    while(fp) {
        String line;
        std::getline(fp, line);

        // Split into parts
        StringList line_parts;
        int last_comma = -1, next_comma = 0;
        while(next_comma != String::npos) {
            next_comma = line.find(',', last_comma+1);

            String next_val;
            if (next_comma == String::npos)
                next_val = line.substr(last_comma + 1);
            else
                next_val = line.substr(last_comma + 1, next_comma - (last_comma+1));

            // Remove quotes from beginning and end
            if (next_val.size() > 2 && next_val[0] == '"' && next_val[next_val.size()-1] == '"')
                next_val = next_val.substr(1, next_val.size() - 2);

            line_parts.push_back(next_val);

            last_comma = next_comma;
        }

        if (is_first) {
            for(int idx = 0; idx < line_parts.size(); idx++) {
                if (line_parts[idx] == "objtype") objtype_idx = idx;
                if (line_parts[idx] == "pos_x") pos_idx = idx;
                if (line_parts[idx] == "orient_x") orient_idx = idx;
                if (line_parts[idx] == "vel_x") vel_idx = idx;
                if (line_parts[idx] == "meshURI") mesh_idx = idx;
            }

            is_first = false;
        }
        else {
            assert(objtype_idx != -1 && pos_idx != -1 && orient_idx != -1 && vel_idx != -1 && mesh_idx != -1);

            if (line_parts[objtype_idx] == "mesh") {
                Vector3d pos(
                    safeLexicalCast<double>(line_parts[pos_idx+0]),
                    safeLexicalCast<double>(line_parts[pos_idx+1]),
                    safeLexicalCast<double>(line_parts[pos_idx+2])
                );
                Quaternion orient(
                    safeLexicalCast<float>(line_parts[orient_idx+0]),
                    safeLexicalCast<float>(line_parts[orient_idx+1]),
                    safeLexicalCast<float>(line_parts[orient_idx+2]),
                    safeLexicalCast<float>(line_parts[orient_idx+3]),
                    Quaternion::XYZW()
                );
                Vector3f vel(
                    safeLexicalCast<float>(line_parts[vel_idx+0]),
                    safeLexicalCast<float>(line_parts[vel_idx+1]),
                    safeLexicalCast<float>(line_parts[vel_idx+2])
                );
                String mesh( line_parts[mesh_idx] );

                
                HostedObjectPtr obj = HostedObject::construct<HostedObject>(mContext, mOH, UUID::random(), false);
                obj->init();
                obj->connect(
                    mSpace,
                    Location( pos, orient, vel, Vector3f::nil(), 0),
                    BoundingSphere3f(Vector3f::nil(), 1.f),
                    mesh,
                    UUID::null());
            }
        }
    }

    fp.close();
    return;
}

}
