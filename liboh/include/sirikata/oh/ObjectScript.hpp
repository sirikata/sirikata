/*  Sirikata liboh -- Object Host
 *  ObjectScript.hpp
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
#ifndef _OBJECT_SCRIPT_HPP_
#define _OBJECT_SCRIPT_HPP_

#include <sirikata/core/util/Logging.hpp>

namespace Sirikata {

/** Script running in a plugin. Can be in C++, or this interface
    can be passed on to a script.
    @see ObjectScriptManager for how to instantiate one of these.
*/
class SIRIKATA_OH_EXPORT ObjectScript {
  public:

    /// Destructor: called from the plugin itself.
    virtual ~ObjectScript(){}


    // This will update the addressables for the entity.
    //Addressables depend upon the space the presence is
    //Different presences have different spaces

     virtual void updateAddressable() { NOT_IMPLEMENTED(ObjectScript); }

     virtual String scriptType() const { return scriptType_;}
     virtual String scriptOptions() const {return scriptOptions_;}
     virtual void scriptTypeIs(String _scriptType) { scriptType_ = _scriptType;}
     virtual void scriptOptionsIs(String _scriptOptions) {scriptOptions_ = _scriptOptions;}

  protected:
     String scriptType_;
     String scriptOptions_;

};

}
#endif
