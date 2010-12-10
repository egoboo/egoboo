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

/// @file egoboo.c
/// @brief Code for the main program process
/// @details

#define DECLARE_GLOBALS

#include "log.h"
#include "clock.h"
#include "system.h"
#include "graphic.h"
#include "network.h"
#include "sound.h"
#include "profile.inl"
#include "ui.h"
#include "font_bmp.h"
#include "input.h"
#include "game.h"
#include "menu.h"

#include "char.inl"
#include "particle.inl"
#include "enchant.inl"
#include "collision.h"

#include "file_formats/scancode_file.h"
#include "file_formats/treasure_table_file.h"
#include "SDL_extensions.h"

#include "egoboo_fileutil.h"
#include "egoboo_setup.h"
#include "egoboo_vfs.h"
#include "egoboo_console.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

#include <SDL.h>
#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int do_ego_proc_begin( ego_process_t * eproc );
static int do_ego_proc_running( ego_process_t * eproc );
static int do_ego_proc_leaving( ego_process_t * eproc );
static int do_ego_proc_run( ego_process_t * eproc, double frameDuration );

static void memory_cleanUp( void );
static int  ego_init_SDL();
static void console_begin();
static void console_end();

static void object_systems_begin( void );
static void object_systems_end( void );

static void _quit_game( ego_process_t * pgame );

static ego_process_t * ego_process_init( ego_process_t * eproc, int argc, char **argv );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static ClockState_t    * _gclock = NULL;
static ego_process_t     _eproc;

static bool_t  screenshot_keyready  = btrue;

static bool_t _sdl_atexit_registered    = bfalse;
static bool_t _sdl_initialized_base     = bfalse;

