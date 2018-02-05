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

Cartman_MouseData mdata = {-1};

//--------------------------------------------------------------------------------------------

Cartman::Input::Input() :
    _mouse(), _keyboard()
{
}

Cartman::Input::~Input()
{
}

//--------------------------------------------------------------------------------------------
bool check_keys( uint32_t resolution )
{
    static int last_tick = -1;
    int tick;

    // 20 ticks per key delay
    tick = Time::now<Time::Unit::Ticks>();
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
        Cartman::Input::get()._keyboard.sdlbuffer = SDL_GetKeyboardState(&(Cartman::Input::get()._keyboard.count));
        Cartman::Input::get()._keyboard.needs_update = false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void Cartman::Input::checkInput()
{
    // ZZ> This function gets all the current player input states
    SDL_Event evt;

    while (SDL_PollEvent(&evt))
    {
        if (!Ego::Core::Console::get().handle_event(&evt))
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
            case SDL_WINDOWEVENT:
                switch (evt.window.event) {
                    case SDL_WINDOWEVENT_HIDDEN:
                        _mouse.on = false;
                        _keyboard.on = false;
                        break;
                    case SDL_WINDOWEVENT_ENTER:
                        _mouse.on = true;
                        break;
                    case SDL_WINDOWEVENT_LEAVE:
                        _mouse.on = false;
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        _keyboard.on = true;
                        _keyboard.needs_update = true;
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        _keyboard.on = false;
                        break;
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
                _mouse.position = Point2f(event->motion.x, event->motion.y);
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
            _mouse.tlx = _mouse.position.x();
            _mouse.tly = _mouse.position.y();

            _mouse.brx = _mouse.position.x();
            _mouse.bry = _mouse.position.y();

            // set the drag window
            _mouse.drag_window = Cartman::Gui::Manager::get().findWindow(_mouse.position.x(), _mouse.position.y());
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

Cartman::Mouse::Mouse() :
    on(true),
    position(), positionOld(),
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

void Cartman::Mouse::update()
{
    if (!on) return;

    positionOld = position;
}

bool Cartman::Mouse::isButtonDown(int button)
{
    return HAS_BITS(b, SDL_BUTTON(button));
}

Cartman::Keyboard::Keyboard() :
    on(true), needs_update(true),
    override(false), count(0), delay(0),
    sdlbuffer(nullptr), state(0), mod(KMOD_NONE)
{}

Cartman::Keyboard::~Keyboard()
{}

bool Cartman::Keyboard::isKeyDown(int key)
{
    SDL_Scancode actualKey = SDL_GetScancodeFromKey(key);
    if (!on || override || (actualKey >= count)) return false;
    if (!sdlbuffer) return false;
    return sdlbuffer[actualKey];
}

bool Cartman::Keyboard::isModDown(int mod)
{
    if (!on || override) return false;
    if (!sdlbuffer) return false;
    return 0 != (this->mod & mod);
}

bool Cartman::Keyboard::isDown(int key, int mod)
{
    return isKeyDown(key)
        && isModDown(mod);
}
