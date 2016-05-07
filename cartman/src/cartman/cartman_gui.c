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

#include "cartman/cartman_gui.h"
#include "cartman/cartman_input.h"
#include "cartman/cartman_math.h"
#include "cartman/cartman_gfx.h"

//--------------------------------------------------------------------------------------------

namespace Cartman {
GUI_Cursor::GUI_Cursor() :
    _surface(Ego::Graphics::SDL::createSurface(8, 8))
{
    uint32_t col = make_rgb(_surface, 255, 255, 255); // white (255,255,255), fully opaquw
    uint32_t loc = make_rgb(_surface, 24, 24, 24);    // black-grey (24,24,24), fully opaque
    uint32_t clr = make_rgba(_surface, 0, 0, 0, 64);  // black (0,0,0), almost transparent (64)

    // Simple triangle
    SDL_Rect rtmp;
    rtmp.x = 0;
    rtmp.y = 0;
    rtmp.w = 8;
    rtmp.h = 1;
    SDL_FillRect(_surface.get(), &rtmp, loc);

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            if (x + y < 8) Ego::Graphics::SDL::putPixel(_surface, x, y, col);
            else Ego::Graphics::SDL::putPixel(_surface, x, y, clr);
        }
    }
}

GUI_Cursor::~GUI_Cursor()
{
}
}

std::vector<std::shared_ptr<Cartman::Window>> _window_lst;
std::shared_ptr<Cartman::GUI_Cursor> _cursor_2;
ui_state_t ui;

void Cartman::GUI::initialize()
{
    for (size_t i = 0; i < MAXWIN; ++i)
    {
        _window_lst.push_back(std::make_shared<Cartman::Window>());
    }
    _cursor_2 = std::make_shared<Cartman::GUI_Cursor>();
}

void Cartman::GUI::uninitialize()
{
    _cursor_2 = nullptr;
    _window_lst.clear();
}

//--------------------------------------------------------------------------------------------

void do_cursor()
{
    bool left_press;

    // This function implements a mouse cursor
    ui.cur_x = Cartman::Input::get()._mouse.x;
    if ( ui.cur_x < 6 )  ui.cur_x = 6;  if ( ui.cur_x > sdl_scr.width - 6 )  ui.cur_x = sdl_scr.width - 6;
    ui.cur_y = Cartman::Input::get()._mouse.y;
    if ( ui.cur_y < 6 )  ui.cur_y = 6;  if ( ui.cur_y > sdl_scr.height - 6 )  ui.cur_y = sdl_scr.height - 6;

    left_press = CART_BUTTONDOWN(SDL_BUTTON_LEFT);

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
    if (( Cartman::Input::get()._mouse.x >= tlx ) && ( Cartman::Input::get()._mouse.x <= brx ) && ( Cartman::Input::get()._mouse.y >= tly ) && ( Cartman::Input::get()._mouse.y <= bry ) && ( 0 != Cartman::Input::get()._mouse.b ) )
    {
        value = ((( Cartman::Input::get()._mouse.y - tly ) * ( maxvalue - minvalue ) ) / ( bry - tly ) ) + minvalue;
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

void show_name(const std::string& newLoadName, const Ego::Math::Colour4f& textColour) {
	gfx_font_ptr->drawText(newLoadName, 0, sdl_scr.height - 16, textColour);
}

//--------------------------------------------------------------------------------------------

namespace Cartman {
Border::Border(int width, int height)
    : texture(std::make_shared<Ego::OpenGL::Texture>()), width(width), height(height) {}

void Border::loadTexture(const std::string& textureFileName) {
    if (!texture->load(textureFileName, gfx_loadImage(textureFileName))) {
        Log::get().warn("unable to load texture \"%s\".\n", textureFileName.c_str());
    }
}

Window::Window() : on(false), border() {}

void Window::load_window(int id, const std::string& loadname, int x, int y, int bx, int by, int sx, int sy, Uint16 mode, cartman_mpd_t * pmesh) {
    if (NULL == pmesh) pmesh = &mesh;

    this->border.loadTexture(loadname);
    this->border.width = bx;
    this->border.height = by;

    this->x = x;
    this->y = y;

    this->surfacex = sx;
    this->surfacey = sy;
    this->on = true;
    this->mode = mode;
    this->id = id;
    this->pmesh = pmesh;
}

bool Window::isOver(int x, int y) const {
    if (!on) {
        return false;
    }
    if (x < this->x + this->border.width || x > this->x + 2 * this->border.width + this->surfacex) {
        return false;
    }
    if (y < this->y + this->border.height || y > this->y + 2 * this->border.height + this->surfacey) {
        return false;
    }
    return true;
}

void Window::renderBackground() const {
    if (!on) return;
    ogl_draw_sprite_2d(border.texture, x, y, surfacex, surfacey);
}

}

std::shared_ptr<Cartman::Window> Cartman::GUI::findWindow(int x, int y)
{
    std::shared_ptr<Cartman::Window> result = nullptr;
    for (auto &window : _window_lst)
    {
        if (window->isOver(x, y))
        {
            continue;
        }
        result = window;
    }
    return result;
}

void Cartman::GUI::render() {
    for (auto window : _window_lst) {
        window->render();
    }
    for (auto window : _window_lst) {
        window->renderBackground();
    }
}
