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

/// @file egolib/Graphics/Font.cpp
/// @brief TTF management
/// @details TrueType font drawing functionality.  Uses the SDL_ttf module
///          to do its business. This depends on SDL_ttf and OpenGL.

#include "egolib/Graphics/Font.hpp"

#include "egolib/Core/StringUtilities.hpp"
#include "egolib/Graphics/FontManager.hpp"
#include "egolib/Core/System.hpp"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/Image/ImageManager.hpp"
#include "egolib/Image/SDL_Image_Extensions.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
namespace Ego {

struct Font::RenderedTextCache : private id::non_copyable {
    std::shared_ptr<Font::LaidTextRenderer> cache;
    uint32_t lastUseInTicks;
    std::string text;
    int width;
    int height;
    int spacing;

    bool isEquivalent(const std::string &t, int w, int h, int s) {
        return w == width && h == height && s == spacing && t == text;
    }

    static bool isLessThan(const std::shared_ptr<RenderedTextCache> &a, const std::shared_ptr<RenderedTextCache> &b) {
        return a->lastUseInTicks < b->lastUseInTicks;
    }
};

struct Font::SizedTextCache : private id::non_copyable {
    uint32_t lastUseInTicks;
    std::string text;
    int width;
    int height;
    int spacing;

    bool isEquivalent(const std::string &t, int s) {
        return s == spacing && t == text;
    }

    static bool isLessThan(const std::shared_ptr<SizedTextCache> &a, const std::shared_ptr<SizedTextCache> &b) {
        return a->lastUseInTicks < b->lastUseInTicks;
    }
};

struct Font::FontAtlas {
    std::shared_ptr<Texture> texture;
    std::unordered_map<uint16_t, SDL_Rect> glyphs;
};

struct Font::LaidOutText {
    std::vector<uint16_t> codepoints;
    std::vector<SDL_Rect> positions;
    const FontAtlas &atlas;

