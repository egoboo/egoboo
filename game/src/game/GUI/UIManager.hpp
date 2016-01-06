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

/// @file game/GUI/UIManager.hpp
/// @details The UIManager contains utilities for the GUI system and stores various shared resources
///		     and properties for GUIComponents.
/// @author Johan Jansen
#pragma once

#include "game/egoboo.h"

//Forward declarations
namespace Ego { class Font; }

class UIManager : public Id::NonCopyable
{
public:
    UIManager();

    ~UIManager();

    enum UIFontType : uint8_t
    {
        FONT_DEFAULT,
        FONT_FLOATING_TEXT,
        FONT_DEBUG,
        FONT_GAME,
        NR_OF_UI_FONTS
    };

    /**
     * @brief
     *  A vertex buffer to render components.
     */
    std::shared_ptr<Ego::VertexBuffer> _vertexBuffer;

    /**
    * @todo: REMOVE these functions
    **/
    std::shared_ptr<Ego::Font> getDefaultFont() const {return getFont(FONT_DEFAULT);}
    std::shared_ptr<Ego::Font> getFloatingTextFont() const {return getFont(FONT_FLOATING_TEXT);}
    std::shared_ptr<Ego::Font> getDebugFont() const {return getFont(FONT_DEBUG);}
    std::shared_ptr<Ego::Font> getGameFont() const {return getFont(FONT_GAME);}

    /**
    * @return
    *   The Font loaded and cached by the UIManager
    **/
    std::shared_ptr<Ego::Font> getFont(const UIFontType type) const { return _fonts[type]; }

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
    void drawImage(const std::shared_ptr<const Ego::Texture>& img, float x, float y, float width, float height, const Ego::Colour4f& tint = Ego::Colour4f::white());

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
    * @param startX
    *   x screen coordinate
    * @param startY
    *   y screen coordinate
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
    float drawBitmapFontString(const float startX, const float startY, const std::string &text, const uint32_t maxWidth, const float alpha);

    //TODO: make private
    void drawBitmapGlyph(int fonttype, float xPos, float yPos, const float alpha);
    void drawQuad2D(const std::shared_ptr<const Ego::Texture>& texture, const ego_frect_t& scr_rect, const ego_frect_t& tx_rect, const bool useAlpha, const Ego::Colour4f& tint = Ego::Colour4f::white());

private:

private:
    std::array<std::shared_ptr<Ego::Font>, NR_OF_UI_FONTS> _fonts;
    int _renderSemaphore;
    std::shared_ptr<Ego::Texture> _bitmapFontTexture;
    Ego::VertexBuffer _textureQuadVertexBuffer;
};
