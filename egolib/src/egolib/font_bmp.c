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
#include "egolib/log.h"

#include "egolib/vfs.h"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"

#include "egolib/extensions/ogl_texture.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int       fontoffset;                 // Line up fonts from top of screen
SDL_Rect  fontrect[NUMFONT];          // The font rectangles
Uint8     fontxspacing[NUMFONT];      // The spacing stuff
Uint8     fontyspacing;

Uint8     asciitofont[256];           // Conversion table

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void font_bmp_init( void )
{
    /// @author BB
    /// @details fill in default values

    Uint16 i, ix, iy, cnt;
    float dx, dy;

    // Mark all as unused
    for ( cnt = 0; cnt < 256; cnt++ )
    {
        asciitofont[cnt] = 255;
    }

    dx = 256.0f / NUMFONTX;
    dy = 256.0f / NUMFONTY;
    for ( i = 0; i < NUMFONT; i++ )
    {
        ix = i % NUMFONTX;
        iy = i / NUMFONTX;

        fontrect[i].x = ix * dx;
        fontrect[i].w = dx;
        fontrect[i].y = iy * dy;
        fontrect[i].h = dy;
        fontxspacing[i] = 0;
    }
    fontyspacing = dy;
}

//--------------------------------------------------------------------------------------------
void font_bmp_load_vfs( oglx_texture_t * tx_font, const char* szBitmap, const char* szSpacing )
{
    /// @author ZZ
    /// @details This function loads the font bitmap and sets up the coordinates
    ///    of each font on that bitmap...  Bitmap must have 16x6 fonts

    int cnt, xsize, ysize;
    int stt_x, stt_y;
    int xspacing, yspacing;
    char cTmp;

    font_bmp_init();

    if ( NULL == tx_font ) return;

    if ( INVALID_GL_ID == ego_texture_load_vfs( tx_font, szBitmap, TRANSCOLOR ) )
    {
        log_error( "load_font() - Cannot load file! (\"%s\")\n", szBitmap );
    }

    // Get the size of the bitmap
    xsize = oglx_texture_getImageWidth( tx_font );
    ysize = oglx_texture_getImageHeight( tx_font );
    if ( 0 == xsize || 0 == ysize )
    {
        log_error( "Bad font size! (%i, %i)\n", xsize, ysize );
    }

    // Figure out the general size of each font
    //int ydiv = ysize / NUMFONTY;
    //int xdiv = xsize / NUMFONTX;

    // Figure out where each font is and its spacing
    ReadContext ctxt(szSpacing);
    if (!ctxt.ensureOpen()) {
        log_error("unable to read font spacing file %s for spacing (%i,%i)\n", szSpacing, xsize, ysize);
    }

    stt_x = 0;
    stt_y = 0;

    // Uniform font height is at the top
    yspacing = vfs_get_next_int(ctxt);
    fontoffset = yspacing;
    for ( cnt = 0; cnt < NUMFONT && ctxt.skipToColon(true); cnt++ )
    {
		vfs_scanf(ctxt._file, "%c", &cTmp); /* @todo Do not use scanf to read a single letter. */
        xspacing = ctxt.readInt();
        if ( asciitofont[( Uint8 )cTmp] == 255 ) asciitofont[( Uint8 )cTmp] = ( Uint8 ) cnt;
        if ( stt_x + xspacing + 1 > 255 )
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
    fontyspacing = ( yspacing >> 1 ) + FONTADD;
}

//--------------------------------------------------------------------------------------------
int font_bmp_length_of_word( const char *szText )
{
    /// @author ZZ
    /// @details This function returns the number of pixels the
    ///    next word will take on screen in the x direction

    // Count all preceeding spaces
    int x = 0;
    int cnt = 0;
    Uint8 cTmp = szText[cnt];

    while ( ' ' == cTmp || '~' == cTmp || C_NEW_LINE_CHAR == cTmp )
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

    while ( ' ' != cTmp && '~' != cTmp && C_NEW_LINE_CHAR != cTmp && CSTR_END != cTmp )
    {
        x += fontxspacing[asciitofont[cTmp]];
        cnt++;
        cTmp = szText[cnt];
    }

    return x;
}
