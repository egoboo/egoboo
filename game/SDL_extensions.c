#include "SDL_extensions.h"
//#include "ogl_extensions.h"
//#include "ogl_debug.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SDLX_screen_info_t sdl_scr;

// create the color masks
// this will work if both endian systems think they have "RGBA" graphics
// if you need a different pixel format (ARGB or BGRA or whatever) this section
// will have to be changed to reflect that

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
const Uint32 rmask = 0x000000ff;
const Uint32 gmask = 0x0000ff00;
const Uint32 bmask = 0x00ff0000;
const Uint32 amask = 0xff000000;
#else
const Uint32 rmask = 0xff000000;
const Uint32 gmask = 0x00ff0000;
const Uint32 bmask = 0x0000ff00;
const Uint32 amask = 0x000000ff;
#endif


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void SDLX_download_sdl_video_flags( Uint32 iflags, SDLX_sdl_video_flags_t * pflags );
static void SDLX_read_sdl_gl_attrib( SDLX_sdl_gl_attrib_t * patt );

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SDL_bool SDLX_Get_Screen_Info( SDLX_screen_info_t * psi, SDL_bool display )
{
    int cnt;
    Uint32 init_flags = 0;
    SDL_Surface * ps;
    const SDL_VideoInfo * pvi;

    memset(psi, 0, sizeof(SDLX_screen_info_t));

    init_flags = SDL_WasInit(SDL_INIT_EVERYTHING);
    if ( 0 == init_flags )
    {
        if (display) fprintf( stderr, "ERROR: SDLX_Get_Screen_Info() called before initializing SDL\n" );
        return SDL_FALSE;
    }
    else if ( HAS_NO_BITS(init_flags, SDL_INIT_VIDEO) )
    {
        if (display) fprintf( stderr, "ERROR: SDLX_Get_Screen_Info() called before initializing SDL video driver\n" );
        return SDL_FALSE;
    }

    ps  = SDL_GetVideoSurface();
    pvi = SDL_GetVideoInfo();
    psi->video_mode_list = SDL_ListModes(ps->format, ps->flags | SDL_FULLSCREEN);

    // log the video driver info
    SDL_VideoDriverName( psi->szDriver, sizeof(psi->szDriver) );
    if (display) fprintf( stdout, "INFO: Using Video Driver - %s\n", psi->szDriver );

    //Grab all the available video modes
    if (NULL != psi->video_mode_list)
    {
        psi->video_mode_list = SDL_ListModes( ps->format, SDL_DOUBLEBUF | SDL_FULLSCREEN | SDL_OPENGL );
        if (display) fprintf( stdout, "INFO: Detecting available full-screen video modes...\n" );
        for ( cnt = 0; NULL != psi->video_mode_list[cnt]; ++cnt )
        {
            if (display) fprintf( stdout, "INFO: \tVideo Mode - %d x %d\n", psi->video_mode_list[cnt]->w, psi->video_mode_list[cnt]->h );
        };
    }

    psi->pscreen         = ps;

    // grab all SDL_GL_* attributes
    SDLX_read_sdl_gl_attrib( &(psi->gl_att) );

    // translate the surface flags into the bitfield
    SDLX_download_sdl_video_flags( ps->flags, &(psi->flags) );

    psi->hw_available = pvi->hw_available;
    psi->wm_available = pvi->wm_available;
    psi->blit_hw      = pvi->blit_hw;
    psi->blit_hw_CC   = pvi->blit_hw_CC;
    psi->blit_hw_A    = pvi->blit_hw_A;
    psi->blit_sw      = pvi->blit_sw;
    psi->blit_sw_CC   = pvi->blit_sw_CC;
    psi->blit_sw_A    = pvi->blit_sw_A;

    psi->d = ps->format->BitsPerPixel;
    psi->x = ps->w;
    psi->y = ps->h;
    psi->z = psi->gl_att.depth_size;

    return SDL_TRUE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void SDLX_output_sdl_gl_attrib( SDLX_sdl_gl_attrib_t * patt )
{
    fprintf( stdout, "INFO: SDL_GL_Attribtes\n" );

#if !defined(__unix__)
    // Under Unix we cannot specify these, we just get whatever format
    // the framebuffer has, specifying depths > the framebuffer one
    // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"

    fprintf( stdout, "INFO: \tSDL_GL_RED_SIZE           == %d\n", patt->color[0] );
    fprintf( stdout, "INFO: \tSDL_GL_GREEN_SIZE         == %d\n", patt->color[1] );
    fprintf( stdout, "INFO: \tSDL_GL_BLUE_SIZE          == %d\n", patt->color[2] );
    fprintf( stdout, "INFO: \tSDL_GL_ALPHA_SIZE         == %d\n", patt->color[3] );
    fprintf( stdout, "INFO: \tSDL_GL_DEPTH_SIZE         == %d\n", patt->depth_size );
#endif

    fprintf( stdout, "INFO: \tSDL_GL_DOUBLEBUFFER       == %d\n", patt->doublebuffer       );
    fprintf( stdout, "INFO: \tSDL_GL_STENCIL_SIZE       == %d\n", patt->stencil_size       );
    fprintf( stdout, "INFO: \tSDL_GL_ACCUM_RED_SIZE     == %d\n", patt->accum[0]           );
    fprintf( stdout, "INFO: \tSDL_GL_ACCUM_GREEN_SIZE   == %d\n", patt->accum[1]           );
    fprintf( stdout, "INFO: \tSDL_GL_ACCUM_BLUE_SIZE    == %d\n", patt->accum[2]           );
    fprintf( stdout, "INFO: \tSDL_GL_ACCUM_ALPHA_SIZE   == %d\n", patt->accum[3]           );
    fprintf( stdout, "INFO: \tSDL_GL_STEREO             == %d\n", patt->stereo             );
    fprintf( stdout, "INFO: \tSDL_GL_MULTISAMPLEBUFFERS == %d\n", patt->multi_buffers      );
    fprintf( stdout, "INFO: \tSDL_GL_MULTISAMPLESAMPLES == %d\n", patt->multi_samples      );
    fprintf( stdout, "INFO: \tSDL_GL_ACCELERATED_VISUAL == %d\n", patt->accelerated_visual );

#if !defined(__unix__)
    // Fedora 7 doesn't suuport SDL_GL_SWAP_CONTROL, but we use this nvidia extension instead.
    fprintf( stdout, "INFO: \tSDL_GL_SWAP_CONTROL       == %d\n", patt->swap_control       );
#endif
}

//------------------------------------------------------------------------------
SDLX_output_sdl_video_flags( SDLX_sdl_video_flags_t flags )
{
    fprintf( stdout, "INFO: SDL flags\n" );

    fprintf( stdout, "INFO: \t%s\n", flags.double_buf  ? "double buffer"    : "single buffer"    );
    fprintf( stdout, "INFO: \t%s\n", flags.full_screen ? "fullscreen"       : "windowed"         );
    fprintf( stdout, "INFO: \t%s\n", flags.hw_surface  ? "hardware surface" : "software surface" );

    if( flags.opengl )
    {
        fprintf( stdout, "INFO: \tOpenGL support\n" );
    }

    if( flags.opengl_blit )
    {
        fprintf( stdout, "INFO: \tOpenGL-compatible blitting\n" );
    }

    if( flags.async_blit )
    {
        fprintf( stdout, "INFO: \tasynchronous blit\n" );
    }

    if( flags.any_format )
    {
        fprintf( stdout, "INFO: \tuse closest format\n" );
    }

    if( flags.hw_palette )
    {
        fprintf( stdout, "INFO: \texclusive palate access\n" );
    }

    if( flags.resizable )
    {
        fprintf( stdout, "INFO: \tresizable window\n" );
    }

    if( flags.no_frame )
    {
        fprintf( stdout, "INFO: \tno external frame\n" );
    }
}

//------------------------------------------------------------------------------
Uint32 SDLX_upload_sdl_video_flags( SDLX_sdl_video_flags_t flags )
{
    Uint32 ret = 0;

    ret |= flags.hw_surface  ? SDL_HWSURFACE  : 0;
    ret |= flags.async_blit  ? SDL_ASYNCBLIT  : 0;
    ret |= flags.any_format  ? SDL_ANYFORMAT  : 0;
    ret |= flags.hw_palette  ? SDL_HWPALETTE  : 0;
    ret |= flags.double_buf  ? SDL_DOUBLEBUF  : 0;
    ret |= flags.full_screen ? SDL_FULLSCREEN : 0;
    ret |= flags.opengl      ? SDL_OPENGL     : 0;
    ret |= flags.opengl_blit ? SDL_OPENGLBLIT : 0;
    ret |= flags.resizable   ? SDL_RESIZABLE  : 0;
    ret |= flags.no_frame    ? SDL_NOFRAME    : 0;

    // "read only"
    ret |= flags.use_hwaccel     ? SDL_HWACCEL     : 0;
    ret |= flags.has_srccolorkey ? SDL_SRCCOLORKEY : 0;
    ret |= flags.use_rleaccelok  ? SDL_RLEACCELOK  : 0;
    ret |= flags.use_rleaccel    ? SDL_RLEACCEL    : 0;
    ret |= flags.use_srcalpha    ? SDL_SRCALPHA    : 0;
    ret |= flags.is_prealloc     ? SDL_PREALLOC    : 0;

    return ret;
}

//------------------------------------------------------------------------------
void SDLX_download_sdl_video_flags( Uint32 iflags, SDLX_sdl_video_flags_t * pflags )
{
    if( NULL != pflags )
    {
        pflags->hw_surface  = HAS_SOME_BITS( iflags, SDL_HWSURFACE  );
        pflags->async_blit  = HAS_SOME_BITS( iflags, SDL_ASYNCBLIT  );
        pflags->any_format  = HAS_SOME_BITS( iflags, SDL_ANYFORMAT  );
        pflags->hw_palette  = HAS_SOME_BITS( iflags, SDL_HWPALETTE  );
        pflags->double_buf  = HAS_SOME_BITS( iflags, SDL_DOUBLEBUF  );
        pflags->full_screen = HAS_SOME_BITS( iflags, SDL_FULLSCREEN );
        pflags->opengl      = HAS_SOME_BITS( iflags, SDL_OPENGL     );
        pflags->opengl_blit = HAS_SOME_BITS( iflags, SDL_OPENGLBLIT );
        pflags->resizable   = HAS_SOME_BITS( iflags, SDL_RESIZABLE  );
        pflags->no_frame    = HAS_SOME_BITS( iflags, SDL_NOFRAME    );

        // "read only"
        pflags->use_hwaccel     = HAS_SOME_BITS( iflags, SDL_HWACCEL     );
        pflags->has_srccolorkey = HAS_SOME_BITS( iflags, SDL_SRCCOLORKEY );
        pflags->use_rleaccelok  = HAS_SOME_BITS( iflags, SDL_RLEACCELOK  );
        pflags->use_rleaccel    = HAS_SOME_BITS( iflags, SDL_RLEACCEL    );
        pflags->use_srcalpha    = HAS_SOME_BITS( iflags, SDL_SRCALPHA    );
        pflags->is_prealloc     = HAS_SOME_BITS( iflags, SDL_PREALLOC    );
    }
}


//------------------------------------------------------------------------------
void SDLX_report_video_parameters( SDLX_video_parameters_t * v )
{
    // BB> make a report

    if (v->flags.opengl)
    {
        SDLX_output_sdl_gl_attrib( &(v->gl_att) );
    }

    SDLX_output_sdl_video_flags( v->flags );
}

//------------------------------------------------------------------------------
void SDLX_read_sdl_gl_attrib( SDLX_sdl_gl_attrib_t * patt )
{
    if( NULL == patt ) return;

    SDL_GL_GetAttribute( SDL_GL_RED_SIZE,               patt->color + 0           );
    SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE,             patt->color + 1           );
    SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE,              patt->color + 2           );
    SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE,             patt->color + 3           );
    SDL_GL_GetAttribute( SDL_GL_BUFFER_SIZE,          &(patt->buffer_size)        );
    SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER,         &(patt->doublebuffer)       );
    SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE,           &(patt->depth_size)         );
    SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE,         &(patt->stencil_size)       );
    SDL_GL_GetAttribute( SDL_GL_ACCUM_RED_SIZE,         patt->accum + 0           );
    SDL_GL_GetAttribute( SDL_GL_ACCUM_GREEN_SIZE,       patt->accum + 1           );
    SDL_GL_GetAttribute( SDL_GL_ACCUM_BLUE_SIZE,        patt->accum + 2           );
    SDL_GL_GetAttribute( SDL_GL_ACCUM_ALPHA_SIZE,       patt->accum + 3           );
    SDL_GL_GetAttribute( SDL_GL_STEREO,               &(patt->stereo)             );

    // if SDL is not aware of the "wglGetPixelFormatAttribivARB" extension (in windows),
    // these parameters always return the same values
    //SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS,   &(patt->multi_buffers)      );
    //SDL_GL_GetAttribute( SDL_GL_MULTISAMPLESAMPLES,   &(patt->multi_samples)      );

    SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL,   &(patt->accelerated_visual) );
    SDL_GL_GetAttribute( SDL_GL_SWAP_CONTROL,         &(patt->swap_control)       );

}

