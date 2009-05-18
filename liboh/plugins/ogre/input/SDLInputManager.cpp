/*  Sirikata liboh -- Ogre Graphics Plugin
 *  SDLInputManager.cpp
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

#if (_WIN32_WINNT < 0x0501)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <util/Standard.hh>
#include <oh/Platform.hpp>

#include <OgreRenderWindow.h>
#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include <SDL_events.h>

//Thank you Apple:
// /System/Library/Frameworks/CoreServices.framework/Headers/../Frameworks/CarbonCore.framework/Headers/MacTypes.h
#ifdef nil
#undef nil
#endif

#include <task/WorkQueue.hpp>
#include <util/Time.hpp>

#include "SDLInputManager.hpp"
#include "InputEvents.hpp"
#include "SDLInputDevice.hpp"

namespace Sirikata { namespace Input {

IdPair::Primary unknownId("UnknownWindowEvent");

static const IdPair::Primary &getWindowEventId(SDL_WindowEventID sdlId) {
    switch (sdlId) {
      case SDL_WINDOWEVENT_NONE:
        return unknownId;
      case SDL_WINDOWEVENT_SHOWN:
        return WindowEvent::Shown;
      case SDL_WINDOWEVENT_HIDDEN:
        return WindowEvent::Hidden;
      case SDL_WINDOWEVENT_EXPOSED:
        return WindowEvent::Exposed;
      case SDL_WINDOWEVENT_MOVED:
        return WindowEvent::Moved;
      case SDL_WINDOWEVENT_RESIZED:
        return WindowEvent::Resized;
      case SDL_WINDOWEVENT_MINIMIZED:
        return WindowEvent::Minimized;
      case SDL_WINDOWEVENT_MAXIMIZED:
        return WindowEvent::Maximized;
      case SDL_WINDOWEVENT_RESTORED:
        return WindowEvent::Restored;
      case SDL_WINDOWEVENT_ENTER:
        return WindowEvent::MouseEnter;
      case SDL_WINDOWEVENT_LEAVE:
        return WindowEvent::MouseLeave;
      case SDL_WINDOWEVENT_FOCUS_GAINED:
        return WindowEvent::FocusGained;
      case SDL_WINDOWEVENT_FOCUS_LOST:
        return WindowEvent::FocusLost;
      case SDL_WINDOWEVENT_CLOSE:
        return WindowEvent::Quit;
    }
    return unknownId;
}


SDLInputManager::SDLInputManager(unsigned int width,unsigned int height, bool fullscreen, int ogrePixelFormat,bool grabCursor, void *&currentWindow)
: Task::GenEventManager(new Task::ThreadSafeWorkQueue) {
    Ogre::PixelFormat fmt = (Ogre::PixelFormat)ogrePixelFormat;
    mWindowContext=0;
    mWidth = width;
    mHeight = height;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
        SILOG(ogre,error,"Couldn't initialize SDL: "<<SDL_GetError());
    }
#if 1//defined(WIN32)
//||defined(__APPLE__)
    if (currentWindow) {
        mWindowID=mWindowID=SDL_CreateWindowFrom(currentWindow);
        SDL_RaiseWindow(mWindowID);
        SDL_ShowWindow(mWindowID);
#ifdef _WIN32
        //SDL_SetEventFilter(SDL_CompatEventFilter, NULL);
        RAWINPUTDEVICE Rid;
        /* we're telling the window, we want it to report raw input events from mice */
        Rid.usUsagePage = 0x01;
        Rid.usUsage = 0x02;
        Rid.dwFlags = RIDEV_INPUTSINK;
        Rid.hwndTarget = (HWND)currentWindow;
        RegisterRawInputDevices(&Rid, 1, sizeof(Rid));

        static SDL_SysWMinfo pInfo;
        SDL_VERSION(&pInfo.version);
        SDL_GetWindowWMInfo(mWindowID,&pInfo);
        // Also, SDL keeps an internal record of the window size
        //  and position. Because SDL does not own the window, it
        //  missed the WM_POSCHANGED message and has no record of
        //  which is then used to trap the mouse "inside the
        //  window". We have to fake a window-move to allow SDL
        //  to catch up, after which we can safely grab input.
        RECT r;
        GetWindowRect(pInfo.window, &r);
        SetWindowPos(pInfo.window, 0, r.left, r.top, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#endif
    }else {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

        mWindowID = SDL_CreateWindow("Sirikata",0,0,width, height, SDL_WINDOW_OPENGL|(fullscreen?SDL_WINDOW_FULLSCREEN:0));
        SDL_GLContext ctx=mWindowContext=SDL_GL_CreateContext(mWindowID);
        SDL_GL_MakeCurrent(mWindowID,ctx);
        SDL_ShowWindow(mWindowID);
        if (!mWindowID) {
            SILOG(ogre,error,"Couldn't created OpenGL window: "<<SDL_GetError());
        }else {
            SDL_SysWMinfo info;
            memset(&info,0,sizeof(SDL_SysWMinfo));
            SDL_VERSION(&info.version);
            SDL_GetWindowWMInfo(mWindowID,&info);
        SDL_RaiseWindow(mWindowID);
        SDL_ShowWindow(mWindowID);
#ifdef __APPLE__
            currentWindow=(void*)info.data;
#else
#ifdef _WIN32
			currentWindow=(void*)info.window;
#else
            currentWindow=(void*)info.info.x11.window;
#endif
#endif
        }
#endif
    }

    SILOG(ogre,info,"Found "<<SDL_GetNumKeyboards()<<" keyboards, "<<
          SDL_GetNumMice()<<" mice, and "<<
          SDL_NumJoysticks() << " joysticks!");

    mKeys.resize(SDL_GetNumKeyboards());
    mMice.resize(SDL_GetNumMice());
    mJoy.resize(SDL_NumJoysticks());

    // Grab means: lock the mouse inside our window!
