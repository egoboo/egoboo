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

/// @file  game/renderer_2d.c
/// @brief Implementation of the 2d renderer functions
/// @details

#include "game/egoboo.h"
#include "game/renderer_2d.h"

//TODO: Remove
#include "game/Core/GameEngine.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "game/GUI/MessageLog.hpp"
// PRIVATE FUNCTIONS
//--------------------------------------------------------------------------------------------

static int  _va_draw_string( float x, float y, const char *format, va_list args );
static void gfx_begin_text();
static void gfx_end_text();

//--------------------------------------------------------------------------------------------
// MODULE "PRIVATE" FUNCTIONS
//--------------------------------------------------------------------------------------------
int _va_draw_string( float x, float y, const char *format, va_list args )
{
    int cnt = 1;
    int x_stt;
    STRING szText;
    Uint8 cTmp;

    const std::shared_ptr<Ego::Texture> &tx_ptr = TextureManager::get().getTexture("mp_data/font_new_shadow");
    if ( nullptr == tx_ptr ) return y;

    if ( vsnprintf( szText, SDL_arraysize( szText ) - 1, format, args ) <= 0 )
    {
        return y;
    }

    gfx_begin_text();
    {
        x_stt = x;
        cnt = 0;
        cTmp = szText[cnt];
        while ( CSTR_END != cTmp )
        {
            Uint8 iTmp;

            // Convert ASCII to our own little font
            if ( '~' == cTmp )
            {
                // Use squiggle for tab
                x = ( std::floor(( float )x / ( float )TABADD ) + 1.0f ) * TABADD;
            }
            else if ( C_LINEFEED_CHAR == cTmp )
            {
                x  = x_stt;
                y += fontyspacing;
            }
            else if ( isspace( cTmp ) )
            {
                // other whitespace
                iTmp = asciitofont[cTmp];
                x += fontxspacing[iTmp] / 2;
            }
            else
            {
                // Normal letter
                iTmp = asciitofont[cTmp];
                _gameEngine->getUIManager()->drawBitmapGlyph(iTmp, x, y, 1.0f);
                x += fontxspacing[iTmp];
            }

            cnt++;
            cTmp = szText[cnt];
        }
    }
    gfx_end_text();

    y += fontyspacing;

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_string_raw( float x, float y, const char *format, ... )
{
    /// @author BB
    /// @details the same as draw string, but it does not use the gfx_begin_2d() ... gfx_end_2d()
    ///    bookends.

    va_list args;
    va_start( args, format );
    y = _va_draw_string( x, y, format, args );
    va_end( args );
    return y;
}

int DisplayMsgs::printf( const char *format, ... )
{
    STRING szTmp;
    
    va_list args;
    va_start( args, format );
    int retval = vsnprintf(szTmp, SDL_arraysize(szTmp), format, args);
    DisplayMsg_print(szTmp);
    va_end( args );

    return retval;
}

void DisplayMsgs::print( const char *text )
{
    _gameEngine->getActivePlayingState()->getMessageLog()->addMessage(text);
}

//--------------------------------------------------------------------------------------------
// MODE CONTROL
//--------------------------------------------------------------------------------------------
void gfx_begin_2d()
{
	auto& renderer = Ego::Renderer::get();
    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_SCISSOR_BIT );

    // Reset the Projection Matrix
    // Set up an orthogonal projection
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
	Matrix4f4f projection = Ego::Math::Transform::ortho(0.0, sdl_scr.x, sdl_scr.y, 0.0, -1.0f, +1.0f);
	renderer.loadMatrix(projection);

    // Reset the Modelview Matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	renderer.loadMatrix(Matrix4f4f::identity());

    // remove any scissor test
	renderer.setScissorTestEnabled(false);

    // don't worry about hidden surfaces
	renderer.setDepthTestEnabled(false);

    // stop culling backward facing polygons
	renderer.setCullingMode(Ego::CullingMode::None);
}

//--------------------------------------------------------------------------------------------
void gfx_end_2d()
{
    // get the old modelview matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // get the old projection matrix
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();

    // - restores the culling mode
    // - restores the culling depth-testing mode
    // - restores the SCISSOR mode
    ATTRIB_POP( __FUNCTION__ );

    // leave the MatrixMode in GL_MODELVIEW
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
}

//--------------------------------------------------------------------------------------------
void gfx_begin_text()
{
    // do not use the ATTRIB_PUSH macro, since the glPopAttrib() is in a different function
    GL_DEBUG( glPushAttrib )( GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );

	auto& renderer = Ego::Renderer::get();
    // do not display the completely transparent portion
    renderer.setAlphaTestEnabled(true);
	renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);

    renderer.setBlendingEnabled(true);
	renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

    // don't worry about hidden surfaces
	renderer.setDepthTestEnabled(false);

    // draw draw front and back faces of polygons
	renderer.setCullingMode(Ego::CullingMode::None);

	renderer.setColour(Ego::Math::Colour4f::white());// GL_CURRENT_BIT
}

//--------------------------------------------------------------------------------------------
void gfx_end_text()
{
    // do not use the ATTRIB_POP macro, since the glPushAttrib() is in a different function
    GL_DEBUG( glPopAttrib )();
}

void draw_quad_2d(const ego_frect_t scr_rect, bool use_alpha, const Ego::Colour4f& tint) {
    Ego::OpenGL::PushAttrib pa(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    {
        auto& renderer = Ego::Renderer::get();
        renderer.getTextureUnit().setActivated(nullptr);
        renderer.setColour(tint);

        if (use_alpha) {
            renderer.setBlendingEnabled(true);
            renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

            renderer.setAlphaTestEnabled(true);
            renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);
        } else {
            renderer.setBlendingEnabled(false);
            renderer.setAlphaTestEnabled(false);
        }

        Ego::VertexBuffer vb(4, Ego::VertexFormatDescriptor::get<Ego::VertexFormat::P2F>());
        {
            struct Vertex {
                float x, y;
            };
            Ego::VertexBufferScopedLock vblck(vb);
            Vertex *vertices = vblck.get<Vertex>();
            vertices[0].x = scr_rect.xmin; vertices[0].y = scr_rect.ymax;
            vertices[1].x = scr_rect.xmax; vertices[1].y = scr_rect.ymax;
            vertices[2].x = scr_rect.xmax; vertices[2].y = scr_rect.ymin;
            vertices[3].x = scr_rect.xmin; vertices[3].y = scr_rect.ymin;
        }
        renderer.render(vb, Ego::PrimitiveType::Quadriliterals, 0, 4);
    }
}