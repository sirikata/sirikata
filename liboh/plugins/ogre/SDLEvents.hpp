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
#include "SDL_events.h"
namespace Sirikata { namespace Graphics {
namespace SDL {
using Sirikata::Task::IdPair;
class KeyEvent : public Task::Event {
public:
    SDL_KeyboardEvent mEvent;
    static IdPair::Secondary getSecondaryId(SDLKey keycode, int whichKeyboard) {
        if (whichKeyboard==0)
            return IdPair::Secondary(keycode);
        char whichkbd[3]={'\0','\0','\0'};
        if (whichKeyboard<10)
            whichkbd[0]=whichKeyboard+'0';
        else {
            whichkbd[0]=(whichKeyboard/10)+'0';
            whichkbd[1]=(whichKeyboard%10)+'0';
        }
        return IdPair::Secondary(whichkbd,keycode);
    }
    KeyEvent(const Task::IdPair::Primary&primary, 
             const SDL_KeyboardEvent&data)
        : Event(IdPair(primary,
                       getSecondaryId(data.keysym.sym,data.which))),mEvent(data){
        
    }
};
class KeyPressed :public KeyEvent {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("KeyPressed");
        return retval;
    }
    KeyPressed(const SDL_KeyboardEvent&data):KeyEvent(getEventId(),data){}
    virtual ~KeyPressed(){}
};
class KeyReleased :public KeyEvent {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("KeyReleased");
        return retval;
    }
    KeyReleased(const SDL_KeyboardEvent&data):KeyEvent(getEventId(),data){}
    virtual ~KeyReleased(){}
};
class KeyChanged :public KeyEvent {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("KeyChanged");
        return retval;
    }

    KeyChanged(const SDL_KeyboardEvent&data):KeyEvent(getEventId(),data){}
    virtual ~KeyChanged(){}
};

class KeyDown :public KeyEvent {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("KeyDown");
        return retval;
    }
    KeyDown(const SDL_KeyboardEvent&data):KeyEvent(getEventId(),data){}
    virtual ~KeyDown(){}
};


template <class SDL_InputType> class ButtonEvent : public Task::Event {
public:
    static IdPair::Secondary getSecondaryId(int button, int whichDevice) {
        if (whichDevice==0)
            return IdPair::Secondary(button);
        char whichkbd[3]={'\0','\0','\0'};
        if (whichDevice<10)
            whichkbd[0]=whichDevice+'0';
        else {
            whichkbd[0]=(whichDevice/10)+'0';
            whichkbd[1]=(whichDevice%10)+'0';
        }
        return IdPair::Secondary(whichkbd,button);
    }
    SDL_InputType mEvent;
    ButtonEvent(const IdPair::Primary&primary, 
                int button, 
                const SDL_InputType&data)
        : Event(IdPair(primary,
                       getSecondaryId(button,data.which))),mEvent(data){
        
    }
};
class MouseButtonPressed :public ButtonEvent<SDL_MouseButtonEvent> {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("MouseButtonPressed");
        return retval;
    }
    MouseButtonPressed(const SDL_MouseButtonEvent&data):ButtonEvent<SDL_MouseButtonEvent>(getEventId(),data.button,data){}
    virtual ~MouseButtonPressed(){}
};
class MouseButtonReleased :public ButtonEvent<SDL_MouseButtonEvent> {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("MouseButtonReleased");
        return retval;
    }
    MouseButtonReleased(const SDL_MouseButtonEvent&data):ButtonEvent<SDL_MouseButtonEvent>(getEventId(),data.button,data){}
    virtual ~MouseButtonReleased(){}
};
class MouseButtonChanged :public ButtonEvent<SDL_MouseButtonEvent> {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("MouseButtonChanged");
        return retval;
    }

    MouseButtonChanged(const SDL_MouseButtonEvent&data):ButtonEvent<SDL_MouseButtonEvent>(getEventId(),data.button,data){}
    virtual ~MouseButtonChanged(){}
};

class MouseButtonDown :public ButtonEvent<SDL_MouseButtonEvent> {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("MouseButtonDown");
        return retval;
    }
    MouseButtonDown(const SDL_MouseButtonEvent&data):ButtonEvent<SDL_MouseButtonEvent>(getEventId(),data.button,data){}
    virtual ~MouseButtonDown(){}
};

