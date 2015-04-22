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

/// @file    egolib/Graphics/Font.hpp
/// @details TrueType font drawing functionality.  Uses the SDL_ttf module
///          to do its business. This depends on SDL_ttf and OpenGL.

#pragma once

#include "egolib/typedef.h"
#include "egolib/Math/Colour4f.hpp"

struct oglx_texture_t;

namespace Ego
{

class Font final
{
public:

    Font(const std::string &fileName, int pointSize);

    ~Font();
        
    /**
     * @brief
     *  Get the size of the given text that only has one line.
     * @param text
     *  the text to draw
     * @param[out] width
     *  the width of the text box (may be nullptr)
     * @param[out] height
     *  the height of the text box (may be nullptr)
     */
    void getTextSize(const std::string &text, int *width, int *height) const;
        
    /**
     * @brief
     *  Get the size of the given text that potentially has multiple lines.
     * @param text
     *  the text to draw
     * @param spacing
     *  the spacing (in pixels) between each line
     * @param[out] width
     *  the width of the text box (may be nullptr)
     * @param[out] height
     *  the height of the text box (may be nullptr)
     */
    void getTextBoxSize(const std::string &text, int spacing, int *width, int *height) const;
        
        
    /**
     * @brief
     *  Draw text that only has one line to a texture.
     * @param tex
     *  the texture to draw to
     * @param text
     *  the text to draw
     * @param colour
     *  the colour of the text (default white)
     */
    void drawTextToTexture(oglx_texture_t *tex, const std::string &text,
                           const Ego::Math::Colour3f &color = Ego::Math::Colour3f::white()) const;
        
#if 0
    /**
     * @brief
     *  Draw text that potentially has multiple lines to a texture.
     * @note this isn't implemented yet
     * @param tex
     *  the texture to draw to
     * @param text
     *  the text to draw
     * @param width
     *  the maximum width (NOT IMPLEMENTED)
     * @param height
     *  the maximum height (NOT IMPLEMENTED)
     * @param spacing
     *  the spacing (in pixels) between each line
     * @param colour
     *  the colour of the text (default white)
     */
    void drawTextBoxToTexture(oglx_texture_t *tex, const std::string &text, int width, int height, int spacing,
                              const Ego::Math::Colour3f &color = Ego::Math::Colour3f::white()) const;
#endif
        
        
    /**
     * @brief
     *  Draw text that only has one line to the screen.
     * @param text
     *  the text to draw
     * @param x
     *  the x position on screen
     * @param y
     *  the y position on screen
     * @param colour
     *  the colour of the text (default white)
     */
    void drawText(const std::string &text, int x, int y,
                    const Ego::Math::Colour4f &colour = Ego::Math::Colour4f::white());
        
    /**
     * @brief
     *  Draw text that potentially has multiple lines to the screen.
     * @param text
     *  the text to draw
     * @param x
     *  the x position on screen
     * @param y
     *  the y position on screen
     * @param width
     *  the maximum width (NOT IMPLEMENTED)
     * @param height
     *  the maximum height (NOT IMPLEMENTED)
     * @param spacing
     *  the spacing (in pixels) between each line
     * @param colour
     *  the colour of the text (default white)
     */
    void drawTextBox(const std::string &text, int x, int y, int width, int height, int spacing,
                     const Ego::Math::Colour4f &colour = Ego::Math::Colour4f::white());
        
    /** 
     * @brief
     *  Get the suggested line spacing for this font.
     * @return
     *  number of pixels suggested for line spacing
     */
    int getLineSpacing() const;
        
private:
    struct StringCacheData;
    typedef std::shared_ptr<StringCacheData> StringCacheDataPtr;
    static bool compareStringCacheData(const StringCacheDataPtr &a, const StringCacheDataPtr &b);
        
    TTF_Font *_ttfFont;
    std::unordered_map<std::string, std::weak_ptr<StringCacheData>> _stringCache;
    std::vector<StringCacheDataPtr> _sortedCache;
};

} // namespace Ego
