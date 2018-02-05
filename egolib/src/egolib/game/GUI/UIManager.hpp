//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/game/GUI/UIManager.hpp
/// @details The UIManager contains utilities for the GUI system and stores various shared resources
///		     and properties for GUI components.
/// @author Johan Jansen
#pragma once

#include "egolib/game/egoboo.h"

// Forward declarations.
namespace Ego {
class Font; 
namespace GUI {
class Material;
}
}

namespace Ego {
namespace GUI {

class UIManager : private idlib::non_copyable {
public:
    UIManager();

    ~UIManager();

    enum UIFontType : uint8_t {
        FONT_DEFAULT,
        FONT_FLOATING_TEXT,
        FONT_DEBUG,
        FONT_GAME,
        NR_OF_UI_FONTS
    };


    /**
    * @todo: REMOVE these functions
    **/
    std::shared_ptr<Font> getDefaultFont() const { return getFont(FONT_DEFAULT); }
    std::shared_ptr<Font> getFloatingTextFont() const { return getFont(FONT_FLOATING_TEXT); }
    std::shared_ptr<Font> getDebugFont() const { return getFont(FONT_DEBUG); }
    std::shared_ptr<Font> getGameFont() const { return getFont(FONT_GAME); }

    /**
    * @return
    *   The Font loaded and cached by the UIManager
    **/
    std::shared_ptr<Font> getFont(const UIFontType type) const { return _fonts[type]; }

    /**
     * @return
     *   Current screen resolution width
     */
    int getScreenWidth() const;

    /**
     * @return
     *   Current screen resolution height
     */
    int getScreenHeight() const;

    /**
     * @brief
     *  Used by the ComponentContainer before rendering GUI components
     */
    void beginRenderUI();

    /**
     * @brief
     *   Tell the rendering system we are finished drawing GUI components
     */
    void endRenderUI();

    /**
     * @brief
     *  Convinience function to draw a 2D image
     */
    void drawImage(const Point2f& position, const Vector2f& size, const std::shared_ptr<const Material>& material);

    /**
    * @brief
    *   dumps the current screen (GL context) to a new bitmap file
    *   right now it dumps it to whatever the current directory is
    * @return
    *   true if successful, false otherwise
    **/
    bool dumpScreenshot();

    /**
    * @brief
    *   Renders a text string using bitmap font
    * @param start
    *   the screen position at which to render the string
    * @param text
    *   the text string to render
    * @param maxWidth
    *   Maximum x width of the string, if it is bigger the function
    *   will wrap to the next line
    * @param alpha
    *   Value between 1.0f (opaque) to 0.0f (transparent)
    * @return
    *   Y screen coordinate of the line below where the text was rendered
    **/
    float drawBitmapFontString(const Vector2f& start, const std::string &text, const uint32_t maxWidth = 0, const float alpha = 1.0f);

    /**
    * @brief
    *   Fill a solid coloured rectangle
    * @param rectangle
    *   the rectangle
    * @param useAlpha
    *   enable or disable alpha channel
    * @param tint
    *   colour of the rectangle (including alpha channel)
    **/
    void fillRectangle(const Rectangle2f& rectangle, const bool useAlpha, const Math::Colour4f& tint = Math::Colour4f::white());

    /**
     * @brief Render a 2D quad.
     * @param source the source rectangle (texture coordinates)
     * @param target the target rectangle (screen coordinates)
     * @param material the material
     */
    void drawQuad2D(const Rectangle2f& scr_rect, const Rectangle2f& tx_rect, const std::shared_ptr<const Material>& material);
    void drawQuad2D(const Rectangle2f& scr_rect, const ego_frect_t& tx_rect, const std::shared_ptr<const Material>& material);

    /// Draw a 2D quad.
    /// @param target the target rectangle in screen coordinates
    /// @param source the source rectangle in texture coordinates
    void drawQuad2d(const Rectangle2f& target, const Rectangle2f& source);
    /// Draw a 2D quadriliteral.
    /// @param target the target rectangle in screen coordinates
    /// @remark The texture coordinate rectangle is ((0,0),(1,1)) if the material is textured.
    void drawQuad2d(const Rectangle2f& target);
private:
    /**
    * @brief
    *   Render a single bitmap glyph at the specified location
    **/
    void drawBitmapGlyph(int fonttype, const Vector2f& position, const float alpha);

public:
    /// @brief Vertex descriptor & vertex buffer to render components.
    VertexDescriptor _vertexDescriptor;
    std::shared_ptr<VertexBuffer> _vertexBuffer;

private:
    std::array<std::shared_ptr<Font>, NR_OF_UI_FONTS> _fonts;
    int _renderSemaphore;
    std::shared_ptr<Texture> _bitmapFontTexture;

    /// @brief Vertex descriptor & vertex buffer to render textured quadriliterals.
    VertexDescriptor _textureQuadVertexDescriptor;
    VertexBuffer _textureQuadVertexBuffer;
};

} // namespace GUI
} // namespace Ego