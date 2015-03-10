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

#include "cartman/cartman_gui.h"
#include "cartman/cartman_input.h"
#include "cartman/cartman_math.h"
#include "cartman/cartman_gfx.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<Cartman_Window>> _window_lst;

void Cartman_GUI_initialize()
{
    for (size_t i = 0; i < MAXWIN; ++i)
    {
        _window_lst.push_back(std::make_shared<Cartman_Window>());
    }
}

void Cartman_GUI_uninitialize()
{
    _window_lst.clear();
}

ui_state_t ui;

SDL_Surface * bmpcursor = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void do_cursor()
{
    bool left_press;

    // This function implements a mouse cursor
    ui.cur_x = Cartman_Input::get()._mouse.x;
    if ( ui.cur_x < 6 )  ui.cur_x = 6;  if ( ui.cur_x > sdl_scr.x - 6 )  ui.cur_x = sdl_scr.x - 6;
    ui.cur_y = Cartman_Input::get()._mouse.y;
    if ( ui.cur_y < 6 )  ui.cur_y = 6;  if ( ui.cur_y > sdl_scr.y - 6 )  ui.cur_y = sdl_scr.y - 6;

    left_press = MOUSE_PRESSED( SDL_BUTTON_LEFT );

    ui.clicked = false;
    if ( left_press && !ui.pressed )
    {
        ui.clicked = true;
    }
    ui.pressed = left_press;
}

//--------------------------------------------------------------------------------------------
void draw_slider( int tlx, int tly, int brx, int bry, int* pvalue, int minvalue, int maxvalue )
{
    int cnt;
    int value;

    float color[4] = {1, 1, 1, 1};

    // Pick a new value
    value = *pvalue;
    if (( Cartman_Input::get()._mouse.x >= tlx ) && ( Cartman_Input::get()._mouse.x <= brx ) && ( Cartman_Input::get()._mouse.y >= tly ) && ( Cartman_Input::get()._mouse.y <= bry ) && ( 0 != Cartman_Input::get()._mouse.b ) )
    {
        value = ((( Cartman_Input::get()._mouse.y - tly ) * ( maxvalue - minvalue ) ) / ( bry - tly ) ) + minvalue;
    }
    if ( value < minvalue ) value = minvalue;
    if ( value > maxvalue ) value = maxvalue;
    *pvalue = value;

    // Draw it
    if (( maxvalue - minvalue ) != 0 )
    {
        float amount;
        cnt = (( value - minvalue ) * 20 / ( maxvalue - minvalue ) ) + 11;

        amount = ( value - minvalue ) / ( float )( maxvalue - minvalue );

        ogl_draw_box_xy( tlx, amount *( bry - tly ) + tly, 0, brx - tlx + 1, 5, color );
    }

}

//--------------------------------------------------------------------------------------------
void show_name( const char *newloadname, SDL_Color fnt_color )
{
    fnt_drawText_OGL_immediate( gfx_font_ptr, fnt_color, 0, sdl_scr.y - 16, "%s", newloadname );
}

//--------------------------------------------------------------------------------------------
void load_window(std::shared_ptr<Cartman_Window> pwin, int id, char *loadname, int x, int y, int bx, int by, int sx, int sy, Uint16 mode, cartman_mpd_t * pmesh)
{
    if ( NULL == pwin ) return;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( INVALID_GL_ID == oglx_texture_load( &( pwin->tex ), loadname, INVALID_KEY ) )
    {
        log_warning( "Cannot load \"%s\".\n", loadname );
    }

    pwin->x        = x;
    pwin->y        = y;
    pwin->borderx  = bx;
    pwin->bordery  = by;
    pwin->surfacex = sx;
    pwin->surfacey = sy;
    pwin->on       = true;
    pwin->mode     = mode;
    pwin->id       = id;
    pwin->pmesh    = pmesh;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool Cartman_Window::isOver(int x, int y) const
{
    if (!on)
    {
        return false;
    }
    if (x < this->x + this->borderx || x > this->x + 2 * this->borderx + this->surfacex)
    {
        return false;
    }
    if (y < this->y + this->borderx || y > this->y + 2 * this->bordery + this->surfacey)
    {
        return false;
    }
    return true;
}

std::shared_ptr<Cartman_Window> find_window(int x, int y)
{
    std::shared_ptr<Cartman_Window> result = nullptr;
    for (auto window : _window_lst)
    {
        if (window->isOver(x, y))
        {
            continue;
        }
        result = window;
    }
    return result;
}