class JoyButtonPressed :public ButtonEvent<SDL_JoyButtonEvent> {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("JoyButtonPressed");
        return retval;
    }
    JoyButtonPressed(const SDL_JoyButtonEvent&data):ButtonEvent<SDL_JoyButtonEvent>(getEventId(),data.button,data){}
    virtual ~JoyButtonPressed(){}
};
class JoyButtonReleased :public ButtonEvent<SDL_JoyButtonEvent> {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("JoyButtonReleased");
        return retval;
    }
    JoyButtonReleased(const SDL_JoyButtonEvent&data):ButtonEvent<SDL_JoyButtonEvent>(getEventId(),data.button,data){}
    virtual ~JoyButtonReleased(){}
};
class JoyButtonChanged :public ButtonEvent<SDL_JoyButtonEvent> {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("JoyButtonChanged");
        return retval;
    }

    JoyButtonChanged(const SDL_JoyButtonEvent&data):ButtonEvent<SDL_JoyButtonEvent>(getEventId(),data.button,data){}
    virtual ~JoyButtonChanged(){}
};

class JoyButtonDown :public ButtonEvent<SDL_JoyButtonEvent> {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("JoyButtonDown");
        return retval;
    }
    JoyButtonDown(const SDL_JoyButtonEvent&data):ButtonEvent<SDL_JoyButtonEvent>(getEventId(),data.button,data){}
    virtual ~JoyButtonDown(){}
};


class JoyHatDown :public ButtonEvent<SDL_JoyHatEvent> {
public:
    const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("JoyHatDown");
        return retval;
    }
    JoyHatDown(const SDL_JoyHatEvent&data):ButtonEvent<SDL_JoyHatEvent>(getEventId(),data.hat,data){}
    virtual ~JoyHatDown(){}
};


class TextInputType {
public:
    const Task::IdPair::Primary &operator()()const {
        static Task::IdPair::Primary retval("TextInput");
        return retval;
    }
    Task::IdPair::Secondary operator()(const SDL_TextInputEvent&data)const {
        return Task::IdPair::Secondary(data.which);
    }
};

class MouseMotionType {
public:
    const Task::IdPair::Primary &operator()()const {
        static Task::IdPair::Primary retval("MouseMotion");
        return retval;
    }
    Task::IdPair::Secondary operator()(const SDL_MouseMotionEvent&data)const {
        return Task::IdPair::Secondary(data.which);
    }
};

class MouseWheelType {
public:
    const Task::IdPair::Primary &operator()()const {
        static Task::IdPair::Primary retval("MouseWheel");
        return retval;
    }
    Task::IdPair::Secondary operator()(const SDL_MouseWheelEvent&data)const {
        return Task::IdPair::Secondary(data.which);
    }
};


class JoyAxisType {
public:
    const Task::IdPair::Primary &operator()()const {
        static Task::IdPair::Primary retval("JoyAxis");
        return retval;
    }
    Task::IdPair::Secondary operator()(const SDL_JoyAxisEvent&data)const {
        return Task::IdPair::Secondary(data.which);
    }
};

class JoyBallType {
public:
    const Task::IdPair::Primary &operator()()const {
        static Task::IdPair::Primary retval("JoyBall");
        return retval;
    }
    Task::IdPair::Secondary operator()(const SDL_JoyBallEvent&data)const {
        return Task::IdPair::Secondary(data.which);
    }
};


class WindowType {
public:
    const Task::IdPair::Primary &operator()()const {
        static Task::IdPair::Primary retval("WindowEvent");
        return retval;
    }
    Task::IdPair::Secondary operator()(const SDL_WindowEvent&data)const {
        return Task::IdPair::Secondary(data.event);
    }
};


template <class SDL_EventType, class TypeFunctor> 
class SDLEvent:public Task::Event {
public:
    SDL_EventType mEvent;
    SDLEvent(const SDL_EventType&sdl)
      : Event(IdPair(TypeFunctor()(),
                     TypeFunctor()(sdl)))
       ,mEvent(sdl) {
    }
};
typedef SDLEvent<SDL_TextInputEvent,TextInputType> TextInput;
typedef SDLEvent<SDL_MouseMotionEvent,MouseMotionType> MouseMotion;
typedef SDLEvent<SDL_MouseWheelEvent,MouseWheelType> MouseWheel;
typedef SDLEvent<SDL_JoyAxisEvent,JoyAxisType> JoyAxis;
typedef SDLEvent<SDL_JoyBallEvent,JoyBallType> JoyBall;
typedef SDLEvent<SDL_WindowEvent,WindowType> WindowChange;
}
} }
