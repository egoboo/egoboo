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
#include "egolib/Math/_Include.hpp"
#include "egolib/Graphics/VertexBuffer.hpp"

namespace Ego {
class Texture;
} // namespace Ego

namespace Ego {

class FontManager;

/**
 * @brief
 *  A representation of a TrueType font.
 */
class Font final : private id::non_copyable {
public:

    /**
     * @brief A container and renderer for laid out text.
     */
    class LaidTextRenderer final : private id::non_copyable {
    public:
        /**
         * @brief
         *  Renders the laid out text.
         * @param x,y
         *  The position on screen to render the text at
         * @param colour
         *  The colour of the rendered text; default is white
         */
        void render(int x, int y, const Math::Colour4f &colour = Math::Colour4f::white());

    protected:
        LaidTextRenderer(const std::shared_ptr<Texture> &atlas, 
                         const VertexDescriptor& vertexDescriptor,
                         const std::shared_ptr<VertexBuffer> &vertexBuffer);
        friend class Font;
    private:
        std::shared_ptr<Texture> _atlas;
        VertexDescriptor _vertexDescriptor;
        std::shared_ptr<VertexBuffer> _vertexBuffer;
    };

private:
    /// This is the maximum size for the two caches as used by
    /// drawText and getTextSize, set this to 0 for no caching
    constexpr static size_t MAX_CACHE_SIZE = 20;

protected:
    Font(const std::string &fileName, int pointSize);
    friend class FontManager;

public:
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
    void getTextSize(const std::string &text, int *width, int *height);

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
    void getTextBoxSize(const std::string &text, int spacing, int *width, int *height);


    /**
     * @brief
     *  Draw text that only has one line to a texture.
     * @param tex
     *  the texture object to draw to
     * @param text
     *  the text to draw
     * @param colour
     *  the colour of the text (default white)
     */
    void drawTextToTexture(Texture *tex, const std::string &text,
                           const Math::Colour3f &color = Math::Colour3f::white());

    /**
     * @brief
     *  Draw text that potentially has multiple lines to a texture.
     * @param tex
     *  the texture object to draw to
     * @param text
     *  the text to draw
     * @param width
     *  the maximum width
     * @param height
     *  the maximum height
     * @param spacing
     *  the spacing (in pixels) between each line
     * @param colour
     *  the colour of the text (default white)
     */
    void drawTextBoxToTexture(Texture *tex, const std::string &text, int width, int height, int spacing,
                              const Math::Colour3f &color = Math::Colour3f::white());


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
                  const Math::Colour4f &colour = Math::Colour4f::white());

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
     *  the maximum width before text wrapping (0 is no limit)
     * @param height
     *  the maximum height before text truncating (0 is no limit)
     * @param spacing
     *  the spacing (in pixels) between each line
     * @param colour
     *  the colour of the text (default white)
     */
    void drawTextBox(const std::string &text, int x, int y, int width, int height, int spacing,
                     const Math::Colour4f &colour = Math::Colour4f::white());

    /**
     * @brief
     *  Create a render cache to render text that only has one line.
     * @param text
     *  The text to cache
     * @param[out] textWidth,textHeight
     *  These are set to the size of the laid text.
     * @sa
     *  drawText
     */
    std::shared_ptr<LaidTextRenderer> layoutText(const std::string &text, int *textWidth, int *textHeight);

    /**
     * @brief
     *  Create a render cache to render text that may have multiple lines.
     * @param text
     *  The text to cache
     * @param width, height
     *  Constraints on the laid text; use 0 for no constraint
     * @param spacing
     *  the spacing (in pixels) between each line
     * @param[out] textWidth,textHeight
     *  These are set to the size of the laid text.
     * @sa
     *  drawTextBox
     */
    std::shared_ptr<LaidTextRenderer> layoutTextBox(const std::string &text, int width, int height, int spacing,
                                                    int *textWidth, int *textHeight);

    /**
     * @brief
     *  Get the suggested line spacing for this font.
     * @return
     *  number of pixels suggested for line spacing
     */
    int getLineSpacing() const;

    /**
    * @brief
    *   Get the maximum pixel height of all glyphs of the loaded font. You may use this height for rendering text as close together
    *   vertically as possible, though adding at least one pixel height to it will space it so they can't touch.
    * @return
    *   The maximum pixel height of all glyphs in the font
    **/
    int getFontHeight() const;

private:
    /// Struct for cached text that has been rendered
    struct RenderedTextCache;
    /// Struct for cached text that has been sized
    struct SizedTextCache;

    struct FontAtlas;

    /// Internal representation of laid out text.
    struct LaidOutText;

    struct LayoutOptions;

    /**
     * @brief
     *  Find given values in the rendered text cache
     * @param text,width,height,spacing
     *  The values to look for in the cache
     * @param update[out]
     *  Is set to @c true when the returned cached struct needs to be updated with the given values, @c false otherwise
     * @return
     *  A cached struct that may need to be updated per @a update
     */
    std::shared_ptr<RenderedTextCache> findInRenderedCache(const std::string &text, int width, int height,
                                                           int spacing, bool *update);

