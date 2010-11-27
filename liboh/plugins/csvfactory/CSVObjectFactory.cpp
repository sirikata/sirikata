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
#include <list>
#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <vector>


namespace Sirikata {

CSVObjectFactory::CSVObjectFactory(ObjectHostContext* ctx, ObjectHost* oh, const SpaceID& space, const String& filename, int32 max_objects, int32 connect_rate)
 : mContext(ctx),
   mOH(oh),
   mSpace(space),
   mFilename(filename),
   mMaxObjects(max_objects),
   mConnectRate(connect_rate)
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

void CSVObjectFactory::generate()
{

    int count =0;
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
    int script_type_idx = -1;
    int script_opts_idx = -1;
    int scale_idx = -1;
    int objid_idx = -1;
    int solid_angle_idx = -1;

    // For each line
    while(fp && (count < mMaxObjects))
    {
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
            for(uint32 idx = 0; idx < line_parts.size(); idx++)
            {

                if (line_parts[idx] == "objtype") objtype_idx = idx;
                if (line_parts[idx] == "pos_x") pos_idx = idx;
                if (line_parts[idx] == "orient_x") orient_idx = idx;
                if (line_parts[idx] == "vel_x") vel_idx = idx;
                if (line_parts[idx] == "meshURI") mesh_idx = idx;
                if (line_parts[idx] == "rot_axis_x") quat_vel_idx = idx;
                if (line_parts[idx] == "script_type") script_type_idx = idx;
                if (line_parts[idx] == "script_options") script_opts_idx = idx;
                if (line_parts[idx] == "scale") scale_idx = idx;
                if(line_parts[idx] == "objid")
                {
                    objid_idx = idx;
                }
                if(line_parts[idx] == "solid_angle")
                {
                    solid_angle_idx = idx;
                }


            }

            is_first = false;
        }
        else {
            //note: script_file is not required, so not checking it with the assert
            assert(objtype_idx != -1 && pos_idx != -1 && orient_idx != -1 && vel_idx != -1 && mesh_idx != -1 && quat_vel_idx != -1);
            //assert(objtype_idx != -1 && pos_idx != -1 && mesh_idx != -1);

            if (line_parts[objtype_idx] == "mesh") {
                Vector3d pos(
                    safeLexicalCast<double>(line_parts[pos_idx+0]),
                    safeLexicalCast<double>(line_parts[pos_idx+1]),
                    safeLexicalCast<double>(line_parts[pos_idx+2])
                );
                Quaternion orient =
                    orient_idx == -1 ?
                    Quaternion(0, 0, 0, 1) :
                    Quaternion(
                        safeLexicalCast<float>(line_parts[orient_idx+0]),
                        safeLexicalCast<float>(line_parts[orient_idx+1]),
                        safeLexicalCast<float>(line_parts[orient_idx+2]),
                        safeLexicalCast<float>(line_parts[orient_idx+3]),
                        Quaternion::XYZW()
                    );
                Vector3f vel =
                    vel_idx == -1 ?
                    Vector3f(0, 0, 0) :
                    Vector3f(
                        safeLexicalCast<float>(line_parts[vel_idx+0]),
                        safeLexicalCast<float>(line_parts[vel_idx+1]),
                        safeLexicalCast<float>(line_parts[vel_idx+2])
                    );

                Vector3f rot_axis =
                    quat_vel_idx == -1 ?
                    Vector3f(0, 0, 0) :
                    Vector3f(
                        safeLexicalCast<float>(line_parts[quat_vel_idx+0]),
                        safeLexicalCast<float>(line_parts[quat_vel_idx+1]),
                        safeLexicalCast<float>(line_parts[quat_vel_idx+2])
                    );

                float angular_speed =
                    quat_vel_idx == -1 ?
                    0 :
                    safeLexicalCast<float>(line_parts[quat_vel_idx+3]);

                String mesh( line_parts[mesh_idx] );

                String scriptType = "";
                String scriptOpts = "";

              	if(script_type_idx != -1)
                {
                    scriptType = line_parts[script_type_idx];
                }
                if(script_opts_idx != -1)
                {
                    scriptOpts = line_parts[script_opts_idx];
                }

                float scale =
                    scale_idx == -1 ?
                    1.f :
                    safeLexicalCast<float>(line_parts[scale_idx], 1.f);

                String solid_angle = "";
                SolidAngle query_angle(SolidAngle::Max);

                if(solid_angle_idx != -1)
                {
                  solid_angle = line_parts[solid_angle_idx];
                  if(solid_angle != "")
                  {

                    query_angle = SolidAngle(atof(solid_angle.c_str()));
                  }
                }


                /*

                  Ticket #134

                */
                String objid = "";
                if(objid_idx != -1)
                {
                    objid = line_parts[objid_idx];
                }

                HostedObjectPtr obj;

                if(objid_idx == -1)
                {
                    obj = HostedObject::construct<HostedObject>(mContext, mOH, UUID::random());

                }
                else
                {
                    obj = HostedObject::construct<HostedObject>(mContext, mOH, UUID(objid, UUID::HumanReadable()));
                }


                obj->init();
                if (scriptType != "")
                    obj->initializeScript(scriptType, scriptOpts);

                ObjectConnectInfo oci;
                oci.object = obj;
                oci.loc = Location( pos, orient, vel, rot_axis, angular_speed);
                oci.bounds = BoundingSphere3f(Vector3f::nil(), scale);
                oci.mesh = mesh;
                oci.query_angle = query_angle;
                mIncompleteObjects.push(oci);

                count++;


            }
        }
    }


    fp.close();

    connectObjects();

    return;
}


void CSVObjectFactory::connectObjects()
{

    if (mContext->stopped())
    {
        std::cout<<"\n\nContext stopped.  Will not get anywhere\n\n";
        return;
    }

    for(int32 i = 0; i < mConnectRate && !mIncompleteObjects.empty(); i++) {
        ObjectConnectInfo oci = mIncompleteObjects.front();
        mIncompleteObjects.pop();

        oci.object->connect(
            mSpace,
            oci.loc, oci.bounds, oci.mesh,
            const_cast<SolidAngle&>(oci.query_angle),
            UUID::null(), NULL
        );
    }

    if (!mIncompleteObjects.empty())
        mContext->mainStrand->post(
            Duration::seconds(1.f),
            std::tr1::bind(&CSVObjectFactory::connectObjects, this)
        );
}

}