//------------------------------------------------------------------------------
void SDLX_synch_video_parameters( SDL_Surface * ret, SDLX_video_parameters_t * v )
{
    // BB> synch values

    if( NULL == ret || NULL == v ) return;

    v->width  = ret->w;
    v->height = ret->h;
    v->depth  = ret->format->BitsPerPixel;

    // translate the surface flags into the bitfield
    SDLX_download_sdl_video_flags( ret->flags, &(v->flags) );

    // grab all SDL_GL_* attributes
    SDLX_read_sdl_gl_attrib( &(v->gl_att) );
};

//------------------------------------------------------------------------------
SDL_bool SDLX_set_sdl_gl_attrib( SDLX_video_parameters_t * v )
{
    if( NULL == v ) return SDL_FALSE;

    if ( v->flags.opengl )
    {
        SDLX_sdl_gl_attrib_t * patt = &(v->gl_att);

#if !defined(__unix__)
        // Under Unix we cannot specify these, we just get whatever format
        // the framebuffer has, specifying depths > the framebuffer one
        // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE,             patt->color[0]           );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE,           patt->color[1]           );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,            patt->color[2]           );
        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE,           patt->color[3]           );
        SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE,          patt->buffer_size        );
#endif

        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,         patt->doublebuffer       );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE,           patt->depth_size         );
        SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE,         patt->stencil_size       );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE,       patt->accum[0]           );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE,     patt->accum[1]           );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE,      patt->accum[2]           );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE,     patt->accum[3]           );
        SDL_GL_SetAttribute( SDL_GL_STEREO,               patt->stereo             );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS,   patt->multi_buffers      );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES,   patt->multi_samples      );
        SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL,   patt->accelerated_visual );

        if( patt->swap_control )
        {
            // Fedora 7 doesn't suuport SDL_GL_SWAP_CONTROL, but we use this nvidia extension instead.
#if defined(__unix__)
            SDL_putenv("__GL_SYNC_TO_VBLANK=1");
#else
            /* Turn on vsync, this works on Windows. */
            SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, patt->swap_control );
