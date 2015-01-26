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

/// @file  game/egoboo.c
/// @brief Code for the main program process
/// @details

#define DECLARE_GLOBALS
#include "game/egoboo.h"
#include "egolib/egolib.h"
#include "game/network.h"
#include "game/audio/AudioSystem.hpp"
#include "game/ui.h"
#include "game/input.h"
#include "game/game.h"
#include "game/menu.h"
#include "game/player.h"
#include "game/graphic.h"
#include "game/graphic_texture.h"
#include "game/renderer_2d.h"
#include "game/char.h"
#include "game/particle.h"
#include "game/enchant.h"
#include "game/collision.h"
#include "game/profiles/Profile.hpp"
#include "game/profiles/ProfileSystem.hpp"
#include "game/module/Module.hpp"

#include "game/ChrList.h"
#include "game/EncList.h"
#include "game/PrtList.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int do_ego_proc_begin( ego_process_t * eproc );
static int do_ego_proc_running( ego_process_t * eproc );
static int do_ego_proc_leaving( ego_process_t * eproc );
static int do_ego_proc_run( ego_process_t * eproc, double frameDuration );

static void memory_cleanUp();
static int  ego_init_SDL();

static void object_systems_begin();
static void object_systems_end();

static void _quit_game( ego_process_t * pgame );

