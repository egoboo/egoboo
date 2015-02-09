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

/// @file game/gui/UIManager.cpp
/// @details The UIManager contains utilities for the GUI system and stores various shared resources
///		     and properties for GUIComponents.
/// @author Johan Jansen

#include "game/gui/UIManager.hpp"
#include "game/graphic.h"
#include "game/renderer_2d.h"
#include "game/graphic_texture.h"

UIManager::UIManager() :
	_defaultFont(nullptr),
	_renderSemaphore(0)
{
	fnt_init();
	_defaultFont = fnt_loadFont("mp_data/Bo_Chen.ttf", 24);
}

UIManager::~UIManager()
{
	fnt_freeFont(_defaultFont);
}

void UIManager::beginRenderUI()
{
	//Handle recusive loops that trigger beginRenderUI
	_renderSemaphore++;
	if(_renderSemaphore > 1) {
		return;
	}

	// do not use the ATTRIB_PUSH macro, since the glPopAttrib() is in a different function
    GL_DEBUG( glPushAttrib )( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT );

    // don't worry about hidden surfaces
	Ego::Renderer::getSingleton()->setDepthTestEnabled(false);

    // draw draw front and back faces of polygons
    oglx_end_culling();                                     				   // GL_ENABLE_BIT

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );                                     // GL_ENABLE_BIT

    // use normal alpha blending
    GL_DEBUG( glEnable )( GL_BLEND );                                          // GL_ENABLE_BIT
    GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );           // GL_COLOR_BUFFER_BIT

    // do not display the completely transparent portion
    GL_DEBUG( glEnable )( GL_ALPHA_TEST );                                     // GL_ENABLE_BIT
    GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                               // GL_COLOR_BUFFER_BIT

    GL_DEBUG( glViewport )( 0, 0, getScreenWidth(), getScreenHeight());                      // GL_VIEWPORT_BIT

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame

    // store the GL_PROJECTION matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();

	fmat_4x4_t projection;
	projection.setOrtho(0.0f, getScreenWidth(), getScreenHeight(), 0.0f, -1.0f, +1.0f);
	Ego::Renderer::getSingleton()->loadMatrix(projection);

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glLoadIdentity )();
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
    GL_DEBUG( glLoadIdentity )();

    // Re-enable any states disabled by gui_beginFrame
    // do not use the ATTRIB_POP macro, since the glPushAttrib() is in a different function
    GL_DEBUG( glPopAttrib )();
}

int UIManager::getScreenWidth() const
{
	return SDL_GetVideoInfo()->current_w;
}

int UIManager::getScreenHeight() const
{
	return SDL_GetVideoInfo()->current_h;
}

void UIManager::drawImage(oglx_texture_t &img, float x, float y, float width, float height, const Ego::Colour4f& tint)
{
    ego_frect_t source;
    source.xmin = 0.0f;
    source.ymin = 0.0f;
    source.xmax = ( float ) oglx_texture_getImageWidth( &img )  / ( float ) oglx_texture_t::getTextureWidth( &img );
    source.ymax = ( float ) oglx_texture_getImageHeight( &img ) / ( float ) oglx_texture_t::getTextureHeight( &img );

    ego_frect_t destination;
    destination.xmin  = x;
    destination.ymin  = y;
    destination.xmax  = x + width;
    destination.ymax  = y + height;

    // Draw the image
    draw_quad_2d(&img, destination, source, true, tint);
}
