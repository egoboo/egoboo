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

/// @file egolib/game/GUI/UIManager.cpp
/// @details The UIManager contains utilities for the GUI system and stores various shared resources
///             and properties for GUI components.
/// @author Johan Jansen

#include "egolib/game/GUI/UIManager.hpp"
#include "egolib/game/graphic.h"
#include "egolib/game/GUI/Material.hpp"
#include "egolib/Graphics/VertexFormat.hpp"
#include "egolib/game/game.h" //TODO: Remove only for DisplayMessagePrintf

namespace Ego::GUI {

UIManager::UIManager() :
    _fonts(),
    _renderSemaphore(0),
    _bitmapFontTexture(TextureManager::get().getTexture("mp_data/font_new_shadow")),
    _vertexDescriptor(descriptor_factory<idlib::vertex_format::P2F>()()),
    _textureQuadVertexDescriptor(descriptor_factory<idlib::vertex_format::P2FT2F>()()),
    _textureQuadVertexBuffer(idlib::video_buffer_manager::get().create_vertex_buffer(4, _textureQuadVertexDescriptor.get_size())) {
    //Load fonts from true-type files
    _fonts[FONT_DEFAULT] = FontManager::get().loadFont("mp_data/Bo_Chen.ttf", 24);
    _fonts[FONT_FLOATING_TEXT] = FontManager::get().loadFont("mp_data/FrostysWinterland.ttf", 24);
    _fonts[FONT_DEBUG] = FontManager::get().loadFont("mp_data/DejaVuSansMono.ttf", 10);
    _fonts[FONT_GAME] = FontManager::get().loadFont("mp_data/IMMORTAL.ttf", 14);

    //Sanity check that all fonts are loaded properly
#ifndef NDEBUG
    for (int i = 0; i < _fonts.size(); ++i) {
        if (!_fonts[i]) {
            auto e = Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "UI manager is missing font with ID ", i, Log::EndOfEntry);
            Log::get() << e;
            throw std::runtime_error(e.getText());
        }
    }
#endif
    _vertexBuffer = idlib::video_buffer_manager::get().create_vertex_buffer(4, _vertexDescriptor.get_size());
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
    renderer.setCullingMode(idlib::culling_mode::none);

    // Use normal alpha blending.
    renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_alpha);
    renderer.setBlendingEnabled(true);

    // Do not display the completely transparent portion.
    renderer.setAlphaTestEnabled(true);
    renderer.setAlphaFunction(idlib::compare_function::greater, 0.0f);

    /// Set the viewport rectangle.
    auto drawableSize = GraphicsSystem::get().window->drawable_size();
    renderer.setViewportRectangle(0, 0, drawableSize.x(), drawableSize.y());

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame.
    auto windowSize = GraphicsSystem::get().window->size();
    Matrix4f4f projection = idlib::orthographic_projection_matrix(0.0f, windowSize.x(), windowSize.y(), 0.0f, -1.0f, +1.0f);
    renderer.setProjectionMatrix(projection);
    renderer.setViewMatrix(idlib::identity<Matrix4f4f>());
    renderer.setWorldMatrix(idlib::identity<Matrix4f4f>());
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
    return GraphicsSystem::get().window->size().x();
}

int UIManager::getScreenHeight() const {
    return GraphicsSystem::get().window->size().y();
}

void UIManager::drawImage(const Point2f& position, const Vector2f& size, const std::shared_ptr<const Material>& material) {
    // Regardless of wether the material is textured or not, compute reasonable texture coordinates.
    auto source = Rectangle2f(Point2f(0.0f, 0.0f), Point2f(1.0f, 1.0f));
    // If the material has a texture, compute proper texture coordinates.
    if (material->getTexture()) {
        auto texture = material->getTexture();
        source = Rectangle2f(Point2f(0, 0),
                             Point2f(static_cast<float>(texture->getSourceWidth()) / static_cast<float>(texture->getWidth()),
                                     static_cast<float>(texture->getSourceHeight()) / static_cast<float>(texture->getHeight())));
    }
    auto target = Rectangle2f(position, position + size);
    // Draw the image
    drawQuad2D(target, source, material);
}