static ego_process_t * ego_process_init( ego_process_t * eproc, int argc, char **argv );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C"
{
#endif
    extern bool config_download( egoboo_config_t * pcfg, bool synch_from_file );
    extern bool config_upload( egoboo_config_t * pcfg );
#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static ClockState_t    * _gclock = NULL;
static ego_process_t     _eproc;

static bool  screenshot_keyready  = true;

static bool _sdl_atexit_registered    = false;
static bool _sdl_initialized_base     = false;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_process_t     * EProc   = &_eproc;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int do_ego_proc_begin( ego_process_t * eproc )
{
    // initialize the virtual filesystem first
    vfs_init( NULL );
    setup_init_base_vfs_paths();

    // Initialize logging next, so that we can use it everywhere.
    log_init("/debug/log.txt", LOG_DEBUG);

    // start initializing the various subsystems
    log_message( "Starting Egoboo " VERSION " ...\n"  );
    log_info( "PhysFS file system version %s has been initialized...\n", vfs_getVersion() );

    sys_initialize();
    clk_init();
	_gclock = clk_create("global clock",1); 
	EGOBOO_ASSERT(NULL != _gclock);

    // read the "setup.txt" file
    setup_read_vfs();

    // download the "setup.txt" values into the cfg struct
    config_download( &cfg, true );

    // do basic system initialization
    ego_init_SDL();
    gfx_system_begin();

    // synchronize the config values with the various game subsystems
    // do this after the ego_init_SDL() and gfx_system_init_OpenGL() in case the config values are clamped
    // to valid values
    config_download( &cfg, true );

    log_info( "Initializing SDL_Image version %d.%d.%d... ", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL );
    GLSetup_SupportedFormats();

    // read all the scantags
    scantag_read_all_vfs( "mp_data/scancode.txt" );

    // load input
    input_settings_load_vfs( "/controls.txt", -1 );

    // initialize the console
    egolib_console_begin();

    // initialize network communication
    net_initialize();

    // initialize the sound system
    _audioSystem.initialize(cfg);
    _audioSystem.loadAllMusic();

    // initialize the random treasure system
    init_random_treasure_tables_vfs( "mp_data/randomtreasure.txt" );

    // make sure that a bunch of stuff gets initialized properly
    object_systems_begin();
    ego_mesh_ctor( PMesh );
    gfx_system_init_all_graphics();
    _profileSystem.begin();

    // setup the system gui
    ui_begin( "mp_data/Bo_Chen.ttf", 24 );

    // clear out the import and remote directories
    vfs_empty_temp_directories();

    // register the memory_cleanUp function to automatically run whenever the program exits
    atexit( memory_cleanUp );

    // initialize the game process (not active)
    game_process_init( GProc );

    // initialize the menu process (active)
    menu_process_init( MProc );
    process_start( PROC_PBASE( MProc ) );

    // Initialize the process
    process_start( PROC_PBASE( eproc ) );

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_running( ego_process_t * eproc )
{
    bool menu_valid, game_valid;

    if ( !process_validate( PROC_PBASE( eproc ) ) ) return -1;

    eproc->was_active  = eproc->base.valid;

    menu_valid = process_validate( PROC_PBASE( MProc ) );
    game_valid = process_validate( PROC_PBASE( GProc ) );
    if ( !menu_valid && !game_valid )
    {
        process_kill( PROC_PBASE( eproc ) );
        return 1;
    }

    if ( eproc->base.paused ) return 0;

    if ( process_running( PROC_PBASE( MProc ) ) )
    {
        // menu settings
        SDL_WM_GrabInput( SDL_GRAB_OFF );
    }
    else
    {
        // in-game settings
        SDL_ShowCursor( cfg.hide_mouse ? SDL_DISABLE : SDL_ENABLE );
        SDL_WM_GrabInput( cfg.grab_mouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
    }

    // Clock updates each frame
    game_update_timers();
    clk_frameStep( _gclock );
    eproc->base.frameDuration = clk_getFrameDuration( _gclock );

    // read the input values
    input_read_all_devices();

    if ( pickedmodule_ready && !process_running( PROC_PBASE( MProc ) ) )
    {
        // a new module has been picked

        // reset the flag
        pickedmodule_ready = false;

        // start the game process
        process_start( PROC_PBASE( GProc ) );
    }

    // Test the panic button
    if ( SDL_KEYDOWN( keyb, SDLK_q ) && SDL_KEYDOWN( keyb, SDLK_LCTRL ) )
    {
        // terminate the program
        process_kill( PROC_PBASE( eproc ) );
    }

    if ( cfg.dev_mode )
    {
        if ( !SDL_KEYDOWN( keyb, SDLK_F10 ) )
        {
            single_frame_keyready = true;
        }
        else if ( single_frame_keyready && SDL_KEYDOWN( keyb, SDLK_F10 ) )
        {
            if ( !single_frame_mode )
            {
                single_frame_mode = true;
            }

            // request one update and one frame
            single_frame_requested  = true;
            single_update_requested = true;
            single_frame_keyready   = false;
        }

    }

    // Check for screenshots
    if ( !SDL_KEYDOWN( keyb, SDLK_F11 ) )
    {
        screenshot_keyready = true;
    }
    else if ( screenshot_keyready && SDL_KEYDOWN( keyb, SDLK_F11 ) )
    {
        screenshot_keyready = false;
        screenshot_requested = true;
    }

    if ( cfg.dev_mode && SDL_KEYDOWN( keyb, SDLK_F9 ) && nullptr != PMod )
    {
        // super secret "I win" button
        //PMod->beat        = true;
        //PMod->exportvalid = true;

        CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
        {
            if ( !VALID_PLA( pchr->is_which_player ) )
            {
                kill_character( cnt, ( CHR_REF )511, false );
            }
        }
        CHR_END_LOOP();
    }

    // handle an escape by passing it on to all active sub-processes
    if ( eproc->escape_requested )
    {
        eproc->escape_requested = false;

        // use the escape key to get out of single frame mode
        single_frame_mode = false;

        if ( process_running( PROC_PBASE( GProc ) ) )
        {
            GProc->escape_requested = true;
        }

        if ( process_running( PROC_PBASE( MProc ) ) )
        {
            MProc->escape_requested = true;
        }
    }

    // run the sub-processes
    game_process_run( GProc, eproc->base.frameDuration );
    menu_process_run( MProc, eproc->base.frameDuration );

    // toggle the free-running mode on the process timers
    if ( cfg.dev_mode )
    {
        bool free_running_keydown = SDL_KEYDOWN( keyb, SDLK_f ) && SDL_KEYDOWN( keyb, SDLK_LCTRL );
        if ( free_running_keydown )
        {
            eproc->free_running_latch_requested = true;
        }

        if ( !free_running_keydown && eproc->free_running_latch_requested )
        {
            eproc->free_running_latch = true;
            eproc->free_running_latch_requested = false;
        }
    }

    if ( eproc->free_running_latch )
    {
        if ( NULL != MProc )
        {
            MProc->gui_timer.free_running = TO_C_BOOL( !MProc->gui_timer.free_running );
        }

        if ( NULL != GProc )
        {
            GProc->ups_timer.free_running = TO_C_BOOL( !GProc->ups_timer.free_running && !egonet_on() );
            GProc->fps_timer.free_running = TO_C_BOOL( !GProc->fps_timer.free_running );
        }

        eproc->free_running_latch = false;
    }

    // a heads up display that can be used to debug values that are used by both the menu and the game
    // do_game_hud();

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_leaving( ego_process_t * eproc )
{
    if ( !process_validate( PROC_PBASE( eproc ) ) ) return -1;

    // make sure that the game is terminated
    if ( !GProc->base.terminated )
    {
        game_process_run( GProc, eproc->base.frameDuration );
    }

    // make sure that the menu is terminated
    if ( !MProc->base.terminated )
    {
        menu_process_run( MProc, eproc->base.frameDuration );
    }

    if ( GProc->base.terminated && MProc->base.terminated )
    {
        process_terminate( PROC_PBASE( eproc ) );
    }

    if ( eproc->base.terminated )
    {
        // hopefully this will only happen once
        object_systems_end();
		if (NULL != _gclock) { clk_destroy(_gclock); _gclock = NULL; }
        egolib_console_end();
        ui_end();
        gfx_system_end();
        setup_clear_base_vfs_paths();
    }

    return eproc->base.terminated ? 0 : 1;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_run( ego_process_t * eproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_validate( PROC_PBASE( eproc ) ) ) return -1;
    eproc->base.frameDuration = frameDuration;

    if ( !eproc->base.paused ) return 0;

    if ( eproc->base.killme )
    {
        eproc->base.state = proc_leaving;
    }

    switch ( eproc->base.state )
    {
        case proc_begin:
            proc_result = do_ego_proc_begin( eproc );

            if ( 1 == proc_result )
            {
                eproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            // proc_result = do_ego_proc_entering( eproc );

            eproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = do_ego_proc_running( eproc );

            if ( 1 == proc_result )
            {
                eproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = do_ego_proc_leaving( eproc );

            if ( 1 == proc_result )
            {
                eproc->base.state  = proc_finish;
                eproc->base.killme = false;
            }
            break;

        case proc_finish:
            process_terminate( PROC_PBASE( eproc ) );
            break;

        default:
            /* do nothing */
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int SDL_main( int argc, char **argv )
{
    /// @author ZZ
    /// @details This is where the program starts and all the high level stuff happens

    int result = 0;

    // initialize the process
    ego_process_init( EProc, argc, argv );

    // turn on all basic services
    do_ego_proc_begin( EProc );

#if defined(EGOBOO_THROTTLED)
    // update the game at the user-defined rate
    EProc->loop_timer.free_running = false;
    MProc->gui_timer.free_running  = false;
    GProc->ups_timer.free_running  = false;
    GProc->fps_timer.free_running  = false;
#else
    // make the game update as fast as possible
    EProc->loop_timer.free_running = true;
    MProc->gui_timer.free_running  = true;
    GProc->ups_timer.free_running  = true;
    GProc->fps_timer.free_running  = true;
#endif

    // run the processes
    gfx_request_clear_screen();
    while ( !EProc->base.killme && !EProc->base.terminated )
    {
        if ( !egolib_timer__throttle( &( EProc->loop_timer ), 100.0f ) )
        {
            // let the OS breathe. It may delay as long as 10ms
            SDL_Delay( 1 );
        }
        else
        {

            // clear the screen if needed
            gfx_do_clear_screen();

            do_ego_proc_running( EProc );

            // flip the graphics page if need be
            gfx_do_flip_pages();

            // let the OS breathe. It may delay as long as 10ms
            if ( !EProc->loop_timer.free_running && update_lag < 3 )
            {
                SDL_Delay( 1 );
            }
        }
    }

    // terminate the game and menu processes
    process_kill( PROC_PBASE( GProc ) );
    process_kill( PROC_PBASE( MProc ) );
    while ( !EProc->base.terminated )
    {
        result = do_ego_proc_leaving( EProc );
    }

    return result;
}

//--------------------------------------------------------------------------------------------
void memory_cleanUp()
{
    /// @author ZF
    /// @details This function releases all loaded things in memory and cleans up everything properly

    log_info( "memory_cleanUp() - Attempting to clean up loaded things in memory... " );

    // quit any existing game
    _quit_game( EProc );

    // synchronize the config values with the various game subsystems
    config_synch( &cfg, true );

    // quit the setup system, making sure that the setup file is written
    setup_write_vfs();
    setup_end();

    // delete all the graphics allocated by SDL and OpenGL
    gfx_system_delete_all_graphics();

    // make sure that the current control configuration is written
    input_settings_save_vfs( "controls.txt", -1 );

    // shut down the ui
    ui_end();

    // shut down the network
    if ( egonet_on() )
    {
        net_shutDown();
    }

    // shut down the clock services
	if (_gclock) { clk_destroy(_gclock); _gclock = NULL; }
    clk_shutdown();

    // deallocate any dynamically allocated collision memory
    collision_system_end();

    // deallocate any dynamically allocated scripting memory
    scripting_system_end();

    // deallocate all dynamically allocated memory for characters, particles, enchants, and models
    object_systems_end();

    log_message( "Success!\n" );
    log_info( "Exiting Egoboo " VERSION " the good way...\n" );

    // shut down the log services
    log_shutdown();
}

//--------------------------------------------------------------------------------------------
int ego_init_SDL()
{
    ego_init_SDL_base();
    input_system_init();

    return _sdl_initialized_base;
}

//--------------------------------------------------------------------------------------------
void ego_init_SDL_base()
{
    if ( _sdl_initialized_base ) return;

    log_info( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
    if ( SDL_Init( 0 ) < 0 )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    if ( !_sdl_atexit_registered )
    {
        atexit( SDL_Quit );
        _sdl_atexit_registered = false;
    }

    log_info( "Intializing SDL Timing Services... " );
    if ( SDL_InitSubSystem( SDL_INIT_TIMER ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    log_info( "Intializing SDL Event Threading... " );
    if ( SDL_InitSubSystem( SDL_INIT_EVENTTHREAD ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    _sdl_initialized_base = true;
}

//--------------------------------------------------------------------------------------------
void object_systems_begin()
{
    /// @author BB
    /// @details initialize all the object systems

    particle_system_begin();
    enchant_system_begin();
    model_system_begin();
}

//--------------------------------------------------------------------------------------------
void object_systems_end()
{
    /// @author BB
    /// @details quit all the object systems

    particle_system_end();
    enchant_system_end();
    _characterList.clear();
    model_system_end();
}

//--------------------------------------------------------------------------------------------
void _quit_game( ego_process_t * pgame )
{
    /// @author ZZ
    /// @details This function exits the game entirely

    if ( process_running( PROC_PBASE( pgame ) ) )
    {
        game_quit_module();
    }

    // tell the game to kill itself
    process_kill( PROC_PBASE( pgame ) );

    // clear out the import and remote directories
    vfs_empty_temp_directories();
}

//--------------------------------------------------------------------------------------------
ego_process_t * ego_process_init( ego_process_t * eproc, int argc, char **argv )
{
    if ( NULL == eproc ) return NULL;

    BLANK_STRUCT_PTR( eproc )

    process_init( PROC_PBASE( eproc ) );

    eproc->argv0 = ( argc > 0 ) ? argv[0] : NULL;

    return eproc;
}

//--------------------------------------------------------------------------------------------
Uint32 egoboo_get_ticks()
{
    Uint32 ticks = 0;

    if ( single_frame_mode )
    {
        ticks = UPDATE_SKIP * update_wld;
    }
    else
    {
        ticks = SDL_GetTicks();
    }

    return ticks;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool config_download( egoboo_config_t * pcfg, bool synch_from_file )
{
    size_t tmp_maxparticles;
    bool rv;

    // synchronize settings from a pre-loaded setup.txt? (this will load setup.txt into *pcfg)
    if ( synch_from_file )
    {
        rv = setup_download( pcfg );
        if ( !rv ) return false;
    }

    // status display
    StatusList.on = pcfg->show_stats;

    // fps display
    fpson = pcfg->fps_allowed;

    // message display
    DisplayMsg_count    = CLIP( pcfg->message_count_req, EGO_MESSAGE_MIN, EGO_MESSAGE_MAX );
    DisplayMsg_on       = pcfg->message_count_req > 0;
    wraptolerance = pcfg->show_stats ? 90 : 32;

    // Get the particle limit
    // if the particle limit has changed, make sure to make not of it
    // number of particles
    tmp_maxparticles = CLIP<Uint16>( pcfg->particle_count_req, 0, MAX_PRT );
    if ( maxparticles != tmp_maxparticles )
    {
        maxparticles = tmp_maxparticles;
        maxparticles_dirty = true;
    }

    // camera options
    _cameraSystem.getCameraOptions().turnMode = pcfg->autoturncamera;

    // sound options
    _audioSystem.setConfiguration(*pcfg);

    // renderer options
    gfx_config_download_from_egoboo_config( &gfx, pcfg );

    // texture options
    oglx_texture_parameters_download_gfx( &tex_params, pcfg );

    return true;
}

//--------------------------------------------------------------------------------------------
bool config_upload( egoboo_config_t * pcfg )
{
    if ( NULL == pcfg ) return false;

    pcfg->autoturncamera = _cameraSystem.getCameraOptions().turnMode;
    pcfg->fps_allowed    = TO_C_BOOL( fpson );

    // number of particles
    pcfg->particle_count_req = CLIP( maxparticles, (size_t)0, (size_t)MAX_PRT );

    // messages
    pcfg->messageon_req     = TO_C_BOOL( DisplayMsg_on );
    pcfg->message_count_req = !DisplayMsg_on ? 0 : std::max( EGO_MESSAGE_MIN, DisplayMsg_count );

    // convert the config values to a setup file
    return setup_upload( pcfg );
}