#endif
        }
    }

    return SDL_TRUE;
}

//------------------------------------------------------------------------------
SDL_Surface * SDLX_RequestVideoMode( SDLX_video_parameters_t * v )
{
    Uint32 flags;
    SDL_Surface * ret = NULL;

    if (NULL == v) return ret;

    // fix bad colordepth
    if ( (0 == v->gl_att.color[0] && 0 == v->gl_att.color[1] && 0 == v->gl_att.color[2]) ||
         (v->gl_att.color[0] + v->gl_att.color[1] + v->gl_att.color[2] > v->depth ) )
    {
        if (v->depth > 24)
        {
            v->gl_att.color[0] = v->gl_att.color[1] = v->gl_att.color[2] = v->depth / 4;
        }
        else
        {
            // do a kludge in case we have something silly like 16 bit "highcolor" mode
            v->gl_att.color[0] = v->gl_att.color[2] = v->depth / 3;
            v->gl_att.color[1] = v->depth - v->gl_att.color[0] - v->gl_att.color[2];
        }

        v->gl_att.color[0] = (v->gl_att.color[0] > 8) ? 8 : v->gl_att.color[0];
        v->gl_att.color[1] = (v->gl_att.color[1] > 8) ? 8 : v->gl_att.color[1];
        v->gl_att.color[2] = (v->gl_att.color[2] > 8) ? 8 : v->gl_att.color[2];
    }

    // fix the alpha value
    v->gl_att.color[3] = v->depth - v->gl_att.color[0] - v->gl_att.color[1] - v->gl_att.color[2];
    v->gl_att.color[3] = (v->gl_att.color[3] < 0) ? 0 : v->gl_att.color[3];


    // the GL_ATTRIB_* parameters must be set before opening the video mode
    v->gl_att.doublebuffer = v->flags.double_buf ? SDL_TRUE : SDL_FALSE;
    v->gl_att.buffer_size  = v->depth;
    SDLX_set_sdl_gl_attrib( v );

    flags = SDLX_upload_sdl_video_flags( v->flags );

    // try a softer video initialization
    // if it fails, then it tries to get the closest possible valid video mode
    ret = NULL;
    if( 0 != SDL_VideoModeOK( v->width, v->height, v->depth, flags ) )
    {
        ret = SDL_SetVideoMode( v->width, v->height, v->depth, flags );        
    }

    if( NULL == ret )
    {
        // could not set the actual video mode
        // assume that the MULTISAMPLE stuff is to blame first

        v->gl_att.multi_samples >>= 1;
        while ( v->gl_att.multi_samples > 1 )
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1               );
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, v->gl_att.multi_samples );

            if( 0 != SDL_VideoModeOK( v->width, v->height, v->depth, flags ) )
            {
                ret = SDL_SetVideoMode( v->width, v->height, v->depth, flags );
                break;
            }

            v->gl_att.multi_samples >>= 1;
        }
    }

    if( NULL == ret )
    {
        // no multisample buffers
        v->gl_att.multi_samples = 0;
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0 );
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0 );

        // just take whatever is closest
        flags |= SDL_ANYFORMAT;
        if( 0 != SDL_VideoModeOK( v->width, v->height, v->depth, flags ) )
        {
            ret = SDL_SetVideoMode( v->width, v->height, v->depth, flags );
        }
    }

    if( NULL == ret )
    {
        fprintf( stdout, "WARN: SDL unable to set video mode with current parameters - \n\t\"%s\"\n", SDL_GetError() );

        v->gl_att.multi_buffers = v->gl_att.multi_samples > 0;

        SDLX_report_video_parameters( v );
    }
    else
    {
        SDLX_Get_Screen_Info(&sdl_scr, SDL_TRUE);

        SDLX_synch_video_parameters( ret, v );

        fprintf( stdout, "INFO: SDL set video mode to the current parameters\n", SDL_GetError() );

        fprintf( stdout, "INFO: SDL window parameters\n" );

        fprintf( stdout, "INFO: \twidth == %d, height == %d, depth == %d\n", v->width, v->height, v->depth );    

        SDLX_report_video_parameters( v );
    }

    return ret;
}

