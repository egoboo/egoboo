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

#pragma once

#include "cartman/cartman_typedef.h"

//--------------------------------------------------------------------------------------------

namespace Cartman
{
    struct Mouse
    {
        bool on;

		Point2f position;
		Point2f positionOld;
        int   b;

        bool relative;
        int   cx, cy;

        bool drag, drag_begin;
        std::shared_ptr<Cartman::Gui::Window> drag_window;
        int drag_mode;
        int tlx, tly, brx, bry;

        Mouse();
        virtual ~Mouse();

        void update();
        /**
         * @brief
         *  Get if a mouse button is down.
         * @param button
         *  the button
         * @return
         *  @a true if the mouse button is down
         */
        bool isButtonDown(int button);
    };
}

#include "cartman/cartman_select.h"

struct Cartman_MouseData {
    // click/drag window
    int             win_id;
    uint16_t        win_mode;
    cartman_mpd_t * win_mesh;
    float           win_mpos_x;
    float           win_mpos_y;
    int             win_fan;
    int             win_fan_x, win_fan_y;
    select_lst_t    win_select;

    // click data
    uint8_t   type;       // Tile fantype
    uint8_t   fx;         // Tile effects
    uint8_t   tx;         // Tile texture
    uint8_t   upper;      // Tile upper bits
    uint16_t  presser;    // Random add for tiles

                        // Rectangle drawing
    int     rect_draw;   // draw it
    int     rect_drag;   // which window id
    int     rect_done;   // which window id
    float   rect_x0;     //
    float   rect_x1;     //
    float   rect_y0;     //
    float   rect_y1;     //
    float   rect_z0;     //
    float   rect_z1;     //

    static Cartman_MouseData *ctor(Cartman_MouseData *self);
};

extern Cartman_MouseData mdata;

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
        const uint8_t *sdlbuffer;
        uint8_t state;
        uint16_t mod;
        Keyboard();
        virtual ~Keyboard();
        bool isKeyDown(int key);
        bool isModDown(int mod);
        bool isDown(int key, int mod);
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

namespace Cartman
{
    struct Input : id::singleton<Input>
    {
    public:
        Mouse _mouse;
        Keyboard _keyboard;
    private:
        friend id::default_new_functor<Input>;
        friend id::default_delete_functor<Input>;
        Input();
        virtual ~Input();
    public:
        void checkInput();
    protected:
        bool onMouse(SDL_Event * event);
        bool onKeyboard(SDL_Event *event);

    };
}


bool check_keys(uint32_t resolution);


#define CART_BUTTONDOWN(button) \
    Cartman::Input::get()._mouse.isButtonDown(button)

#define CART_KEYDOWN(key) \
    Cartman::Input::get()._keyboard.isKeyDown(key)

#define CART_KEYMOD(mod) \
    Cartman::Input::get()._keyboard.isModDown(mod)

#define CART_KEYDOWN_MOD(key,mod) \
    Cartman::Input::get()._keyboard.isDown(key,mod)
