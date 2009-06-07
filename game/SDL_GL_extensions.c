#include "SDL_GL_extensions.h"
#include "ogl_debug.h"

//---------------------------------------------------------------------
// create the mask
// this will work if both endian systems think they have "RGBA" graphics
// if you need a different pixel format (ARGB or BGRA or whatever) this section
// will have to be changed to reflect that
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)

const Uint32 sdl_a_shift = 24;
const Uint32 sdl_b_shift = 16;
const Uint32 sdl_g_shift =  8;
const Uint32 sdl_r_shift =  0;

const Uint32 sdl_a_mask = ( Uint32 )( 0xFF << 24 );
const Uint32 sdl_b_mask = ( Uint32 )( 0xFF << 16 );
const Uint32 sdl_g_mask = ( Uint32 )( 0xFF <<  8 );
const Uint32 sdl_r_mask = ( Uint32 )( 0xFF <<  0 );

#else

const Uint32 sdl_a_shift = 0;
const Uint32 sdl_b_shift = 8;
const Uint32 sdl_g_shift = 16;
const Uint32 sdl_r_shift = 24;

const Uint32 sdl_a_mask = ( Uint32 )( 0xFF <<  0 );
const Uint32 sdl_b_mask = ( Uint32 )( 0xFF <<  8 );
const Uint32 sdl_g_mask = ( Uint32 )( 0xFF << 16 );
const Uint32 sdl_r_mask = ( Uint32 )( 0xFF << 24 );

#endif

//---------------------------------------------------------------------
//-   Global function stolen from Jonathan Fisher
//- who stole it from gl_font.c test program from SDL_ttf ;)
//---------------------------------------------------------------------
int powerOfTwo( int input )
{
    int value = 1;

    while ( value < input )
    {
        value <<= 1;
    }
    return value;
}

//---------------------------------------------------------------------
//- Global function stolen from Jonathan Fisher
//- who stole it from gl_font.c test program from SDL_ttf ;)
//---------------------------------------------------------------------
SDL_bool SDL_GL_uploadSurface( SDL_Surface *surface, GLuint tx_id, GLfloat *texCoords )
{
    int w, h;
    SDL_Surface *image;
    SDL_Rect area;
    Uint32 saved_flags;
    Uint8  saved_alpha;

    // Use the surface width & height expanded to the next powers of two
    w = powerOfTwo( surface->w );
    h = powerOfTwo( surface->h );

    texCoords[0] = 0.0f;
    texCoords[1] = 0.0f;
    texCoords[2] = ( GLfloat )surface->w / w;
    texCoords[3] = ( GLfloat )surface->h / h;

    image = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, amask );
    if ( image == NULL )
    {
        return SDL_FALSE;
    }

    // Save the alpha blending attributes
    saved_flags = surface->flags & ( SDL_SRCALPHA | SDL_RLEACCELOK );
    saved_alpha = surface->format->alpha;
    if ( ( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
    {
        SDL_SetAlpha( surface, 0, 0 );
    }

    // Copy the surface into the texture image
    area.x = 0;
    area.y = 0;
    area.w = surface->w;
    area.h = surface->h;
    SDL_BlitSurface( surface, &area, image, &area );

    // Restore the blending attributes
    if ( ( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
    {
        SDL_SetAlpha( surface, saved_flags, saved_alpha );
    }

    // Send the texture to OpenGL
    oglx_bind( GL_TEXTURE_2D, tx_id, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR, 0 );
    oglx_upload_2d( GL_TRUE, w, h, image->pixels );

    // Don't need the extra image anymore
    SDL_FreeSurface( image );

    return SDL_TRUE;
}

//------------------------------------------------------------------------------
SDL_bool SDL_GL_set_gl_mode(struct s_oglx_video_parameters * v)
{
    /// @details BB> this function applies OpenGL settings. Must have a valid SDL_Surface to do any good.

    if (NULL == v || !SDL_WasInit(SDL_INIT_VIDEO)) return SDL_FALSE;

    oglx_Get_Screen_Info(&ogl_caps);

    if ( v->multisample )
    {
        glEnable(GL_MULTISAMPLE_ARB);
        //glEnable(GL_MULTISAMPLE);
    };

    //Enable perspective correction?
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, v->perspective );

    //Enable dithering?
    if ( v->dither ) glEnable( GL_DITHER );
    else glDisable( GL_DITHER );

    //Enable gourad v->shading? (Important!)
    glShadeModel( v->shading );

    //Enable v->antialiasing?
    if ( v->antialiasing )
    {
        glEnable( GL_LINE_SMOOTH );
        glHint( GL_LINE_SMOOTH_HINT,    GL_NICEST );

        glEnable( GL_POINT_SMOOTH );
        glHint( GL_POINT_SMOOTH_HINT,   GL_NICEST );

        glDisable( GL_POLYGON_SMOOTH );
        glHint( GL_POLYGON_SMOOTH_HINT,    GL_FASTEST );

        // PLEASE do not turn this on unless you use
        //  glEnable(GL_BLEND);
        //  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // before every single draw command
        //
        //glEnable(GL_POLYGON_SMOOTH);
        //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    }
    else
    {
        glDisable( GL_POINT_SMOOTH );
        glDisable( GL_LINE_SMOOTH );
        glDisable( GL_POLYGON_SMOOTH );
    }

    // anisotropic filtering
    if ( v->userAnisotropy > ogl_caps.maxAnisotropy )
    {
        v->userAnisotropy = ogl_caps.maxAnisotropy;
    }

    if ( v->userAnisotropy > 0 )
    {
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, v->userAnisotropy );
    };

    //fill mode
    glPolygonMode( GL_FRONT, GL_FILL );
    glPolygonMode( GL_BACK,  GL_FILL );

    /* Disable OpenGL lighting */
    glDisable( GL_LIGHTING );

    /* Backface culling */
    glEnable( GL_CULL_FACE );  // This seems implied - DDOI
    glCullFace( GL_BACK );

    return SDL_TRUE;
}

//------------------------------------------------------------------------------
SDLX_video_parameters_t * SDL_GL_set_mode(SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, oglx_video_parameters_t * gl_new)
{
    /// @details BB> let SDL_GL try to set a new video mode.

    SDLX_video_parameters_t param_old, param_new;
    SDLX_video_parameters_t * retval = NULL;

    // initialize v_old and param_old
    if (NULL == v_old)
    {
        SDLX_video_parameters_default( &param_old );
        v_old = &param_old;
    }
    else
    {
        memcpy(&param_old, v_old, sizeof(SDLX_video_parameters_t));
    }

    // use the sdl extensions to set the SDL video mode
    retval = SDLX_set_mode( v_old, v_new );

    // set the opengl parameters
    if ( NULL != retval->surface && retval->flags.opengl )
    {
        // correct the multisampling 
        gl_new->multisample = retval->gl_att.multi_samples > 1;

        SDL_GL_set_gl_mode( gl_new );
    }

    return retval;
};

