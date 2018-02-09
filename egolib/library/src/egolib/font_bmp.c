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

/// @file egolib/font_bmp.c
/// @brief bitmapped font stuff
/// @details

#include "egolib/font_bmp.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/Graphics/TextureManager.hpp"

#include "egolib/vfs.h"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"

int       fontoffset;                 // Line up fonts from top of screen
SDL_Rect  fontrect[NUMFONT];          // The font rectangles
uint8_t   fontxspacing[NUMFONT];      // The spacing stuff
uint8_t   fontyspacing;

uint8_t   asciitofont[256];           // Conversion table

void font_bmp_init()
{
    /// @author BB
    /// @details fill in default values

    // Mark all as unused
    for (uint16_t i = 0; i < 256; ++i) {
        asciitofont[i] = 255;
    }

    const float dx = 256.0f / NUMFONTX;
    const float dy = 256.0f / NUMFONTY;
    for (uint16_t i = 0; i < NUMFONT; ++i) {
        uint16_t ix = i % NUMFONTX;
        uint16_t iy = i / NUMFONTX;

        fontrect[i].x = ix * dx;
        fontrect[i].w = dx;
        fontrect[i].y = iy * dy;
        fontrect[i].h = dy;
        fontxspacing[i] = 0;
    }
    fontyspacing = dy;
}

void font_bmp_load_vfs( const std::string& szBitmap, const char* szSpacing )
{
    /// @author ZZ
    /// @details This function loads the font bitmap and sets up the coordinates
    ///    of each font on that bitmap...  Bitmap must have 16x6 fonts
    font_bmp_init();

    const std::shared_ptr<Ego::Texture> &fontTexture = Ego::TextureManager::get().getTexture(szBitmap);
	if (fontTexture->isDefault())
    {
        auto e = Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "`", szBitmap, "`", ": ", "unable to load file", Log::EndOfEntry);
        Log::get() << e;
        throw std::runtime_error(e.getText());
    }

    // Get the size of the bitmap
    int xsize = fontTexture->getSourceWidth();
    int ysize = fontTexture->getSourceHeight();
    if ( 0 == xsize || 0 == ysize )
    {
        auto e = Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "`", szBitmap, "`", ": ", "bad font size ", "(", xsize, ", ", ysize, ")", Log::EndOfEntry);
        Log::get() << e;
        throw std::runtime_error(e.getText());
    }

    // Figure out where each font is and its spacing
    ReadContext ctxt(szSpacing);

    uint32_t stt_x = 0;
    uint32_t stt_y = 0;

    // Uniform font height is at the top
    uint32_t yspacing = vfs_get_next_int(ctxt);
    fontoffset = yspacing;
    for (size_t cnt = 0; cnt < NUMFONT && ctxt.skipToColon(true); ++cnt)
    {
        uint8_t chr = static_cast<uint8_t>(ctxt.readCharacterLiteral());
        uint32_t xspacing = ctxt.readIntegerLiteral();
        
        if (asciitofont[chr] == 255) {
            asciitofont[chr] = static_cast<uint8_t>(cnt);
        }

        if (stt_x + xspacing + 1 > 255)
        {
            stt_x = 0;
            stt_y += yspacing;
        }

        fontrect[cnt].x = stt_x;
        fontrect[cnt].w = xspacing;
        fontrect[cnt].y = stt_y;
        fontrect[cnt].h = yspacing - 2;
        fontxspacing[cnt] = xspacing + 1;

        stt_x += xspacing + 1;
    }

    // Space between lines
    fontyspacing = (yspacing / 2) + FONTADD;
}

//--------------------------------------------------------------------------------------------
int font_bmp_length_of_word( const std::string& szText )
{
    /// @author ZZ
    /// @details This function returns the number of pixels the
    ///    next word will take on screen in the x direction

    // Count all preceeding spaces
    int x = 0;
    int cnt = 0;
    uint8_t cTmp = szText[cnt];

    while ( ' ' == cTmp || '~' == cTmp || C_LINEFEED_CHAR == cTmp )
    {
        if ( ' ' == cTmp )
        {
            x += fontxspacing[asciitofont[cTmp]];
        }
        else if ( '~' == cTmp )
        {
            x = ( x & TABAND ) + TABADD;
        }

        cnt++;
        cTmp = szText[cnt];
    }

    while ( ' ' != cTmp && '~' != cTmp && C_LINEFEED_CHAR != cTmp && CSTR_END != cTmp )
    {
        x += fontxspacing[asciitofont[cTmp]];
        cnt++;
        cTmp = szText[cnt];
    }

    return x;
}
