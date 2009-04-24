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
#include <oh/Platform.hpp>

#include <OgreRenderWindow.h>
#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include <util/Time.hpp>
#include "SDLInputManager.hpp"
#include "SDLEvents.hpp"

namespace Sirikata { namespace Graphics {
class PressedKeys {public:
    std::vector <std::tr1::unordered_map<SDLKey,SDL_KeyboardEvent> >mPressed;

};
class PressedMouseButtons {public:
    std::vector <std::tr1::unordered_map<int,SDL_MouseButtonEvent> >mPressed;

};
class PressedJoyButtons {public:
    std::vector <std::tr1::unordered_map<int,SDL_JoyButtonEvent> >mPressed;

};


SDLInputManager::SDLInputManager(unsigned int width,unsigned int height, bool fullscreen, const Ogre::PixelFormat&fmt,bool grabCursor, void *&currentWindow){
    mWindowContext=0;
    mPressedKeys=new PressedKeys;
    mPressedMouseButtons=new PressedMouseButtons;
    mPressedJoyButtons=new PressedJoyButtons;
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
        SDL_GetWindowWMInfo(windowID,&pInfo);
        // Also, SDL keeps an internal record of the window size
        //  and position. Because SDL does not own the window, it
        //  missed the WM_POSCHANGED message and has no record of
        //  either size or position. It defaults to {0, 0, 0, 0},
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
            currentWindow=(void*)info.info.x11.window;
#endif
        }
#endif
    }   
    // Grab means: lock the mouse inside our window!
    SDL_ShowCursor(SDL_DISABLE);   // SDL_ENABLE to show the mouse cursor (default)
    if (grabCursor) 
        SDL_SetWindowGrab(mWindowID,SDL_GRAB_ON); // SDL_GRAB_OFF to not grab input (default)
    
}
bool SDLInputManager::tick(Time currentTime, Duration frameTime){
    using namespace Sirikata::Graphics::SDL;
#ifndef _WIN32
    SDL_GL_SwapBuffers();
#endif
    SDL_Event event[16];
    bool continueRendering=true;
    if (0) {
        size_t i;
        for (i=0;i<mPressedKeys->mPressed.size();++i){
            for (std::tr1::unordered_map<SDLKey,SDL_KeyboardEvent>::iterator j=mPressedKeys->mPressed[i].begin();
                 j!=mPressedKeys->mPressed[i].end();
                 ++j){
                fire(Task::EventPtr(new KeyDown(j->second)));    
            }
        }
        for (i=0;i<mPressedMouseButtons->mPressed.size();++i){
            for (std::tr1::unordered_map<int,SDL_MouseButtonEvent>::iterator j=mPressedMouseButtons->mPressed[i].begin();
                 j!=mPressedMouseButtons->mPressed[i].end();
                 ++j){
                fire(Task::EventPtr(new MouseButtonDown(j->second)));    
            }
        }
        for (i=0;i<mPressedJoyButtons->mPressed.size();++i){
            for (std::tr1::unordered_map<int,SDL_JoyButtonEvent>::iterator j=mPressedJoyButtons->mPressed[i].begin();
                 j!=mPressedJoyButtons->mPressed[i].end();
                 ++j){
                fire(Task::EventPtr(new JoyButtonDown(j->second)));    
            }
        }
    }
    while(SDL_PollEvent(&event[0]))
    {
       SILOG(ogre,debug,"Event type "<<(int)event->type);
        switch(event->type)
        { 
          case SDL_KEYDOWN:
            fire(Task::EventPtr(new KeyPressed(event->key)));
            fire(Task::EventPtr(new KeyChanged(event->key)));
            fire(Task::EventPtr(new KeyDown(event->key)));
            if (mPressedKeys->mPressed.size()<=event->key.which)
                mPressedKeys->mPressed.resize(event->key.which+1);
            mPressedKeys->mPressed[event->key.which][event->key.keysym.sym]=event->key;
            break;
          case SDL_KEYUP:
            if (event->key.which<mPressedKeys->mPressed.size()) {
                std::tr1::unordered_map<SDLKey,SDL_KeyboardEvent>::iterator where=mPressedKeys->mPressed[event->key.which].find(event->key.keysym.sym);
                if (where==mPressedKeys->mPressed[event->key.which].end()) {
                    SILOG(ogre,error,"Double release of key with keycode "<<event->key.keysym.sym<<" on keyboard "<<event->key.which);
                }else {
                    mPressedKeys->mPressed[event->key.which].erase(where);
                }
            } else {
                SILOG(ogre,error,"Release of key "<<event->key.keysym.sym << " from new keyboard "<<event->key.which); 
            }        
            fire(Task::EventPtr(new KeyChanged(event->key)));
            fire(Task::EventPtr(new KeyReleased(event->key)));
            break;

          case SDL_MOUSEBUTTONDOWN:
            fire(Task::EventPtr(new MouseButtonPressed(event->button)));
            fire(Task::EventPtr(new MouseButtonChanged(event->button)));
            fire(Task::EventPtr(new MouseButtonDown(event->button)));
            if (mPressedMouseButtons->mPressed.size()<=event->button.which)
                mPressedMouseButtons->mPressed.resize(event->button.which+1);
            mPressedMouseButtons->mPressed[event->button.which][event->button.button]=event->button;
            break;
          case SDL_MOUSEBUTTONUP:
            if (event->button.which<mPressedMouseButtons->mPressed.size()) {
                std::tr1::unordered_map<int,SDL_MouseButtonEvent>::iterator where=mPressedMouseButtons->mPressed[event->button.which].find(event->button.button);
                if (where==mPressedMouseButtons->mPressed[event->button.which].end()) {
                    SILOG(ogre,error,"Double release of button with buttoncode "<<event->button.button<<" on buttonboard "<<event->button.which);
                }else {
                    mPressedMouseButtons->mPressed[event->button.which].erase(where);
                }
            } else {
                SILOG(ogre,error,"Release of button "<<event->button.button << " from new mouse "<<event->button.which);
            }        
            fire(Task::EventPtr(new MouseButtonChanged(event->button)));
            fire(Task::EventPtr(new MouseButtonReleased(event->button)));
            break;

          case SDL_JOYBUTTONDOWN:
            fire(Task::EventPtr(new JoyButtonPressed(event->jbutton)));
            fire(Task::EventPtr(new JoyButtonChanged(event->jbutton)));
            fire(Task::EventPtr(new JoyButtonDown(event->jbutton)));
            if (mPressedJoyButtons->mPressed.size()<=event->jbutton.which)
                mPressedJoyButtons->mPressed.resize(event->jbutton.which+1);
            mPressedJoyButtons->mPressed[event->jbutton.which][event->jbutton.button]=event->jbutton;
            break;
          case SDL_JOYBUTTONUP:
            if (event->jbutton.which<mPressedJoyButtons->mPressed.size()) {
                std::tr1::unordered_map<int,SDL_JoyButtonEvent>::iterator where=mPressedJoyButtons->mPressed[event->jbutton.which].find(event->jbutton.button);
                if (where==mPressedJoyButtons->mPressed[event->jbutton.which].end()) {
                    SILOG(ogre,error,"Double release of button with buttoncode "<<event->jbutton.button<<" on buttonboard "<<event->jbutton.which);
                }else {
                    mPressedJoyButtons->mPressed[event->jbutton.which].erase(where);
                }
            } else {   
                SILOG(ogre,error,"Release of button "<<event->jbutton.button << " from new joy "<<event->jbutton.which);
            } 
    
            fire(Task::EventPtr(new JoyButtonChanged(event->jbutton)));
            fire(Task::EventPtr(new JoyButtonReleased(event->jbutton)));
            break;
            
          case SDL_TEXTINPUT:
            fire(Task::EventPtr(new TextInput(event->text)));
            break;
          case SDL_MOUSEMOTION: 
            fire(Task::EventPtr(new MouseMotion(event->motion)));
            break;
          case SDL_MOUSEWHEEL: 
            fire(Task::EventPtr(new MouseWheel(event->wheel)));
            break;
          case SDL_JOYAXISMOTION: 
            fire(Task::EventPtr(new JoyAxis(event->jaxis)));
            break;
          case SDL_JOYHATMOTION: 
            fire(Task::EventPtr(new JoyHatDown(event->jhat)));
            break;
          case SDL_JOYBALLMOTION:
            fire(Task::EventPtr(new JoyBall(event->jball)));
            break;
            
          case SDL_WINDOWEVENT:
            fire(Task::EventPtr(new WindowChange(event->window)));
			if (event->window.event==13) {
              SILOG(ogre,debug,"quitting\n");  
              continueRendering=false;
			}
            break;
            
          case SDL_QUIT:
            SILOG(ogre,debug,"quitting\n");  
            continueRendering=false;
            break;
          default:
            SILOG(ogre,error,"I don't know what this event is!\n");
        }
    }
    temporary_processEventQueue(Task::AbsTime::now()+Duration::seconds(.01));
    return continueRendering;
     
}
SDLInputManager::~SDLInputManager(){
    delete mPressedKeys;    
    delete mPressedMouseButtons;
    delete mPressedJoyButtons;
    SDL_GL_DeleteContext(mWindowContext);
    SDL_DestroyWindow(mWindowID);
    SDL_Quit();

}
} }
