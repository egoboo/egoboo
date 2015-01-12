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

#include "cartman_gui.h"
#include "cartman_input.h"
#include "cartman_math.h"
#include "cartman_gfx.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

window_t window_lst[MAXWIN];
ui_state_t ui;

SDL_Surface * bmpcursor = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void do_cursor()
{
    bool left_press;

    // This function implements a mouse cursor
    ui.cur_x = mos.x;  if ( ui.cur_x < 6 )  ui.cur_x = 6;  if ( ui.cur_x > sdl_scr.x - 6 )  ui.cur_x = sdl_scr.x - 6;
    ui.cur_y = mos.y;  if ( ui.cur_y < 6 )  ui.cur_y = 6;  if ( ui.cur_y > sdl_scr.y - 6 )  ui.cur_y = sdl_scr.y - 6;

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
    if (( mos.x >= tlx ) && ( mos.x <= brx ) && ( mos.y >= tly ) && ( mos.y <= bry ) && ( 0 != mos.b ) )
    {
        value = ((( mos.y - tly ) * ( maxvalue - minvalue ) ) / ( bry - tly ) ) + minvalue;
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
void load_window( window_t * pwin, int id, char *loadname, int x, int y, int bx, int by, int sx, int sy, Uint16 mode, cartman_mpd_t * pmesh )
{
    if ( NULL == pwin ) return;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( INVALID_GL_ID == oglx_texture_Load( &( pwin->tex ), loadname, INVALID_KEY ) )
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
window_t * find_window( int x, int y )
{
    window_t * retval = NULL;
    int cnt;

    for ( cnt = 0; cnt < MAXWIN; cnt++ )
    {
        window_t * pwin = window_lst + cnt;

        if ( !pwin->on ) continue;

        if ( x < pwin->x + pwin->borderx || x > pwin->x + 2*pwin->borderx + pwin->surfacex )
        {
            continue;
        }

        if ( y < pwin->y + pwin->borderx || y > pwin->y + 2*pwin->bordery + pwin->surfacey )
        {
            continue;
        }

        retval = pwin;
    }

    return retval;
}
