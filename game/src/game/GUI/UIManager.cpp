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
    _renderSemaphore(0),
    _bitmapFontTexture(TextureManager::get().getTexture("mp_data/font_new_shadow")),
    _textureQuadVertexBuffer(4, Ego::VertexFormatDescriptor::get<Ego::VertexFormat::P2FT2F>())
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
			std::ostringstream os;
			os << __FILE__ << ":" << __LINE__ << ": UI manager is missing font with ID " << i << std::endl;
			Log::get().error("%s",os.str().c_str());
			throw std::runtime_error(os.str());
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

	renderer.setViewportRectangle(0, 0, sdl_scr.drawWidth, sdl_scr.drawHeight);

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

void UIManager::drawImage(const std::shared_ptr<const Ego::Texture>& img, float x, float y, float width, float height, const Ego::Colour4f& tint)
{
    ego_frect_t source;
    source.xmin = 0.0f;
    source.ymin = 0.0f;
    source.xmax = static_cast<float>(img->getSourceWidth())  / static_cast<float>(img->getWidth());
    source.ymax = static_cast<float>(img->getSourceHeight()) / static_cast<float>(img->getHeight());

    ego_frect_t destination;
    destination.xmin  = x;
    destination.ymin  = y;
    destination.xmax  = x + width;
    destination.ymax  = y + height;

    // Draw the image
    drawQuad2D(img, destination, source, true, tint);
}

