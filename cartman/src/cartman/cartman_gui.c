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
#include "egolib/Image/ImageManager.hpp"
#include "egolib/Image/SDL_Image_Extensions.h"

//--------------------------------------------------------------------------------------------

ui_state_t ui;

//--------------------------------------------------------------------------------------------

namespace Cartman { namespace Gui {

Cursor::Cursor() :
    _surface(Ego::ImageManager::get().createImage(8, 8)) {
	auto col = Ego::Math::Colour3b::white(); // opaque (255) white (255, 255, 255)
	auto loc = Ego::Math::Colour3b(24, 24, 24); // opaque (255) black-grey (24, 24, 24)
	auto clr = Ego::Math::Colour4b(Ego::Math::Colour3b::black(), 64); // almost transparent (64) black (0, 0, 0)

	auto rtmp = Rectangle2f(Point2f(0, 0), Point2f(8, 1));
    Ego::fill(_surface.get(), Ego::Math::Colour3b(24, 24, 24), rtmp);

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (x + y < 8) Ego::set_pixel(_surface.get(), col, { x, y });
            else Ego::set_pixel(_surface.get(), clr, { x, y });
        }
    }
}

Cursor::~Cursor() {}

} } // namespace Cartman::Gui

//--------------------------------------------------------------------------------------------

namespace Cartman {
namespace Gui {

Manager::Manager() {
    for (size_t i = 0; i < MAXWIN; ++i) {
       windowList.push_back(std::make_shared<Window>());
    }
    cursor = std::make_shared<Cursor>();
}

Manager::~Manager() {
    cursor = nullptr;
    windowList.clear();
}

std::shared_ptr<Window> Manager::findWindow(int x, int y) {
    std::shared_ptr<Window> result = nullptr;
    for (auto& window : windowList) {
        if (window->isOver(Point2f(x, y))) {
            continue;
        }
        result = window;
    }
    return result;
}

void Manager::render() {
    for (auto& window : windowList) {
        window->render();
    }
    for (auto& window : windowList) {
        window->renderBackground();
    }
}

} }  // namespace Cartman::Gui

//--------------------------------------------------------------------------------------------

namespace Cartman { namespace Gui {

Border::Border(Vector2f size)
    : texture(Ego::Renderer::get().createTexture()), size(size) {}

void Border::loadTexture(const std::string& textureFileName) {
    if (!texture->load(textureFileName, gfx_loadImage(textureFileName))) {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to load texture ",
                                         "`", textureFileName, "`", Log::EndOfEntry);
    }
}

} } // namespace Cartman::Gui

//--------------------------------------------------------------------------------------------

namespace Cartman { namespace Gui {

Window::Window() : on(false), border() {}

void Window::load_window(int id, const std::string& loadname, Point2f position, Vector2f borderSize, Vector2f size, uint16_t mode, cartman_mpd_t * pmesh) {
    if (NULL == pmesh) pmesh = &mesh;

    this->border.loadTexture(loadname);
    this->border.size = borderSize;

    this->position = position;

    this->size = size;
    this->on = true;
    this->mode = mode;
    this->id = id;
    this->pmesh = pmesh;
}

bool Window::isOver(Point2f p) const {
    if (!on) {
        return false;
    }
    /// @todo Shouldn't this be <tt>position.x() + border.size.width()</tt> (and
    /// <tt>position.x() + size.width() - 2 * borderSize.width()</tt>?
    if (p.x() < this->position.x() + this->border.size.x() ||
        p.x() > this->position.x() + 2 * this->border.size.x() + this->size.x()) {
        return false;
    }
    /// @todo Shouldn't this be <tt>position.y() + border.size.height()</tt> (and
    /// <tt>position.y() + size.height() - 2 * borderSize.height()</tt>?
    if (p.y() < this->position.y() + this->border.size.y() ||
        p.y() > this->position.y() + 2 * this->border.size.y() + this->size.y()) {
        return false;
    }
    return true;
}

void Window::renderBackground() const {
    if (!on) return;
    ogl_draw_sprite_2d(border.texture, position.x(), position.y(), size.x(), size.y());
}

} } // namespace Cartman::Gui

//--------------------------------------------------------------------------------------------

void do_cursor() {
    bool left_press;
    auto windowSize = Ego::GraphicsSystem::get().window->getSize();
    // This function implements a mouse cursor
    ui.cursorPosition.x() = Ego::Math::constrain((float)Cartman::Input::get()._mouse.position.x(), 6.0f, windowSize.x() - 6);
    ui.cursorPosition.y() = Ego::Math::constrain((float)Cartman::Input::get()._mouse.position.y(), 6.0f, windowSize.y() - 6);

    left_press = CART_BUTTONDOWN(SDL_BUTTON_LEFT);

    ui.clicked = false;
    if (left_press && !ui.pressed) {
        ui.clicked = true;
    }
    ui.pressed = left_press;
}

void draw_slider(int tlx, int tly, int brx, int bry, int* pvalue, int minvalue, int maxvalue) {
    int cnt;
    int value;

    Ego::Math::Colour4f colour = Ego::Math::Colour4f::white();

    // Pick a new value
    value = *pvalue;
    if ((Cartman::Input::get()._mouse.position.x() >= tlx) && (Cartman::Input::get()._mouse.position.x() <= brx) && (Cartman::Input::get()._mouse.position.y() >= tly) && (Cartman::Input::get()._mouse.position.y() <= bry) && (0 != Cartman::Input::get()._mouse.b)) {
        value = (((Cartman::Input::get()._mouse.position.y() - tly) * (maxvalue - minvalue)) / (bry - tly)) + minvalue;
    }
    if (value < minvalue) value = minvalue;
    if (value > maxvalue) value = maxvalue;
    *pvalue = value;

    // Draw it
    if ((maxvalue - minvalue) != 0) {
        float amount;
        cnt = ((value - minvalue) * 20 / (maxvalue - minvalue)) + 11;

        amount = (value - minvalue) / (float)(maxvalue - minvalue);

        ogl_draw_box_xy(tlx, amount *(bry - tly) + tly, 0, brx - tlx + 1, 5, colour);
    }
}

void show_name(const std::string& newLoadName, const Ego::Math::Colour4f& textColour) {
    auto windowSize = Ego::GraphicsSystem::get().window->getSize();
    gfx_font_ptr->drawText(newLoadName, 0, windowSize.y() - 16, textColour);
}