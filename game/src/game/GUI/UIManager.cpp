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
///             and properties for GUIComponents.
/// @author Johan Jansen

#include "game/GUI/UIManager.hpp"
#include "game/graphic.h"
#include "game/renderer_2d.h"

UIManager::UIManager() :
    _fonts(),
    _renderSemaphore(0)
{
    //Load fonts from true-type files
    _fonts[FONT_DEFAULT] = Ego::FontManager::loadFont("mp_data/Bo_Chen.ttf", 24); 
    _fonts[FONT_FLOATING_TEXT] = Ego::FontManager::loadFont("mp_data/FrostysWinterland.ttf", 24); 
    _fonts[FONT_DEBUG] = Ego::FontManager::loadFont("mp_data/DejaVuSansMono.ttf", 10); 
    _fonts[FONT_GAME] = Ego::FontManager::loadFont("mp_data/IMMORTAL.ttf", 14); 

    //Sanity check that all fonts are loaded properly
#ifndef NDEBUG
    for(int i = 0; i < _fonts.size(); ++i)
    {
        if(!_fonts[i])
        {
            log_error("UIManager missing font with ID %d!", i);
        }
    }
#endif

    const auto& vertexFormat = Ego::VertexFormatDescriptor::get<Ego::VertexFormat::P2F>();
    _vertexBuffer = std::make_shared<Ego::VertexBuffer>(4, vertexFormat);
}

UIManager::~UIManager()
{
    _vertexBuffer = nullptr;
    // free fonts before font manager
    for(std::shared_ptr<Ego::Font> &font : _fonts)
    {
        font.reset();
    }
}

void UIManager::beginRenderUI()
{
    //Handle recusive loops that trigger beginRenderUI
    _renderSemaphore++;
    if(_renderSemaphore > 1) {
        return;
    }

	auto& renderer = Ego::Renderer::get();

    // do not use the ATTRIB_PUSH macro, since the glPopAttrib() is in a different function
    GL_DEBUG( glPushAttrib )( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT );

    // don't worry about hidden surfaces
	renderer.setDepthTestEnabled(false);

    // draw draw front and back faces of polygons
	renderer.setCullingMode(Ego::CullingMode::None);

    // use normal alpha blending
	renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);
	renderer.setBlendingEnabled(true);

    // do not display the completely transparent portion
	renderer.setAlphaTestEnabled(true);
	renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);

	renderer.setViewportRectangle(0, 0, getScreenWidth(), getScreenHeight());

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame

    // store the GL_PROJECTION matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
	Matrix4f4f projection = Ego::Math::Transform::ortho(0.0f, getScreenWidth(), getScreenHeight(), 0.0f, -1.0f, +1.0f);
	renderer.loadMatrix(projection);

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
	renderer.loadMatrix(Matrix4f4f::identity());
}

void UIManager::endRenderUI()
{
    //Handle recusive loops that trigger beginRenderUI
    _renderSemaphore--;
    if(_renderSemaphore > 0) {
        return;
    }

    // Restore the GL_PROJECTION matrix
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();

    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    Ego::Renderer::get().loadMatrix(Matrix4f4f::identity());

    // Re-enable any states disabled by gui_beginFrame
    // do not use the ATTRIB_POP macro, since the glPushAttrib() is in a different function
    GL_DEBUG( glPopAttrib )();
}

int UIManager::getScreenWidth() const
{
    return sdl_scr.x;
}

int UIManager::getScreenHeight() const
{
    return sdl_scr.y;
}

void UIManager::drawImage(const oglx_texture_t &img, float x, float y, float width, float height, const Ego::Colour4f& tint)
{
    ego_frect_t source;
    source.xmin = 0.0f;
    source.ymin = 0.0f;
    source.xmax = ( float ) img.getSourceWidth()  / ( float ) img.getWidth();
    source.ymax = ( float ) img.getSourceHeight() / ( float ) img.getHeight();

    ego_frect_t destination;
    destination.xmin  = x;
    destination.ymin  = y;
    destination.xmax  = x + width;
    destination.ymax  = y + height;

    // Draw the image
    draw_quad_2d(&img, destination, source, true, tint);
}