//------------------------------------------------------------------------------
SDL_bool SDLX_sdl_video_flags_default(SDLX_sdl_video_flags_t * pflags)
{
    if( NULL == pflags ) return SDL_FALSE;

    memset(pflags, 0, sizeof(SDLX_sdl_video_flags_t));

    pflags->double_buf  = 1;
    pflags->full_screen = 1;
    pflags->opengl      = 1;

    return  SDL_TRUE;
};

//------------------------------------------------------------------------------
SDL_bool SDLX_sdl_gl_attrib_default(SDLX_sdl_gl_attrib_t * patt)
{
    if( NULL == patt ) return SDL_FALSE;

    memset(patt, 0, sizeof(SDLX_sdl_gl_attrib_t));

    patt->multi_buffers      = 1;
    patt->multi_samples      = 16;
    patt->accelerated_visual = 1;

    patt->color[0]    = 8;
    patt->color[1]    = 8;
    patt->color[2]    = 8;
    patt->color[2]    = 8;
    patt->buffer_size = 32;

    patt->depth_size = 8;

    return  SDL_TRUE;
}

//------------------------------------------------------------------------------
SDL_bool SDLX_video_parameters_default(SDLX_video_parameters_t * v)
{
    if (NULL == v) return SDL_FALSE;

    v->surface = NULL;
    v->width   = 640;
    v->height  = 480;
    v->depth   =  32;

    SDLX_sdl_video_flags_default( &(v->flags ) );
    SDLX_sdl_gl_attrib_default  ( &(v->gl_att) );

    return SDL_TRUE;
}

