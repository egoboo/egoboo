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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - GLtexture.c
* Loads texture files into OpenGL textures.
*/

#include "ogl_texture.h"
#include "graphic.h"
#include "log.h"

#include "egoboo.h" // GAC - Needed for Win32 stuff

#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define ErrorImage_width 2
#define ErrorImage_height 2

static GLboolean ErrorImage_defined = GL_FALSE;

typedef GLubyte SET_PACKING( image_row_t[ErrorImage_width][4], 1 );

static GLubyte ErrorImage[ErrorImage_height][ErrorImage_width][4];

void ErrorImage_create( void )
{
    // BB > define a default "error texture"

    int i, j;

    for ( i = 0; i < ErrorImage_height; i++ )
    {
        for ( j = 0; j < ErrorImage_width; j++ )
        {
            if ( 0 == (( i&0x1 ) ^( j&0x1 ) ) )
            {
                ErrorImage[i][j][0] = ( GLubyte ) 255;
                ErrorImage[i][j][1] = ( GLubyte ) 0;
                ErrorImage[i][j][2] = ( GLubyte ) 0;
            }
            else
            {
                ErrorImage[i][j][0] = ( GLubyte ) 0;
                ErrorImage[i][j][1] = ( GLubyte ) 255;
                ErrorImage[i][j][2] = ( GLubyte ) 255;
            }

            ErrorImage[i][j][3] = ( GLubyte ) 255;
        }
    }

    ErrorImage_defined = GL_TRUE;
}

