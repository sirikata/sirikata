/*  Sirikata liboh -- Object Host
 *  ObjectScriptManager.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
#ifndef _OBJECT_SCRIPT_MANAGER_HPP_
#define _OBJECT_SCRIPT_MANAGER_HPP_

namespace Sirikata {

class HostedObject;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
class ObjectScript;

/** Script factory -- generally have one per shared dynamic library.
    To access an instance, @see ObjectScriptManagerFactory. */
class SIRIKATA_OH_EXPORT ObjectScriptManager  {
  public:
    /** Create a script linked to this HostedObject.
        Called by HostedObject::initializeScripted().
    */
    virtual ObjectScript *createObjectScript(HostedObjectPtr ho,
                                             const String &args)=0;
    /// Delete this ObjectScript instance.
    virtual void destroyObjectScript(ObjectScript*toDestroy)=0;
    /// Destructor: called from the plugin itself.
    virtual ~ObjectScriptManager(){}
};

}
#endif