/*
    SDL_ShowCursor(SDL_DISABLE);   // SDL_ENABLE to show the mouse cursor (default)
    if (grabCursor)
        SDL_SetWindowGrab(mWindowID,SDL_GRAB_ON); // SDL_GRAB_OFF to not grab input (default)
*/
}
bool SDLInputManager::tick(Time currentTime, Duration frameTime){
#ifndef _WIN32
    SDL_GL_SwapBuffers();
#endif
    SDL_Event event[16];
    bool continueRendering=true;
    while(SDL_PollEvent(&event[0]))
    {
        EventPtr toFire;
        SILOG(ogre,debug,"Event type "<<(int)event->type);
        switch(event->type)
        {
          case SDL_KEYDOWN:
          case SDL_KEYUP:
            mKeys[event->key.which]->fireButton(
                mKeys[event->key.which],
                this, 
                event->key.keysym.sym, 
                (event->key.state == SDL_PRESSED), 
                event->key.keysym.mod);
            break;
          case SDL_MOUSEBUTTONDOWN:
          case SDL_MOUSEBUTTONUP:
            mMice[event->button.which]->fireButton(
                mMice[event->button.which],
                this, 
                event->button.button,
                (event->button.state == SDL_PRESSED),
                (1<<SDL_GetCurrentCursor(event->button.which))>>1);
            mMice[event->button.which]->firePointerClick(
                mMice[event->button.which],
                this, 
                event->button.x,
                event->button.y,
                SDL_GetCurrentCursor(event->button.which),
                event->button.button,
                event->button.state == SDL_PRESSED);
            break;
          case SDL_JOYBUTTONDOWN:
          case SDL_JOYBUTTONUP:
            mJoy[event->jbutton.which]->fireButton(
                mJoy[event->jbutton.which],
                this, 
                event->jbutton.button,
                event->jbutton.state == SDL_PRESSED);
            break;
          case SDL_TEXTINPUT:
            fire(Task::EventPtr(new TextInputEvent(
                                    mKeys[event->text.which], 
                                    event->text.text)));
            break;
          case SDL_MOUSEMOTION:
            mMice[event->motion.which]->fireMotion(
                mMice[event->motion.which],
                this,
                event->motion);
            break;
          case SDL_MOUSEWHEEL:
            mMice[event->wheel.which]->fireWheel(
                mMice[event->wheel.which],
                this, 
                event->wheel.x, 
                event->wheel.y);
            break;
          case SDL_JOYAXISMOTION:
            mJoy[event->wheel.which]->fireAxis(
                mJoy[event->wheel.which],
                this, 
                event->jaxis.axis, 
                AxisValue::fromCentered(event->jaxis.value/32767.));
            break;
          case SDL_JOYHATMOTION:
            mJoy[event->wheel.which]->fireHat(
                mJoy[event->wheel.which],
                this, 
                event->jhat.hat, 
                event->jhat.value);
            break;
          case SDL_JOYBALLMOTION:
            mJoy[event->wheel.which]->fireBall(
                mJoy[event->wheel.which],
                this, 
                event->jball.ball, 
                event->jball.xrel, 
                event->jball.yrel);
            break;
          case SDL_WINDOWEVENT:
            fire(Task::EventPtr(new WindowEvent(
                    getWindowEventId((SDL_WindowEventID)event->window.event),
                    event->window.data1,
                    event->window.data2)));
			if (event->window.event==SDL_WINDOWEVENT_CLOSE) {
                // FIXME: Provide a better means to abort quit events. See below.

                // FIXME: Quit only when the event has been successfully processed
                // and only if nobody has canceled it (i.e. a confirmation dialog)
                SILOG(ogre,debug,"quitting\n");
                continueRendering=false;
            }
            break;

          case SDL_QUIT:
            fire(Task::EventPtr(new WindowEvent(WindowEvent::Quit, -1, -1)));
            // FIXME: Quit only when the event has been successfully processed
            // and only if nobody has canceled it (i.e. a confirmation dialog)
            SILOG(ogre,debug,"quitting\n");
            continueRendering=false;
            break;
          default:
            SILOG(ogre,error,"I don't know what this event is!\n");
        }
    }
    getWorkQueue()->dequeueUntil(Task::AbsTime::now()+Duration::seconds(.01));
    return continueRendering;

}
SDLInputManager::~SDLInputManager(){
    SDL_GL_DeleteContext(mWindowContext);
    SDL_DestroyWindow(mWindowID);
    SDL_Quit();
	delete getWorkQueue();
}

} }