bool UIManager::dumpScreenshot()
{
    int i;
    bool saved     = false;
    STRING szFilename, szResolvedFilename;

    // find a valid file name
    bool savefound = false;
    i = 0;
    while ( !savefound && ( i < 100 ) )
    {
        snprintf( szFilename, SDL_arraysize( szFilename ), "ego%02d.png", i );

        // lame way of checking if the file already exists...
        savefound = !vfs_exists( szFilename );
        if ( !savefound )
        {
            i++;
        }
    }

    if ( !savefound ) return false;

    // convert the file path to the correct write path
    strncpy( szResolvedFilename, szFilename, SDL_arraysize( szFilename ) );

    // if we are not using OpenGL, use SDL to dump the screen
    if (HAS_NO_BITS(SDL_GetWindowFlags(sdl_scr.window), SDL_WINDOW_OPENGL))
    {
        return IMG_SavePNG_RW(SDL_GetWindowSurface(sdl_scr.window), vfs_openRWopsWrite(szResolvedFilename), 1);
    }

    // we ARE using OpenGL
    {
        Ego::OpenGL::PushClientAttrib pca(GL_CLIENT_PIXEL_STORE_BIT);
        {
            SDL_Surface *temp;

            // create a SDL surface
            const auto& pixelFormatDescriptor = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8>();
            temp = SDL_CreateRGBSurface(0, sdl_scr.x, sdl_scr.y,
                                        pixelFormatDescriptor.getColorDepth().getDepth(),
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
                SDL_Rect rect = {0, 0, 0, 0};
                if (0 == rect.w && 0 == rect.h) {
                    rect.w = sdl_scr.x;
                    rect.h = sdl_scr.y;
                }
                if (rect.w > 0 && rect.h > 0) {
                    int y;
                    Uint8 * pixels;

                    GL_DEBUG(glGetError)();

                    //// use the allocated screen to tell OpenGL about the row length (including the lapse) in pixels
                    //// stolen from SDL ;)
                    // GL_DEBUG(glPixelStorei)(GL_UNPACK_ROW_LENGTH, temp->pitch / temp->format->BytesPerPixel );
                    // EGOBOO_ASSERT( GL_NO_ERROR == GL_DEBUG(glGetError)() );

                    //// since we have specified the row actual length and will give a pointer to the actual pixel buffer,
                    //// it is not necesssaty to mess with the alignment
                    // GL_DEBUG(glPixelStorei)(GL_UNPACK_ALIGNMENT, 1 );
                    // EGOBOO_ASSERT( GL_NO_ERROR == GL_DEBUG(glGetError)() );

                    // ARGH! Must copy the pixels row-by-row, since the OpenGL video memory is flipped vertically
                    // relative to the SDL Screen memory

                    // this is supposed to be a DirectX thing, so it needs to be tested out on glx
                    // there should probably be [SCREENSHOT_INVERT] and [SCREENSHOT_VALID] keys in setup.txt
                    pixels = (Uint8 *)temp->pixels;
                    for (y = rect.y; y < rect.y + rect.h; y++) {
                        GL_DEBUG(glReadPixels)(rect.x, (rect.h - y) - 1, rect.w, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels);
                        pixels += temp->pitch;
                    }
                    EGOBOO_ASSERT(GL_NO_ERROR == GL_DEBUG(glGetError)());
                }

                SDL_UnlockSurface(temp);

                // Save the file as a .bmp
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

float UIManager::drawBitmapFontString(const float startX, const float startY, const std::string &text, const uint32_t maxWidth, const float alpha)
{
    //Check if alpha is visible
    if(alpha <= 0.0f) {
        return startY;
    }

    //Current render position
    float x = startX;
    float y = startY;

    for(size_t cnt = 0; cnt < text.length(); ++cnt)
    {
        const uint8_t cTmp = text[cnt];

        // Check each new word for wrapping
        if ('~' == cTmp || C_LINEFEED_CHAR == cTmp || C_CARRIAGE_RETURN_CHAR == cTmp || std::isspace(cTmp)) {
            int endx = x + font_bmp_length_of_word(text.c_str() + cnt - 1);

            if (endx > maxWidth) {

                // Wrap the end and cut off spaces and tabs
                x = startX + fontyspacing;
                y += fontyspacing;
                while (std::isspace(text[cnt]) || '~' == text[cnt]) {
                    cnt++;
                }

                continue;
            }
        }

        // Use squiggle for tab
        if ('~' == cTmp) {
            x = (std::floor(x / TABADD) + 1.0f) * TABADD;
        }

        //Linefeed
        else if (C_LINEFEED_CHAR == cTmp) {
            x = startX;
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
            drawBitmapGlyph(iTmp, x, y, alpha);
            x += fontxspacing[iTmp];
        }
    }

    return y + fontyspacing;
}

void UIManager::drawBitmapGlyph(int fonttype, float xPos, float yPos, const float alpha)
{
    static constexpr GLfloat DX = 2.0f / 512.0f;
    static constexpr GLfloat DY = 1.0f / 256.0f;
    static constexpr GLfloat BORDER = 1.0f / 512.0f;

    ego_frect_t sc_rect;
    sc_rect.xmin = xPos;
    sc_rect.xmax = xPos + fontrect[fonttype].w;
    sc_rect.ymin = yPos + fontoffset - fontrect[fonttype].h;
    sc_rect.ymax = yPos + fontoffset;

    ego_frect_t tx_rect;
    tx_rect.xmin = fontrect[fonttype].x * DX;
    tx_rect.xmax = tx_rect.xmin + fontrect[fonttype].w * DX;
    tx_rect.ymin = fontrect[fonttype].y * DY;
    tx_rect.ymax = tx_rect.ymin + fontrect[fonttype].h * DY;

    // shrink the texture size slightly
    tx_rect.xmin += BORDER;
    tx_rect.xmax -= BORDER;
    tx_rect.ymin += BORDER;
    tx_rect.ymax -= BORDER;

    drawQuad2D(_bitmapFontTexture, sc_rect, tx_rect, true, Ego::Colour4f(1.0f, 1.0f, 1.0f, alpha));
}

void UIManager::drawQuad2D(const std::shared_ptr<const Ego::Texture>& texture, const ego_frect_t& scr_rect, const ego_frect_t& tx_rect, const bool useAlpha, const Ego::Colour4f& tint)
{
    Ego::OpenGL::PushAttrib pa(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    {
        auto& renderer = Ego::Renderer::get();
        renderer.getTextureUnit().setActivated(texture.get());
        renderer.setColour(tint);

        if (useAlpha) {
            renderer.setBlendingEnabled(true);
            renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

            renderer.setAlphaTestEnabled(true);
            renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);
        }
        else {
            renderer.setBlendingEnabled(false);
            renderer.setAlphaTestEnabled(false);
        }

        struct Vertex {
            float x, y;
            float s, t;
        };
        Ego::VertexBufferScopedLock vblck(_textureQuadVertexBuffer);
        Vertex *vertices = vblck.get<Vertex>();
        vertices[0].x = scr_rect.xmin; vertices[0].y = scr_rect.ymax; vertices[0].s = tx_rect.xmin; vertices[0].t = tx_rect.ymax;
        vertices[1].x = scr_rect.xmax; vertices[1].y = scr_rect.ymax; vertices[1].s = tx_rect.xmax; vertices[1].t = tx_rect.ymax;
        vertices[2].x = scr_rect.xmax; vertices[2].y = scr_rect.ymin; vertices[2].s = tx_rect.xmax; vertices[2].t = tx_rect.ymin;
        vertices[3].x = scr_rect.xmin; vertices[3].y = scr_rect.ymin; vertices[3].s = tx_rect.xmin; vertices[3].t = tx_rect.ymin;

        renderer.render(_textureQuadVertexBuffer, Ego::PrimitiveType::Quadriliterals, 0, 4);
    }
}
