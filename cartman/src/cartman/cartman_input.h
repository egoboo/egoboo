#pragma once

//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egolib/egolib.h"

#include "cartman/cartman_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct Cartman_Window;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

namespace Cartman
{
    struct Mouse
    {
        bool on;

        int   x, y;
        int   x_old, y_old;
        int   b;

        bool relative;
        int   cx, cy;

        bool drag, drag_begin;
        std::shared_ptr<Cartman_Window> drag_window;
        int drag_mode;
        int tlx, tly, brx, bry;

        Mouse();
        virtual ~Mouse();

        static void update(Mouse *self);
        /**
        * @brief
        *  Get if a mouse button is down.
        * @param self
        *  the mouse
        * @param button
        *  the button
        * @return
        *  @a true if the mouse button is down
        */
        static bool isButtonDown(Mouse *self, int button);
    };
}


//--------------------------------------------------------------------------------------------

namespace Cartman
{
    struct Keyboard
    {
        bool on;                //< Is the keyboard alive?
        bool override;          //< has the console overridden the keyboard?
        int count;
        int delay;

        bool needs_update;
        Uint8 *sdlbuffer;
        Uint8 state;
        SDLMod mod;
        Keyboard();
        virtual ~Keyboard();
        static bool isKeyDown(Cartman::Keyboard *self, int key);
        static bool isModDown(Cartman::Keyboard *self, int mod);
        static bool isDown(Cartman::Keyboard *self, int key, int mod);
    };
}



//KMOD_NONE No modifiers applicable
//KMOD_NUM Numlock is down
//KMOD_CAPS Capslock is down
//KMOD_LCTRL Left Control is down
//KMOD_RCTRL Right Control is down
//KMOD_RSHIFT Right Shift is down
//KMOD_LSHIFT Left Shift is down
//KMOD_RALT Right Alt is down
//KMOD_LALT Left Alt is down
//KMOD_CTRL A Control key is down
//KMOD_SHIFT A Shift key is down
//KMOD_ALT An Alt key is down

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

namespace Cartman
{
    struct Input
    {
    private:
        static Input *_singleton;
    public:
        /**
         * @brief
         *  Get the input system singleton.
         * @return
         *  the input system singleton
         * @throw std::logic_error
         *  if the input system is not initialized
         */
        static Input& get();
        /**
         * @brief
         *  Initialize the input system.
         */
        static void initialize();
        /**
         * @brief
         *  Uninitialize the input system-
         */
        static void uninitialize();

        Mouse _mouse;
        Keyboard _keyboard;
        Input();
        virtual ~Input();
        void checkInput();
    protected:
        bool onMouse(SDL_Event * event);
        bool onKeyboard(SDL_Event *event);

    };
}


bool check_keys(Uint32 resolution);


#define CART_BUTTONDOWN(button) \
    Cartman::Mouse::isButtonDown(&(Cartman::Input::get()._mouse),button)

#define CART_KEYDOWN(key) \
    Cartman::Keyboard::isKeyDown(&(Cartman::Input::get()._keyboard),key)

#define CART_KEYMOD(mod) \
    Cartman::Keyboard::isModDown(&(Cartman::Input::get()._keyboard),mod)

#define CART_KEYDOWN_MOD(key,mod) \
    Cartman::Keyboard::isDown(&(Cartman::Input::get()._keyboard),key,mod)