static void * _top_con = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_process_t     * EProc   = &_eproc;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int do_ego_proc_begin( ego_process_t * eproc )
{
    // initialize the virtual filesystem first
    vfs_init();
    egoboo_setup_vfs_paths();

    // Initialize logging next, so that we can use it everywhere.
    log_init( vfs_resolveWriteFilename( "/debug/log.txt" ) );
    log_setLoggingLevel( 3 );

    // start initializing the various subsystems
    log_message( "Starting Egoboo " VERSION " ...\n" );
    log_info( "PhysFS file system version %s has been initialized...\n", vfs_getVersion() );

    sys_initialize();
    clk_init();
    _gclock = clk_create( "global clock", -1 );

    // read the "setup.txt" file
    setup_read_vfs();

    // download the "setup.txt" values into the cfg struct
    setup_download( &cfg );

    // do basic system initialization
    ego_init_SDL();
    gfx_system_begin();
    console_begin();
    net_initialize();

    log_info( "Initializing SDL_Image version %d.%d.%d... ", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL );
    GLSetup_SupportedFormats();

    // read all the scantags
    scantag_read_all_vfs( "mp_data/scancode.txt" );

    if ( fs_ensureUserFile( "controls.txt", btrue ) )
    {
        input_settings_load_vfs( "/controls.txt" );
    }

    // synchronoze the config values with the various game subsystems
    // do this acter the ego_init_SDL() and ogl_init() in case the config values are clamped
    // to valid values
    setup_synch( &cfg );

    // initialize the sound system
    sound_initialize();
    load_all_music_sounds_vfs();

    // initialize the random treasure system
    init_random_treasure_tables_vfs( "mp_data/randomtreasure.txt" );

    // make sure that a bunch of stuff gets initialized properly
    object_systems_begin();
    game_module_init( PMod );
    mesh_ctor( PMesh );
    init_all_graphics();
    profile_system_begin();

    // setup the menu system's gui
    ui_begin( vfs_resolveReadFilename( "mp_data/Bo_Chen.ttf" ), 24 );
    font_bmp_load_vfs( "mp_data/font_new_shadow", "mp_data/font.txt" );  // must be done after init_all_graphics()

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
    bool_t menu_valid, game_valid;

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
        SDL_ShowCursor( SDL_ENABLE );
    }
    else
    {
        // in-game settings
        SDL_ShowCursor( cfg.hide_mouse ? SDL_DISABLE : SDL_ENABLE );
        SDL_WM_GrabInput( cfg.grab_mouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
    }

    // Clock updates each frame
    clk_frameStep( _gclock );
    eproc->frameDuration = clk_getFrameDuration( _gclock );

    // read the input values
    input_read();

    if ( pickedmodule_ready && !process_running( PROC_PBASE( MProc ) ) )
    {
        // a new module has been picked

        // reset the flag
        pickedmodule_ready = bfalse;

        // start the game process
        process_start( PROC_PBASE( GProc ) );
    }

    // Test the panic button
    if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
    {
        // terminate the program
        process_kill( PROC_PBASE( eproc ) );
    }

    if ( cfg.dev_mode )
    {
        if ( !SDLKEYDOWN( SDLK_F10 ) )
        {
            single_frame_keyready = btrue;
        }
        else if ( single_frame_keyready && SDLKEYDOWN( SDLK_F10 ) )
        {
            if ( !single_frame_mode )
            {
                single_frame_mode = btrue;
            }

            // request one update and one frame
            single_frame_requested  = btrue;
            single_update_requested = btrue;
            single_frame_keyready   = bfalse;
        }

    }

    // Check for screenshots
    if ( !SDLKEYDOWN( SDLK_F11 ) )
    {
        screenshot_keyready = btrue;
    }
    else if ( screenshot_keyready && SDLKEYDOWN( SDLK_F11 ) && keyb.on )
    {
        screenshot_keyready = bfalse;
        screenshot_requested = btrue;
    }

    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F9 ) && NULL != PMod && PMod->active )
    {
        // super secret "I win" button
        //PMod->beat        = btrue;
        //PMod->exportvalid = btrue;

        CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
        {
            if ( !VALID_PLA( pchr->is_which_player ) )
            {
                kill_character( cnt, ( CHR_REF )511, bfalse );
            }
        }
        CHR_END_LOOP();
    }

    // handle an escape by passing it on to all active sub-processes
    if ( eproc->escape_requested )
    {
        eproc->escape_requested = bfalse;

        // use the escape key to get out of single frame mode
        single_frame_mode = bfalse;

        if ( process_running( PROC_PBASE( GProc ) ) )
        {
            GProc->escape_requested = btrue;
        }

        if ( process_running( PROC_PBASE( MProc ) ) )
        {
            MProc->escape_requested = btrue;
        }
    }

    // run the sub-processes
    do_game_proc_run( GProc, EProc->frameDuration );
    do_menu_proc_run( MProc, EProc->frameDuration );

    // a heads up display that can be used to debug values that are used by both the menu and the game
    // do_game_hud();

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_leaving( ego_process_t * eproc )
{
    if ( !process_validate( PROC_PBASE( eproc ) ) ) return -1;

    // make sure that the
    if ( !GProc->base.terminated )
    {
        do_game_proc_run( GProc, eproc->frameDuration );
    }

    if ( !MProc->base.terminated )
    {
        do_menu_proc_run( MProc, eproc->frameDuration );
    }

    if ( GProc->base.terminated && MProc->base.terminated )
    {
        process_terminate( PROC_PBASE( eproc ) );
    }

    if ( eproc->base.terminated )
    {
        // hopefully this will only happen once
        object_systems_end();
        clk_destroy( &_gclock );
        console_end();
        ui_end();
        gfx_system_end();
        egoboo_clear_vfs_paths();
    }

    return eproc->base.terminated ? 0 : 1;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_run( ego_process_t * eproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_validate( PROC_PBASE( eproc ) ) ) return -1;
    eproc->base.dtime = frameDuration;

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
                eproc->base.killme = bfalse;
            }
            break;

        case proc_finish:
            process_terminate( PROC_PBASE( eproc ) );
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int SDL_main( int argc, char **argv )
{
    /// @details ZZ@> This is where the program starts and all the high level stuff happens

    int result = 0;

    // initialize the process
    ego_process_init( EProc, argc, argv );

    // turn on all basic services
    do_ego_proc_begin( EProc );

    // run the processes
    request_clear_screen();
    while ( !EProc->base.killme && !EProc->base.terminated )
    {
        // put a throttle on the ego process
        EProc->ticks_now = SDL_GetTicks();
        if ( EProc->ticks_now < EProc->ticks_next ) continue;

        // update the timer: 10ms delay between loops
        EProc->ticks_next = EProc->ticks_now + 10;

        // clear the screen if needed
        do_clear_screen();

        do_ego_proc_running( EProc );

        // flip the graphics page if need be
        do_flip_pages();

        // let the OS breathe. It may delay as long as 10ms
        if ( update_lag < 3 )
        {
            SDL_Delay( 1 );
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
void memory_cleanUp( void )
{
    /// @details ZF@> This function releases all loaded things in memory and cleans up everything properly

    log_info( "memory_cleanUp() - Attempting to clean up loaded things in memory... " );

    // quit any existing game
    _quit_game( EProc );

    // synchronoze the config values with the various game subsystems
    setup_synch( &cfg );

    // quit the setup system, making sure that the setup file is written
    setup_upload( &cfg );
    setup_write();
    setup_quit();

    // delete all the graphics allocated by SDL and OpenGL
    delete_all_graphics();

    // make sure that the current control configuration is written
    input_settings_save_vfs( "controls.txt" );

    // shut down the ui
    ui_end();

    // shut down the network
    if ( PNet->on )
    {
        net_shutDown();
    }

    // shut down the clock services
    clk_destroy( &_gclock );
    clk_shutdown();

    // deallocate any dynamically allocated collision memory
    collision_system_end();

    // deallocate any dynamically allocated scripting memory
    scripting_system_end();

    // deallocate all dynamically allocated memory for characters, particles, and enchants
    object_systems_end();

    // clean up any remaining models that might have dynamic data
    MadList_dtor();

    log_message( "Success!\n" );
    log_info( "Exiting Egoboo " VERSION " the good way...\n" );

    // shut down the log services
    log_shutdown();
}

//--------------------------------------------------------------------------------------------
int ego_init_SDL()
{
    ego_init_SDL_base();
    input_init();

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
        _sdl_atexit_registered = bfalse;
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

    _sdl_initialized_base = btrue;
}

//--------------------------------------------------------------------------------------------
void console_begin()
{
    /// @details BB@> initialize the console. This must happen after the screen has been defines,
    ///     otherwise sdl_scr.x == sdl_scr.y == 0 and the screen will be defined to
    ///     have no area...

    SDL_Rect blah = {0, 0, sdl_scr.x, sdl_scr.y / 4};

#if defined(USE_LUA_CONSOLE)
    _top_con = lua_console_create( NULL, blah );
#else
    // without a callback, this console just dumps the input and generates no output
    _top_con = egoboo_console_create( NULL, blah, NULL, NULL );
#endif
}

//--------------------------------------------------------------------------------------------
void console_end()
{
    /// @details BB@> de-initialize the top console

#if defined(USE_LUA_CONSOLE)
    {
        lua_console_t * ptmp = ( lua_console_t* )_top_con;
        lua_console_destroy( &ptmp );
    }
#else
    // without a callback, this console just dumps the input and generates no output
    {
        egoboo_console_t * ptmp = ( egoboo_console_t* )_top_con;
        egoboo_console_destroy( &ptmp, SDL_TRUE );
    }
#endif

    _top_con = NULL;
}

//--------------------------------------------------------------------------------------------
void object_systems_begin( void )
{
    /// @details BB@> initialize all the object systems

    particle_system_begin();
    enchant_system_begin();
    character_system_begin();
}

//--------------------------------------------------------------------------------------------
void object_systems_end( void )
{
    /// @details BB@> quit all the object systems

    particle_system_end();
    enchant_system_end();
    character_system_end();
}

//--------------------------------------------------------------------------------------------
void _quit_game( ego_process_t * pgame )
{
    /// @details ZZ@> This function exits the game entirely

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

    memset( eproc, 0, sizeof( *eproc ) );

    process_init( PROC_PBASE( eproc ) );

    eproc->argv0 = ( argc > 0 ) ? argv[0] : NULL;

    return eproc;
}

//--------------------------------------------------------------------------------------------
void egoboo_clear_vfs_paths()
{
    /// @details BB@> clear out the basic mount points

    vfs_remove_mount_point( "mp_data" );
    vfs_remove_mount_point( "mp_modules" );
    vfs_remove_mount_point( "mp_players" );
    vfs_remove_mount_point( "mp_remote" );
}

//--------------------------------------------------------------------------------------------
void egoboo_setup_vfs_paths()
{
    /// @details BB@> set the basic mount points used by the main program

    //---- tell the vfs to add the basic search paths
    vfs_set_base_search_paths();

    //---- mount all of the default global directories

    // mount the global basicdat directory t the beginning of the list
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat", "mp_data", 1 );

    // Create a mount point for the /user/modules directory
    vfs_add_mount_point( fs_getUserDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /data/modules directory
    vfs_add_mount_point( fs_getDataDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /user/players directory
    vfs_add_mount_point( fs_getUserDirectory(), "players", "mp_players", 1 );

    // Create a mount point for the /data/players directory
    //vfs_add_mount_point( fs_getDataDirectory(), "players", "mp_players", 1 );     //ZF> Let's remove the local players folder since it caused so many problems for people

    // Create a mount point for the /user/remote directory
    vfs_add_mount_point( fs_getUserDirectory(), "import", "mp_import", 1 );

    // Create a mount point for the /user/remote directory
    vfs_add_mount_point( fs_getUserDirectory(), "remote", "mp_remote", 1 );
}

//--------------------------------------------------------------------------------------------
Uint32 egoboo_get_ticks( void )
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
