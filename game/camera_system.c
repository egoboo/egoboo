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

/// @file camera_system.c
/// @brief
/// @details

#include "camera_system.h"

#include "../egolib/egoboo_setup.h"
#include "../egolib/extensions/SDL_extensions.h"
#include "../egolib/_math.inl"

#include "network.h"
#include "mesh.h"
#include "graphic.h"
#include "game.h"
#include "player.h"

#include "ChrList.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_camera_players;
typedef struct s_camera_players camera_players_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_ext_camera_iterator
{
    int                 index;
    ext_camera_list_t * list;
};

//--------------------------------------------------------------------------------------------
/// who is supposed to be in-view for this camera?
struct s_camera_players
{
    size_t  count;
    CHR_REF who[MAX_LOCAL_PLAYERS];
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// extended data for each camera
struct s_ext_camera
{
    bool_t           on;
    camera_players_t targets;
    ego_frect_t      screen;

    camera_t         which;

    int              last_frame;
    int              render_list;
    int              do_list;
};

static ext_camera_t * ext_camera_ctor( ext_camera_t * ptr );
static ext_camera_t * ext_camera_dtor( ext_camera_t * ptr );
static ext_camera_t * ext_camera_reinit( ext_camera_t * ptr );
static egolib_rv ext_camera_begin( ext_camera_t * pext, GLint * mode );
static egolib_rv ext_camera_end( ext_camera_t * pext, GLint mode );
static bool_t ext_camera_free( ext_camera_t * ptr );
static bool_t ext_camera_update_projection( ext_camera_t * ptr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a struct to hold camera lists
struct s_ext_camera_list
{
    size_t       count;
    ext_camera_t lst[MAX_CAMERAS];
};

ext_camera_list_t * ext_camera_list_ctor( ext_camera_list_t * );
ext_camera_list_t * ext_camera_list_dtor( ext_camera_list_t * );
ext_camera_list_t * ext_camera_list_free( ext_camera_list_t * );

ext_camera_list_t * ext_camera_list_reinit( ext_camera_list_t * );

//--------------------------------------------------------------------------------------------
// private variables
//--------------------------------------------------------------------------------------------

static bool_t               _camera_system_initialized = bfalse;

static ext_camera_list_t    _camera_lst;

//--------------------------------------------------------------------------------------------
// private funcitons
//--------------------------------------------------------------------------------------------

static egolib_rv _camera_system_autoformat_cameras( int cameras );
static egolib_rv _camera_system_autoset_targets( void );
static egolib_rv _camera_system_begin_camera_ptr( ext_camera_t * pcam );
static egolib_rv _camera_system_end_camera_ptr( ext_camera_t * pcam );

//--------------------------------------------------------------------------------------------
// ext_camera_t implementation
//--------------------------------------------------------------------------------------------

ext_camera_t * ext_camera_ctor( ext_camera_t * ptr )
{
    int tnc;

    if ( NULL == ptr ) return NULL;

    BLANK_STRUCT_PTR( ptr );

    // construct the actual camera
    camera_ctor( &( ptr->which ) );

    // invalidate the targets the camera is tracking
    for ( tnc = 0; tnc < MAX_LOCAL_PLAYERS; tnc++ )
    {
        ptr->targets.who[tnc] = INVALID_CHR_REF;
    }

    // set the renderlist and dolist links to invalid values
    ptr->render_list = -1;
    ptr->do_list     = -1;

    // what was the last game_frame_all when the camera rendered itself?
    ptr->last_frame  = -1;

    // assume that the camera is fullscreen
    ext_camera_set_screen( ptr, 0, 0, sdl_scr.x, sdl_scr.y );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ext_camera_t * ext_camera_dtor( ext_camera_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    ext_camera_free( ptr );

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool_t ext_camera_free( ext_camera_t * ptr )
{
    renderlist_mgr_t * rmgr_ptr = NULL;
    dolist_mgr_t     * dmgr_ptr = NULL;

    if ( NULL == ptr ) return bfalse;

    if ( !ptr->on ) return btrue;

    // free any locked renderlist
    rmgr_ptr = gfx_system_get_renderlist_mgr();
    if ( -1 != ptr->render_list )
    {
        renderlist_mgr_free_one( rmgr_ptr, ptr->render_list );
        ptr->render_list = -1;
    }

    // free any locked dolist
    dmgr_ptr = gfx_system_get_dolist_mgr();
    if ( -1 != ptr->do_list )
    {
        dolist_mgr_free_one( dmgr_ptr, ptr->render_list );
        ptr->do_list = -1;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ext_camera_t * ext_camera_reinit( ext_camera_t * ptr )
{
    ptr = ext_camera_dtor( ptr );
    ptr = ext_camera_ctor( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool_t ext_camera_update_projection( ext_camera_t * ptr )
{
    float frustum_near, frustum_far, aspect_ratio;

    if ( NULL == ptr ) return bfalse;

    //---- set the camera's projection matrix
    aspect_ratio = ( ptr->screen.xmax - ptr->screen.xmin ) / ( ptr->screen.ymax - ptr->screen.ymin );

    // the nearest we will have to worry about is 1/2 of a tile
    frustum_near = GRID_ISIZE * 0.25f;
    // set the maximum depth to be the "largest possible size" of a mesh
    frustum_far  = GRID_ISIZE * 256 * SQRT_TWO;

    camera_gluPerspective( &( ptr->which ), CAM_FOV, aspect_ratio, frustum_near, frustum_far );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ext_camera_set_screen( ext_camera_t * ptr, float xmin, float ymin, float xmax, float ymax )
{
    if ( NULL == ptr ) return bfalse;

    // set the screen
    ptr->screen.xmin = xmin;
    ptr->screen.ymin = ymin;
    ptr->screen.xmax = xmax;
    ptr->screen.ymax = ymax;

    return ext_camera_update_projection( ptr );
}

//--------------------------------------------------------------------------------------------
bool_t ext_camera_get_screen( ext_camera_t * pext, ego_frect_t * prect )
{
    // a NULL camera is an error
    if ( NULL == pext ) return bfalse;

    // an invalid camera gives "nothing"
    if ( !pext->on )
    {
        BLANK_STRUCT_PTR( prect );
        return bfalse;
    }

    // a NULL prect is an accident
    if ( NULL == prect ) return btrue;

    // copy the data
    memmove( prect, &( pext->screen ), sizeof( *prect ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
// INITIALIZATION
//--------------------------------------------------------------------------------------------

bool_t camera_system_is_started( void )
{
    return _camera_system_initialized;
}

//--------------------------------------------------------------------------------------------
egolib_rv camera_system_begin( int camera_count )
{
    if ( !_camera_system_initialized )
    {
        // fix an out-of-range camera count
        // allow for 0 cameras.
        if ( camera_count < 0 || camera_count > MAX_CAMERAS )
        {
            camera_count = 1;
        }

        // construct the cameras
        ext_camera_list_ctor( &_camera_lst );

        // we're initialized.
        // !set this here, or bad things happen!
        _camera_system_initialized = btrue;

        // reset all the cameras
        camera_system_init( camera_count );
    }

    return _camera_system_initialized ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egolib_rv camera_system_end( void )
{
    if ( _camera_system_initialized )
    {
        // de-construct the cameras
        ext_camera_list_dtor( &_camera_lst );

        _camera_system_initialized = bfalse;
    }

    return !_camera_system_initialized ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egolib_rv _camera_system_begin_camera_ptr( ext_camera_t * pcam )
{
    renderlist_mgr_t * rmgr_ptr = NULL;
    dolist_mgr_t     * dmgr_ptr = NULL;
    renderlist_t     * rlst_ptr = NULL;

    if ( NULL == pcam || ( rv_success != camera_system_begin( -1 ) ) )
    {
        return rv_error;
    }

    if ( pcam->on ) return rv_success;

    // re-initialize the camera
    pcam = ext_camera_reinit( pcam );
    if ( NULL == pcam ) return rv_error;

    rmgr_ptr = gfx_system_get_renderlist_mgr();
    if ( NULL == rmgr_ptr )
    {
        return rv_error;
    }

    dmgr_ptr = gfx_system_get_dolist_mgr();
    if ( NULL == dmgr_ptr )
    {
        return rv_error;
    }

    // make the default viewport fullscreen
    pcam->screen.xmin = 0.0f;
    pcam->screen.xmax = sdl_scr.x;
    pcam->screen.ymin = 0.0f;
    pcam->screen.ymax = sdl_scr.y;

    // lock a renderlist for this camera
    pcam->render_list = renderlist_mgr_get_free_idx( rmgr_ptr );

    // connect the renderlist to a mesh
    rlst_ptr = renderlist_mgr_get_ptr( rmgr_ptr, pcam->render_list );
    renderlist_attach_mesh( rlst_ptr, PMesh );

    // lock a dolist for this camera
    pcam->do_list = dolist_mgr_get_free_idx( dmgr_ptr );

    // turn the camera on
    pcam->on = btrue;

    return pcam->on ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egolib_rv camera_system_begin_camera( int index )
{
    if ( rv_success != camera_system_begin( -1 ) )
    {
        return rv_error;
    }

    if ( index < 0 || index >= MAX_CAMERAS )
    {
        return rv_error;
    }

    return _camera_system_begin_camera_ptr( _camera_lst.lst + index );
}

//--------------------------------------------------------------------------------------------
egolib_rv _camera_system_end_camera_ptr( ext_camera_t * pcam )
{
    renderlist_mgr_t * rmgr_ptr = NULL;
    dolist_mgr_t     * dmgr_ptr = NULL;

    // !do not try to start the camera system here!
    // An external function could try to deactivate a camera after
    // it has already ended the camera system.
    if ( !camera_system_is_started() )
    {
        return rv_fail;
    }

    if ( NULL == pcam )
    {
        return rv_error;
    }

    if ( !pcam->on ) return rv_success;

    // get a pointer to the renderlist manager
    // do this after testing whether the camera is on
    rmgr_ptr = gfx_system_get_renderlist_mgr();
    if ( NULL == rmgr_ptr )
    {
        return rv_fail;
    }

    // get a pointer to the dolist manager
    // do this after testing whether the camera is on
    dmgr_ptr = gfx_system_get_dolist_mgr();
    if ( NULL == dmgr_ptr )
    {
        return rv_error;
    }

    // turn the camera off
    pcam->on = bfalse;

    // reset the camera targets
    pcam->targets.count = 0;

    // free any locked renderlist
    if ( -1 != pcam->render_list )
    {
        renderlist_mgr_free_one( rmgr_ptr, pcam->render_list );
        pcam->render_list = -1;
    }

    // free any locked dolist
    if ( -1 != pcam->do_list )
    {
        dolist_mgr_free_one( dmgr_ptr, pcam->do_list );
        pcam->do_list = -1;
    }

    return !pcam->on ? rv_success : rv_fail;

}

//--------------------------------------------------------------------------------------------
egolib_rv camera_system_end_camera( int index )
{
    // !do not try to start the camera system here!
    // An external function could try to deactivate a camera after
    // it has already ended the camera system.
    if ( !camera_system_is_started() )
    {
        return rv_fail;
    }

    if ( index < 0 || index >= MAX_CAMERAS )
    {
        return rv_error;
    }

    return _camera_system_end_camera_ptr( _camera_lst.lst + index );
}

//--------------------------------------------------------------------------------------------
egolib_rv camera_system_init( int camera_count )
{
    egolib_rv tmp_retval;

    if ( !camera_system_is_started() )
    {
        return rv_fail;
    }

    // set-up the cameras
    tmp_retval = _camera_system_autoformat_cameras( camera_count );
    if ( rv_error == tmp_retval )
    {
        return tmp_retval;
    }

    tmp_retval = _camera_system_autoset_targets();
    if ( rv_error == tmp_retval )
    {
        return tmp_retval;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
camera_t * camera_system_get_main( void )
{
    int cnt;
    camera_t * retval = NULL;

    if ( !camera_system_is_started() ) return NULL;

    // search for a valid camera
    retval = NULL;
    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        if ( _camera_lst.lst[cnt].on )
        {
            retval = &( _camera_lst.lst[cnt].which );
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ext_camera_list_t * camera_system_get_list( void )
{
    if ( !camera_system_is_started() ) return NULL;

    return &_camera_lst;
}

//--------------------------------------------------------------------------------------------
// EXTERNAL FUNCTIONS
//--------------------------------------------------------------------------------------------

egolib_rv camera_system_reset( ego_mpd_t * pmesh )
{
    int cnt;

    if ( !camera_system_is_started() )
    {
        return rv_fail;
    }

    // reset each valid camera
    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        ext_camera_t * pcam = _camera_lst.lst + cnt;

        if ( !pcam->on ) continue;

        camera_reset( &( pcam->which ), pmesh, pcam->targets.who, pcam->targets.count );
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv camera_system_move( ego_mpd_t * pmesh )
{
    int cnt;

    if ( !camera_system_is_started() )
    {
        return rv_fail;
    }

    // move each valid camera
    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        ext_camera_t * pcam = _camera_lst.lst + cnt;

        if ( !pcam->on ) continue;

        camera_move( &( pcam->which ), pmesh, pcam->targets.who, pcam->targets.count );
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv camera_system_reset_targets( ego_mpd_t * pmesh )
{
    int cnt;

    if ( !camera_system_is_started() )
    {
        return rv_fail;
    }

    // move each valid camera
    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        ext_camera_t * pcam = _camera_lst.lst + cnt;

        if ( !pcam->on ) continue;

        camera_reset_target( &( pcam->which ), pmesh, pcam->targets.who, pcam->targets.count );
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv ext_camera_begin( ext_camera_t * pext, GLint * mode )
{
    /// how much bigger is mProjection_big than mProjection?

    if ( NULL == pext )
    {
        return rv_error;
    }

    // grab the initial matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, mode );

    // scissor the output to the this area
    GL_DEBUG( glEnable )( GL_SCISSOR_TEST );
    GL_DEBUG( glScissor )( pext->screen.xmin, sdl_scr.y - pext->screen.ymax, pext->screen.xmax - pext->screen.xmin, pext->screen.ymax - pext->screen.ymin );

    // set the viewport
    GL_DEBUG( glViewport )( pext->screen.xmin, sdl_scr.y - pext->screen.ymax, pext->screen.xmax - pext->screen.xmin, pext->screen.ymax - pext->screen.ymin );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv ext_camera_end( ext_camera_t * pext, GLint mode )
{
    if ( NULL == pext )
    {
        return rv_error;
    }

    // return the old modelview mode
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    // return the old projection mode
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    // make the viewport the entire screen
    glViewport( 0, 0, sdl_scr.x, sdl_scr.y );

    // turn off the scissor mode
    glDisable( GL_SCISSOR_TEST );

    // return the matrix mode to whatever it was before
    glMatrixMode( mode );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv camera_system_render_all( renderer_ptr_t prend )
{
    GLint mode;
    int cnt;

    egolib_rv retval = rv_error;
    ext_camera_t * pext;

    camera_t * main_cam_ptr = PCamera;

    if ( NULL == prend )
    {
        retval = rv_error;
        goto camera_system_render_all_end;
    }

    if ( rv_success != camera_system_begin( -1 ) )
    {
        retval = rv_fail;
        goto camera_system_render_all_end;
    }

    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        pext = _camera_lst.lst + cnt;

        // only cameras that are on
        if ( !pext->on )
        {
            continue;
        }

        // set the "global" camera pointer to this camera
        set_PCamera( &( pext->which ) );

        // has this camera already rendered this frame?
        if ( pext->last_frame >= 0 && ( Uint32 )pext->last_frame >= game_frame_all )
        {
            continue;
        }

        // set up everything for this camera
        if ( rv_error == ext_camera_begin( pext, &mode ) )
        {
            retval = rv_error;
            goto camera_system_render_all_end;
        }

        // render the world for this camera
        prend( &( pext->which ), pext->render_list, pext->do_list );

        // undo the camera setup
        if ( rv_error == ext_camera_end( pext, mode ) )
        {
            retval = rv_error;
            goto camera_system_render_all_end;
        }

        // empty the pipeline
        //GL_DEBUG( glFlush() );

        pext->last_frame = game_frame_all;
    }

    retval = rv_success;

camera_system_render_all_end:

    // reset the "global" camera pointer to whatever it was
    set_PCamera( main_cam_ptr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv _camera_system_autoformat_cameras( int cameras )
{
    // 1/2 of border between panes in pixels
    static const int border = 1;

    float  aspect_ratio;
    bool_t widescreen;

    ext_camera_t * pcam;
    int cam_count;
    egolib_rv retval;

    if ( !camera_system_is_started() )
    {
        return rv_fail;
    }

    // turn off all the cameras
    ext_camera_list_reinit( &_camera_lst );

    aspect_ratio = ( float )sdl_scr.x / ( float )sdl_scr.y;
    widescreen = ( aspect_ratio > ( 4.0f / 3.0f ) );

    retval = rv_fail;
    cam_count = 0;
    pcam = _camera_lst.lst;
    if ( widescreen )
    {
        switch ( cameras )
        {
            default:
            case 1:
                // fullscreen
                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, 0.0f, sdl_scr.x, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                retval = rv_success;
                break;

            case 2:
                // wider than tall, so windows are side-by side
                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                retval = rv_success;
                break;

            case 3:
                // wider than tall, so windows are side-by side

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, 0.0f, sdl_scr.x / 3.0f - border, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, sdl_scr.x / 3.0f + border, 0.0f, 2.0f * sdl_scr.x / 3.0f - border, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 2.0f * sdl_scr.x / 3.0f + border, 0.0f, sdl_scr.x, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                retval = rv_success;
                break;

            case 4:
                // 4 panes
                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y * 0.5f - border ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y * 0.5f - border ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, sdl_scr.y * 0.5f + border, sdl_scr.x * 0.5f - border, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, sdl_scr.x * 0.5f + border, sdl_scr.y * 0.5f + border, sdl_scr.x, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                retval = rv_success;
                break;

        }

    }
    else
    {
        switch ( cameras )
        {
            default:
            case 1:
                // fullscreen

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, 0.0f, sdl_scr.x, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                retval = rv_success;
                break;

            case 2:
                if ( sdl_scr.x  >= sdl_scr.y )
                {
                    // wider than tall, so windows are side-by side

                    if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                    {
                        if ( ext_camera_set_screen( pcam, 0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y ) )
                        {
                            cam_count++;
                            pcam++;
                        }
                    }

                    if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                    {
                        if ( ext_camera_set_screen( pcam, sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y ) )
                        {
                            cam_count++;
                            pcam++;
                        }
                    }
                }
                else
                {
                    // taller than wide so, windows are one-over-the-other
                    if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                    {
                        if ( ext_camera_set_screen( pcam, 0.0f, 0.0f, sdl_scr.x, sdl_scr.y * 0.5f - border ) )
                        {
                            cam_count++;
                            pcam++;
                        }
                    }

                    if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                    {
                        if ( ext_camera_set_screen( pcam, 0.0f, sdl_scr.y * 0.5f + border, sdl_scr.x, sdl_scr.y ) )
                        {
                            cam_count++;
                            pcam++;
                        }
                    }
                }

                retval = rv_success;
                break;

            case 3:
                // more square, so 4 panes, but one is blank

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y * 0.5f - border ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y * 0.5f - border ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, sdl_scr.y * 0.5f + border, sdl_scr.x, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                retval = rv_success;
                break;

            case 4:
                // 4 panes

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, 0.0f, sdl_scr.x * 0.5f - border, sdl_scr.y * 0.5f - border ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, sdl_scr.x * 0.5f + border, 0.0f, sdl_scr.x, sdl_scr.y * 0.5f - border ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, 0.0f, sdl_scr.y * 0.5f + border, sdl_scr.x * 0.5f - border, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                if ( rv_success == _camera_system_begin_camera_ptr( pcam ) )
                {
                    if ( ext_camera_set_screen( pcam, sdl_scr.x * 0.5f + border, sdl_scr.y * 0.5f + border, sdl_scr.x, sdl_scr.y ) )
                    {
                        cam_count++;
                        pcam++;
                    }
                }

                retval = rv_success;
                break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv _camera_system_autoset_targets( void )
{
    // spread the targets out over all the cameras

    int cnt, target_count;
    ext_camera_t * pcam, * pcam_end;

    if ( !camera_system_is_started() )
    {
        return rv_fail;
    }

    pcam     = NULL;
    pcam_end = _camera_lst.lst + MAX_CAMERAS;

    // find a valid camera
    for ( pcam  = _camera_lst.lst ; pcam < pcam_end; pcam++ )
    {
        if ( pcam->on ) break;
    }

    // is there at least one valid camera?
    target_count = 0;
    if ( pcam < pcam_end )
    {
        // put all the valid players into camera 0
        for ( cnt = 0; cnt < MAX_PLAYER; cnt++ )
        {
            player_t * ppla;

            // in case the camera has looped around
            if ( pcam >= pcam_end )
            {
                // find a valid camera
                for ( pcam = _camera_lst.lst; pcam < pcam_end; pcam++ )
                {
                    // only use a camera that is on, and is not full
                    if ( pcam->on && pcam->targets.count < MAX_LOCAL_PLAYERS ) break;
                }
            }
            else if ( !pcam->on )
            {
                for ( /* nothing */ ; pcam < pcam_end; pcam++ )
                {
                    // only use a camera that is on, and is not full
                    if ( pcam->on && pcam->targets.count < MAX_LOCAL_PLAYERS ) break;
                }
            }

            // if we can't find any valid camera, get out of here
            if ( pcam >= pcam_end )
            {
                break;
            }

            // only look at valid players
            ppla = PlaStack_get_ptr( cnt );
            if ( !ppla->valid || !VALID_CHR_RANGE( ppla->index ) ) continue;

            // only look at local players
            if ( NULL == ppla->pdevice ) continue;

            // store the target
            pcam->targets.who[pcam->targets.count] = ppla->index;
            pcam->targets.count++;

            // advance the camera to the next camera
            target_count++;
            pcam++;
        }
    }

    if ( pcam < pcam_end )
    {
        // still not enough things to track for the number of cameras.
        // should not happen unless I am messing with the camera code...
        for ( cnt = 0; cnt < StatusList.count && pcam < pcam_end; cnt++ )
        {
            chr_t * pchr;
            CHR_REF blah;

            // if we landed on an inactive camera, find the next active one
            if ( !pcam->on )
            {
                for ( /* nothing */ ; pcam < pcam_end; pcam++ )
                {
                    // only use a camera that is on, and is not full
                    if ( pcam->on && pcam->targets.count < MAX_LOCAL_PLAYERS ) break;
                }
            }

            // only go until we have added someone to every active camera
            if ( pcam >= pcam_end ) break;

            // grab someone on the status list
            blah = StatusList.lst[cnt].who;

            // get a pointer, if allowed
            if ( !VALID_CHR_RANGE( blah ) ) continue;
            pchr = ChrList_get_ptr( blah );

            // ignore local players
            if ( pchr->islocalplayer ) continue;

            // store the target
            pcam->targets.who[pcam->targets.count] = blah;
            pcam->targets.count++;

            // advance the camera to the next camera
            target_count++;
            pcam++;
        }

        // turn off all cameras with no targets
        for ( /* nothing */ ; pcam < pcam_end; pcam++ )
        {
            ext_camera_reinit( pcam );
        }
    }

    return ( target_count > 0 ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ext_camera_list_t * ext_camera_list_ctor( ext_camera_list_t * plst )
{
    int cnt;

    if ( NULL == plst ) return plst;

    BLANK_STRUCT_PTR( plst );

    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        ext_camera_ctor( plst->lst + cnt );
    }
    plst->count = MAX_CAMERAS;

    return plst;
}

//--------------------------------------------------------------------------------------------
ext_camera_list_t * ext_camera_list_dtor( ext_camera_list_t * plst )
{
    int cnt;

    if ( NULL == plst ) return plst;

    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        ext_camera_dtor( plst->lst + cnt );
    }

    BLANK_STRUCT_PTR( plst );

    return plst;
}

//--------------------------------------------------------------------------------------------
ext_camera_list_t * ext_camera_list_free( ext_camera_list_t * plst )
{
    int cnt;

    if ( NULL == plst ) return plst;

    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        ext_camera_free( plst->lst + cnt );
    }
    plst->count = 0;

    return plst;
}

//--------------------------------------------------------------------------------------------
ext_camera_list_t * ext_camera_list_reinit( ext_camera_list_t * plst )
{
    plst = ext_camera_list_dtor( plst );
    plst = ext_camera_list_ctor( plst );

    return plst;
}

//--------------------------------------------------------------------------------------------
int camera_list_find_target_index( ext_camera_list_t * plst, const CHR_REF itarget )
{
    int cnt, tnc;
    int retval;
    ext_camera_t * pext;

    if ( NULL == plst || !VALID_CHR_RANGE( itarget ) ) return -1;

    retval = -1;
    for ( cnt = 0; cnt < MAX_CAMERAS && -1 == retval; cnt++ )
    {
        pext = plst->lst + cnt;

        if ( pext->on )
        {
            for ( tnc = 0; tnc < pext->targets.count; tnc++ )
            {
                if ( itarget == pext->targets.who[tnc] )
                {
                    retval = cnt;
                    break;
                }
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
camera_t * camera_list_find_target( ext_camera_list_t * plst, const CHR_REF itarget )
{
    int index;

    index = camera_list_find_target_index( plst, itarget );

    return camera_list_get_camera_index( plst, index );
}

//--------------------------------------------------------------------------------------------
ext_camera_t * camera_list_get_ext_camera_index( ext_camera_list_t * plst, int index )
{
    ext_camera_t * retval = NULL;

    if ( NULL == plst ) return NULL;

    // use the index to get a camera
    retval = NULL;
    if ( index >= 0 && index < MAX_CAMERAS )
    {
        ext_camera_t * pext = plst->lst + index;

        if ( pext->on )
        {
            retval = pext;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
camera_t * camera_list_get_camera_index( ext_camera_list_t * plst, int index )
{
    ext_camera_t * pext;
    camera_t * retval = NULL;

    // grab the ext camera
    pext = camera_list_get_ext_camera_index( plst, index );

    // get the camera_t or the default camera
    if ( NULL == pext )
    {
        retval = PCamera;
    }
    else
    {
        retval = &( pext->which );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ext_camera_iterator_t * camera_list_iterator_begin( ext_camera_list_t * plst )
{
    int cnt, index;
    ext_camera_iterator_t * retval = NULL;

    if ( NULL == plst ) return NULL;

    // find an index
    index = -1;
    for ( cnt = 0; cnt < MAX_CAMERAS; cnt++ )
    {
        if ( plst->lst[cnt].on )
        {
            index = cnt;
        }
    }

    retval = NULL;
    if ( -1 != index )
    {
        retval = EGOBOO_NEW( ext_camera_iterator_t );
        if ( NULL != retval )
        {
            retval->index = index;
            retval->list  = plst;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ext_camera_iterator_t * camera_list_iterator_next( ext_camera_iterator_t * it )
{
    ext_camera_list_t * plst;

    if ( NULL == it ) return NULL;

    plst = it->list;
    if ( NULL == plst )
    {
        goto camera_list_iterator_next_fail;
    }

    if ( it->index >= 0 && it->index < MAX_CAMERAS )
    {
        for ( it->index++; it->index < MAX_CAMERAS; it->index++ )
        {
            if ( plst->lst[it->index].on )
            {
                break;
            }
        }
    }

    if ( it->index < 0 || it->index >= MAX_CAMERAS )
    {
        goto camera_list_iterator_next_fail;
    }

    return it;

camera_list_iterator_next_fail:

    it = camera_list_iterator_end( it );

    return it;
}

//--------------------------------------------------------------------------------------------
ext_camera_iterator_t * camera_list_iterator_end( ext_camera_iterator_t * it )
{
    EGOBOO_DELETE( it );

    return it;
}

//--------------------------------------------------------------------------------------------
camera_t * camera_list_iterator_get_camera( ext_camera_iterator_t * it )
{
    ext_camera_t * pext;
    camera_t * retval = NULL;

    ext_camera_list_t * plst;

    if ( NULL == it ) return NULL;

    plst = it->list;
    if ( NULL == plst )
    {
        return NULL;
    }

    retval = NULL;
    if ( it->index >= 0 && it->index < MAX_CAMERAS )
    {
        pext = plst->lst + it->index;

        if ( pext->on )
        {
            retval = &( pext->which );
        }
    }

    return retval;
}
