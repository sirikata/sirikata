#include <oh/Platform.hpp>
#include <OgreRenderWindow.h>
#include <SDL.h>
#include <SDL_video.h>
#include <util/Time.hpp>
#include "SDLInputManager.hpp"

namespace Sirikata { namespace Graphics {
SDLInputManager::SDLInputManager(unsigned int width,unsigned int height, bool fullscreen, const Ogre::PixelFormat&fmt,bool grabCursor){
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
        SILOG(ogre,error,"Couldn't initialize SDL: "<<SDL_GetError());
    }

#ifdef WIN32
   if (!fullscreen)
   {
      // the window is not fullscreen:

      // This is necessary to allow the window to move
      //  on WIN32 systems. Without this, the window resets
      //  to the smallest possible size after moving.
      SDL_SetVideoMode(width, height, 0, 0); // first 0: BitPerPixel, 
                                             // second 0: flags (fullscreen/...)
                                             // neither are needed as Ogre sets these
   }

   static SDL_SysWMinfo pInfo;
   SDL_VERSION(&pInfo.version);
   SDL_GetWMInfo(&pInfo);

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
#else
   SDL_Init(SDL_INIT_VIDEO);
   SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
   SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
   SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
   SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

   SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

   mScreen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL|(fullscreen?SDL_FULLSCREEN:0));
   if (!mScreen) {
       SILOG(ogre,error,"Couldn't created OpenGL window: "<<SDL_GetError());
   }
#endif

   // Grab means: lock the mouse inside our window!
   SDL_ShowCursor(SDL_DISABLE);   // SDL_ENABLE to show the mouse cursor (default)
   if (grabCursor) 
       SDL_WM_GrabInput(SDL_GRAB_ON); // SDL_GRAB_OFF to not grab input (default)

}
bool SDLInputManager::tick(Time currentTime, Duration frameTime){
#ifndef _WIN32
    SDL_GL_SwapBuffers();
#endif
    SDL_Event event[16];
    bool continueRendering=true;

    while(SDL_PollEvent(&event[0]))
    {
       
     switch(event[0].type)
     { 
        case SDL_KEYDOWN:         
          SILOG(ogre,insane,"Oh! Key press\n");
        break;
        case SDL_MOUSEMOTION: 
          SILOG(ogre,insane,"MouseMotion\n");
        break;
        case SDL_QUIT:
          SILOG(ogre,debug,"quitting\n");  
          continueRendering=false;
        break;
        default:
          SILOG(ogre,error,"I don't know what this event is!\n");
     }
     if (!continueRendering) {
         printf("AH");
     }
    }
    return continueRendering;
     
}

} }
