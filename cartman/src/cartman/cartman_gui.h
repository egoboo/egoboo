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
#include "cartman/cartman_map.h"

//--------------------------------------------------------------------------------------------

/// @brief The number of windows.
#define MAXWIN 8


#define DEFAULT_RESOLUTION 8

//--------------------------------------------------------------------------------------------

namespace Cartman { namespace Gui {

struct Border {
    /// @brief The border texutre.
    std::shared_ptr<Ego::Texture> texture;

    /// @brief The size of the border. Default is <tt>Size2i()</tt>.
    Vector2f size;

    /**
     * @brief Construct this border.
     * @param size the border size. Default is <tt>Size2i()</tt>.
     */
    Border(Vector2f size = Vector2f());

    /**
     * @brief Load a texture to be used as the border texture.
     * @param pathname the pathname of the texture
     */
    void loadTexture(const std::string& pathname);

}; // struct Border

} } // namespace Cartman::Gui

//--------------------------------------------------------------------------------------------

namespace Cartman { namespace Gui {

struct Window {
    /// @brief The default window width.
    static constexpr int defaultWidth = 200;
    /// @brief The default window height.
    static constexpr int defaultHeight = 200;
    /// @brief Is the window enabled?
    /// @todo Should be <tt>bool enabled</tt>.
    uint8_t on;

    /// @brief The window position.
    Point2f position;

    /// @brief The window border.
    Border border;

    /// @brief The window size.
	Vector2f size;

    /// @brief Unique ID of this window.
    int id;
    /// @brief The display mode of this window.
    uint16_t mode;
    /// @brief The mesh,
    cartman_mpd_t *pmesh;  // which mesh

    /**
     * @brief Construct this window.
     * @todo Add corresponding destructor.
     */
    Window();

    /**
     * @brief
     *  Get if the cursor is over this window.
     * @param self
     *  the window
     * @param p
     *  the cursor position (world coordinates)
     * @return
     *  @a true if the mouse is over this window and the window is "on", @a false otherwise
     */
    bool isOver(Point2f p) const;

    void load_window(int id, const std::string& loadname, Point2f position, Vector2f borderSize, Vector2f size, uint16_t mode, cartman_mpd_t * pmesh);
    /**
     * @brief
     *  Render the window.
     */
    void render();
    void renderBackground() const;

}; // struct Manager

} } // namespace Cartman::Gui

//--------------------------------------------------------------------------------------------

namespace Cartman { namespace Gui {

struct Manager : public id::singleton<Manager> {
protected:
    friend id::default_new_functor<Manager>;
    friend id::default_delete_functor<Manager>;
    Manager();
    ~Manager();
public:
    std::shared_ptr<Window> findWindow(int x, int y);
    void render();
    std::vector<std::shared_ptr<Cartman::Gui::Window>> windowList;
    std::shared_ptr<Cartman::Gui::Cursor> cursor;
}; // struct Manager

} } // namespace Cartman::Gui


//--------------------------------------------------------------------------------------------

struct ui_state_t {
    /// @brief The cursor position.
    Point2f cursorPosition;

    bool pressed;                //
    bool clicked;                //
    bool pending_click;

    bool GrabMouse;
    bool HideMouse;
};

extern ui_state_t ui;

//--------------------------------------------------------------------------------------------

namespace Cartman {
namespace Gui {
/**
 * @brief An image cursor.
 * @todo The SDL surface is not used as it seems.
 */
struct Cursor {
    /// @brief The cursor image.
    std::shared_ptr<SDL_Surface> _surface;

    /// @brief Construct this cursor.
    Cursor();

    /// @brief Destruct this cursor.
    virtual ~Cursor();

}; // struct Cursor

} } // namespace Cartman::Gui

//--------------------------------------------------------------------------------------------

void do_cursor();
void draw_slider( int tlx, int tly, int brx, int bry, int* pvalue, int minvalue, int maxvalue );
void show_name(const std::string& newLoadName, const Ego::Math::Colour4f& textColour);

inline float SCREEN_TO_REAL(float VAL, float CAM, float ZOOM) {
    const auto gridSize = Info<float>::Grid::Size();
    const auto defaultWindowWidth = (float)Cartman::Gui::Window::defaultWidth;
    return (VAL * (float)DEFAULT_RESOLUTION * gridSize / defaultWindowWidth / ZOOM + CAM);
}
inline float REAL_TO_SCREEN(float VAL, float CAM, float ZOOM) {
    const auto gridSize = Info<float>::Grid::Size();
    const auto defaultWindowWidth = (float)Cartman::Gui::Window::defaultWidth;
    return ((VAL - CAM) / (float)DEFAULT_RESOLUTION / gridSize * defaultWindowWidth * ZOOM);
}
