#include "SDL_GL_extensions.h"
#include "ogl_debug.h"

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#define LOCAL_STDOUT ((NULL == _SDL_GL_stdout) ? stdout : _SDL_GL_stdout)

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static FILE * _SDL_GL_stdout = NULL;

//---------------------------------------------------------------------
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
// -   Global function stolen from Jonathan Fisher
// - who stole it from gl_font.c test program from SDL_ttf ;)
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
// - Global function stolen from Jonathan Fisher
// - who stole it from gl_font.c test program from SDL_ttf ;)
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
    // / @details BB> this function applies OpenGL settings. Must have a valid SDL_Surface to do any good.

    if (NULL == v || !SDL_WasInit(SDL_INIT_VIDEO)) return SDL_FALSE;

    oglx_Get_Screen_Info(&ogl_caps);

    if ( v->multisample_arb )
    {
        GL_DEBUG(glDisable)(GL_MULTISAMPLE);
        GL_DEBUG(glEnable)(GL_MULTISAMPLE_ARB);
    }
    else if ( v->multisample )
    {
        GL_DEBUG(glEnable)(GL_MULTISAMPLE);
    }
    else
    {
        GL_DEBUG(glDisable)(GL_MULTISAMPLE);
        GL_DEBUG(glDisable)(GL_MULTISAMPLE_ARB);
    }

    // Enable perspective correction?
    GL_DEBUG(glHint)(GL_PERSPECTIVE_CORRECTION_HINT, v->perspective );

    // Enable dithering?
    if ( v->dither ) GL_DEBUG(glEnable)(GL_DITHER );
    else GL_DEBUG(glDisable)(GL_DITHER );

    // Enable gourad v->shading? (Important!)
    GL_DEBUG(glShadeModel)(v->shading );

    // Enable v->antialiasing?
    if ( v->antialiasing )
    {
        GL_DEBUG(glEnable)(GL_LINE_SMOOTH );
        GL_DEBUG(glHint)(GL_LINE_SMOOTH_HINT,    GL_NICEST );

        GL_DEBUG(glEnable)(GL_POINT_SMOOTH );
        GL_DEBUG(glHint)(GL_POINT_SMOOTH_HINT,   GL_NICEST );

        GL_DEBUG(glDisable)(GL_POLYGON_SMOOTH );
        GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT,    GL_FASTEST );

        // PLEASE do not turn this on unless you use
        //  GL_DEBUG(glEnable)(GL_BLEND);
        //  GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // before every single draw command
        //
        // GL_DEBUG(glEnable)(GL_POLYGON_SMOOTH);
        // GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    }
    else
    {
        GL_DEBUG(glDisable)(GL_POINT_SMOOTH );
        GL_DEBUG(glDisable)(GL_LINE_SMOOTH );
        GL_DEBUG(glDisable)(GL_POLYGON_SMOOTH );
    }

    // anisotropic filtering
    if ( v->userAnisotropy > 1.0f )
    {
        // limit the userAnisotropy top be in a valid range
        if ( v->userAnisotropy > ogl_caps.maxAnisotropy )
        {
            v->userAnisotropy = ogl_caps.maxAnisotropy;
        }

        GL_DEBUG(glTexParameterf)(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, v->userAnisotropy );
    };

    // fill mode
    GL_DEBUG(glPolygonMode)(GL_FRONT, GL_FILL );
    GL_DEBUG(glPolygonMode)(GL_BACK,  GL_FILL );

    /* Disable OpenGL lighting */
    GL_DEBUG(glDisable)(GL_LIGHTING );

    /* Backface culling */
    GL_DEBUG(glEnable)(GL_CULL_FACE );  // This seems implied - DDOI
    GL_DEBUG(glCullFace)(GL_BACK );

    return SDL_TRUE;
}

//------------------------------------------------------------------------------
void SDL_GL_report_mode( SDLX_video_parameters_t * retval )
{
    SDL_Surface * surface = (NULL == retval) ? NULL : retval->surface;

    SDLX_set_stdout( LOCAL_STDOUT );
    SDLX_report_mode( surface, retval );

    if ( NULL != retval && retval->flags.opengl )
    {
        oglx_set_stdout( LOCAL_STDOUT );
        oglx_report_caps();
    }

    fflush( LOCAL_STDOUT );
}

//------------------------------------------------------------------------------
SDLX_video_parameters_t * SDL_GL_set_mode(SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, oglx_video_parameters_t * gl_new)
{
    // / @details BB> let SDL_GL try to set a new video mode.

    SDLX_video_parameters_t param_old;
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
    retval = SDLX_set_mode( v_old, v_new, SDL_FALSE );

    // report on the success or failure to set the mode
    SDL_GL_report_mode( retval );

    // set the opengl parameters
    gl_new->multisample     = GL_FALSE;
    gl_new->multisample_arb = GL_FALSE;
    if ( NULL != retval->surface && retval->flags.opengl )
    {
        // correct the multisampling
        gl_new->multisample_arb = retval->gl_att.multi_samples > 1;

        SDL_GL_set_gl_mode( gl_new );
    }

    return retval;
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FILE * SDL_GL_set_stdout(FILE * pfile)
{
    FILE * pfile_old = _SDL_GL_stdout;

    if ( NULL == pfile )
    {
        _SDL_GL_stdout = stdout;
    }
    else
    {
        _SDL_GL_stdout = pfile;
    }

    return pfile_old;
}