    LaidOutText(const FontAtlas &a) :
        atlas(a) {}
};

struct Font::LayoutOptions {
    int maxWidth = 0;  ///< The maximum width in pixels of the constraining box; set to 0 for no constraint.
    int maxHeight = 0; ///< The maximum height in pixels of the constraining box; set to 0 for no constraint.
    int spacing = 0; ///< The space in pixels added between lines; if 0, it's set to @c Font::getFontSpacing()
    bool interpretNewlines = true; ///< If @c false, newline characters do not create a new line.
    int *textWidth = nullptr; ///< Set to the laid text's width
    int *textHeight = nullptr; ///< Set to the laid text's height
};

Font::LaidTextRenderer::LaidTextRenderer(const std::shared_ptr<Texture> &atlas,
                                         const VertexDescriptor& vertexDescriptor,
                                         const std::shared_ptr<VertexBuffer> &vertexBuffer) :
    _atlas(atlas),
    _vertexDescriptor(vertexDescriptor),
    _vertexBuffer(vertexBuffer) {}

void Font::LaidTextRenderer::render(int x, int y, const Math::Colour4f &colour) {
    struct MatrixStack {
        MatrixStack() :matrix(Renderer::get().getProjectionMatrix()) {}
        ~MatrixStack() { Renderer::get().setProjectionMatrix(matrix); }
        const Matrix4f4f matrix;
    };

    auto &renderer = Renderer::get();
    MatrixStack stack;

    Vector3f pos(static_cast<float>(x), static_cast<float>(y), 0.0f);
    Matrix4f4f transMat = Math::Transform::translation(pos);
    Matrix4f4f projMatrix = stack.matrix * transMat;

    renderer.setProjectionMatrix(projMatrix);
    renderer.setBlendingEnabled(true);
    renderer.setColour(colour);
    renderer.getTextureUnit().setActivated(_atlas.get());
    renderer.render(*(_vertexBuffer.get()), _vertexDescriptor, PrimitiveType::Quadriliterals, 0, _vertexBuffer->getNumberOfVertices());
}

Font::Font(const std::string &fileName, int pointSize) :
    _ttfFont(),
    _renderedCache(),
    _sizedCache() {
    _ttfFont = TTF_OpenFontRW(vfs_openRWopsRead(fileName), 1, pointSize);

    if (_ttfFont == nullptr) {
        throw id::environment_error(__FILE__, __LINE__, "SDL_ttf", TTF_GetError());
    }

    // Create an texture atlas for ASCII characters
    std::vector<uint16_t> ascii;
    for (int i = 0x20; i < 0x7F; i++)
        ascii.push_back(i);
    _atlases.push_back(createFontAtlas(ascii));
}

Font::~Font() {
    TTF_CloseFont(_ttfFont);
}

void Font::getTextSize(const std::string &text, int *width, int *height) {
    bool updateCache = true;
    std::shared_ptr<SizedTextCache> cache = findInSizedCache(text, 0, &updateCache);

    if (updateCache) {
        int ourWidth = 0;
        int ourHeight = 0;

        LayoutOptions options;
        options.textWidth = &ourWidth;
        options.textHeight = &ourHeight;
        options.interpretNewlines = false;

        layout(text, options);

        cache->text = text;
        cache->width = ourWidth;
        cache->height = ourHeight;
        cache->spacing = 0;
    }

    if (width) *width = cache->width;
    if (height) *height = cache->height;

    cache->lastUseInTicks = Core::System::get().getSystemService().getTicks();
}

void Font::getTextBoxSize(const std::string &text, int spacing, int *width, int *height) {
    bool updateCache = true;
    auto cache = findInSizedCache(text, spacing, &updateCache);

    if (updateCache) {
        int ourWidth = 0;
        int ourHeight = 0;

        LayoutOptions options;
        options.textWidth = &ourWidth;
        options.textHeight = &ourHeight;

        layout(text, options);

        cache->text = text;
        cache->width = ourWidth;
        cache->height = ourHeight;
        cache->spacing = spacing;
    }

    if (width) *width = cache->width;
    if (height) *height = cache->height;

    cache->lastUseInTicks = Core::System::get().getSystemService().getTicks();
}

void Font::drawTextToTexture(Texture *tex, const std::string &text, const Math::Colour3f &colour) {
    LayoutOptions options;

    std::shared_ptr<SDL_Surface> surface = layoutToTexture(text, options, colour);

    std::string name = "Font text '" + text + "'";
    tex->load(name, surface);
    tex->setAddressModeS(TextureAddressMode::Clamp);
    tex->setAddressModeT(TextureAddressMode::Clamp);
}

void Font::drawTextBoxToTexture(Texture *tex, const std::string &text, int width, int height, int spacing,
                            const Math::Colour3f &colour) {
    LayoutOptions options;
    options.maxWidth = width;
    options.maxHeight = height;
    options.spacing = spacing;

    std::shared_ptr<SDL_Surface> surface = layoutToTexture(text, options, colour);

    std::string name = "Font textbox '" + text + "'";
    tex->load(name, surface);
    tex->setAddressModeS(TextureAddressMode::Clamp);
    tex->setAddressModeT(TextureAddressMode::Clamp);
}

void Font::drawText(const std::string &text, int x, int y, const Math::Colour4f &colour) {
    if (text.empty()) return;

    bool updateCache = true;
    auto cache = findInRenderedCache(text, 0, 0, 0, &updateCache);

    if (updateCache) {
        cache->cache = layoutText(text, nullptr, nullptr);
        cache->text = text;
        cache->width = 0;
        cache->height = 0;
        cache->spacing = 0;
    }

    cache->cache->render(x, y, colour);

    cache->lastUseInTicks = Core::System::get().getSystemService().getTicks();
}

void Font::drawTextBox(const std::string &text, int x, int y, int width, int height, int spacing, const Math::Colour4f &colour) {
    if (text.empty()) return;

    bool updateCache = true;
    auto cache = findInRenderedCache(text, width, height, spacing, &updateCache);

    if (updateCache) {
        cache->cache = layoutTextBox(text, width, height, spacing, nullptr, nullptr);
        cache->text = text;
        cache->width = width;
        cache->height = height;
        cache->spacing = spacing;
    }

    cache->cache->render(x, y, colour);

    cache->lastUseInTicks = Core::System::get().getSystemService().getTicks();
}

std::shared_ptr<Font::LaidTextRenderer> Font::layoutText(const std::string &text, int *textWidth, int *textHeight) {
    LayoutOptions options;
    options.textWidth = textWidth;
    options.textHeight = textHeight;
    options.interpretNewlines = false;

    return layoutToBuffer(text, options);
}

std::shared_ptr<Font::LaidTextRenderer> Font::layoutTextBox(const std::string &text, int width, int height, int spacing,
                                                            int *textWidth, int *textHeight) {
    LayoutOptions options;
    options.textWidth = textWidth;
    options.textHeight = textHeight;
    options.maxWidth = width;
    options.maxHeight = height;
    options.spacing = spacing;

    return layoutToBuffer(text, options);
}

int Font::getLineSpacing() const {
    return TTF_FontLineSkip(_ttfFont);
}

int Font::getFontHeight() const {
    return TTF_FontHeight(_ttfFont);
}

void Font::layoutLine(const std::vector<uint16_t> &codepoints, size_t pos, int maxWidth, bool useNewlines,
                      const FontAtlas &atlas, size_t *endPos, std::vector<uint16_t> &usedChars,
                      std::vector<SDL_Rect> &positions, int *lineWidth, int *lineHeight) {
    bool useWidth = maxWidth > 0;
    uint16_t lastCodepoint = 0;
    size_t lastWordStartPosInChars = pos;
    size_t lastWordStartPosInUsedChars = 0;
    int x = 0;
    size_t currentPos;

    int maxLineWidth = 0;
    int maxLineHeight = 0;
    for (currentPos = pos; currentPos < codepoints.size(); currentPos++) {
        uint16_t codepoint = codepoints[currentPos];
        if (codepoint != '\n' && !TTF_GlyphIsProvided(_ttfFont, codepoint))
            continue;
        SDL_Rect rect = {0, 0, 0, 0};
        if (codepoint != '\n') {
            auto glyph = atlas.glyphs.find(codepoint);
            SDL_assert(glyph != atlas.glyphs.end());
            rect = glyph->second;
        }
        if (codepoint == ' ') {
            lastWordStartPosInChars = currentPos + 1;
            lastWordStartPosInUsedChars = usedChars.size() + 1;
        }

        if ((useNewlines && codepoint == '\n') || (useWidth && maxWidth - x < rect.w)) {
            if (codepoint != '\n' && codepoint != ' ' && lastWordStartPosInUsedChars > 0) {
                currentPos = lastWordStartPosInChars;
                auto wordStart = usedChars.begin();
                std::advance(wordStart, lastWordStartPosInUsedChars);
                usedChars.erase(wordStart, usedChars.end());
                auto wordPosStart = positions.begin();
                std::advance(wordPosStart, lastWordStartPosInUsedChars);
                positions.erase(wordPosStart, positions.end());

                maxLineWidth = 0;
                maxLineHeight = 0;
                for (size_t i = 0; i < usedChars.size(); i++) {
                    uint16_t ch = usedChars[i];
                    auto glyph = atlas.glyphs.find(ch);
                    SDL_Rect pos = positions[i];
                    SDL_Rect r = glyph->second;
                    if (maxLineHeight < r.h)
                        maxLineHeight = r.h;
                    if (maxLineWidth < pos.x + r.w)
                        maxLineWidth = pos.x + r.w;
                }
            } else if (useNewlines && codepoint == '\n') {
                currentPos++;
            }
            break;
        }
        if (codepoint == '\n')
            continue;

        if (maxLineWidth < x + rect.w)
            maxLineWidth = x + rect.w;
        if (maxLineHeight < rect.h)
            maxLineHeight = rect.h;

        int minx, advance;
        TTF_GlyphMetrics(_ttfFont, codepoint, &minx, nullptr, nullptr, nullptr, &advance);
        x += getFontKerning(lastCodepoint, codepoint);
        SDL_assert(x >= 0);
        SDL_Rect dst = {x, 0, rect.w, rect.h};
        if (minx < 0)
            dst.x += minx;
        positions.push_back(dst);
        usedChars.push_back(codepoint);
        x += advance;
        lastCodepoint = codepoint;
    }
    *endPos = currentPos;
    *lineWidth = maxLineWidth;
    *lineHeight = maxLineHeight;
}

Font::LaidOutText Font::layout(const std::string &text, const LayoutOptions &options) {
    bool useHeight = options.maxHeight > 0;
    int spacing = options.spacing;
    if (spacing <= 0)
        spacing = getLineSpacing();

    std::vector<uint16_t> codepoints = splitUTF8StringToCodepoints(text);

    const FontAtlas *currentAtlas = nullptr;
    for (const FontAtlas &atlas : _atlases) {
        bool containsAllChars = std::all_of(codepoints.begin(), codepoints.end(), [&atlas](uint16_t pos) {
            return pos == ' ' || pos == '\n' || atlas.glyphs.find(pos) != atlas.glyphs.end();
        });

        if (containsAllChars) {
            currentAtlas = &atlas;
            break;
        }
    }

    if (currentAtlas == nullptr) {
        _atlases.push_back(createFontAtlas(codepoints));
        currentAtlas = &(_atlases.at(_atlases.size() - 1));
    }

    LaidOutText laidText(*currentAtlas);

    int maxLineWidth = 0;
    int y = 0;
    int maxHeight = 0;

    for (size_t pos = 0; pos < codepoints.size(); ) {
        std::vector<SDL_Rect> linePos;
        std::vector<uint16_t> lineUsedChars;
        int lineWidth;
        int lineHeight;
        size_t newPos = pos;

        layoutLine(codepoints, pos, options.maxWidth, options.interpretNewlines, laidText.atlas, &newPos,
                   lineUsedChars, linePos, &lineWidth, &lineHeight);

        if (newPos == pos)
            break;

        pos = newPos;

        if (useHeight && options.maxHeight - y < lineHeight)
            break;

        if (maxLineWidth < lineWidth)
            maxLineWidth = lineWidth;

        laidText.codepoints.insert(laidText.codepoints.end(), lineUsedChars.begin(), lineUsedChars.end());

        for (const SDL_Rect &rect : linePos) {
            SDL_Rect newRect = rect;
            newRect.y += y;
            laidText.positions.push_back(newRect);
        }

        int ourHeight = y + lineHeight;
        y += spacing;
        if (maxHeight < ourHeight)
            maxHeight = ourHeight;
    }

    if (options.textWidth) *(options.textWidth) = maxLineWidth;
    if (options.textHeight) *(options.textHeight) = maxHeight;

    return laidText;
}

std::shared_ptr<Font::LaidTextRenderer> Font::layoutToBuffer(const std::string &text, const LayoutOptions &options) {
    struct TextVertex {
        float x;
        float y;
        float z;
        float u;
        float v;
    };

    LaidOutText laidText = layout(text, options);

    const auto &vertexDesc = VertexFormatFactory::get(VertexFormat::P3FT2F);
    std::shared_ptr<VertexBuffer> buffer = std::make_shared<VertexBuffer>(4 * laidText.codepoints.size(), vertexDesc.getVertexSize());

    TextVertex *vertices = reinterpret_cast<TextVertex *>(buffer->lock());
    float texWidth = laidText.atlas.texture->getWidth();
    float texHeight = laidText.atlas.texture->getHeight();

    for (size_t i = 0; i < laidText.codepoints.size(); i++) {
        uint16_t chr = laidText.codepoints[i];
        SDL_Rect charPos = laidText.positions[i];
        SDL_Rect glyphPos = laidText.atlas.glyphs.at(chr);

        float xMin = charPos.x;
        float xMax = charPos.x + charPos.w;
        float yMin = charPos.y;
        float yMax = charPos.y + charPos.h;

        float uMin = (glyphPos.x) / texWidth;
        float uMax = (glyphPos.x + glyphPos.w) / texWidth;
        float vMin = (glyphPos.y) / texHeight;
        float vMax = (glyphPos.y + glyphPos.h) / texHeight;

        vertices[i * 4 + 0].x = xMin;
        vertices[i * 4 + 0].y = yMin;
        vertices[i * 4 + 0].z = 0;
        vertices[i * 4 + 0].u = uMin;
        vertices[i * 4 + 0].v = vMin;

        vertices[i * 4 + 1].x = xMax;
        vertices[i * 4 + 1].y = yMin;
        vertices[i * 4 + 1].z = 0;
        vertices[i * 4 + 1].u = uMax;
        vertices[i * 4 + 1].v = vMin;

        vertices[i * 4 + 2].x = xMax;
        vertices[i * 4 + 2].y = yMax;
        vertices[i * 4 + 2].z = 0;
        vertices[i * 4 + 2].u = uMax;
        vertices[i * 4 + 2].v = vMax;

        vertices[i * 4 + 3].x = xMin;
        vertices[i * 4 + 3].y = yMax;
        vertices[i * 4 + 3].z = 0;
        vertices[i * 4 + 3].u = uMin;
        vertices[i * 4 + 3].v = vMax;
    }
    buffer->unlock();
    return std::shared_ptr<LaidTextRenderer>(new LaidTextRenderer(laidText.atlas.texture, vertexDesc, buffer));
}

std::shared_ptr<SDL_Surface> Font::layoutToTexture(const std::string &text, const LayoutOptions &options,
                                                   const Math::Colour3f &colour) {
    LayoutOptions ourOptions = options;
    int surfWidth, surfHeight;
    ourOptions.textWidth = &surfWidth;
    ourOptions.textHeight = &surfHeight;

    LaidOutText laidText = layout(text, ourOptions);

    if (options.textWidth) *(options.textWidth) = surfWidth;
    if (options.textHeight) *(options.textHeight) = surfHeight;

    auto pfd = PixelFormatDescriptor::get<PixelFormat::R8G8B8A8>();

    auto colorByte = Math::Colour3b(colour);

    auto surf = ImageManager::get().createImage(surfWidth, surfHeight, pfd);
    SDL::fillSurface(*surf, Math::Colour4b(colorByte, 0));
    SDL::setBlendMode(*surf, SDL::BlendMode::NoBlending);

    auto atlasSurf = laidText.atlas.texture->m_source;
    auto oldMod = SDL::getColourMod(*atlasSurf);
    SDL::setColourMod(*atlasSurf, colorByte);

    for (size_t i = 0; i < laidText.codepoints.size(); i++) {
        uint16_t chr = laidText.codepoints[i];
        SDL_Rect charPos = laidText.positions[i];
        SDL_Rect glyphPos = laidText.atlas.glyphs.at(chr);
        SDL_BlitSurface(atlasSurf.get(), &glyphPos, surf.get(), &charPos);
    }

    SDL::setColourMod(*atlasSurf, oldMod);

    return surf;
}

Font::FontAtlas Font::createFontAtlas(const std::vector<std::string> &chars) const {
    std::vector<uint16_t> codepoints;
    for (const std::string & chr : chars)
        codepoints.push_back(convertUTF8ToCodepoint(chr));
    return createFontAtlas(codepoints);
}

Font::FontAtlas Font::createFontAtlas(const std::vector<uint16_t> &codepoints) const {
    SDL_Color white = {255, 255, 255, 255};
    std::vector<SDL_Surface *> images;
    std::vector<SDL_Rect> pos;

    for (uint16_t cp : codepoints) {
        SDL_Surface *surf = nullptr;
        if (TTF_GlyphIsProvided(_ttfFont, cp))
            surf = TTF_RenderGlyph_Blended(_ttfFont, cp, white);
        images.push_back(surf);
    }

    int currentMaxSize = 128;
    int maxTextureSize = Ego::Renderer::get().getInfo()->getMaximumTextureSize();
    std::shared_ptr<SDL_Surface> atlas = nullptr;

    auto pfd = PixelFormatDescriptor::get<PixelFormat::R8G8B8A8>();

    while (currentMaxSize <= maxTextureSize) {
        atlas = ImageManager::get().createImage(currentMaxSize, currentMaxSize, pfd);
        SDL::fillSurface(*atlas, Math::Colour4b(Math::Colour3b::white(), 0));
        SDL::setBlendMode(*atlas, SDL::BlendMode::NoBlending);

        int x = 0, y = 0;
        int currentHeight = 0;
        bool fits = true;

        for (SDL_Surface *surf : images) {
            if (surf == nullptr) {
                pos.push_back({0, 0, 0, 0});
                continue;
            }

            if (currentMaxSize < surf->w || currentMaxSize < surf->h) {
                fits = false;
                break;
            }

            if (currentMaxSize - x < surf->w) {
                y += currentHeight + 1;
                x = 0;
                currentHeight = 0;
            }

            if (currentHeight < surf->h)
                currentHeight = surf->h;

            if (currentMaxSize - y < currentHeight) {
                fits = false;
                break;
            }

            SDL_Rect dst = {x, y, surf->w, surf->h};
            SDL_BlitSurface(surf, nullptr, atlas.get(), &dst);
            pos.push_back(dst);
            x += surf->w + 1;
        }

        if (fits)
            break;

        Log::get() << Log::Entry::create(Log::Level::Debug, __FILE__, __LINE__, "unable to fit atlas into a texture of size ", currentMaxSize, ", trying texture of size ", currentMaxSize * 2, " instead", Log::EndOfEntry);
        currentMaxSize <<= 1;
        pos.clear();
        atlas = nullptr;
    }

    if (!atlas) {
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to fit a atlas into a texture of size ", maxTextureSize, Log::EndOfEntry);
        SDL_assert(atlas);
    }

    FontAtlas retval;
    for (int i = 0; i < images.size(); i++) {
        if (images[i] == nullptr)
            continue;
        retval.glyphs.insert(std::make_pair(codepoints[i], pos[i]));
        SDL_FreeSurface(images[i]);
    }

    retval.texture = Renderer::get().createTexture();
    retval.texture->load("font atlas", atlas);
    retval.texture->setAddressModeS(TextureAddressMode::Clamp);
    retval.texture->setAddressModeT(TextureAddressMode::Clamp);
    return retval;
}

std::shared_ptr<Font::RenderedTextCache> Font::findInRenderedCache(const std::string &text, int width, int height,
                                                                   int spacing, bool *update) {
    if (MAX_CACHE_SIZE == 0) {
        *update = true;
        return std::make_shared<RenderedTextCache>();
    }

    if (MAX_CACHE_SIZE == 1) {
        *update = true;
        if (_renderedCache.empty())
            _renderedCache.emplace_back(std::make_shared<RenderedTextCache>());
        return _renderedCache[0];
    }

    std::shared_ptr<Font::RenderedTextCache> cache;
    bool updateCache = true;

    auto cacheIterator = std::find_if(_renderedCache.begin(), _renderedCache.end(),
                                      [&](const std::shared_ptr<Font::RenderedTextCache> &ptr) {
        return ptr->isEquivalent(text, width, height, spacing);
    });

    if (cacheIterator != _renderedCache.end()) {
        cache = *cacheIterator;
        updateCache = false;
    } else if (_renderedCache.size() < MAX_CACHE_SIZE) {
        cache = std::make_shared<RenderedTextCache>();
        _renderedCache.push_back(cache);
    } else {
        std::sort(_renderedCache.begin(), _renderedCache.end(), RenderedTextCache::isLessThan);
        cache = _renderedCache.at(0);
    }

    if (update) *update = updateCache;
    return cache;
}

std::shared_ptr<Font::SizedTextCache> Font::findInSizedCache(const std::string &text, int spacing, bool *update) {
    if (MAX_CACHE_SIZE == 0) {
        *update = true;
        return std::make_shared<SizedTextCache>();
    }

    if (MAX_CACHE_SIZE == 1) {
        *update = true;
        if (_sizedCache.empty())
            _sizedCache.emplace_back(std::make_shared<SizedTextCache>());
        return _sizedCache[0];
    }

    std::shared_ptr<Font::SizedTextCache> cache;
    bool updateCache = true;

    auto cacheIterator = std::find_if(_sizedCache.begin(), _sizedCache.end(),
                                      [&](const std::shared_ptr<Font::SizedTextCache> &ptr) {
        return ptr->isEquivalent(text, spacing);
    });

    if (cacheIterator != _sizedCache.end()) {
        cache = *cacheIterator;
        updateCache = false;
    } else if (_sizedCache.size() < MAX_CACHE_SIZE) {
        cache = std::make_shared<SizedTextCache>();
        _sizedCache.push_back(cache);
    } else {
        std::sort(_sizedCache.begin(), _sizedCache.end(), SizedTextCache::isLessThan);
        cache = _sizedCache.at(0);
    }

    if (update) *update = updateCache;
    return cache;
}

uint16_t Font::convertUTF8ToCodepoint(const std::string &string, size_t *pos) {
    size_t tmpPos = 0;
    if (pos == nullptr) pos = &tmpPos;

    uint16_t retval = 0;

    unsigned char tmp = string.at(*pos);
    *pos += 1;

    if (tmp < 0x80) {
        retval |= tmp;
    } else if (tmp < 0xC2) {
        throw std::invalid_argument("UTF-8 character invalid");
    } else if (tmp < 0xE0) {
        retval |= (tmp & 0x1F) << 6;

        tmp = string.at(*pos);
        *pos += 1;
        if ((tmp & 0xC0) != 0x80)
            throw std::invalid_argument("UTF-8 character invalid");
        retval |= (tmp & 0x3F);
    } else if (tmp < 0xF0) {
        retval |= (tmp & 0xF) << 12;

        tmp = string.at(*pos);
        *pos += 1;
        if ((tmp & 0xC0) != 0x80 || (retval == 0 && tmp < 0xA0))
            throw std::invalid_argument("UTF-8 character invalid");
        retval |= (tmp & 0x3F) << 6;

        tmp = string.at(*pos);
        *pos += 1;
        if ((tmp & 0xC0) != 0x80)
            throw std::invalid_argument("UTF-8 character invalid");
        retval |= (tmp & 0x3F);
    } else {
        throw std::out_of_range("UTF-8 character does not fit in a 16-bit codepoint");
    }

    return retval;
}

std::vector<uint16_t> Font::splitUTF8StringToCodepoints(const std::string &text) {
    std::vector<uint16_t> retval;
    size_t pos = 0;
    while (pos < text.size())
        retval.push_back(convertUTF8ToCodepoint(text, &pos));
    return retval;
}
    
#define TTF_VERSION_NUM SDL_VERSIONNUM(SDL_TTF_MAJOR_VERSION, SDL_TTF_MINOR_VERSION, SDL_TTF_PATCHLEVEL)
#define TTF_VERSION_2_0_14 SDL_VERSIONNUM(2, 0, 14)
#define TTF_VERSION_2_0_13 SDL_VERSIONNUM(2, 0, 13)
    
// SDL_ttf 2.0.14 has the correct function we need, so just use that.
#if TTF_VERSION_NUM >= TTF_VERSION_2_0_14

int Font::getFontKerning(uint16_t prev, uint16_t curr) const {
    return TTF_GetFontKerningSizeGlyphs(_ttfFont, prev, curr);
}

#else

// This function is used when we're running with SDL_ttf 2.0.12
int Font::getFontKerning_2_0_12(TTF_Font *font, uint16_t prev, uint16_t curr) {
    static auto realKerningFunc =
#if TTF_VERSION_NUM == TTF_VERSION_2_0_13
        reinterpret_cast<int(*)(TTF_Font *, int, int)>(TTF_GetFontKerningSize);
#else
        TTF_GetFontKerningSize;
#endif
    
    // Abuse the fact that TTF_GlyphIsProvided just calls FT_Get_Char_Index and returns the value
    // instead of returning if the value is not equal to 0
    int prev_index = TTF_GlyphIsProvided(font, prev);
    int next_index = TTF_GlyphIsProvided(font, curr);
    
    return realKerningFunc(font, prev_index, next_index);
}

int Font::getFontKerning(uint16_t prev, uint16_t curr) const {
    static int(*kerningFunc)(TTF_Font *, uint16_t, uint16_t) = nullptr;
    
    if (!kerningFunc) {
        const SDL_version *ttfVer = TTF_Linked_Version();
        if (SDL_VERSIONNUM(ttfVer->major, ttfVer->minor, ttfVer->patch) == TTF_VERSION_2_0_13) {
#if TTF_VERSION_NUM == TTF_VERSION_2_0_13
            kerningFunc = TTF_GetFontKerningSize;
#else
            kerningFunc = reinterpret_cast<int(*)(TTF_Font *, uint16_t, uint16_t)>(TTF_GetFontKerningSize);
#endif
        } else {
            kerningFunc = getFontKerning_2_0_12;
        }
    }
    
    return kerningFunc(_ttfFont, prev, curr);
}
#endif

#undef USING_FIXED_API
#undef IS_KERNING_FIXED

} // namespace Ego