bool UIManager::dumpScreenshot() {
    auto make_filename = [](size_t i) {
        std::stringstream stream;
        stream << "ego" << std::setfill('0') << std::setw(2) << i << ".png";
        return stream.str();
    };

    auto file = make_filename(0);
    auto file_exists = vfs_exists(file);

    for (auto i = 1; file_exists && i < 100; ++i) {
        file = make_filename(i);
        file_exists = vfs_exists(file);
    }

    if (file_exists) return false;

    try
    {
        auto screenshot = GraphicsSystem::get().window->getContents();
        ImageManager::get().save_as_png(screenshot, file);
    }
    catch (...)
    {
        DisplayMsg_printf("failed to save screenshot to %s", file.c_str());
        return false;
    }
    DisplayMsg_printf("screenshot saved to %s", file.c_str());
    return true;
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

    // The target rectangle.
    auto sc_rect = Rectangle2f(Point2f(position.x(),                        position.y() + fontoffset - fontrect[fonttype].h),
                               Point2f(position.x() + fontrect[fonttype].w, position.y() + fontoffset));

    ego_frect_t tx_rect;
    tx_rect.xmin = fontrect[fonttype].x * DX;
    tx_rect.ymin = fontrect[fonttype].y * DY;
    tx_rect.xmax = tx_rect.xmin + fontrect[fonttype].w * DX;
    tx_rect.ymax = tx_rect.ymin + fontrect[fonttype].h * DY;

    // shrink the texture size slightly
    // FIXME: This might result in xmin > xmax or ymin > ymax.
    tx_rect.xmin += BORDER;
    tx_rect.ymin += BORDER;
    tx_rect.xmax -= BORDER;
    tx_rect.ymax -= BORDER;

    drawQuad2D(sc_rect, tx_rect, std::make_shared<Material>(_bitmapFontTexture, Colour4f(Colour3f::white(), alpha), true));
}

void UIManager::drawQuad2D(const Rectangle2f& scr_rect, const Rectangle2f& tx_rect, const std::shared_ptr<const Material>& material) {
    material->apply();
    drawQuad2d(scr_rect, tx_rect);
}

void UIManager::drawQuad2D(const Rectangle2f& scr_rect, const ego_frect_t& tx_rect, const std::shared_ptr<const Material>& material) {
    auto tx_rect_2 = Rectangle2f(Point2f(tx_rect.xmin, tx_rect.ymin),
                                 Point2f(tx_rect.xmax, tx_rect.ymax));
    drawQuad2D(scr_rect, tx_rect_2, material);
}

void UIManager::fillRectangle(const Rectangle2f& rectangle, const bool useAlpha, const Colour4f& tint) {
    auto material = std::make_shared<Material>(nullptr, tint, useAlpha);
    material->apply();
    drawQuad2d(rectangle);
}

void UIManager::drawQuad2d(const Rectangle2f& target, const Rectangle2f& source) {
    struct Vertex {
        float x, y;
        float s, t;
    };
    auto& renderer = Renderer::get();
    {
        idlib::vertex_buffer_scoped_lock vblck(*_textureQuadVertexBuffer);
        Vertex *v = vblck.get<Vertex>();
        // left/bottom
        v->x = target.get_min().x(); v->y = target.get_max().y();
        v->s = source.get_min().x(); v->t = source.get_max().y();
        v++;

        // right/bottom
        v->x = target.get_max().x(); v->y = target.get_max().y();
        v->s = source.get_max().x(); v->t = source.get_max().y();
        v++;

        // right/top
        v->x = target.get_max().x(); v->y = target.get_min().y();
        v->s = source.get_max().x(); v->t = source.get_min().y();
        v++;

        // left/top
        v->x = target.get_min().x(); v->y = target.get_min().y();
        v->s = source.get_min().x(); v->t = source.get_min().y();
        v++;
    }
    renderer.render(*_textureQuadVertexBuffer, _textureQuadVertexDescriptor, idlib::primitive_type::quadriliterals, 0, 4);
}

void UIManager::drawQuad2d(const Rectangle2f& target) {
    drawQuad2d(target, Rectangle2f(Point2f(0.0f, 0.0), Point2f(1.0f, 1.0f)));
}

} // namespace Ego::GUI
