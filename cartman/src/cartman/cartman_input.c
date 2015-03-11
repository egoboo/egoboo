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

Cartman::Input *Cartman::Input::_singleton = nullptr;

Cartman::Input& Cartman::Input::get()
{
    if (!_singleton)
    {
        throw std::logic_error("input system not initialized");
    }
    return *_singleton;
}

void Cartman::Input::initialize()
{
    if (!_singleton)
    {
        _singleton = new Input();
    }

}

void Cartman::Input::uninitialize()
{
    if (_singleton)
    {
        delete _singleton;
        _singleton = nullptr;
    }
}

Cartman::Input::Input() :
    _mouse(), _keyboard()
{
}

Cartman::Input::~Input()
{
}

//--------------------------------------------------------------------------------------------
bool check_keys( Uint32 resolution )
{
    static int last_tick = -1;
    int tick;

    // 20 ticks per key delay
    tick = SDL_GetTicks();
    if ( tick < last_tick + resolution ) return false;
    last_tick = tick;

    if (Cartman::Input::get()._keyboard.delay > 0)
    {
        Cartman::Input::get()._keyboard.delay--;
        return false;
    }

    if (!Cartman::Input::get()._keyboard.on)
    {
        return false;
    }
    if (Cartman::Input::get()._keyboard.needs_update)
    {
        Cartman::Input::get()._keyboard.sdlbuffer = SDL_GetKeyState(&(Cartman::Input::get()._keyboard.count));
        Cartman::Input::get()._keyboard.needs_update = false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void Cartman::Input::checkInput()
{
    // ZZ> This function gets all the current player input states
    SDL_Event evt;

    while ( SDL_PollEvent( &evt ) )
    {
        if ( NULL == egolib_console_handle_events( &evt ) )
        {
            _keyboard.override = true;
            continue;
        }
        else
        {
            _keyboard.override = false;
        }

        switch (evt.type)
        {
            case SDL_ACTIVEEVENT:

                if (HAS_BITS( evt.active.state, SDL_APPMOUSEFOCUS))
                {
                    _mouse.on = ( 1 == evt.active.gain );
                }

                if ( HAS_BITS( evt.active.state, SDL_APPINPUTFOCUS ) )
                {
                    _keyboard.on = (1 == evt.active.gain);
                    if (_keyboard.on) _keyboard.needs_update = true;
                }

                if (HAS_BITS(evt.active.state, SDL_APPACTIVE))
                {
                    if ( 1 != evt.active.gain )
                    {
                        _mouse.on = false;
                        _keyboard.on = false;
                    }
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEMOTION:
                onMouse( &evt );
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                onKeyboard( &evt );
                break;
        }
    }
};

//--------------------------------------------------------------------------------------------
bool Cartman::Input::onMouse(SDL_Event *event)
{
    bool handled = false;

    if (!event || !_mouse.on)
    {
        return false;
    }

    if (0 == _mouse.b)
    {
        _mouse.drag = false;
        _mouse.drag_begin = false;

        // set mdata??
    }
    switch (event->type)
    {
        case SDL_MOUSEBUTTONDOWN:
            switch (event->button.button)
            {
                case SDL_BUTTON_LEFT:
                    _mouse.b |= SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    _mouse.b |= SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    _mouse.b |= SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = true;
            handled = true;
            break;
        case SDL_MOUSEBUTTONUP:
            switch (event->button.button)
            {
                case SDL_BUTTON_LEFT:
                    _mouse.b &= ~SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    _mouse.b &= ~SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    _mouse.b &= ~SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = false;
            handled = true;
            break;

        case SDL_MOUSEMOTION:
            _mouse.b = event->motion.state;
            if (_mouse.drag)
            {
                if (0 != _mouse.b)
                {
                    _mouse.brx = event->motion.x;
                    _mouse.bry = event->motion.y;
                }
                else
                {
                    _mouse.drag = false;
                }
            }

            if (_mouse.relative)
            {
                _mouse.cx = event->motion.xrel;
                _mouse.cy = event->motion.yrel;
            }
            else
            {
                _mouse.x = event->motion.x;
                _mouse.y = event->motion.y;
            }
            break;
    }

    if (0 != _mouse.b)
    {
        if (_mouse.drag_begin)
        {
            // start dragging
            _mouse.drag = true;
        }
        else if (!_mouse.drag)
        {
            // set the dragging to begin the next mouse time the mouse moves
            _mouse.drag_begin = true;

            // initialize the drag rect
            _mouse.tlx = _mouse.x;
            _mouse.tly = _mouse.y;

            _mouse.brx = _mouse.x;
            _mouse.bry = _mouse.y;

            // set the drag window
            _mouse.drag_window = Cartman::GUI::findWindow(_mouse.x, _mouse.y);
            _mouse.drag_mode = (NULL == _mouse.drag_window)
                             ? 0 : _mouse.drag_mode;
        }
    }
    return handled;
}

//--------------------------------------------------------------------------------------------
bool Cartman::Input::onKeyboard(SDL_Event *event)
{
    bool handled = false;

    if (NULL == event || !_keyboard.on) return false;

    switch (event->type)
    {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            _keyboard.state = event->key.state;
            _keyboard.mod = event->key.keysym.mod;
            _keyboard.needs_update = true;
            handled = true;
            break;
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Cartman::Mouse::Mouse() :
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

Cartman::Mouse::~Mouse()
{
}

void Cartman::Mouse::update(Cartman::Mouse *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr != self");
    }
    if (!self->on) return;

    self->x_old = self->x;
    self->y_old = self->y;
}

bool Cartman::Mouse::isButtonDown(Cartman::Mouse *self, int button)
{
    return HAS_BITS(self->b, SDL_BUTTON(button));
}

Cartman::Keyboard::Keyboard() :
    on(true), needs_update(true),
    override(false), count(0), delay(0),
    sdlbuffer(nullptr), state(0), mod(KMOD_NONE)
{}

Cartman::Keyboard::~Keyboard()
{}

bool Cartman::Keyboard::isKeyDown(Cartman::Keyboard *self,int key)
{
    if (!self->on || self->override || (key >= self->count)) return false;
    if (!self->sdlbuffer) return false;
    return self->sdlbuffer[key];
}

bool Cartman::Keyboard::isModDown(Cartman::Keyboard *self, int mod)
{
    if (!self->on || self->override) return false;
    if (!self->sdlbuffer) return false;
    return 0 != (self->state & mod);
}

bool Cartman::Keyboard::isDown(Cartman::Keyboard *self, int key, int mod)
{
    return Cartman::Keyboard::isKeyDown(self, key)
        && Cartman::Keyboard::isModDown(self, mod);
}
