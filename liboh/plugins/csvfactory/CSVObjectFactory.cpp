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
T safeLexicalCast(const String& orig, T default_val) {
    if (orig.empty())
        return default_val;
    return boost::lexical_cast<T>(orig);
}

template<typename T>
T safeLexicalCast(const String& orig) {
    return safeLexicalCast<T>(orig, (T)0);
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

    int quat_vel_idx = -1;
    int script_file_idx = -1;
    int scale_idx = -1;


    // For each line
    while(fp) {
        String line;
        std::getline(fp, line);
        // First char is # and not the first non whitespace char
	// then this is a comment
        if(line.length() > 0 && line.at(0) == '#')
       {
         continue;   
       } 
        // Split into parts
        StringList line_parts;
        int last_comma = -1;
        String::size_type next_comma = 0;
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
            for(uint32 idx = 0; idx < line_parts.size(); idx++) {
                if (line_parts[idx] == "objtype") objtype_idx = idx;
                if (line_parts[idx] == "pos_x") pos_idx = idx;
                if (line_parts[idx] == "orient_x") orient_idx = idx;
                if (line_parts[idx] == "vel_x") vel_idx = idx;
                if (line_parts[idx] == "meshURI") mesh_idx = idx;
                if (line_parts[idx] == "rot_axis_x") quat_vel_idx = idx;
                if (line_parts[idx] == "script_file") script_file_idx = idx;
                if (line_parts[idx] == "scale") scale_idx = idx;
            }

            is_first = false;
        }
        else {
            //note: script_file is not required, so not checking it witht he assert
            assert(objtype_idx != -1 && pos_idx != -1 && orient_idx != -1 && vel_idx != -1 && mesh_idx != -1 && quat_vel_idx != -1);

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

                Vector3f rot_axis(
                    safeLexicalCast<float>(line_parts[quat_vel_idx+0]),
                    safeLexicalCast<float>(line_parts[quat_vel_idx+1]),
                    safeLexicalCast<float>(line_parts[quat_vel_idx+2])
                );
                
                float angular_speed = safeLexicalCast<float>(line_parts[quat_vel_idx+3]);

                String mesh( line_parts[mesh_idx] );


                String scriptFile = "";
                String scriptType = "";
                if (script_file_idx != -1)
                {
                    std::cout<<"\n\nLength of line_parts: "<<line_parts.size();
                    std::cout<<"\n\nIndex: "<<script_file_idx<<"\n\n";
                    std::cout.flush();
                    if (script_file_idx<line_parts.size()) {
                        scriptFile = line_parts[script_file_idx];
                        std::cout<<"\n\nGot a script file:  "<<scriptFile<<"\n\n";
                        scriptType = line_parts[script_file_idx+1];
                    }
                }


                float scale = 1.f;
                if (scale_idx != -1)
                    scale = safeLexicalCast<float>(line_parts[scale_idx], 1.f);


                HostedObjectPtr obj = HostedObject::construct<HostedObject>(mContext, mOH, UUID::random(), false);
                obj->init();
                obj->connect(
                    mSpace,
                    Location( pos, orient, vel, rot_axis, angular_speed),
                    BoundingSphere3f(Vector3f::nil(), scale),
                    mesh,
                    UUID::null(),
                    scriptFile,
                    scriptType);
            }
        }
    }

    fp.close();
    return;
}

}
