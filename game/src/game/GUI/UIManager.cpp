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

/// @file game/GUI/UIManager.cpp
/// @details The UIManager contains utilities for the GUI system and stores various shared resources
///             and properties for GUI components.
/// @author Johan Jansen

#include "game/GUI/UIManager.hpp"
#include "game/graphic.h"
#include "game/game.h" //TODO: Remove only for DisplayMessagePrintf

namespace Ego {
namespace GUI {

UIManager::UIManager() :
    _fonts(),
    _renderSemaphore(0),
    _bitmapFontTexture(TextureManager::get().getTexture("mp_data/font_new_shadow")),
    _textureQuadVertexBuffer(4, GraphicsUtilities::get<VertexFormat::P2FT2F>()) {
    //Load fonts from true-type files
    _fonts[FONT_DEFAULT] = FontManager::get().loadFont("mp_data/Bo_Chen.ttf", 24);
    _fonts[FONT_FLOATING_TEXT] = FontManager::get().loadFont("mp_data/FrostysWinterland.ttf", 24);
    _fonts[FONT_DEBUG] = FontManager::get().loadFont("mp_data/DejaVuSansMono.ttf", 10);
    _fonts[FONT_GAME] = FontManager::get().loadFont("mp_data/IMMORTAL.ttf", 14);

    //Sanity check that all fonts are loaded properly
#ifndef NDEBUG
    for (int i = 0; i < _fonts.size(); ++i) {
        if (!_fonts[i]) {
            std::ostringstream os;
            os << __FILE__ << ":" << __LINE__ << ": UI manager is missing font with ID " << i << std::endl;
            Log::get().error("%s", os.str().c_str());
            throw std::runtime_error(os.str());
        }
    }
#endif

    const auto& vertexFormat = GraphicsUtilities::get<VertexFormat::P2F>();
    _vertexBuffer = std::make_shared<VertexBuffer>(4, vertexFormat);
}

UIManager::~UIManager() {
    _vertexBuffer = nullptr;
    // free fonts before font manager
    for (std::shared_ptr<Font> &font : _fonts) {
        font.reset();
    }
}

void UIManager::beginRenderUI() {
    //Handle recusive loops that trigger beginRenderUI
    _renderSemaphore++;
    if (_renderSemaphore > 1) {
        return;
    }

    auto& renderer = Renderer::get();

    // do not use the ATTRIB_PUSH macro, since the glPopAttrib() is in a different function
    GL_DEBUG(glPushAttrib)(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

    // Don't worry about hidden surfaces.
    renderer.setDepthTestEnabled(false);

    // Draw draw front and back faces of polygons.
    renderer.setCullingMode(CullingMode::None);

    // Use normal alpha blending.
    renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);
    renderer.setBlendingEnabled(true);

    // Do not display the completely transparent portion.
    renderer.setAlphaTestEnabled(true);
    renderer.setAlphaFunction(CompareFunction::Greater, 0.0f);

    /// Set the viewport rectangle.
    auto drawableSize = Ego::GraphicsSystem::window->getDrawableSize();
    renderer.setViewportRectangle(0, 0, drawableSize.width(), drawableSize.height());

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame.
    auto windowSize = Ego::GraphicsSystem::window->getSize();
    Matrix4f4f projection = Transform::ortho(0.0f, windowSize.width(), windowSize.height(), 0.0f, -1.0f, +1.0f);
    renderer.setProjectionMatrix(projection);
    renderer.setViewMatrix(Matrix4f4f::identity());
    renderer.setWorldMatrix(Matrix4f4f::identity());
}

void UIManager::endRenderUI() {
    //Handle recusive loops that trigger beginRenderUI
    _renderSemaphore--;
    if (_renderSemaphore > 0) {
        return;
    }

    // Re-enable any states disabled by gui_beginFrame
    // do not use the ATTRIB_POP macro, since the glPushAttrib() is in a different function
    GL_DEBUG(glPopAttrib)();
}

int UIManager::getScreenWidth() const {
    return Ego::GraphicsSystem::window->getSize().width();
}

int UIManager::getScreenHeight() const {
    return Ego::GraphicsSystem::window->getSize().height();
}

void UIManager::drawImage(const std::shared_ptr<const Texture>& img, const Point2f& position, const Vector2f& size, const Colour4f& tint) {
    auto source = Rectangle2f(Point2f(0, 0),
                              Point2f(static_cast<float>(img->getSourceWidth()) / static_cast<float>(img->getWidth()),
                                      static_cast<float>(img->getSourceHeight()) / static_cast<float>(img->getHeight())));
    auto target = Rectangle2f(position,
                              position + size);
    // Draw the image
    drawQuad2D(img, target, source, true, tint);
}

bool UIManager::dumpScreenshot() {
    int i;
    bool saved = false;
    STRING szFilename, szResolvedFilename;

    // find a valid file name
    bool savefound = false;
    i = 0;
    while (!savefound && (i < 100)) {
        snprintf(szFilename, SDL_arraysize(szFilename), "ego%02d.png", i);

        // lame way of checking if the file already exists...
        savefound = !vfs_exists(szFilename);
        if (!savefound) {
            i++;
        }
    }

    if (!savefound) return false;

    // convert the file path to the correct write path
    strncpy(szResolvedFilename, szFilename, SDL_arraysize(szFilename));

    // if we are not using OpenGL, use SDL to dump the screen
    if (HAS_NO_BITS(SDL_GetWindowFlags(Ego::GraphicsSystem::window->get()), SDL_WINDOW_OPENGL)) {
        return IMG_SavePNG_RW(SDL_GetWindowSurface(Ego::GraphicsSystem::window->get()), vfs_openRWopsWrite(szResolvedFilename), 1);
    }

    // we ARE using OpenGL
    {
        OpenGL::PushClientAttrib pca(GL_CLIENT_PIXEL_STORE_BIT);
        {
            SDL_Surface *temp;

            // create a SDL surface
            const auto& pixelFormatDescriptor = PixelFormatDescriptor::get<PixelFormat::R8G8B8>();
            auto drawableSize = Ego::GraphicsSystem::window->getDrawableSize();
            temp = SDL_CreateRGBSurface(0, drawableSize.width(), drawableSize.height(),
                                        pixelFormatDescriptor.getColourDepth().getDepth(),
                                        pixelFormatDescriptor.getRedMask(),
                                        pixelFormatDescriptor.getGreenMask(),
                                        pixelFormatDescriptor.getBlueMask(),
                                        pixelFormatDescriptor.getAlphaMask());

            if (NULL == temp) {
                //Something went wrong
                SDL_FreeSurface(temp);
                return false;
            }

            //Now lock the surface so that we can read it
            if (-1 != SDL_LockSurface(temp)) {
                SDL_Rect rect = {0, 0, drawableSize.width(), drawableSize.height()};

                int y;
                uint8_t * pixels;

                GL_DEBUG(glGetError)();

                // ARGH! Must copy the pixels row-by-row, since the OpenGL video memory is flipped vertically
                // relative to the SDL Screen memory

                // this is supposed to be a DirectX thing, so it needs to be tested out on glx
                // there should probably be [SCREENSHOT_INVERT] and [SCREENSHOT_VALID] keys in setup.txt
                pixels = (uint8_t *)temp->pixels;
                for (y = rect.y; y < rect.y + rect.h; y++) {
                    GL_DEBUG(glReadPixels)(rect.x, (rect.h - y) - 1, rect.w, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels);
                    pixels += temp->pitch;
                }
                EGOBOO_ASSERT(GL_NO_ERROR == GL_DEBUG(glGetError)());

                SDL_UnlockSurface(temp);

                // Save the file as a .png
                saved = (-1 != IMG_SavePNG_RW(temp, vfs_openRWopsWrite(szResolvedFilename), 1));
            }

            // free the SDL surface
            SDL_FreeSurface(temp);
            if (saved) {
                // tell the user what we did
                DisplayMsg_printf("Saved to %s", szFilename);
            }
        }
    }

    return savefound && saved;
}

float UIManager::drawBitmapFontString(const Vector2f& start, const std::string &text, const uint32_t maxWidth, const float alpha) {
    //Check if alpha is visible
    if (alpha <= 0.0f) {
        return start.y();
    }

    //Current render position
    float x = start.x();
    float y = start.y();

    for (size_t cnt = 0; cnt < text.length(); ++cnt) {
        const uint8_t cTmp = text[cnt];

        // Check each new word for wrapping
        if (maxWidth > 0) {
            if ('~' == cTmp || C_LINEFEED_CHAR == cTmp || C_CARRIAGE_RETURN_CHAR == cTmp || std::isspace(cTmp)) {
                int endx = x + font_bmp_length_of_word(text.c_str() + cnt - 1);

                if (endx > maxWidth) {

                    // Wrap the end and cut off spaces and tabs
                    x = start.x() + fontyspacing;
                    y += fontyspacing;
                    while (std::isspace(text[cnt]) || '~' == text[cnt]) {
                        cnt++;
                    }

                    continue;
                }
            }
        }

        // Use squiggle for tab
        if ('~' == cTmp) {
            x = (std::floor(x / TABADD) + 1.0f) * TABADD;
        }

        //Linefeed
        else if (C_LINEFEED_CHAR == cTmp) {
            x = start.x();
            y += fontyspacing;
        }

        //other whitespace
        else if (std::isspace(cTmp)) {
            uint8_t iTmp = asciitofont[cTmp];
            x += fontxspacing[iTmp] / 2;
        }

        // Normal letter
        else {
            uint8_t iTmp = asciitofont[cTmp];
            drawBitmapGlyph(iTmp, Vector2f(x, y), alpha);
            x += fontxspacing[iTmp];
        }
    }

    return y + fontyspacing;
}

void UIManager::drawBitmapGlyph(int fonttype, const Vector2f& position, const float alpha) {
    static constexpr float DX = 2.0f / 512.0f;
    static constexpr float DY = 1.0f / 256.0f;
    static constexpr float BORDER = 1.0f / 512.0f;

    /// @todo This code is problematic as it seems
    /// to rely on that (x|y)min can be greater than (x|y)max and
    /// in some cases. 
    ego_frect_t sc_rect;
    sc_rect.xmin = position.x();
    sc_rect.ymin = position.y() + fontoffset - fontrect[fonttype].h;
    sc_rect.xmax = position.x() + fontrect[fonttype].w;
    sc_rect.ymax = position.y() + fontoffset;

    ego_frect_t tx_rect;
    tx_rect.xmin = fontrect[fonttype].x * DX;
    tx_rect.ymin = fontrect[fonttype].y * DY;
    tx_rect.xmax = tx_rect.xmin + fontrect[fonttype].w * DX;
    tx_rect.ymax = tx_rect.ymin + fontrect[fonttype].h * DY;

    // shrink the texture size slightly
    tx_rect.xmin += BORDER;
    tx_rect.ymin += BORDER;
    tx_rect.xmax -= BORDER;
    tx_rect.ymax -= BORDER;

    drawQuad2D(_bitmapFontTexture, sc_rect, tx_rect, true, Colour4f(Colour3f::white(), alpha));
}

void UIManager::drawQuad2D(const std::shared_ptr<const Texture>& texture, const Rectangle2f& scr_rect, const Rectangle2f& tx_rect, const bool useAlpha, const Colour4f& tint) {
    auto& renderer = Renderer::get();
    renderer.getTextureUnit().setActivated(texture.get());
    renderer.setColour(tint);

    if (useAlpha) {
        renderer.setBlendingEnabled(true);
        renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);

        renderer.setAlphaTestEnabled(true);
        renderer.setAlphaFunction(CompareFunction::Greater, 0.0f);
    } else {
        renderer.setBlendingEnabled(false);
        renderer.setAlphaTestEnabled(false);
    }