void ErrorImage_bind( GLenum target, GLuint id )
{
    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

        glBindTexture( target, id );

        glTexParameteri( target, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( target, GL_TEXTURE_WRAP_T, GL_REPEAT );
        glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        if ( target == GL_TEXTURE_1D )
        {
            glTexImage1D( GL_TEXTURE_1D, 0, GL_RGBA, ErrorImage_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, ErrorImage );
        }
        else
        {
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, ErrorImage_width, ErrorImage_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ErrorImage );
        }
    }
    glPopClientAttrib();

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// The next two functions are borrowed from the gl_font.c test program from SDL_ttf
int powerOfTwo( int input )
{
    int value = 1;

    while ( value < input )
    {
        value <<= 1;
    }

    return value;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
GLtexture * GLtexture_new( GLtexture * ptx )
{
    GLtexture_delete( ptx );
    if ( !ErrorImage_defined ) ErrorImage_create();

    memset( ptx, 0, sizeof( GLtexture ) );

    // only need one textureID per texture
    // do not need to ask for a new id, even if we change the texture data
    glGenTextures( 1, &( ptx->textureID ) );

    // set the image to be clamped in s and t
    ptx->wrap_s = GL_CLAMP;
    ptx->wrap_t = GL_CLAMP;

    return ptx;
}

//--------------------------------------------------------------------------------------------
void    GLtexture_delete( GLtexture * ptx )
{
    // actually delete the OpenGL texture data
    glDeleteTextures( 1, &ptx->textureID );
    ptx->textureID = INVALID_TX_ID;

    // set the image to be clamped in s and t
    ptx->wrap_s = GL_CLAMP;
    ptx->wrap_t = GL_CLAMP;

    // Reset the other data
    ptx->imgH = ptx->imgW = ptx->txW = ptx->txH  = 0;
    ptx->name[0] = '\0';
}

//--------------------------------------------------------------------------------------------
Uint32  GLtexture_Convert( GLenum tx_target, GLtexture *texture, SDL_Surface * image, Uint32 key )
{
    SDL_PixelFormat * pformat;
    SDL_PixelFormat   tmpformat;
    if ( NULL == texture || NULL == image ) return INVALID_TX_ID;

    // make sure the old texture has been freed
    GLtexture_Release( texture );
    if ( NULL == image ) return INVALID_TX_ID;

    /* set the color key, if valid */
    if ( NULL != image->format && NULL != image->format->palette && INVALID_KEY != key )
    {
        SDL_SetColorKey( image, SDL_SRCCOLORKEY, key );
    }

    /* Set the texture's alpha */
    texture->alpha = FP8_TO_FLOAT( image->format->alpha );

    /* Set the original image's size (incase it's not an exact square of a power of two) */
    texture->imgH = image->h;
    texture->imgW = image->w;

    /* Determine the correct power of two greater than or equal to the original image's size */
    texture->txH = powerOfTwo( image->h );
    texture->txW = powerOfTwo( image->w );

    displaySurface  = SDL_GetVideoSurface();
    pformat = displaySurface->format;
    memcpy( &tmpformat, displaySurface->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format
    if ( 0 != ( image->flags & ( SDL_SRCALPHA | SDL_SRCCOLORKEY ) ) )
    {
        // the source image has an alpha channel
        /// @todo need to take into account the possible SDL_PixelFormat::Rloss, SDL_PixelFormat::Gloss, ...
        /// parameters

        int i;

        // create the mask
        // this will work if both endian systems think they have "RGBA" graphics
        // if you need a different pixel format (ARGB or BGRA or whatever) this section
        // will have to be changed to reflect that
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        tmpformat.Amask = ( Uint32 )( 0xFF << 24 );
        tmpformat.Bmask = ( Uint32 )( 0xFF << 16 );
        tmpformat.Gmask = ( Uint32 )( 0xFF <<  8 );
        tmpformat.Rmask = ( Uint32 )( 0xFF <<  0 );
#else
        tmpformat.Amask = ( Uint32 )( 0xFF <<  0 );
        tmpformat.Bmask = ( Uint32 )( 0xFF <<  8 );
        tmpformat.Gmask = ( Uint32 )( 0xFF << 16 );
        tmpformat.Rmask = ( Uint32 )( 0xFF << 24 );
#endif

        tmpformat.BitsPerPixel  = displaySurface->format->BitsPerPixel;
        tmpformat.BytesPerPixel = displaySurface->format->BytesPerPixel;

        for ( i = 0; i < scrz && ( tmpformat.Amask & ( 1 << i ) ) == 0; i++ );
        if ( 0 == ( tmpformat.Amask & ( 1 << i ) ) )
        {
            // no alpha bits available
            tmpformat.Ashift = 0;
            tmpformat.Aloss  = 8;
        }
        else
        {
            // normal alpha channel
            tmpformat.Ashift = i;
            tmpformat.Aloss  = 0;
        }

        pformat = &tmpformat;
    }
    else
    {
        // the source image has no alpha channel
        // convert it to the screen format

        // correct the bits and bytes per pixel
        tmpformat.BitsPerPixel  = 32 - ( tmpformat.Rloss + tmpformat.Gloss + tmpformat.Bloss + tmpformat.Aloss );
        tmpformat.BytesPerPixel = tmpformat.BitsPerPixel / 8 + (( tmpformat.BitsPerPixel % 8 ) > 0 ? 1 : 0 );

        pformat = &tmpformat;
    }

    {
        //convert the image format to the correct format
        SDL_Surface * tmp;
        Uint32 convert_flags = SDL_SWSURFACE;
        tmp = SDL_ConvertSurface( image, pformat, convert_flags );
        SDL_FreeSurface( image );
        image = tmp;

        // fix the alpha channel on the new SDL_Surface.  For some reason, SDL wants to create
        // surface with surface->format->alpha==0x00 which causes a problem if we have to
        // use the SDL_BlitSurface() function below.  With the surface alpha set to zero, the
        // image will be converted to BLACK!
        //
        // The following statement tells SDL
        //   1) to set the image to opaque
        //   2) not to alpha blend the surface on blit
        SDL_SetAlpha( image, 0, SDL_ALPHA_OPAQUE );
        SDL_SetColorKey( image, 0, 0 );
    }

    // create a texture that is acceptable to OpenGL (height and width are powers of 2)
    if ( texture->imgH != texture->txH || texture->imgW != texture->txW )
    {
        SDL_Surface * tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, texture->txW, texture->txH, pformat->BitsPerPixel, pformat->Rmask, pformat->Gmask, pformat->Bmask, pformat->Amask );

        SDL_BlitSurface( image, &image->clip_rect, tmp, &image->clip_rect );
        SDL_FreeSurface( image );
        image = tmp;
    };

    /* Generate an OpenGL texture ID */
    if ( 0 == texture->textureID || INVALID_TX_ID == texture->textureID )
    {
        glGenTextures( 1, &texture->textureID );
    }

    texture->texture_target = tx_target;

    /* Set up some parameters for the format of the OpenGL texture */
    GLtexture_Bind( texture );

    /* actually create the OpenGL textures */
    if ( image->format->Aloss == 8 && tx_target == GL_TEXTURE_2D )
    {
        if ( texturefilter >= TX_MIPMAP && tx_target == GL_TEXTURE_2D )
            gluBuild2DMipmaps( tx_target, GL_RGB, image->w, image->h, GL_BGR_EXT, GL_UNSIGNED_BYTE, image->pixels );    //With mipmapping
        else
            glTexImage2D( tx_target, 0, GL_RGB, image->w, image->h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, image->pixels );
    }
    else
    {
        if ( texturefilter >= TX_MIPMAP && tx_target == GL_TEXTURE_2D )
            gluBuild2DMipmaps( GL_TEXTURE_2D, 4, image->w, image->h, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels );    //With mipmapping
        else
            glTexImage2D( tx_target, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels );
    }

    SDL_FreeSurface( image );

    texture->texture_target = tx_target;

    return texture->textureID;
}

//--------------------------------------------------------------------------------------------
void    GLtexture_Bind( GLtexture *texture )
{
    GLenum target;
    GLuint id;

    target = GL_TEXTURE_2D;
    id     = INVALID_TX_ID;
    if ( NULL != texture && 0 != texture->texture_target )
    {
        target = texture->texture_target;
        id     = texture->textureID;
    }

    if ( !glIsEnabled( target ) )
    {
        glEnable( target );
    };

    glBindTexture( target, id );
    if ( NULL == texture )
    {
        glTexParameteri( target, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( target, GL_TEXTURE_WRAP_T, GL_REPEAT );
    }
    else
    {
        glTexParameteri( target, GL_TEXTURE_WRAP_S, texture->wrap_s );
        glTexParameteri( target, GL_TEXTURE_WRAP_T, texture->wrap_t );
    }

    if ( texturefilter >= TX_ANISOTROPIC )
    {
        //Anisotropic filtered!
        glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameterf( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );
    }
    else
    {
        switch ( texturefilter )
        {
                // Unfiltered
            case TX_UNFILTERED:
                glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
                glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
                break;

                // Linear filtered
            case TX_LINEAR:
                glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                break;

                // Bilinear interpolation
            case TX_MIPMAP:
                glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
                glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                break;

                // Bilinear interpolation
            case TX_BILINEAR:
                glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
                glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                break;

                // Trilinear filtered (quality 1)
            case TX_TRILINEAR_1:
                glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
                glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                break;

                // Trilinear filtered (quality 2)
            case TX_TRILINEAR_2:
                glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                break;

        };
    }

}

//--------------------------------------------------------------------------------------------
Uint32  GLtexture_Load( GLenum tx_target, GLtexture *texture, const char *filename, Uint32 key )
{
    STRING fullname;
    Uint32 retval;
    Uint8 type = 0;
    SDL_Surface * image = NULL;

    // get rid of any old data
    GLtexture_Release( texture );

    // load the image
    image = NULL;
    if ( use_sdl_image )
    {
        // try all different formats
        for ( type = 0; type < maxformattypes; type++ )
        {
            snprintf( fullname, sizeof( fullname ), "%s%s", filename, TxFormatSupported[type] );
            image = IMG_Load( fullname );
            if ( NULL != image ) break;
        }
    }
    else
    {
        // normal SDL only supports bmp
        snprintf( fullname, sizeof( fullname ), "%s.bmp", filename );
        image = SDL_LoadBMP( fullname );
    }

    //We could not load the image
    if ( NULL == image ) return INVALID_TX_ID;

    retval = GLtexture_Convert( tx_target, texture, image, key );
    strncpy( texture->name, fullname, sizeof( texture->name ) );

    texture->wrap_s = GL_REPEAT;
    texture->wrap_t = GL_REPEAT;

    return retval;
}

//--------------------------------------------------------------------------------------------
GLuint  GLtexture_GetTextureID( GLtexture *texture )
{
    return texture->textureID;
}

//--------------------------------------------------------------------------------------------
GLsizei GLtexture_GetImageHeight( GLtexture *texture )
{
    return texture->imgH;
}

//--------------------------------------------------------------------------------------------
GLsizei GLtexture_GetImageWidth( GLtexture *texture )
{
    return texture->imgW;
}

//--------------------------------------------------------------------------------------------
void    GLtexture_SetAlpha( GLtexture *texture, GLfloat alpha )
{
    texture->alpha = alpha;
}

//--------------------------------------------------------------------------------------------
GLfloat GLtexture_GetAlpha( GLtexture *texture )
{
    return texture->alpha;
}

//--------------------------------------------------------------------------------------------
void    GLtexture_Release( GLtexture *texture )
{
    if ( !ErrorImage_defined ) ErrorImage_create();

    // Bind an "error texture" to this texture
    if ( INVALID_TX_ID == texture->textureID )
    {
        glGenTextures( 1, &texture->textureID );
    }

    ErrorImage_bind( GL_TEXTURE_2D, texture->textureID );

    // Reset the other data
    texture->imgW = texture->txW = ErrorImage_width;
    texture->imgH = texture->txH = ErrorImage_height;
    strncpy( texture->name, "ErrorImage", sizeof( texture->name ) );

    // set the image to be repeat in s and t
    texture->wrap_s = GL_REPEAT;
    texture->wrap_t = GL_REPEAT;
}

//--------------------------------------------------------------------------------------------
GLsizei GLtexture_GetTextureWidth( GLtexture *texture )
{
    return texture->txW;
}

//--------------------------------------------------------------------------------------------
GLsizei GLtexture_GetTextureHeight( GLtexture *texture )
{
    return texture->txH;
}