//------------------------------------------------------------------------------
SDLX_video_parameters_t * SDLX_set_mode(SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new )
{
    /// @details BB> let SDL try to set a new video mode.

    SDLX_video_parameters_t   param_old, param_new;
    SDLX_video_parameters_t * retval = NULL;
    SDL_Surface             * surface;

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

    // initialize v_new and param_new
    if (NULL == v_new)
    {
        SDLX_video_parameters_default( &param_new );
        v_new = &param_new;
    }
    else
    {
        memcpy(&param_new, v_new, sizeof(SDLX_video_parameters_t));
    }

    // assume any problem with setting the graphics mode is with the multisampling
    surface = SDLX_RequestVideoMode(&param_new);

    if ( NULL != surface )
    {
        param_new.surface = surface;
        if( NULL != v_new )
        {
            memcpy(v_new, &param_new, sizeof(SDLX_video_parameters_t));
        }
        retval = v_new;
    }
    else
    {
        surface = SDLX_RequestVideoMode( &param_old );

        if (NULL == surface)
        {
            //log_error("Could not restore the old video mode. Terminating.\n");
            exit(-1);
        }
        else
        {
            param_old.surface = surface;
            if( NULL != v_old )
            {
                memcpy(v_old, &param_old, sizeof(SDLX_video_parameters_t));
            }
        }

        retval = v_old;
    }

    return retval;
};

//--------------------------------------------------------------------------------------------
SDL_bool SDLX_ExpandFormat(SDL_PixelFormat * pformat)
{
    // use the basic screen format to create a surface with proper alpha support

    int i;

    if ( NULL == pformat ) return SDL_FALSE;

    if ( pformat->BitsPerPixel > 24 )
    {
        pformat->Amask = amask;
        pformat->Bmask = bmask;
        pformat->Gmask = gmask;
        pformat->Rmask = rmask;

        for ( i = 0; i < sdl_scr.d && ( pformat->Amask & ( 1 << i ) ) == 0; i++ );

        if ( 0 == (pformat->Amask & ( 1 << i )) )
        {
            // no alpha bits available
            pformat->Ashift = 0;
            pformat->Aloss  = 8;
        }
        else
        {
            // normal alpha channel
            pformat->Ashift = i;
            pformat->Aloss  = 0;
        }
    }

    return SDL_TRUE;
}
