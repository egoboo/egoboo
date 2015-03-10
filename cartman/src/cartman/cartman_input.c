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

#include "cartman/cartman_input.h"

#include "cartman/cartman_gui.h"
#include "cartman/cartman_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if 0
Cartman_Input g_input;
#endif

Cartman_Input *_singleton = nullptr;

Cartman_Input& Cartman_Input::get()
{
    if (!_singleton)
    {
        throw std::logic_error("input system not initialized");
    }
    return *_singleton;
}

void Cartman_Input::initialize()
{
    if (!_singleton)
    {
        _singleton = new Cartman_Input();
    }

}

void Cartman_Input::uninitialize()
{
    if (_singleton)
    {
        delete _singleton;
        _singleton = nullptr;
    }
}

Cartman_Input::Cartman_Input() :
    _mouse(), _keyboard()
{
}

Cartman_Input::~Cartman_Input()
{
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool check_input_mouse( SDL_Event * pevt );
static bool check_input_keyboard( SDL_Event * pevt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
bool check_keys( Uint32 resolution )
{
    static int last_tick = -1;
    int tick;

    // 20 ticks per key delay
    tick = SDL_GetTicks();
    if ( tick < last_tick + resolution ) return false;
    last_tick = tick;

    if (Cartman_Input::get()._keyboard.delay > 0)
    {
        Cartman_Input::get()._keyboard.delay--;
        return false;
    }

    if (!Cartman_Input::get()._keyboard.on)
    {
        return false;
    }
    if (Cartman_Input::get()._keyboard.needs_update)
    {
        Cartman_Input::get()._keyboard.sdlbuffer = SDL_GetKeyState(&(Cartman_Input::get()._keyboard.count));
        Cartman_Input::get()._keyboard.needs_update = false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void check_input()
{
    // ZZ> This function gets all the current player input states
    SDL_Event evt;

    while ( SDL_PollEvent( &evt ) )
    {
        if ( NULL == egolib_console_handle_events( &evt ) )
        {
            Cartman_Input::get()._keyboard.override = true;
            continue;
        }
        else
        {
            Cartman_Input::get()._keyboard.override = false;
        }

        switch ( evt.type )
        {
            case SDL_ACTIVEEVENT:

                if ( HAS_BITS( evt.active.state, SDL_APPMOUSEFOCUS ) )
                {
                    Cartman_Input::get()._mouse.on = ( 1 == evt.active.gain );
                }

                if ( HAS_BITS( evt.active.state, SDL_APPINPUTFOCUS ) )
                {
                    Cartman_Input::get()._keyboard.on = (1 == evt.active.gain);
                    if (Cartman_Input::get()._keyboard.on) Cartman_Input::get()._keyboard.needs_update = true;
                }

                if ( HAS_BITS( evt.active.state, SDL_APPACTIVE ) )
                {
                    if ( 1 != evt.active.gain )
                    {
                        Cartman_Input::get()._mouse.on = false;
                        Cartman_Input::get()._keyboard.on = false;
                    }
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEMOTION:
                check_input_mouse( &evt );
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                check_input_keyboard( &evt );
                break;
        }
    }
};

//--------------------------------------------------------------------------------------------
bool check_input_mouse(SDL_Event *event)
{
    bool handled = false;

    if (!event || !Cartman_Input::get()._mouse.on)
    {
        return false;
    }

    if (0 == Cartman_Input::get()._mouse.b)
    {
        Cartman_Input::get()._mouse.drag = false;
        Cartman_Input::get()._mouse.drag_begin = false;

        // set mdata??
    }

    switch (event->type)
    {
        case SDL_MOUSEBUTTONDOWN:
            switch (event->button.button)
            {
                case SDL_BUTTON_LEFT:
                    Cartman_Input::get()._mouse.b |= SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    Cartman_Input::get()._mouse.b |= SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    Cartman_Input::get()._mouse.b |= SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = true;
            handled = true;
            break;

        case SDL_MOUSEBUTTONUP:
            switch (event->button.button)
            {
                case SDL_BUTTON_LEFT:
                    Cartman_Input::get()._mouse.b &= ~SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    Cartman_Input::get()._mouse.b &= ~SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    Cartman_Input::get()._mouse.b &= ~SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = false;
            handled = true;
            break;

        case SDL_MOUSEMOTION:
            Cartman_Input::get()._mouse.b = event->motion.state;
            if (Cartman_Input::get()._mouse.drag)
            {
                if (0 != Cartman_Input::get()._mouse.b)
                {
                    Cartman_Input::get()._mouse.brx = event->motion.x;
                    Cartman_Input::get()._mouse.bry = event->motion.y;
                }
                else
                {
                    Cartman_Input::get()._mouse.drag = false;
                }
            }

            if (Cartman_Input::get()._mouse.relative)
            {
                Cartman_Input::get()._mouse.cx = event->motion.xrel;
                Cartman_Input::get()._mouse.cy = event->motion.yrel;
            }
            else
            {
                Cartman_Input::get()._mouse.x = event->motion.x;
                Cartman_Input::get()._mouse.y = event->motion.y;
            }
            break;
    }

    if (0 != Cartman_Input::get()._mouse.b)
    {
        if (Cartman_Input::get()._mouse.drag_begin)
        {
            // start dragging
            Cartman_Input::get()._mouse.drag = true;
        }
        else if (!Cartman_Input::get()._mouse.drag)
        {
            // set the dragging to begin the next mouse time the mouse moves
            Cartman_Input::get()._mouse.drag_begin = true;

            // initialize the drag rect
            Cartman_Input::get()._mouse.tlx = Cartman_Input::get()._mouse.x;
            Cartman_Input::get()._mouse.tly = Cartman_Input::get()._mouse.y;

            Cartman_Input::get()._mouse.brx = Cartman_Input::get()._mouse.x;
            Cartman_Input::get()._mouse.bry = Cartman_Input::get()._mouse.y;

            // set the drag window
            Cartman_Input::get()._mouse.drag_window = find_window(Cartman_Input::get()._mouse.x, Cartman_Input::get()._mouse.y);
            Cartman_Input::get()._mouse.drag_mode = (NULL == Cartman_Input::get()._mouse.drag_window)
                                     ? 0 : Cartman_Input::get()._mouse.drag_mode;
        }
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
bool check_input_keyboard(SDL_Event *event)
{
    bool handled = false;

    if (NULL == event || !Cartman_Input::get()._keyboard.on) return false;

    switch (event->type)
    {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            Cartman_Input::get()._keyboard.state = event->key.state;
            Cartman_Input::get()._keyboard.mod = event->key.keysym.mod;
            Cartman_Input::get()._keyboard.needs_update = true;
            handled = true;
            break;
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Cartman_Mouse::Cartman_Mouse() :
    on(true),
    x(0), y(0), x_old(0), y_old(0),
    b(0), relative(false),
    cx(0), cy(0),
    drag(false), drag_begin(false),
    drag_window(nullptr),
    drag_mode(0),
    tlx(0), tly(0), brx(0), bry(0)
{
}

#if 0
Cartman_Mouse *Cartman_Mouse::ctor(Cartman_Mouse *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr != self");
    }
    memset(self, 0, sizeof(Cartman_Mouse));
    self->on = true;
    return self;
}
#endif

Cartman_Mouse::~Cartman_Mouse()
{
}

#if 0
void Cartman_Mouse::dtor(Cartman_Mouse *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr != self");
    }
    memset(self, 0, sizeof(Cartman_Mouse));
}
#endif

void Cartman_Mouse::update(Cartman_Mouse *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr != self");
    }
    if (!self->on) return;

    self->x_old = self->x;
    self->y_old = self->y;
}

bool Cartman_Mouse::isButtonDown(Cartman_Mouse *self, int button)
{
    return HAS_BITS(self->b, SDL_BUTTON(button));
}

Cartman_Keyboard::Cartman_Keyboard() :
    on(true), needs_update(true),
    override(false), count(0), delay(0),
    sdlbuffer(nullptr), state(0), mod(KMOD_NONE)
{}

#if 0
Cartman_Keyboard *Cartman_Keyboard::ctor(Cartman_Keyboard *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr != self");
    }
    memset(self, 0, sizeof(Cartman_Keyboard));
    self->on = true;
    self->needs_update = true;
    return self;
}
#endif

Cartman_Keyboard::~Cartman_Keyboard()
{}

#if 0
void Cartman_Keyboard::dtor(Cartman_Keyboard *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr != self");
    }
    memset(self, 0, sizeof(Cartman_Keyboard));
}
#endif

bool Cartman_Keyboard::isKeyDown(Cartman_Keyboard *self,int key)
{
    if (!self->on || self->override || (key >= self->count)) return false;
    if (!self->sdlbuffer) return false;
    return self->sdlbuffer[key];
}

bool Cartman_Keyboard::isModDown(Cartman_Keyboard *self, int mod)
{
    if (!self->on || self->override) return false;
    if (!self->sdlbuffer) return false;
    return 0 != (self->state & mod);
}

bool Cartman_Keyboard::isDown(Cartman_Keyboard *self, int key, int mod)
{
    return Cartman_Keyboard::isKeyDown(self, key)
        && Cartman_Keyboard::isModDown(self, mod);
}
