/*  Sirikata
 *  CSVObjectFactory.hpp
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

#ifndef _SIRIKATA_REST_OBJECT_FACTORY_HPP_
#define _SIRIKATA_REST_OBJECT_FACTORY_HPP_

#include <sirikata/oh/ObjectFactory.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <vector>
#include <list>
#include <sirikata/proxyobject/SimulationFactory.hpp>
//#include "../csvfactory/CSVObjectFactory.hpp"


namespace Sirikata {


class RestoreObjFactory : public ObjectFactory {

public:
    
    RestoreObjFactory(ObjectHostContext* ctx, ObjectHost* oh, const SpaceID& space, const String& filename, int32 max_objects, int32 connect_rate);
    
    virtual ~RestoreObjFactory() {}
    virtual void generate();

private:

    
    // Connects one batch of objects and sets up another callback for more
    // additions if necessary.
    void execEnts();
    void parseEntArgs(String toParse);
    
    ObjectHostContext* mContext;
    ObjectHost* mOH;
    String mFilename;
    int32 mMaxObjects;

    std::queue<String> mIncompleteEnts;
    int32 mConnectRate;
};

} // namespace Sirikata

#endif //_SIRIKATA_REST_OBJECT_FACTORY_HPP_
