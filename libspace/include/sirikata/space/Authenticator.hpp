/*  Sirikata
 *  Authenticator.hpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_SPACE_AUTHENTICATOR_HPP_
#define _SIRIKATA_SPACE_AUTHENTICATOR_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/util/Factory.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/space/SpaceContext.hpp>

namespace Sirikata {

class Authenticator : public Service {
public:
    typedef std::tr1::function<void(bool)> Callback;

    virtual ~Authenticator() {}

    // Service Interface
    virtual void start() {}
    virtual void stop() {}

    /** Try to authenticate a user given the requested object ID and
     *  associated authentication information. The callback is invoked to
     *  provide the result, including failure due to timeout.
     */
    virtual void authenticate(const UUID& obj_id, MemoryReference auth, Callback cb) = 0;
};

class SIRIKATA_SPACE_EXPORT AuthenticatorFactory
    : public AutoSingleton<AuthenticatorFactory>,
      public Factory2<Authenticator*, SpaceContext*, const String &>
{
  public:
    static AuthenticatorFactory& getSingleton();
    static void destroy();
}; // class AuthenticatorFactory

} // namespace Sirikata

#endif //_SIRIKATA_SPACE_AUTHENTICATOR_HPP_
