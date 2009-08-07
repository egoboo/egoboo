// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

// /
// / @file
// / @brief Egoboo OpenGL texture interface
// / @details Implements OpenGL texture loading using SDL_image

#include "ogl_texture.h"
#include "ogl_debug.h"
#include "SDL_GL_extensions.h"

#include "graphic.h"

#include "egoboo_setup.h"
#include "egoboo_strutil.h"

#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define VALID_VALUE        0x32B04E67
#define ErrorImage_width   2
#define ErrorImage_height  2

#define VALID_BINDING( BIND ) ( (0 != (BIND)) && (INVALID_TX_ID != (BIND)) )
#define ERROR_BINDING( BIND ) ( ErrorImage_binding == (BIND) )

#define VALID_TEXTURE( PTEX ) ( (NULL != (PTEX)) && (VALID_VALUE == (PTEX)->valid) )

static GLuint ErrorImage_binding = INVALID_TX_ID;

oglx_texture_parameters_t tex_params = {TX_UNFILTERED, 0};

static GLboolean ErrorImage_defined = GL_FALSE;

typedef GLubyte image_row_t[ErrorImage_width][4];

static GLubyte ErrorImage[ErrorImage_height][ErrorImage_width][4];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void ErrorImage_bind(GLenum target, GLuint id);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ErrorImage_create(void)
{
    // BB > define a default "error texture"

    int i, j;

    if ( INVALID_TX_ID != ErrorImage_binding ) return;

    GL_DEBUG(glGenTextures)( 1, &ErrorImage_binding );

    for (i = 0; i < ErrorImage_height; i++)
    {
        for (j = 0; j < ErrorImage_width; j++)
        {
            if ( 0 == ((i&0x1) ^ (j&0x1)) )
            {
                ErrorImage[i][j][0] = (GLubyte) 255;
                ErrorImage[i][j][1] = (GLubyte) 0;
                ErrorImage[i][j][2] = (GLubyte) 0;
            }
            else
            {
                ErrorImage[i][j][0] = (GLubyte) 0;
                ErrorImage[i][j][1] = (GLubyte) 255;
                ErrorImage[i][j][2] = (GLubyte) 255;
            }

            ErrorImage[i][j][3] = (GLubyte) 255;
        }
    }

    ErrorImage_bind( GL_TEXTURE_2D, ErrorImage_binding );

    ErrorImage_defined = GL_TRUE;
}

