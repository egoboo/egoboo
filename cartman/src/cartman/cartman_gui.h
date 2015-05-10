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
#include "cartman/cartman_map.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_Font;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_ui_state;
typedef struct s_ui_state ui_state_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXWIN 8            // Number of windows

#define DEFAULT_WINDOW_W 200
#define DEFAULT_WINDOW_H 200
#define DEFAULT_RESOLUTION 8

#define SCREEN_TO_REAL(VAL,CAM,ZOOM) ( (VAL) * (float)DEFAULT_RESOLUTION * TILE_FSIZE  / (float)DEFAULT_WINDOW_W / (ZOOM) + (CAM) );
#define REAL_TO_SCREEN(VAL,CAM,ZOOM) ( ( (VAL) - (CAM) ) / (float)DEFAULT_RESOLUTION / TILE_FSIZE * (float)DEFAULT_WINDOW_W * (ZOOM)  );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct Cartman_Window
{
    Uint8             on;       // Draw it?

    oglx_texture_t    *tex;      // Window images

    // Window position
    int               x;
    int               y;

    // Window border size
    int               borderx;
    int               bordery;

    // Window surface size
    int               surfacex;
    int               surfacey;

    // window data
    int id;                // unique window id
    Uint16 mode;           // display mode bits
    cartman_mpd_t *pmesh;  // which mesh

    /**
     * @brief
     *  Get if the cursor is over this window.
     * @param self
     *  the window
     * @param x, y
     *  the cursor position (world coordinates)
     * @return
     *  @a true if the mouse is over this window and the window is "on", @a false otherwise
     */
    bool isOver(int x,int y) const;
};

//--------------------------------------------------------------------------------------------
struct s_ui_state
{
    /// @brief The cursor position.
    /// @todo Use Point2i and rename to cursorPosition;
    int cur_x, cur_y;

    bool pressed;                //
    bool clicked;                //
    bool pending_click;

    bool GrabMouse;
    bool HideMouse;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct Cartman_GUI_Cursor;

namespace Cartman
{
    struct GUI
    {
        static void initialize();
        static void uninitialize();
        static std::shared_ptr<Cartman_Window> findWindow(int x, int y);
    };
}


extern std::vector<std::shared_ptr<Cartman_Window>> _window_lst;
extern std::shared_ptr<Cartman_GUI_Cursor> _cursor_2;
extern ui_state_t ui;

/**
 * @brief
 *  An image cursor.
 * @todo
 *  The SDL surface is not used as it seems.
 */
struct Cartman_GUI_Cursor
{
    /// The cursor image.
    SDL_Surface *_surface;
    /// Create a cursor.
    Cartman_GUI_Cursor();
    /// Destroy a cursor.
    virtual ~Cartman_GUI_Cursor();
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void do_cursor();
void draw_slider( int tlx, int tly, int brx, int bry, int* pvalue, int minvalue, int maxvalue );
void show_name( const char *newloadname, SDL_Color fnt_color );
void load_window(std::shared_ptr<Cartman_Window> pwin, int id, const char *loadname, int mapx, int mapy, int bx, int by, int sx, int sy, Uint16 mode, cartman_mpd_t * pmesh);

