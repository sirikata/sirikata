/*  Sirikata liboh -- Object Host
 *  MonoVWObjectScript.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef _MONO_VWOBJECT_SCRIPT_HPP
#define _MONO_VWOBJECT_SCRIPT_HPP

#include "MonoDefs.hpp"
#include "MonoObject.hpp"
#include "MonoDomain.hpp"
#include "MonoMethodLookupCache.hpp"
#include <sirikata/oh/ObjectScript.hpp>

namespace Sirikata {
class HostedObject;

class MonoVWObjectScript : public ObjectScript {
    HostedObjectPtr mParent;
    Mono::Domain mDomain;
    Mono::Object mObject;

    Mono::ThreadSafeSingleMethodLookupCache mProcessRPCCache;
    Mono::ThreadSafeSingleMethodLookupCache mProcessMessageCache;
public:
    typedef std::map<String, String> ArgumentMap;

    MonoVWObjectScript(Mono::MonoSystem*, HostedObjectPtr ho, const ArgumentMap& args);
    virtual ~MonoVWObjectScript();

    /** Returns true if this script is valid, i.e. if it was successfully loaded
     *  and initialized.
     */
    bool valid() const;
};

}
#endif