void ErrorImage_bind(GLenum target, GLuint id)
{
    GL_DEBUG(glPushClientAttrib)( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        GL_DEBUG(glPixelStorei)(GL_UNPACK_ALIGNMENT, 1);

        GL_DEBUG(glBindTexture)(target, id);

        GL_DEBUG(glTexParameteri)(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        GL_DEBUG(glTexParameteri)(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
        GL_DEBUG(glTexParameteri)(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL_DEBUG(glTexParameteri)(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        if (target == GL_TEXTURE_1D)
        {
            GL_DEBUG(glTexImage1D)(GL_TEXTURE_1D, 0, GL_RGBA, ErrorImage_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, ErrorImage);
        }
        else
        {
            GL_DEBUG(glTexImage2D)(GL_TEXTURE_2D, 0, GL_RGBA, ErrorImage_width, ErrorImage_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ErrorImage);
        }
    }
    GL_DEBUG(glPopClientAttrib)();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
oglx_texture * oglx_texture_new(oglx_texture * ptex)
{
    if ( NULL == ptex ) return ptex;

    memset( ptex, 0, sizeof(oglx_texture) );

    // only need one textureID per texture
    // do not need to ask for a new id, even if we change the texture data
    GL_DEBUG(glGenTextures)( 1, &(ptex->base.binding) );

    // set the flag validity flag
    if ( VALID_BINDING(ptex->base.binding) && !ERROR_BINDING(ptex->base.binding) )
    {
        ptex->valid = VALID_VALUE;
    }
    else
    {
        ptex->valid = ~VALID_VALUE;
    }

    // set to 2d texture by default
    ptex->base.target = GL_TEXTURE_2D;

    // set the image to be clamped in s and t
    ptex->base.wrap_s = GL_CLAMP;
    ptex->base.wrap_t = GL_CLAMP;

    return ptex;
}

//--------------------------------------------------------------------------------------------
void oglx_texture_delete(oglx_texture * ptex)
{
    if ( !VALID_TEXTURE(ptex) )  return;

    // set a bad value for ptex->valid
    ptex->valid = ~VALID_VALUE;

    // actually delete the OpenGL texture data
    if ( VALID_BINDING(ptex->base.binding) )
    {
        GL_DEBUG(glDeleteTextures)( 1, &ptex->base.binding );
        ptex->base.binding = INVALID_TX_ID;
    }

    // set the image to be clamped in s and t
    ptex->base.wrap_s = GL_CLAMP;
    ptex->base.wrap_t = GL_CLAMP;

    // Reset the other data
    ptex->imgH = ptex->imgW = ptex->base.width = ptex->base.height  = 0;
    ptex->name[0] = '\0';

    if ( NULL != ptex->surface )
    {
        SDL_FreeSurface( ptex->surface );
        ptex->surface = NULL;
    }
}

//--------------------------------------------------------------------------------------------
GLuint oglx_texture_Convert( GLenum tx_target, oglx_texture *ptex, SDL_Surface * image, Uint32 key )
{
    int               src_imgW, src_imgH;
    SDL_Surface     * screen;
    SDL_PixelFormat * pformat;
    SDL_PixelFormat   tmpformat;
    bool_t            use_alpha;

    SDLX_screen_info_t sdl_scr;

    if ( NULL == ptex ) return INVALID_TX_ID;

    SDLX_Get_Screen_Info(&sdl_scr, bfalse);

    // make sure the old texture has been freed
    oglx_texture_Release( ptex );

    if ( NULL == image ) return INVALID_TX_ID;

    /* set the color key, if valid */
    if ( NULL != image->format && NULL != image->format->palette && INVALID_KEY != key )
    {
        SDL_SetColorKey( image, SDL_SRCCOLORKEY, key );
    };

    /* Set the ptex's alpha */
    ptex->alpha = image->format->alpha / 255.0f;

    /* Set the original image's size (incase it's not an exact square of a power of two) */
    src_imgH = image->h;
    src_imgW = image->w;

    // adjust the texture target
    tx_target = ((1 == image->h) && (image->w > 1)) ? GL_TEXTURE_1D : GL_TEXTURE_2D;

    /* Determine the correct power of two greater than or equal to the original image's size */
    ptex->base.height = powerOfTwo( image->h );
    ptex->base.width  = powerOfTwo( image->w );

    screen  = SDL_GetVideoSurface();
    pformat = screen->format;
    memcpy( &tmpformat, screen->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format

    // if( ogl_caps.alpha_bits > 0 )
    {
        // convert the image to a 32-bit pixel format
        tmpformat.Amask  = sdl_a_mask;
        tmpformat.Ashift = sdl_a_shift;
        tmpformat.Aloss  = 0;

        tmpformat.Bmask  = sdl_b_mask;
        tmpformat.Bshift = sdl_a_shift;
        tmpformat.Bloss  = 0;

        tmpformat.Gmask  = sdl_g_mask;
        tmpformat.Gshift = sdl_g_shift;
        tmpformat.Gloss  = 0;

        tmpformat.Rmask  = sdl_r_mask;
        tmpformat.Rshift = sdl_r_shift;
        tmpformat.Rloss = 0;

        // make the pixel size match the screen format
        tmpformat.BitsPerPixel  = 32;
        tmpformat.BytesPerPixel = 4;
    }
    // else
    // {
    //   // convert the image to a 24-bit pixel format without alpha
    //   // convert the image to a 32-bit pixel format
    //   tmpformat.Amask  = 0;
    //   tmpformat.Ashift = sdl_a_shift;
    //   tmpformat.Aloss  = 8;

    //   tmpformat.Bmask  = sdl_b_mask;
    //   tmpformat.Bshift = sdl_a_shift;
    //   tmpformat.Bloss  = 0;

    //   tmpformat.Gmask  = sdl_g_mask;
    //   tmpformat.Gshift = sdl_g_shift;
    //   tmpformat.Gloss  = 0;

    //   tmpformat.Rmask  = sdl_r_mask;
    //   tmpformat.Rshift = sdl_r_shift;
    //   tmpformat.Rloss = 0;

    //   // make the pixel size match the screen format
    //   tmpformat.BitsPerPixel  = 24;
    //   tmpformat.BytesPerPixel = 3;
    // }

    {
        SDL_Surface * tmp;
        Uint32 convert_flags;

        // convert the image format to the correct format
        convert_flags = SDL_SWSURFACE;
        tmp = SDL_ConvertSurface( image, &tmpformat, convert_flags );
        SDL_FreeSurface( image );
        image = tmp;

        // fix the alpha channel on the new SDL_Surface.  For some reason, SDL wants to create
        // surface with surface->format->alpha==0x00 which causes a problem if we have to
        // use the SDL_BlitSurface() function below.  With the surface alpha set to zero, the
        // image will be converted to BLACK!
        //
        // The following statement tells SDL
        //  1) to set the image to opaque
        //  2) not to alpha blend the surface on blit
        SDL_SetAlpha( image, 0, SDL_ALPHA_OPAQUE );
        SDL_SetColorKey( image, 0, 0 );
    }

    // create a ptex that is acceptable to OpenGL (height and width are powers of 2)
    if ( src_imgH != ptex->base.height || src_imgW != ptex->base.width )
    {
        SDL_Surface * tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, ptex->base.width, ptex->base.height, tmpformat.BitsPerPixel, tmpformat.Rmask, tmpformat.Gmask, tmpformat.Bmask, tmpformat.Amask );

        SDL_BlitSurface( image, &image->clip_rect, tmp, &image->clip_rect );
        SDL_FreeSurface( image );
        image = tmp;
    };

    /* Generate an OpenGL texture ID */
    if ( !VALID_BINDING(ptex->base.binding) || ERROR_BINDING(ptex->base.binding) )
    {
        GL_DEBUG(glGenTextures)( 1, &ptex->base.binding );
    }

    ptex->base.target  = tx_target;
    ptex->surface      = image;

    /* Set up some parameters for the format of the oglx_texture */
    ptex->base_valid = btrue;
    oglx_texture_Bind( ptex );

    /* actually create the OpenGL textures */
    use_alpha = !( 8 == image->format->Aloss );
    if ( tx_target == GL_TEXTURE_2D )
    {
        if ( tex_params.texturefilter >= TX_MIPMAP )
        {
            oglx_upload_2d_mipmap(use_alpha, image->w, image->h, image->pixels);
        }
        else
        {
            oglx_upload_2d(use_alpha, image->w, image->h, image->pixels);
        }
    }
    else if (tx_target == GL_TEXTURE_1D)
    {
        oglx_upload_1d(use_alpha, image->w, image->pixels);
    }
    else
    {
        assert(0);
    }

    ptex->base_valid = bfalse;
    oglx_grab_texture_state( tx_target, 0, ptex );

    ptex->alpha = 1.0f;
    ptex->imgW  = src_imgW;
    ptex->imgH  = src_imgH;
    strncpy( ptex->name, "SDL_Surface()", SDL_arraysize(ptex->name) );

    return ptex->base.binding;
}

//--------------------------------------------------------------------------------------------
GLuint oglx_texture_Load( GLenum tx_target, oglx_texture *ptex, const char *filename, Uint32 key )
{
    GLuint retval;
    SDL_Surface * image;

    if ( VALID_TEXTURE(ptex) )
    {
        // release any old texture
        oglx_texture_Release( ptex );
    }
    else
    {
        // clean out any uninitialied data
        ptex = oglx_texture_new(ptex);
        if ( NULL == ptex ) return INVALID_TX_ID;
    }

    image = IMG_Load( filename );
    if ( NULL == image ) return INVALID_TX_ID;

    retval = oglx_texture_Convert( tx_target, ptex, image, key );

    if ( !VALID_BINDING(retval) )
    {
        oglx_texture_delete(ptex);
    }
    else
    {
        strncpy( ptex->name, filename, sizeof(ptex->name) );

        ptex->base.wrap_s = GL_REPEAT;
        ptex->base.wrap_t = GL_REPEAT;
    }

    return retval;
}

/********************> oglx_texture_GetTextureID() <*****/
GLuint  oglx_texture_GetTextureID( oglx_texture *texture )
{
    return (NULL == texture) ? INVALID_TX_ID : texture->base.binding;
}

/********************> oglx_texture_GetImageHeight() <*****/
GLsizei  oglx_texture_GetImageHeight( oglx_texture *texture )
{
    return (NULL == texture) ? 0 : texture->imgH;
}

/********************> oglx_texture_GetImageWidth() <*****/
GLsizei  oglx_texture_GetImageWidth( oglx_texture *texture )
{
    return (NULL == texture) ? 0 : texture->imgW;
}

/********************> oglx_texture_GetTextureWidth() <*****/
GLsizei  oglx_texture_GetTextureWidth( oglx_texture *texture )
{
    return (NULL == texture) ? 0 : texture->base.width;
}

/********************> oglx_texture_GetTextureHeight() <*****/
GLsizei  oglx_texture_GetTextureHeight( oglx_texture *texture )
{
    return (NULL == texture) ? 0 : texture->base.height;
}

/********************> oglx_texture_SetAlpha() <*****/
void  oglx_texture_SetAlpha( oglx_texture *texture, GLfloat alpha )
{
    if( NULL != texture )
    {
        texture->alpha = alpha;
    }
}

/********************> oglx_texture_GetAlpha() <*****/
GLfloat  oglx_texture_GetAlpha( oglx_texture *texture )
{
    return (NULL == texture) ? 0 : texture->alpha;
}

/********************> oglx_texture_Release() <*****/
void  oglx_texture_Release( oglx_texture *texture )
{
    if ( !VALID_TEXTURE(texture) ) return;

    // delete any existing SDL surface
    if ( NULL != texture->surface )
    {
        SDL_FreeSurface( texture->surface );
        texture->surface = NULL;
    }

    if (!ErrorImage_defined) ErrorImage_create();

    // Bind the error texture instead of the old texture
    ErrorImage_bind( texture->base.target, texture->base.binding );

    // Reset the other data
    texture->imgW = texture->base.width = ErrorImage_width;
    texture->imgH = texture->base.height = ErrorImage_height;
    strncpy( texture->name, "ErrorImage", sizeof(texture->name) );

    // set the image to be repeat in s and t
    texture->base.wrap_s = GL_REPEAT;
    texture->base.wrap_t = GL_REPEAT;

    oglx_grab_texture_state( GL_TEXTURE_2D, 0, texture );
}

/********************> oglx_texture_Release() <*****/
void oglx_texture_Bind( oglx_texture *texture )
{
    int    filt_type, anisotropy;
    GLenum target;
    GLuint id;
    GLint wrap_s, wrap_t;

    // make sure the error texture exists
    if (!ErrorImage_defined) ErrorImage_create();

    // assume the texture is going to be the error texture
    target = GL_TEXTURE_2D;
    wrap_s = wrap_t = GL_REPEAT;
    id = ErrorImage_binding;

    if ( NULL == texture )
    {
        // NULL texture means white blob
        id = INVALID_TX_ID;
    }
    else if ( VALID_TEXTURE(texture) && VALID_BINDING(texture->base.binding) )
    {
        // grab the info from the texture
        target = texture->base.target;
        id     = texture->base.binding;
        wrap_s = texture->base.wrap_s;
        wrap_t = texture->base.wrap_t;
    }

    filt_type  = tex_params.texturefilter;
    anisotropy = tex_params.userAnisotropy;

    if ( !GL_DEBUG(glIsEnabled)( target ) )
    {
        GL_DEBUG(glEnable)( target );
    };

    if ( filt_type >= TX_ANISOTROPIC )
    {
        // Anisotropic filtered!
        oglx_bind(target, id, wrap_s, wrap_t, GL_LINEAR, GL_LINEAR, anisotropy);
    }
    else
    {
        switch ( filt_type )
        {
                // Unfiltered
            case TX_UNFILTERED:
                oglx_bind(target, id, wrap_s, wrap_t, GL_NEAREST, GL_LINEAR, 0);
                break;

                // Linear filtered
            case TX_LINEAR:
                oglx_bind(target, id, wrap_s, wrap_t, GL_LINEAR, GL_LINEAR, 0);
                break;

                // Bilinear interpolation
            case TX_MIPMAP:
                oglx_bind(target, id, wrap_s, wrap_t, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR, 0);
                break;

                // Bilinear interpolation
            case TX_BILINEAR:
                oglx_bind(target, id, wrap_s, wrap_t, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 0);
                break;

                // Trilinear filtered (quality 1)
            case TX_TRILINEAR_1:
                oglx_bind(target, id, wrap_s, wrap_t, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR, 0);
                break;

                // Trilinear filtered (quality 2)
            case TX_TRILINEAR_2:
                oglx_bind(target, id, wrap_s, wrap_t, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
                break;
        };
    }

    if( NULL != texture && !texture->base_valid )
    {
        oglx_grab_texture_state( target, 0, texture );
    }
};

//---------------------------------------------------------------------------------------------
GLboolean oglx_texture_Valid( oglx_texture *ptex )
{
    return VALID_TEXTURE( ptex );
}

//---------------------------------------------------------------------------------------------
void oglx_grab_texture_state(GLenum target, GLint level, oglx_texture * texture)
{
    if( NULL == texture ) return;

    gl_grab_texture_state( target, level, &(texture->base) );

    texture->base_valid = GL_TRUE;
}

