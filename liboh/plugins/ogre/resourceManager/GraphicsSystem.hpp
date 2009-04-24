/*  Meru
 *  GraphicsSystem.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _GRAPHICS_SYSTEM_HPP_
#define _GRAPHICS_SYSTEM_HPP_

#include "MeruDefs.hpp"
#include "Singleton.hpp"
#include "Factory.hpp"
#include "Proxy.hpp"

namespace Meru {

  class GraphicsEntity;
  class GraphicsLight;
  class GraphicsInputHandler;
  class GraphicsRenderSettings;
  class GraphicsUI;
  class Input;

  /** Graphics system is the interface for a rendering/scene graph/physics
   *  library for Meru.
   */
  class GraphicsSystem : public ManualSingleton<GraphicsSystem> {
  public:
      typedef std::tr1::function<void(int)> InputCallback;
      /** Callback for password dialogs.  Should have the form
       *    void callback(bool success, const String& username, const String& password, bool remember)
       */
      typedef std::tr1::function<void(bool, const String &, const String &, bool)> PasswordCallback;

    GraphicsSystem();
    virtual ~GraphicsSystem();

    /** Do any setup required for the GraphicsSystem.
     *  \returns false if setup failed
     */
    virtual bool initialize() = 0;

    /** Render the current frame.
     *  \returns false if the graphics system has requested termination, true otherwise
     */
    virtual bool render() = 0;

    /** Do any cleanup before quitting. */
    virtual void cleanup() = 0;

    /** Attempt to login and, if successful, switch from login view to the
     *  main scene view.
     *  \param name username
     *  \param passwd password
     *  \param server server to connect to
     *  \param port server port to connect to
     */
    virtual void login(String name, String passwd, String server, String port) = 0;

    /** Create an graphics entity for this specific GraphicsSystem.
     *  \param parentProxyObject the VWObject that has state that the graphics entity refers to.
     *  \param name a unique name for this entity
     *  \param character if true, create a character entity
     *  \returns the entity
     */
    virtual GraphicsEntity* createEntity(WeakProxyPtr parentProxyObject, const String& name, bool character) = 0;

    /** Create a graphics light for this specific Graphics system.
     *  \param parentProxyObject the VWObject that has state that the graphics light refers to.
     * \param name a unique name for this light.
     * \returns the light
     */
    virtual GraphicsLight* createLight(WeakProxyPtr parentProxyObject, const String& name, const LightInfo &lightInfo) = 0;

    /**
     * Displays a dialog and returns the result to the callback
     */
    virtual void showInputDialog(const String &title, const String &message, std::vector<String>, InputCallback callback) = 0;

    /**
     * Displays a dialog prompting the user for a username and password.
     * The callback gets the following parameters:
     *    bool - false means the user hit cancel instead of logging in
     *    string - username
     *    string - password
     *    bool - true means remember the username and password
     */
    virtual void promptPassword(const String &username, PasswordCallback callback) = 0;

    /** Get the RenderSettings for this GraphicsSystem.
     *  \returns a pointer to a GraphicsRenderSettings object for this GraphicsSystem
     */
    virtual GraphicsRenderSettings* getRenderSettings() = 0;

    virtual void pushCursor(const String &cursorName) = 0;
    virtual void popCursor() = 0;
    virtual void showCursor(bool visible) = 0;

    virtual Input* input() = 0;

  protected:
  };

  typedef Factory<GraphicsSystem> GraphicsSystemFactory;

} // namespace Meru

#endif //_GRAPHICS_SYSTEM_HPP_