    struct Vertex {
        float x, y;
        float s, t;
    };
    {
        VertexBufferScopedLock vblck(_textureQuadVertexBuffer);
        Vertex *vertices = vblck.get<Vertex>();
        vertices[0].x = scr_rect.getMin().x(); vertices[0].y = scr_rect.getMax().y(); vertices[0].s = tx_rect.getMin().x(); vertices[0].t = tx_rect.getMax().y();
        vertices[1].x = scr_rect.getMax().x(); vertices[1].y = scr_rect.getMax().y(); vertices[1].s = tx_rect.getMax().x(); vertices[1].t = tx_rect.getMax().y();
        vertices[2].x = scr_rect.getMax().x(); vertices[2].y = scr_rect.getMin().y(); vertices[2].s = tx_rect.getMax().x(); vertices[2].t = tx_rect.getMin().y();
        vertices[3].x = scr_rect.getMin().x(); vertices[3].y = scr_rect.getMin().y(); vertices[3].s = tx_rect.getMin().x(); vertices[3].t = tx_rect.getMin().y();
    }
    renderer.render(_textureQuadVertexBuffer, PrimitiveType::Quadriliterals, 0, 4);
}

void UIManager::drawQuad2D(const std::shared_ptr<const Texture>& texture, const ego_frect_t& scr_rect, const ego_frect_t& tx_rect, const bool useAlpha, const Colour4f& tint) {
    auto scr_rect_2 = Rectangle2f(Point2f(scr_rect.xmin, scr_rect.ymin),
                                  Point2f(scr_rect.xmax, scr_rect.ymax));
    auto tx_rect_2 = Rectangle2f(Point2f(tx_rect.xmin, tx_rect.ymin),
                                 Point2f(tx_rect.xmax, tx_rect.ymax));
    drawQuad2D(texture, scr_rect_2, tx_rect_2, useAlpha, tint);
}

void UIManager::fillRectangle(const Rectangle2f& rectangle, const bool useAlpha, const Colour4f& tint) {
    auto& renderer = Renderer::get();
    renderer.getTextureUnit().setActivated(nullptr);
    renderer.setColour(tint);

    if (useAlpha) {
        renderer.setBlendingEnabled(true);
        renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);

        renderer.setAlphaTestEnabled(true);
        renderer.setAlphaFunction(CompareFunction::Greater, 0.0f);
    } else {
        renderer.setBlendingEnabled(false);
        renderer.setAlphaTestEnabled(false);
    }

    struct Vertex {
        float x, y;
    };
    {
        VertexBufferScopedLock vblck(*_vertexBuffer);
        Vertex *vertices = vblck.get<Vertex>();
        vertices[0].x = rectangle.getMin().x(); vertices[0].y = rectangle.getMax().y();
        vertices[1].x = rectangle.getMax().x(); vertices[1].y = rectangle.getMax().y();
        vertices[2].x = rectangle.getMax().x(); vertices[2].y = rectangle.getMin().y();
        vertices[3].x = rectangle.getMin().x(); vertices[3].y = rectangle.getMin().y();
        renderer.render(*_vertexBuffer, PrimitiveType::Quadriliterals, 0, 4);
    }
}

} // namespace GUI
} // namepsace Ego