    /**
     * @brief
     *  Find given values in the sized text cache
     * @param text,spacing
     *  The values to look for in the cache
     * @param update[out]
     *  Is @c true when the returned cached struct needs to be updated with the given values, @c false otherwise
     * @return
     *  A cached struct that may need to be updated per @a update
     */
    std::shared_ptr<SizedTextCache> findInSizedCache(const std::string &text, int spacing, bool *update);

    /**
     * @brief
     *  Layout text into a LaidTextRenderer.
     * @see layout
     */
    std::shared_ptr<LaidTextRenderer> layoutToBuffer(const std::string &text, const LayoutOptions &options);

    /**
     * @brief
     *  Layout text into a SDL_Surface.
     * @see layout
     */
    std::shared_ptr<SDL_Surface> layoutToTexture(const std::string &text, const LayoutOptions &options,
                                                 const Math::Colour3f &colour);

    /**
     * @brief
     *  Main function for laying out text.
     * @param text
     *  The text to layout
     * @param options
     *  The options that dictate how the text is laid out.
     * @return
     *  The laid out text as a group of codepoints and their positions and sizes.
     */
    LaidOutText layout(const std::string &text, const LayoutOptions &options);

    /**
     * @brief
     *  Layouts a single line
     * @param codepoints
     *  List of codepoints representing text
     * @param pos
     *  Start position in @c chars
     * @param maxWidth
     *  The maximum width of the line, or 0 for no limit
     * @param useNewlines
     *  If @c false, newlines (<tt>'\\n'</tt>) do not create a new line.
     * @param atlas
     *  The font atlas to use.
     * @param[out] endPos
     *  The character after the last character in this line
     * @param[out] usedCodepoints
     *  List of added codepoints
     * @param[out] positions
     *  List of positions of the added codepoints
     * @param[out] lineWidth
     *  Width of the line created
     * @param[out] lineHeight
     *  Height of the line created
     */
    void layoutLine(const std::vector<uint16_t> &codepoints, size_t pos, int maxWidth, bool useNewlines,
                    const FontAtlas &atlas, size_t *endPos, std::vector<uint16_t> &usedCodepoints,
                    std::vector<SDL_Rect> &positions, int *lineWidth, int *lineHeight);

    /**
     * @brief
     *  Creates a texture altas from the given codepoints.
     * @param chars
     *  The list of characters to add to the atlas
     * @return
     *  The created texture atlas
     */
    FontAtlas createFontAtlas(const std::vector<uint16_t> &codepoints) const;

    /**
     * @brief
     *  Creates a texture atlas from the given UTF-8 characters.
     * @see createFontAtlas(const std::vector<uint16_t> &)
     */
    FontAtlas createFontAtlas(const std::vector<std::string> &chars) const;

    /**
     * @brief
     *  Gets the kerning between two given codepoints.
     * @return
     *  The kerning in pixels
     * @note
     *  Use this instead of @a TTF_GetFontKerningSize because SDL_ttf version 2.0.12 takes
     *  glyph indices instead of codepoints.
     */
    int getFontKerning(uint16_t prevCodepoint, uint16_t nextCodepoint) const;
    
    /**
     * @brief
     *  Gets the kerning between two given codepoints.
     * @return
     *  The kerning in pixels
     * @note
     *  Helper function for Font::getFontKerning(uint16_t, uint16_t), used when not compiling for
     *  SDL_ttf 2.0.14 and not running on SDL_ttf 2.0.13.
     */
    static int getFontKerning_2_0_12(TTF_Font *font, uint16_t prevCodepoint, uint16_t nextCodepoint);

    /**
     * @brief
     *  Returns the given UTF-8 character's codepoint.
     * @param string
     *  The UTF-8 character
     * @param[in,out] pos
     *  The position in @a string to start at and will contain the position after the
     *  character that has been converted. If @c pos is @c nullptr, it will assumed to be 0.
     * @return
     *  The character's codepoint as a 16-bit integer as required by SDL_ttf
     * @throws
     *  @c std::invalid_argument if the character is an invalid UTF-8 character
     *  @c std::out_of_range if the character's codepoint is outside the the range of 16-bits
     */
    static uint16_t convertUTF8ToCodepoint(const std::string &string, size_t *pos = nullptr);

    /**
     * @brief
     *  Returns the codepoints of the given UTF-8 string.
     * @param text
     *  A UTF-8 string.
     * @return
     *  The list of codepoints of the given string.
     */
    static std::vector<uint16_t> splitUTF8StringToCodepoints(const std::string &text);

    TTF_Font *_ttfFont;

    std::vector<std::shared_ptr<RenderedTextCache>> _renderedCache;
    std::vector<std::shared_ptr<SizedTextCache>> _sizedCache;
    std::vector<FontAtlas> _atlases;
};

} // namespace Ego
