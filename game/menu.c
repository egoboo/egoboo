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

/// @file menu.c
/// @brief Implements the main menu tree, using the code in Ui.*
/// @details

#include "menu.h"

#include "particle.inl"
#include "mad.h"
#include "char.inl"
#include "profile.inl"

#include "game.h"
#include "quest.h"

#include "controls_file.h"
#include "scancode_file.h"
#include "ui.h"
#include "log.h"
#include "link.h"
#include "game.h"
#include "texture.h"
#include "module_file.h"

// To allow changing settings
#include "sound.h"
#include "input.h"
#include "camera.h"
#include "graphic.h"

#include "egoboo_math.h"
#include "egoboo_vfs.h"
#include "egoboo_typedef.h"
#include "egoboo_fileutil.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"

#include "egoboo.h"

#include "SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The possible states of the menu state machine
enum e_menu_states
{
    MM_Begin,
    MM_Entering,
    MM_Running,
    MM_Leaving,
    MM_Finish
};

#define MENU_STACK_COUNT   256
#define MAXWIDGET          100
#define MENU_MAX_GAMETIPS  100

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// "Slidy" buttons used in some of the menus.  They're shiny.
struct s_SlidyButtonState
{
    char **buttons;
    float lerp;
    int top;
    int left;
};
typedef struct s_SlidyButtonState mnu_SlidyButtonState_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the data to display a chosen player in the load player menu
struct s_ChoosePlayer_element
{
    CAP_REF           cap_ref;  ///< the character profile reference from "data.txt"
    TX_REF            tx_ref;   ///< the index of the icon texture
    int               skin_ref; ///< the index of the object skin from "skin.txt"
    chop_definition_t chop;     ///< the chop data from "naming.txt". put this here so we can generate a name without loading an entire profile
};
typedef struct s_ChoosePlayer_element ChoosePlayer_element_t;

ChoosePlayer_element_t * ChoosePlayer_ctor( ChoosePlayer_element_t * ptr );
ChoosePlayer_element_t * ChoosePlayer_dtor( ChoosePlayer_element_t * ptr );

bool_t ChoosePlayer_init( ChoosePlayer_element_t * ptr );
bool_t ChoosePlayer_dealloc( ChoosePlayer_element_t * ptr );

//--------------------------------------------------------------------------------------------

/// The data that menu.c uses to store the users' choice of players
struct s_ChoosePlayer_list
{
    int                    count;                         ///< the profiles that have been loaded
    ChoosePlayer_element_t lst[MAXIMPORTPERPLAYER + 1];   ///< the profile data
};
typedef struct s_ChoosePlayer_list ChoosePlayer_list_t;

ChoosePlayer_list_t * ChoosePlayer_list_dealloc( ChoosePlayer_list_t * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the module data that the menu system needs
struct s_mnu_module
{
    EGO_PROFILE_STUFF                           ///< the "base class" of a profile obbject

    mod_file_t base;                               ///< the data for the "base class" of the module

    // extended data
    TX_REF tex_index;                              ///< the index of the module's tile image

    STRING vfs_path;                               ///< the virtual pathname of the module
    STRING dest_path;                              ///< the path that module data can be written into
};
typedef struct s_mnu_module mnu_module_t;

#define VALID_MOD_RANGE( IMOD ) ( ((IMOD) >= 0) && ((IMOD) < MAX_MODULE) )
#define VALID_MOD( IMOD )       ( VALID_MOD_RANGE( IMOD ) && IMOD < mnu_ModList.count && mnu_ModList.lst[IMOD].loaded )
#define INVALID_MOD( IMOD )     ( !VALID_MOD_RANGE( IMOD ) || IMOD >= mnu_ModList.count || !mnu_ModList.lst[IMOD].loaded )

INSTANTIATE_STACK_STATIC( mnu_module_t, mnu_ModList, MAX_MODULE );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data that menu.c uses to store the users' choice of players
struct s_GameTips
{
    //These are loaded only once
    Uint8  count;                         //< Number of global tips loaded
    STRING hint[MENU_MAX_GAMETIPS];       //< The global hints/tips

    //These are loaded for every module
    Uint8  local_count;                   //< Number of module specific tips loaded
    STRING local_hint[MENU_MAX_GAMETIPS]; //< Module specific hints and tips
};
typedef struct s_GameTips GameTips_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_SelectedPlayer_element
{
    Uint32  input;
    int     player;
};
typedef struct s_SelectedPlayer_element SelectedPlayer_element_t;

egoboo_rv SelectedPlayer_element_init( SelectedPlayer_element_t * ptr );

//--------------------------------------------------------------------------------------------
struct s_SelectedPlayer_list
{
    int count;
    SelectedPlayer_element_t lst[MAX_PLAYER];
};
typedef struct s_SelectedPlayer_list SelectedPlayer_list_t;

#define SELECTED_PLAYER_LIST_INIT { 0 }

// implementation of the SelectedPlayer_list_t
static egoboo_rv SelectedPlayer_list_init( SelectedPlayer_list_t * sp_lst );
static bool_t    SelectedPlayer_list_check_loadplayer( SelectedPlayer_list_t * sp_lst, int loadplayer_idx );
static bool_t    SelectedPlayer_list_add( SelectedPlayer_list_t * sp_lst, int loadplayer_idx );
static bool_t    SelectedPlayer_list_remove( SelectedPlayer_list_t * sp_lst, int loadplayer_idx );
static bool_t    SelectedPlayer_list_add_input( SelectedPlayer_list_t * sp_lst, int loadplayer_idx, Uint32 input_bits );
static bool_t    SelectedPlayer_list_remove_input( SelectedPlayer_list_t * sp_lst, int loadplayer_idx, Uint32 input_bits );
static int       SelectedPlayer_list_index_from_loadplayer( SelectedPlayer_list_t * sp_lst, int loadplayer_idx );

//--------------------------------------------------------------------------------------------
// declaration of "private" variables
//--------------------------------------------------------------------------------------------

static int          mnu_stack_index = 0;
static which_menu_t mnu_stack[MENU_STACK_COUNT];

static which_menu_t mnu_whichMenu = emnu_Main;

static module_filter_t mnu_moduleFilter = FILTER_OFF;

static ui_Widget_t mnu_widgetList[MAXWIDGET];

static int selectedModule = -1;

/* Copyright text variables.  Change these to change how the copyright text appears */
static const char * copyrightText = "Welcome to Egoboo!\nhttp://egoboo.sourceforge.net\nVersion " VERSION "\n";
static int  copyrightLeft = 0;
static int  copyrightTop  = 0;

/* Options info text variables.  Change these to change how the options text appears */
static const char * tipText = "Put a tip in this box";
static int tipTextLeft = 0;
static int tipTextTop  = 0;

/* Button position for the "easy" menus, like the main one */
static int buttonLeft = 0;
static int buttonTop = 0;

static menu_process_t    _mproc;

static GameTips_t mnu_GameTip = { 0 };

static mnu_SlidyButtonState_t mnu_SlidyButtonState = { NULL };

static SelectedPlayer_list_t mnu_SelectedList = SELECTED_PLAYER_LIST_INIT;

//--------------------------------------------------------------------------------------------
// declaration of public variables
//--------------------------------------------------------------------------------------------

#define TITLE_TEXTURE_COUNT   MAX_MODULE
#define INVALID_TITLE_TEXTURE TITLE_TEXTURE_COUNT

INSTANTIATE_STACK_STATIC( oglx_texture_t, TxTitleImage, TITLE_TEXTURE_COUNT ); // OpenGL title image surfaces

menu_process_t * MProc             = &_mproc;
bool_t           start_new_player  = bfalse;
bool_t           module_list_valid = bfalse;

/* The font used for drawing text.  It's smaller than the button font */
Font *menuFont = NULL;

bool_t mnu_draw_background = btrue;

LoadPlayer_list_t mnu_loadplayer     = LOADPLAYER_LIST_INIT;

//--------------------------------------------------------------------------------------------
// "private" function prototypes
//--------------------------------------------------------------------------------------------

// Implementation of the mnu_stack
static bool_t       mnu_stack_push( which_menu_t menu );
static which_menu_t mnu_stack_pop();
static which_menu_t mnu_stack_peek();
static void         mnu_stack_clear();

// Implementation of the mnu_SlidyButton array
static void mnu_SlidyButton_init( float lerp, const char *button_text[] );
static void mnu_SlidyButton_update_all( float deltaTime );
static void mnu_SlidyButton_draw_all();

// implementation of "private" TxTitleImage functions
static void             TxTitleImage_clear_data();
static void             TxTitleImage_release_one( const TX_REF index );
static void             TxTitleImage_ctor();
static void             TxTitleImage_release_all();
static void             TxTitleImage_dtor();
static oglx_texture_t * TxTitleImage_get_ptr( const TX_REF itex );

// tipText functions
static void tipText_set_position( Font * font, const char * text, int spacing );

// copyrightText functions
static void copyrightText_set_position( Font * font, const char * text, int spacing );

// implementation of "private" ModList functions
static void mnu_ModList_release_images();
void        mnu_ModList_release_all();

// "process" management
static int do_menu_proc_begin( menu_process_t * mproc );
static int do_menu_proc_running( menu_process_t * mproc );
static int do_menu_proc_leaving( menu_process_t * mproc );

// the hint system
static void   mnu_GameTip_load_global_vfs();
static bool_t mnu_GameTip_load_local_vfs();

// "private" module utility
static void   mnu_load_all_module_info();

// "private" asset function
static TX_REF mnu_get_icon_ref( const CAP_REF icap, const TX_REF default_ref );

// implementation of the autoformatting
static void autoformat_init_slidy_buttons();
static void autoformat_init_tip_text();
static void autoformat_init_copyright_text();

// misc other stuff
static void      mnu_release_one_module( const MOD_REF imod );
static void      mnu_load_all_module_images_vfs( LoadPlayer_list_t * lp_lst );
static egoboo_rv mnu_set_local_import_list( Import_list_t * imp_lst, SelectedPlayer_list_t * sp_lst );
static egoboo_rv mnu_set_selected_list( LoadPlayer_list_t * dst, LoadPlayer_list_t * src, SelectedPlayer_list_t * sp_lst );
static egoboo_rv mnu_copy_local_imports( Import_list_t * imp_lst );

//--------------------------------------------------------------------------------------------
// implementation of the menu stack
//--------------------------------------------------------------------------------------------
bool_t mnu_stack_push( which_menu_t menu )
{
    mnu_stack_index = CLIP( mnu_stack_index, 0, MENU_STACK_COUNT ) ;

    if ( mnu_stack_index >= MENU_STACK_COUNT ) return bfalse;

    mnu_stack[mnu_stack_index] = menu;
    mnu_stack_index++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
which_menu_t mnu_stack_pop()
{
    if ( mnu_stack_index < 0 )
    {
        mnu_stack_index = 0;
        return emnu_Main;
    }
    if ( mnu_stack_index > MENU_STACK_COUNT )
    {
        mnu_stack_index = MENU_STACK_COUNT;
    }

    if ( 0 == mnu_stack_index ) return emnu_Main;

    mnu_stack_index--;
    return mnu_stack[mnu_stack_index];
}

//--------------------------------------------------------------------------------------------
which_menu_t mnu_stack_peek()
{
    which_menu_t return_menu = emnu_Main;

    if ( mnu_stack_index > 0 )
    {
        return_menu = mnu_stack[mnu_stack_index-1];
    }

    return return_menu;
}

//--------------------------------------------------------------------------------------------
void mnu_stack_clear()
{
    mnu_stack_index = 0;
    mnu_stack[0] = emnu_Main;
}

//--------------------------------------------------------------------------------------------
// The implementation of the menu process
//--------------------------------------------------------------------------------------------
int do_menu_proc_begin( menu_process_t * mproc )
{
    // reset the fps counter
    menu_fps_clock        = 0;
    menu_fps_loops        = 0;

    stabilized_menu_fps        = TARGET_FPS;
    stabilized_menu_fps_sum    = 0.1f * TARGET_FPS;
    stabilized_menu_fps_weight = 0.1f;

    // play some music
    sound_play_song( MENU_SONG, 0, -1 );

    // initialize all these structures
    menu_system_begin();        // start the menu menu

    // load all module info at menu initialization
    // this will not change unless a new module is downloaded for a network menu?
    mnu_load_all_module_info();

    // initialize the process state
    mproc->base.valid = btrue;

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_running( menu_process_t * mproc )
{
    int menuResult;

    if ( !process_validate( PROC_PBASE( mproc ) ) ) return -1;

    mproc->was_active = mproc->base.valid;

    if ( mproc->base.paused ) return 0;

    // play the menu music
    mnu_draw_background = !process_running( PROC_PBASE( GProc ) );
    menuResult          = game_do_menu( mproc );

    switch ( menuResult )
    {
        case MENU_SELECT:
            // go ahead and start the game
            process_pause( PROC_PBASE( mproc ) );
            break;

        case MENU_QUIT:
            // the user selected "quit"
            process_kill( PROC_PBASE( mproc ) );
            break;
    }

    if ( mnu_get_menu_depth() <= GProc->menu_depth )
    {
        GProc->menu_depth   = -1;
        GProc->escape_latch = bfalse;

        // We have exited the menu and restarted the game
        GProc->mod_paused = bfalse;
        process_pause( PROC_PBASE( MProc ) );
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_leaving( menu_process_t * mproc )
{
    if ( !process_validate( PROC_PBASE( mproc ) ) ) return -1;

    // terminate the menu system
    menu_system_end();

    // finish the menu song
    sound_finish_song( 500 );

    // reset the fps counter
    menu_fps_clock        = 0;
    menu_fps_loops        = 0;

    stabilized_menu_fps        = TARGET_FPS;
    stabilized_menu_fps_sum    = 0.1f * TARGET_FPS;
    stabilized_menu_fps_weight = 0.1f;

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_run( menu_process_t * mproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_validate( PROC_PBASE( mproc ) ) ) return -1;
    mproc->base.dtime = frameDuration;

    if ( mproc->base.paused ) return 0;

    if ( mproc->base.killme )
    {
        mproc->base.state = proc_leaving;
    }

    switch ( mproc->base.state )
    {
        case proc_begin:
            proc_result = do_menu_proc_begin( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            // proc_result = do_menu_proc_entering( mproc );

            mproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = do_menu_proc_running( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = do_menu_proc_leaving( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state  = proc_finish;
                mproc->base.killme = bfalse;
            }
            break;

        case proc_finish:
            process_terminate( PROC_PBASE( mproc ) );
            break;

        default:
        case proc_invalid:
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
menu_process_t * menu_process_init( menu_process_t * mproc )
{
    if ( NULL == mproc ) return NULL;

    memset( mproc, 0, sizeof( *mproc ) );

    process_init( PROC_PBASE( mproc ) );

    return mproc;
}

//--------------------------------------------------------------------------------------------
// Code for global initialization/deinitialization of the menu system
//--------------------------------------------------------------------------------------------
int menu_system_begin()
{
    // initializes the menu system
    //
    // Loads resources for the menus, and figures out where things should
    // be positioned.  If we ever allow changing resolution on the fly, this
    // function will have to be updated/called more than once.

    autoformat_init( &gfx );

    menuFont = ui_loadFont( vfs_resolveReadFilename( "mp_data/Bo_Chen.ttf" ), 18 );
    if ( NULL == menuFont )
    {
        log_error( "Could not load the menu font! (\"mp_data/Bo_Chen.ttf\")\n" );
        return 0;
    }

    // Figure out where to draw the copyright text
    copyrightText_set_position( menuFont, copyrightText, 20 );

    // Figure out where to draw the options text
    tipText_set_position( menuFont, tipText, 20 );

    // construct the TxTitleImage array
    TxTitleImage_ctor();

    // Load game hints
    mnu_GameTip_load_global_vfs();

    return 1;
}

//--------------------------------------------------------------------------------------------
void menu_system_end()
{
    // initializes the menu system
    //
    // Loads resources for the menus, and figures out where things should
    // be positioned.  If we ever allow changing resolution on the fly, this
    // function will have to be updated/called more than once.

    // if this has not been done before yet, do it now
    LoadPlayer_list_dealloc( &mnu_loadplayer );

    if ( NULL != menuFont )
    {
        fnt_freeFont( menuFont );
        menuFont = NULL;
    }

    // destruct the TxTitleImage array
    TxTitleImage_dtor();
}

//--------------------------------------------------------------------------------------------
// Interface for starting and stopping menus
//--------------------------------------------------------------------------------------------
bool_t mnu_begin_menu( which_menu_t which )
{
    if ( !mnu_stack_push( mnu_whichMenu ) ) return bfalse;
    mnu_whichMenu = which;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void mnu_end_menu()
{
    mnu_whichMenu = mnu_stack_pop();
}

//--------------------------------------------------------------------------------------------
int mnu_get_menu_depth()
{
    return mnu_stack_index;
}
//--------------------------------------------------------------------------------------------
// Implementations of the various menus
//--------------------------------------------------------------------------------------------
int doMainMenu( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static oglx_texture_t logo;

    // static float lerp;
    static int menuChoice = 0;
    static SDL_Rect bg_rect, logo_rect;

    /* Button labels.  Defined here for consistency's sake, rather than leaving them as constants */
    static const char *sz_buttons[] =
    {
        "New Game",
        "Load Game",
        "Options",
        "Quit",
        ""
    };

    float fminw = 1, fminh = 1, fmin = 1;
    int result = 0;

    switch ( menuState )
    {
        case MM_Begin:

            menuChoice = 0;
            menuState = MM_Entering;

            //Special xmas theme
            if ( check_time( SEASON_CHRISTMAS ) )
            {
                // load the menu image
                ego_texture_load_vfs( &background, "mp_data/menu/menu_xmas", INVALID_KEY );

                // load the logo image
                ego_texture_load_vfs( &logo,       "mp_data/menu/snowy_logo", INVALID_KEY );
            }

            //Special Halloween theme
            else if ( check_time( SEASON_HALLOWEEN ) )
            {
                // load the menu image
                ego_texture_load_vfs( &background, "mp_data/menu/menu_halloween", INVALID_KEY );

                // load the logo image
                ego_texture_load_vfs( &logo,       "mp_data/menu/creepy_logo", INVALID_KEY );
            }

            //Default egoboo theme
            else
            {
                // load the menu image
                ego_texture_load_vfs( &background, "mp_data/menu/menu_main", INVALID_KEY );

                // load the logo image
                ego_texture_load_vfs( &logo,       "mp_data/menu/menu_logo", INVALID_KEY );
            }

            // calculate the centered position of the background
            fminw = ( float ) MIN( GFX_WIDTH , background.imgW ) / ( float ) background.imgW;
            fminh = ( float ) MIN( GFX_HEIGHT, background.imgH ) / ( float ) background.imgW;
            fmin  = MIN( fminw, fminh );

            bg_rect.w = background.imgW * fmin;
            bg_rect.h = background.imgH * fmin;
            bg_rect.x = ( GFX_WIDTH  - bg_rect.w ) * 0.5f;
            bg_rect.y = ( GFX_HEIGHT - bg_rect.h ) * 0.5f;

            // calculate the position of the logo
            fmin  = MIN( bg_rect.w * 0.5f / logo.imgW, bg_rect.h * 0.5f / logo.imgH );

            logo_rect.x = bg_rect.x;
            logo_rect.y = bg_rect.y;
            logo_rect.w = logo.imgW * fmin;
            logo_rect.h = logo.imgH * fmin;

            mnu_SlidyButton_init( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, bg_rect.x,   bg_rect.y,   bg_rect.w,   bg_rect.h, NULL );
                ui_drawImage( 0, &logo,       logo_rect.x, logo_rect.y, logo_rect.w, logo_rect.h, NULL );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( -deltaTime );

            // Let lerp wind down relative to the time elapsed
            if ( mnu_SlidyButtonState.lerp <= 0.0f )
            {
                menuState = MM_Running;
            }
            break;

        case MM_Running:
            // Do normal run
            // Background

            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, bg_rect.x,   bg_rect.y,   bg_rect.w,   bg_rect.h, NULL );
                ui_drawImage( 0, &logo,       logo_rect.x, logo_rect.y, logo_rect.w, logo_rect.h, NULL );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            // Buttons
            if ( BUTTON_UP == ui_doButton( 1, sz_buttons[0], NULL, buttonLeft, buttonTop, 200, 30 ) )
            {
                // begin single player stuff
                menuChoice = 1;
            }
            if ( BUTTON_UP == ui_doButton( 2, sz_buttons[1], NULL, buttonLeft, buttonTop + 35, 200, 30 ) )
            {
                // begin multi player stuff
                menuChoice = 2;
            }
            if ( BUTTON_UP == ui_doButton( 3, sz_buttons[2], NULL, buttonLeft, buttonTop + 35 * 2, 200, 30 ) )
            {
                // go to options menu
                menuChoice = 3;
            }
            if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 4, sz_buttons[3], NULL, buttonLeft, buttonTop + 35 * 3, 200, 30 ) )
            {
                // quit game
                menuChoice = 4;
            }
            if ( menuChoice != 0 )
            {
                menuState = MM_Leaving;
                mnu_SlidyButton_init( 0.0f, sz_buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, bg_rect.x,   bg_rect.y,   bg_rect.w,   bg_rect.h, NULL );
                ui_drawImage( 0, &logo,       logo_rect.x, logo_rect.y, logo_rect.w, logo_rect.h, NULL );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            // Buttons
            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( deltaTime );
            if ( mnu_SlidyButtonState.lerp >= 1.0f )
            {
                menuState = MM_Finish;
            }
            break;

        case MM_Finish:
            // Free the background texture; don't need to hold onto it
            oglx_texture_Release( &background );
            menuState = MM_Begin;  // Make sure this all resets next time

            // reset the ui
            ui_Reset();

            // Set the next menu to load
            result = menuChoice;
            break;
    };

    return result;
}

//--------------------------------------------------------------------------------------------
int doSinglePlayerMenu( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int menuChoice;

    static const char *sz_buttons[] =
    {
        "New Player",
        "Load Saved Player",
        "Back",
        ""
    };

    int result = 0;

    switch ( menuState )
    {
        case MM_Begin:
            // Load resources for this menu
            ego_texture_load_vfs( &background, "mp_data/menu/menu_advent", TRANSCOLOR );
            menuChoice = 0;

            menuState = MM_Entering;

            mnu_SlidyButton_init( 1.0f, sz_buttons );

            // Let this fall through

        case MM_Entering:
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            // Draw the background image
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0, NULL );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( -deltaTime );
            if ( mnu_SlidyButtonState.lerp <= 0.0f )
                menuState = MM_Running;

            break;

        case MM_Running:

            // Draw the background image
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0, NULL );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            // Buttons
            if ( BUTTON_UP == ui_doButton( 1, sz_buttons[0], NULL, buttonLeft, buttonTop, 200, 30 ) )
            {
                menuChoice = 1;
            }
            if ( BUTTON_UP == ui_doButton( 2, sz_buttons[1], NULL, buttonLeft, buttonTop + 35, 200, 30 ) )
            {
                menuChoice = 2;
            }
            if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 3, sz_buttons[2], NULL, buttonLeft, buttonTop + 35 * 2, 200, 30 ) )
            {
                menuChoice = 3;     //back
            }
            if ( menuChoice != 0 )
            {
                menuState = MM_Leaving;
                mnu_SlidyButton_init( 0.0f, sz_buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0, NULL );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( deltaTime );
            if ( mnu_SlidyButtonState.lerp >= 1.0f )
            {
                menuState = MM_Finish;
            }
            break;

        case MM_Finish:
            // Release the background texture
            oglx_texture_Release( &background );

            // reset the ui
            ui_Reset();

            // Set the next menu to load
            result = menuChoice;

            // And make sure that if we come back to this menu, it resets
            // properly
            menuState = MM_Begin;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
static int cmp_mod_ref_mult = 1;

int cmp_mod_ref( const void * vref1, const void * vref2 )
{
    /// @details BB@> Sort MOD REF values based on the rank of the module that they point to.
    ///               Trap all stupid values.

    MOD_REF * pref1 = ( MOD_REF * )vref1;
    MOD_REF * pref2 = ( MOD_REF * )vref2;

    int retval = 0;

    if ( NULL == pref1 && NULL == pref2 )
    {
        return 0;
    }
    else if ( NULL == pref1 )
    {
        return 1;
    }
    else if ( NULL == pref2 )
    {
        return -1;
    }

    if ( *pref1 > mnu_ModList.count && *pref2 > mnu_ModList.count )
    {
        return 0;
    }
    else if ( *pref1 > mnu_ModList.count )
    {
        return 1;
    }
    else if ( *pref2 > mnu_ModList.count )
    {
        return -1;
    }

    // if they are beaten, float them to the end of the list
    retval = ( int )mnu_ModList.lst[*pref1].base.beaten - ( int )mnu_ModList.lst[*pref2].base.beaten;

    if ( 0 == retval )
    {
        // I want to uot the "newest" == "hardest" modules at the front, but this should be opposite for
        // beginner modules
        retval = cmp_mod_ref_mult * strncmp( mnu_ModList.lst[*pref1].base.rank, mnu_ModList.lst[*pref2].base.rank, RANKSIZE );
    }

    if ( 0 == retval )
    {
        retval = strncmp( mnu_ModList.lst[*pref1].base.longname, mnu_ModList.lst[*pref2].base.longname, sizeof( STRING ) );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int doChooseModule( float deltaTime )
{
    /// @details Choose the module

    static oglx_texture_t background;
    static int menuState = MM_Begin;
    static Uint8 keycooldown;
    static char* filterText = "All Modules";

    static int validModules_count;
    static MOD_REF validModules[MAX_MODULE];

    static int moduleMenuOffsetX;
    static int moduleMenuOffsetY;

    static int startIndex = 0;
    static int int_module = -1;
    static int ext_module = -1;

    // keep a local list of the currently selected players
    static LoadPlayer_list_t loc_selectedplayer = LOADPLAYER_LIST_INIT;

    int result = 0;
    int i, x, y;
    MOD_REF imod;

    switch ( menuState )
    {
        case MM_Begin:

            // copy over the selected players from mnu_SelectedList to loc_selectedplayer
            mnu_set_selected_list( &loc_selectedplayer, &mnu_loadplayer, &mnu_SelectedList );

            // reload the module data, if necessary
            if ( !module_list_valid )
            {
                mnu_load_all_module_info();
            }

            // Reload all modules, something might be unlocked
            mnu_load_all_module_images_vfs( &loc_selectedplayer );

            // Reset which module we are selecting
            startIndex = 0;
            ext_module = int_module = -1;

            // reset the global module selection index
            selectedModule = -1;

            // blank out the valid modules
            validModules_count = 0;
            for ( i = 0; i < MAX_MODULE; i++ )
            {
                memset( validModules + i, 0, sizeof( MOD_REF ) );
            }

            // Figure out at what offset we want to draw the module menu.
            moduleMenuOffsetX = ( GFX_WIDTH  - 640 ) / 2;
            moduleMenuOffsetX = MAX( 0, moduleMenuOffsetX );

            moduleMenuOffsetY = ( GFX_HEIGHT - 480 ) / 2;
            moduleMenuOffsetY = MAX( 0, moduleMenuOffsetY );

            menuState = MM_Entering;

            // fall through...

        case MM_Entering:
            menuState = MM_Running;

            if ( !module_list_valid )
            {
                mnu_load_all_module_info();
                mnu_load_all_module_images_vfs( &loc_selectedplayer );
            }

            // Find the modules that we want to allow loading for.  If start_new_player
            // is true, we want ones that don't allow imports (e.g. starter modules).
            // Otherwise, we want modules that allow imports
            validModules_count = 0;
            for ( imod = 0; imod < mnu_ModList.count; imod++ )
            {
                // if this module is not valid given the game options and the
                // selected players, skip it
                if ( !mnu_test_module_by_index( &loc_selectedplayer, imod, 0, NULL ) ) continue;

                if ( start_new_player && 0 == mnu_ModList.lst[imod].base.importamount )
                {
                    // starter module
                    validModules[validModules_count] = REF_TO_INT( imod );
                    validModules_count++;
                }
                else
                {
                    if ( FILTER_OFF != mnu_moduleFilter && mnu_ModList.lst[imod].base.moduletype != mnu_moduleFilter ) continue;
                    if ( mnu_SelectedList.count > mnu_ModList.lst[imod].base.importamount ) continue;
                    if ( mnu_SelectedList.count < mnu_ModList.lst[imod].base.minplayers ) continue;
                    if ( mnu_SelectedList.count > mnu_ModList.lst[imod].base.maxplayers ) continue;

                    // regular module
                    validModules[validModules_count] = REF_TO_INT( imod );
                    validModules_count++;
                }
            }

            // sort the modules by difficulty. easiest to hardeest for starting a new character
            // hardest to easiest for loading a module
            cmp_mod_ref_mult = start_new_player ? 1 : -1;
            qsort( validModules, validModules_count, sizeof( MOD_REF ), cmp_mod_ref );

            // load background depending on current filter
            if ( start_new_player )
            {
                ego_texture_load_vfs( &background, "mp_data/menu/menu_advent", TRANSCOLOR );
            }
            else switch ( mnu_moduleFilter )
                {
                    case FILTER_MAIN: ego_texture_load_vfs( &background, "mp_data/menu/menu_draco", TRANSCOLOR ); break;
                    case FILTER_SIDE: ego_texture_load_vfs( &background, "mp_data/menu/menu_sidequest", TRANSCOLOR ); break;
                    case FILTER_TOWN: ego_texture_load_vfs( &background, "mp_data/menu/menu_town", TRANSCOLOR ); break;
                    case FILTER_FUN:  ego_texture_load_vfs( &background, "mp_data/menu/menu_funquest", TRANSCOLOR ); break;

                    default:
                    case FILTER_OFF: ego_texture_load_vfs( &background, "mp_data/menu/menu_allquest", TRANSCOLOR ); break;
                }

            // set the tip text
            if ( 0 == validModules_count )
            {
                tipText_set_position( menuFont, "Sorry, there are no valid games!\n Please press the \"Back\" button.", 20 );
            }
            else if ( validModules_count <= 3 )
            {
                tipText_set_position( menuFont, "Press an icon to select a game.", 20 );
            }
            else
            {
                tipText_set_position( menuFont, "Press an icon to select a game.\nUse the mouse wheel or the \"<-\" and \"->\" buttons to scroll.", 20 );
            }

            // fall through for now...

        case MM_Running:
            {
                GLXvector4f beat_tint = { 0.5f, 0.25f, 0.25f, 1.0f };
                GLXvector4f normal_tint = { 1.0f, 1.0f, 1.0f, 1.0f };

                if ( !module_list_valid )
                {
                    mnu_load_all_module_info();
                    mnu_load_all_module_images_vfs( &loc_selectedplayer );
                }

                // Draw the background
                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
                x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
                y = GFX_HEIGHT - background.imgH;

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, x, y, 0, 0, NULL );
                }

                // use the mouse wheel to scan the modules
                if ( cursor_wheel_event_pending() )
                {
                    if ( cursor.z > 0 )
                    {
                        startIndex++;
                    }
                    else if ( cursor.z < 0 )
                    {
                        startIndex--;
                    }

                    cursor_finish_wheel_event();
                }

                //Allow arrow keys to scroll as well
                if ( SDLKEYDOWN( SDLK_RIGHT ) )
                {
                    if ( 0 == keycooldown )
                    {
                        startIndex++;
                        keycooldown = 5;
                    }
                }
                else if ( SDLKEYDOWN( SDLK_LEFT ) )
                {
                    if ( 0 == keycooldown )
                    {
                        startIndex--;
                        keycooldown = 5;
                    }
                }
                else keycooldown = 0;
                if ( keycooldown > 0 ) keycooldown--;

                // Draw the arrows to pick modules
                if ( validModules_count > 3 )
                {
                    if ( BUTTON_UP == ui_doButton( 1051, "<-", NULL, moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, 30, 30 ) )
                    {
                        startIndex--;
                    }
                    if ( BUTTON_UP == ui_doButton( 1052, "->", NULL, moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30 ) )
                    {
                        startIndex++;
                    }
                }

                // restrict the range to valid values
                startIndex = CLIP( startIndex, 0, validModules_count - 3 );

                // Draw buttons for the modules that can be selected
                x = 93;
                y = 20;
                for ( i = startIndex; i < MIN( startIndex + 3, validModules_count ); i++ )
                {
                    // fix the menu images in case one or more of them are undefined
                    MOD_REF          imod       = validModules[i];
                    TX_REF           tex_offset = mnu_ModList.lst[imod].tex_index;
                    oglx_texture_t * ptex       = TxTitleImage_get_ptr( tex_offset );

                    GLfloat * img_tint = normal_tint;

                    if ( mnu_ModList.lst[imod].base.beaten )
                    {
                        img_tint = beat_tint;
                    }

                    if ( ui_doImageButton( i, ptex, moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138, img_tint ) )
                    {
                        int_module = i;
                        ext_module = (( int_module < 0 ) || ( int_module > validModules_count ) ) ? -1 : validModules[int_module];
                    }

                    //Draw a text over the image explaining what it means
                    if ( mnu_ModList.lst[imod].base.beaten )
                    {
                        ui_drawTextBox( NULL, "BEATEN", moduleMenuOffsetX + x + 32, moduleMenuOffsetY + y + 64, 64, 30, 20 );
                    }

                    x += 138 + 20;  // Width of the button, and the spacing between buttons
                }

                // Draw an empty button as the backdrop for the module text
                ui_drawButton( UI_Nothing, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 250, NULL );

                // Draw the text description of the selected module
                if ( ext_module > -1 && ext_module < mnu_ModList.count )
                {
                    char    buffer[1024]  = EMPTY_CSTR;
                    const char * rank_string, * name_string;
                    char  * carat = buffer, * carat_end = buffer + SDL_arraysize( buffer );

                    mod_file_t * pmod = &( mnu_ModList.lst[ext_module].base );

                    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

                    name_string = "Unnamed";
                    if ( CSTR_END != pmod->longname[0] )
                    {
                        name_string = pmod->longname;
                    }
                    carat += snprintf( carat, carat_end - carat - 1, "%s\n", name_string );

                    rank_string = "Unranked";
                    if ( CSTR_END != pmod->rank[0] )
                    {
                        rank_string = pmod->rank;
                    }
                    carat += snprintf( carat, carat_end - carat - 1, "Difficulty: %s\n", rank_string );

                    if ( pmod->maxplayers > 1 )
                    {
                        if ( pmod->minplayers == pmod->maxplayers )
                        {
                            carat += snprintf( carat, carat_end - carat - 1, "%d Players\n", pmod->minplayers );
                        }
                        else
                        {
                            carat += snprintf( carat, carat_end - carat - 1, "%d - %d Players\n", pmod->minplayers, pmod->maxplayers );
                        }
                    }
                    else
                    {
                        if ( 0 != pmod->importamount )
                        {
                            carat += snprintf( carat, carat_end - carat - 1, "Single Player\n" );
                        }
                        else
                        {
                            carat += snprintf( carat, carat_end - carat - 1, "Starter Module\n" );
                        }
                    }
                    carat += snprintf( carat, carat_end - carat - 1, " \n" );

                    for ( i = 0; i < SUMMARYLINES; i++ )
                    {
                        carat += snprintf( carat, carat_end - carat - 1, "%s\n", pmod->summary[i] );
                    }

                    // Draw a text box
                    ui_drawTextBox( menuFont, buffer, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230, 20 );
                }

                // And draw the next & back buttons
                if ( ext_module > -1 )
                {
                    if ( SDLKEYDOWN( SDLK_RETURN ) || BUTTON_UP == ui_doButton( 53, "Select Module", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30 ) )
                    {
                        // go to the next menu with this module selected
                        menuState = MM_Leaving;
                    }
                }

                if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 54, "Back", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30 ) )
                {
                    // Signal doMenu to go back to the previous menu
                    int_module = -1;
                    ext_module = -1;
                    menuState = MM_Leaving;
                }

                //Do the module filter button
                if ( !start_new_player )
                {
                    bool_t click_button;

                    // unly display the filter name
                    ui_doButton( 55, filterText, NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 390, 200, 30 );

                    // use the ">" button to change since we are already using arrows to indicate "spin control"-like widgets
                    click_button = ( BUTTON_UP == ui_doButton( 56, ">", NULL, moduleMenuOffsetX + 532, moduleMenuOffsetY + 390, 30, 30 ) );

                    if ( click_button )
                    {
                        //Reload the modules with the new filter
                        menuState = MM_Entering;

                        //Swap to the next filter
                        mnu_moduleFilter = CLIP( mnu_moduleFilter, FILTER_NORMAL_BEGIN, FILTER_NORMAL_END );

                        mnu_moduleFilter = ( module_filter_t )( mnu_moduleFilter + 1 );

                        if ( mnu_moduleFilter > FILTER_NORMAL_END ) mnu_moduleFilter = FILTER_NORMAL_BEGIN;

                        switch ( mnu_moduleFilter )
                        {
                            case FILTER_MAIN:    filterText = "Main Quest";       break;
                            case FILTER_SIDE:    filterText = "Sidequests";       break;
                            case FILTER_TOWN:    filterText = "Towns and Cities"; break;
                            case FILTER_FUN:     filterText = "Fun Modules";      break;
                            case FILTER_STARTER: filterText = "Starter Modules";  break;
                        default: case FILTER_OFF:     filterText = "All Modules";      break;
                        }
                    }
                }

                // the tool-tip text
                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
                ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );
            }
            break;

        case MM_Leaving:
            menuState = MM_Finish;
            // fall through for now

        case MM_Finish:
            oglx_texture_Release( &background );

            pickedmodule_index         = -1;
            pickedmodule_path[0]       = CSTR_END;
            pickedmodule_name[0]       = CSTR_END;
            pickedmodule_write_path[0] = CSTR_END;

            menuState = MM_Begin;
            if ( -1 == ext_module )
            {
                result = -1;
            }
            else
            {
                // Save the name of the module that we've picked
                pickedmodule_index = ext_module;

                strncpy( pickedmodule_path,       mnu_ModList_get_vfs_path( pickedmodule_index ), SDL_arraysize( pickedmodule_path ) );
                strncpy( pickedmodule_name,       mnu_ModList_get_name( pickedmodule_index ), SDL_arraysize( pickedmodule_name ) );
                strncpy( pickedmodule_write_path, mnu_ModList_get_dest_path( pickedmodule_index ), SDL_arraysize( pickedmodule_write_path ) );

                if ( !game_choose_module( ext_module, -1 ) )
                {
                    log_warning( "Tried to select an invalid module. index == %d\n", ext_module );
                    result = -1;
                }
                else
                {
                    pickedmodule_ready = btrue;
                    result = ( PMod->importamount > 0 ) ? 1 : 2;
                }
            }

            // just blank out the data since we do not own it
            LoadPlayer_list_init( &loc_selectedplayer );

            // post the selected module
            selectedModule = ext_module;

            // reset the ui
            ui_Reset();

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
bool_t doChoosePlayer_load_profiles( LoadPlayer_element_t * loadplayer_ptr, ChoosePlayer_list_t * chooseplayer )
{
    int    i;
    STRING  szFilename;

    CAP_REF   cap_ref;
    cap_t   * cap_ptr = NULL;

    ChoosePlayer_element_t * chooseplayer_ptr;

    // free any allocated data
    ChoosePlayer_list_dealloc( chooseplayer );

    if ( NULL == loadplayer_ptr ) return bfalse;

    // grab the loadplayer_idx data
    if ( !LOADED_CAP( loadplayer_ptr->cap_ref ) )
    {
        return bfalse;
    }
    cap_ptr = CapStack.lst + loadplayer_ptr->cap_ref;

    // go to the next element in the list
    chooseplayer_ptr = chooseplayer->lst + chooseplayer->count;
    chooseplayer->count++;

    // blank out the data
    ChoosePlayer_ctor( chooseplayer_ptr );

    // set the index of this object
    chooseplayer_ptr->cap_ref = loadplayer_ptr->cap_ref;

    // copy the skin
    chooseplayer_ptr->skin_ref = loadplayer_ptr->skin_ref;
    chooseplayer_ptr->skin_ref = CLIP( chooseplayer_ptr->skin_ref, 0, MAX_SKIN - 1 );

    // copy the texture reference over
    chooseplayer_ptr->tx_ref = loadplayer_ptr->tx_ref;

    // copy the chop info
    memmove( &( chooseplayer_ptr->chop ), &( loadplayer_ptr->chop ), sizeof( chooseplayer_ptr->chop ) );

    // make sure the book data is loaded
    if ( 0 == bookicon_count )
    {
        load_one_profile_vfs( "mp_data/globalobjects/book.obj", SPELLBOOK );
    }

    // grab the inventory data
    for ( i = 0; i < MAXIMPORTOBJECTS; i++ )
    {
        int slot = i + 1;

        snprintf( szFilename, SDL_arraysize( szFilename ), "%s/%d.obj", loadplayer_ptr->dir, i );

        // load the profile
        cap_ref = load_one_character_profile_vfs( szFilename, slot, bfalse );
        if ( LOADED_CAP( cap_ref ) )
        {
            cap_ptr = CapStack.lst + cap_ref;

            // go to the next element in the list
            chooseplayer_ptr = chooseplayer->lst + chooseplayer->count;
            chooseplayer->count++;

            // sace the cap reference
            chooseplayer_ptr->cap_ref = cap_ref;

            // get the skin info
            chooseplayer_ptr->skin_ref = cap_ptr->skin_override % MAX_SKIN;
            chooseplayer_ptr->skin_ref = CLIP( chooseplayer_ptr->skin_ref, 0, MAX_SKIN - 1 );

            // load the icon of the skin
            snprintf( szFilename, SDL_arraysize( szFilename ), "%s/%d.obj/icon%d", loadplayer_ptr->dir, i, chooseplayer_ptr->skin_ref );
            chooseplayer_ptr->tx_ref = TxTexture_load_one_vfs( szFilename, ( TX_REF )INVALID_TX_TEXTURE, INVALID_KEY );

            // load the chop info from "naming.txt"
            snprintf( szFilename, SDL_arraysize( szFilename ), "%s/%d.obj/naming.txt", loadplayer_ptr->dir, i );
            chop_load_vfs( &chop_mem, szFilename, &( chooseplayer_ptr->chop ) );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t doChoosePlayer_show_stats( LoadPlayer_element_t * loadplayer_ptr, int mode, const int x, const int y, const int width, const int height )
{
    int i, x1, y1;

    static ChoosePlayer_list_t objects = { 0 };

    if ( NULL == loadplayer_ptr ) mode = 1;

    // handle the profile data
    switch ( mode )
    {
        case 0: // load new loadplayer data

            if ( !doChoosePlayer_load_profiles( loadplayer_ptr, &objects ) )
            {
                loadplayer_ptr = NULL;
            }
            break;

        case 1: // unload loadplayer data

            // release all of the temporary profiles
            ChoosePlayer_list_dealloc( &objects );

            loadplayer_ptr = NULL;

            break;
    }

    // do the actual display
    x1 = x + 25;
    y1 = y + 10;
    if ( NULL != loadplayer_ptr && objects.count > 0 )
    {
        CAP_REF icap = objects.lst[0].cap_ref;

        if ( LOADED_CAP( icap ) )
        {
            STRING temp_string;
            cap_t * pcap = CapStack.lst + icap;
            Uint8 skin = MAX( pcap->skin_override, 0 );

            ui_drawButton( UI_Nothing, x, y, width, height, NULL );

            // fix class name capitalization
            pcap->classname[0] = toupper( pcap->classname[0] );

            //Character level and class
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
            snprintf( temp_string, SDL_arraysize( temp_string ), "A level %d %s", pcap->level_override + 1, pcap->classname );
            ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 20;

            // Armor
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
            snprintf( temp_string, SDL_arraysize( temp_string ), "Wearing %s %s", pcap->skinname[skin], HAS_SOME_BITS( pcap->skindressy, 1 << skin ) ? "(Light)" : "(Heavy)" );
            ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 30;

            // Life and mana (can be less than maximum if not in easy mode)
            if ( cfg.difficulty >= GAME_NORMAL )
            {
                snprintf( temp_string, SDL_arraysize( temp_string ), "Life: %d/%d", MIN( FP8_TO_INT( pcap->life_spawn ), ( int )pcap->life_stat.val.from ), ( int )pcap->life_stat.val.from );
                ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 20;

                y1 = ui_drawBar( 0, x1, y1, FP8_TO_INT( pcap->life_spawn ), ( int )pcap->life_stat.val.from, pcap->lifecolor );

                if ( pcap->mana_stat.val.from > 0 )
                {
                    snprintf( temp_string, SDL_arraysize( temp_string ), "Mana: %d/%d", MIN( FP8_TO_INT( pcap->mana_spawn ), ( int )pcap->mana_stat.val.from ), ( int )pcap->mana_stat.val.from );
                    ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 20;

                    y1 = ui_drawBar( 0, x1, y1, FP8_TO_INT( pcap->mana_spawn ), ( int )pcap->mana_stat.val.from, pcap->manacolor );
                }
            }
            else
            {
                snprintf( temp_string, SDL_arraysize( temp_string ), "Life: %d", ( int )pcap->life_stat.val.from );
                ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 20;

                y1 = ui_drawBar( 0, x1, y1, ( int )pcap->life_stat.val.from, ( int )pcap->life_stat.val.from, pcap->lifecolor );

                if ( pcap->mana_stat.val.from > 0 )
                {
                    snprintf( temp_string, SDL_arraysize( temp_string ), "Mana: %d", ( int )pcap->mana_stat.val.from );
                    ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 20;

                    y1 = ui_drawBar( 0, x1, y1, ( int )pcap->mana_stat.val.from, ( int )pcap->mana_stat.val.from, pcap->manacolor );
                }
            }
            y1 += 10;

            //SWID
            ui_drawTextBox( menuFont, "Stats", x1, y1, 0, 0, 20 ); y1 += 20;

            snprintf( temp_string, SDL_arraysize( temp_string ), "  Str: %s (%d)", describe_value( pcap->strength_stat.val.from,     60, NULL ), ( int )pcap->strength_stat.val.from );
            ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 20;

            snprintf( temp_string, SDL_arraysize( temp_string ), "  Wis: %s (%d)", describe_value( pcap->wisdom_stat.val.from,       60, NULL ), ( int )pcap->wisdom_stat.val.from );
            ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 20;

            snprintf( temp_string, SDL_arraysize( temp_string ), "  Int: %s (%d)", describe_value( pcap->intelligence_stat.val.from, 60, NULL ), ( int )pcap->intelligence_stat.val.from );
            ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 20;

            snprintf( temp_string, SDL_arraysize( temp_string ), "  Dex: %s (%d)", describe_value( pcap->dexterity_stat.val.from,    60, NULL ), ( int )pcap->dexterity_stat.val.from );
            ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, 20 ); y1 += 30;

            //Inventory
            if ( objects.count > 1 )
            {
                ChoosePlayer_element_t * chooseplayer_ptr;

                ui_drawTextBox( menuFont, "Inventory", x1, y1, 0, 0, 20 ); y1 += 20;

                for ( i = 1; i < objects.count; i++ )
                {
                    chooseplayer_ptr = objects.lst + i;

                    icap = chooseplayer_ptr->cap_ref;
                    if ( LOADED_CAP( icap ) )
                    {
                        TX_REF  icon_ref;
                        cap_t * pcap = CapStack.lst + icap;

                        STRING itemname;
                        if ( pcap->nameknown ) strncpy( itemname, chop_create( &chop_mem, &( chooseplayer_ptr->chop ) ), SDL_arraysize( itemname ) );
                        else                   strncpy( itemname, pcap->classname,   SDL_arraysize( itemname ) );

                        //draw the icon for this item
                        icon_ref = mnu_get_icon_ref( icap, chooseplayer_ptr->tx_ref );
                        ui_drawImage( 0, TxTexture_get_ptr( icon_ref ), x1, y1, 32, 32, NULL );

                        if ( icap == SLOT_LEFT + 1 )
                        {
                            snprintf( temp_string, SDL_arraysize( temp_string ), "  Left: %s", itemname );
                            ui_drawTextBox( menuFont, temp_string, x1 + 32, y1 + 6, 0, 0, 20 ); y1 += 32;
                        }
                        else if ( icap == SLOT_RIGHT + 1 )
                        {
                            snprintf( temp_string, SDL_arraysize( temp_string ), "  Right: %s", itemname );
                            ui_drawTextBox( menuFont, temp_string, x1 + 32, y1 + 6, 0, 0, 20 ); y1 += 32;
                        }
                        else
                        {
                            snprintf( temp_string, SDL_arraysize( temp_string ), "  Item: %s", itemname );
                            ui_drawTextBox( menuFont, temp_string, x1 + 32, y1 + 6, 0, 0, 20 ); y1 += 32;
                        }
                    }
                }
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int doChoosePlayer( float deltaTime )
{
    const int x0 = 20, y0 = 20, icon_size = 42, text_width = 175, button_repeat = 47;

    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int    startIndex = 0;
    static int    last_player = -1;
    static bool_t new_player = bfalse;
    static int numVertical, numHorizontal;
    static Uint32 BitsInput[4];
    static bool_t device_on[4];
    static const char * button_text[] = { "N/A", "Back", ""};

    int result = 0;
    int i, j, x, y;

    switch ( menuState )
    {
        case MM_Begin:
            TxTexture_free_one(( TX_REF )TX_BARS );

            SelectedPlayer_list_init( &mnu_SelectedList );

            TxTexture_load_one_vfs( "mp_data/nullicon", ( TX_REF )ICON_NULL, INVALID_KEY );

            TxTexture_load_one_vfs( "mp_data/keybicon", ( TX_REF )ICON_KEYB, INVALID_KEY );
            BitsInput[0] = INPUT_BITS_KEYBOARD;
            device_on[0] = keyb.on;

            TxTexture_load_one_vfs( "mp_data/mousicon", ( TX_REF )ICON_MOUS, INVALID_KEY );
            BitsInput[1] = INPUT_BITS_MOUSE;
            device_on[1] = mous.on;

            TxTexture_load_one_vfs( "mp_data/joyaicon", ( TX_REF )ICON_JOYA, INVALID_KEY );
            BitsInput[2] = INPUT_BITS_JOYA;
            device_on[2] = joy[0].on;

            TxTexture_load_one_vfs( "mp_data/joybicon", ( TX_REF )ICON_JOYB, INVALID_KEY );
            BitsInput[3] = INPUT_BITS_JOYB;
            device_on[3] = joy[1].on;

            ego_texture_load_vfs( &background, "mp_data/menu/menu_sleepy", TRANSCOLOR );

            TxTexture_load_one_vfs( "mp_data/bars", ( TX_REF )TX_BARS, INVALID_KEY );

            // load information for all the players that could be imported
            LoadPlayer_list_import_all( &mnu_loadplayer, "mp_players", btrue );

            // reset button 0, or it will mess up the menu.
            // must do it before mnu_SlidyButton_init()
            button_text[0] = "N/A";

            mnu_SlidyButton_init( 1.0f, button_text );

            numVertical   = ( buttonTop - y0 ) / button_repeat - 1;
            numHorizontal = 1;

            x = x0;
            y = y0;
            for ( i = 0; i < numVertical; i++ )
            {
                int m = i * 5;

                ui_initWidget( mnu_widgetList + m, m, NULL, NULL, NULL, x, y, text_width, icon_size );
                ui_widgetAddMask( mnu_widgetList + m, UI_BITS_CLICKED );

                for ( j = 0, m++; j < 4; j++, m++ )
                {
                    ui_initWidget( mnu_widgetList + m, m, menuFont, NULL, TxTexture_get_ptr(( TX_REF )( ICON_KEYB + j ) ), x + text_width + j*icon_size, y, icon_size, icon_size );
                    ui_widgetAddMask( mnu_widgetList + m, UI_BITS_CLICKED );
                };

                y += button_repeat;
            };

            if ( mnu_loadplayer.count < 10 )
            {
                tipText_set_position( menuFont, "Choose an input device to select your player(s)", 20 );
            }
            else
            {
                tipText_set_position( menuFont, "Choose an input device to select your player(s)\nUse the mouse wheel to scroll.", 20 );
            }

            menuState = MM_Entering;
            // fall through

        case MM_Entering:

            /*GL_DEBUG(glColor4f)(1, 1, 1, 1 - mnu_SlidyButtonState.lerp );
            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( -deltaTime );
            // Let lerp wind down relative to the time elapsed
            if ( mnu_SlidyButtonState.lerp <= 0.0f )
            {
                menuState = MM_Running;
            }*/

            // Simply fall through
            // menuState = MM_Running;
            // break;

        case MM_Running:
            // Figure out how many players we can show without scrolling

            if ( 0 == mnu_SelectedList.count )
            {
                button_text[0] = "";
            }
            else if ( 1 == mnu_SelectedList.count )
            {
                button_text[0] = "Select Player";
            }
            else
            {
                button_text[0] = "Select Players";
            }

            // Draw the background
            x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
            y = GFX_HEIGHT - background.imgH;

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, x, y, 0, 0, NULL );
            }

            // use the mouse wheel to scan the characters
            if ( cursor_wheel_event_pending() )
            {
                if ( cursor.z > 0 )
                {
                    if ( startIndex + numVertical < mnu_loadplayer.count )
                    {
                        startIndex++;
                    }
                }
                else if ( cursor.z < 0 )
                {
                    if ( startIndex > 0 )
                    {
                        startIndex--;
                    }
                }

                cursor_finish_wheel_event();
            }

            // Draw the lplayer selection buttons
            x = x0;
            y = y0;
            for ( i = 0; i < numVertical; i++ )
            {
                int lplayer;
                int splayer;
                LoadPlayer_element_t * loadplayer_ptr = NULL;
                int m = i * 5;

                lplayer = i + startIndex;
                if ( !VALID_LOADPLAYER_IDX( mnu_loadplayer, lplayer ) ) continue;

                loadplayer_ptr = mnu_loadplayer.lst + lplayer;
                splayer        = SelectedPlayer_list_index_from_loadplayer( &mnu_SelectedList,  lplayer );

                // do the character button
                mnu_widgetList[m].img  = TxTexture_get_ptr( loadplayer_ptr->tx_ref );
                mnu_widgetList[m].text = loadplayer_ptr->name;
                if ( INVALID_PLAYER != splayer )
                {
                    SET_BIT( mnu_widgetList[m].state, UI_BITS_CLICKED );
                }
                else
                {
                    UNSET_BIT( mnu_widgetList[m].state, UI_BITS_CLICKED );
                }

                if ( BUTTON_DOWN == ui_doWidget( mnu_widgetList + m ) )
                {
                    if ( HAS_SOME_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) && !SelectedPlayer_list_check_loadplayer( &mnu_SelectedList,  lplayer ) )
                    {
                        // button has become cursor_clicked
                        // SelectedPlayer_list_add( &mnu_SelectedList, lplayer);
                        last_player = lplayer;
                        new_player  = btrue;
                    }
                    else if ( HAS_NO_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) && SelectedPlayer_list_check_loadplayer( &mnu_SelectedList,  lplayer ) )
                    {
                        // button has become unclicked
                        if ( SelectedPlayer_list_remove( &mnu_SelectedList,  lplayer ) )
                        {
                            last_player = -1;
                        }
                    }
                }

                // do each of the input buttons
                for ( j = 0, m++; j < 4; j++, m++ )
                {
                    /// @note check for devices being turned on and off each time
                    /// technically we COULD recognize joysticks being inserted and removed while the
                    /// game is running but we don't. I will set this up to handle such future behavior
                    /// anyway :)
                    switch ( j )
                    {
                        case INPUT_DEVICE_KEYBOARD: device_on[j] = keyb.on; break;
                        case INPUT_DEVICE_MOUSE:    device_on[j] = mous.on; break;
                        case INPUT_DEVICE_JOY_A:  device_on[j] = joy[0].on; break;
                        case INPUT_DEVICE_JOY_B:  device_on[j] = joy[1].on; break;
                        default: device_on[j] = bfalse;
                    }

                    // replace any not on device with a null icon
                    if ( !device_on[j] )
                    {
                        mnu_widgetList[m].img = TxTexture_get_ptr(( TX_REF )ICON_NULL );

                        // draw the widget, even though it will not do anything
                        // the menu would looks funny if odd columns missing
                        ui_doWidget( mnu_widgetList + m );
                    }

                    // make the button states reflect the chosen input devices
                    if ( INVALID_PLAYER == splayer || HAS_NO_BITS( mnu_SelectedList.lst[ splayer ].input, BitsInput[j] ) )
                    {
                        UNSET_BIT( mnu_widgetList[m].state, UI_BITS_CLICKED );
                    }
                    else if ( HAS_SOME_BITS( mnu_SelectedList.lst[splayer].input, BitsInput[j] ) )
                    {
                        SET_BIT( mnu_widgetList[m].state, UI_BITS_CLICKED );
                    }

                    if ( BUTTON_DOWN == ui_doWidget( mnu_widgetList + m ) )
                    {
                        if ( HAS_SOME_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) )
                        {
                            // button has become cursor_clicked
                            if ( INVALID_PLAYER == splayer )
                            {
                                if ( SelectedPlayer_list_add( &mnu_SelectedList,  lplayer ) )
                                {
                                    last_player = lplayer;
                                    new_player  = btrue;
                                }
                            }
                            if ( SelectedPlayer_list_add_input( &mnu_SelectedList,  lplayer, BitsInput[j] ) )
                            {
                                last_player = lplayer;
                                new_player  = btrue;
                            }
                        }
                        else if ( INVALID_PLAYER != splayer && HAS_NO_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) )
                        {
                            // button has become unclicked
                            if ( SelectedPlayer_list_remove_input( &mnu_SelectedList,  lplayer, BitsInput[j] ) )
                            {
                                last_player = -1;
                            }
                        }
                    }
                }
            }

            // Buttons for going ahead

            // Continue
            if ( 0 != mnu_SelectedList.count )
            {
                if ( SDLKEYDOWN( SDLK_RETURN ) || BUTTON_UP == ui_doButton( 100, button_text[0], NULL, buttonLeft, buttonTop, 200, 30 ) )
                {
                    menuState = MM_Leaving;
                }
            }

            // Back
            if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 101, button_text[1], NULL, buttonLeft, buttonTop + 35, 200, 30 ) )
            {
                SelectedPlayer_list_init( &mnu_SelectedList );
                menuState = MM_Leaving;
            }

            // show the stats
            if ( VALID_LOADPLAYER_IDX( mnu_loadplayer, last_player ) )
            {
                LoadPlayer_element_t * loadplayer_ptr = mnu_loadplayer.lst + last_player;
                if ( new_player )
                {
                    // load and display the new lplayer data
                    new_player = bfalse;
                    doChoosePlayer_show_stats( loadplayer_ptr, 0, GFX_WIDTH - 400, 10, 350, GFX_HEIGHT - 60 );
                }
                else
                {
                    // just display the new lplayer data
                    doChoosePlayer_show_stats( loadplayer_ptr, 2, GFX_WIDTH - 400, 10, 350, GFX_HEIGHT - 60 );
                }
            }
            else
            {
                doChoosePlayer_show_stats( NULL, 1, GFX_WIDTH - 100, 10, 100, GFX_HEIGHT - 60 );
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            /*
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG(glColor4f)(1, 1, 1, 1 - mnu_SlidyButtonState.lerp );
            // Buttons
            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( deltaTime );
            if ( mnu_SlidyButtonState.lerp >= 1.0f )
            {
                menuState = MM_Finish;
            }
            */

            // Simply fall through
            //  menuState = MM_Finish;
            //  break;

        case MM_Finish:

            // release all of the temporary profiles
            doChoosePlayer_show_stats( NULL, 1, 0, 0, 0, 0 );

            oglx_texture_Release( &background );
            TxTexture_free_one(( TX_REF )TX_BARS );

            menuState = MM_Begin;
            if ( 0 == mnu_SelectedList.count )
            {
                result = -1;
            }
            else
            {
                // set up the slots and the import stuff for the selected players
                if ( rv_success == mnu_set_local_import_list( &ImportList, &mnu_SelectedList ) )
                {
                    game_copy_imports( &ImportList );
                }
                else
                {
                    // erase the data in the import folder
                    vfs_removeDirectoryAndContents( "import", VFS_TRUE );
                }

                // if there are no selected players, there is no reason to keep this data
                if ( 0 == mnu_SelectedList.count )
                {
                    LoadPlayer_list_dealloc( &mnu_loadplayer );
                }

                result = 1;
            }

            // reset the ui
            ui_Reset();

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
int doOptions( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int menuChoice = 0;

    static const char *sz_buttons[] =
    {
        "Game Options",
        "Audio Options",
        "Input Controls",
        "Video Settings",
        "Back",
        ""
    };

    int result = 0;

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            ego_texture_load_vfs( &background, "mp_data/menu/menu_gnome", TRANSCOLOR );
            menuChoice = 0;
            menuState = MM_Entering;

            tipText_set_position( menuFont, "Change your audio, input and video\nsettings here.", 20 );

            mnu_SlidyButton_init( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            // Draw the background
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( -deltaTime );

            // Let lerp wind down relative to the time elapsed
            if ( mnu_SlidyButtonState.lerp <= 0.0f )
            {
                menuState = MM_Running;
            }
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            // "Options" text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            // Buttons
            if ( BUTTON_UP == ui_doButton( 1, sz_buttons[0], NULL, buttonLeft, buttonTop, 200, 30 ) )
            {
                // game options
                menuChoice = 5;
            }
            if ( BUTTON_UP == ui_doButton( 2, sz_buttons[1], NULL, buttonLeft, buttonTop + 35, 200, 30 ) )
            {
                // audio options
                menuChoice = 1;
            }
            if ( BUTTON_UP == ui_doButton( 3, sz_buttons[2], NULL, buttonLeft, buttonTop + 35 * 2, 200, 30 ) )
            {
                // input options
                menuChoice = 2;
            }
            if ( BUTTON_UP == ui_doButton( 4, sz_buttons[3], NULL, buttonLeft, buttonTop + 35 * 3, 200, 30 ) )
            {
                // video options
                menuChoice = 3;
            }
            if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 5, sz_buttons[4], NULL, buttonLeft, buttonTop + 35 * 4, 200, 30 ) )
            {
                // back to main menu
                menuChoice = 4;
            }
            if ( menuChoice != 0 )
            {
                menuState = MM_Leaving;
                mnu_SlidyButton_init( 0.0f, sz_buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            // Buttons
            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( deltaTime );
            if ( mnu_SlidyButtonState.lerp >= 1.0f )
            {
                menuState = MM_Finish;
            }
            break;

        case MM_Finish:
            // Free the background texture; don't need to hold onto it
            oglx_texture_Release( &background );
            menuState = MM_Begin;  // Make sure this all resets next time

            // reset the ui
            ui_Reset();

            // Set the next menu to load
            result = menuChoice;
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
int doInputOptions( float deltaTime )
{
    static int menuState = MM_Begin;
    static int menuChoice = 0;
    static int waitingforinput = -1;

    static STRING inputOptionsButtons[CONTROL_COMMAND_COUNT + 2];

    Sint8  result = 0;
    static Sint32 player = 0;

    Uint32              i;
    Sint32              idevice, iicon;
    device_controls_t * pdevice;

    pdevice = NULL;
    if ( player >= 0 && player < input_device_count )
    {
        pdevice = controls + player;
    };

    idevice = player;
    if ( NULL == pdevice )
    {
        idevice = -1;
    }

    iicon = MIN( 4, idevice );
    if ( idevice < 0 )
    {
        iicon = -1;
    }

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            menuChoice = 0;
            menuState = MM_Entering;
            // let this fall through into MM_Entering

            waitingforinput = -1;

            for ( i = 0; i < CONTROL_COMMAND_COUNT; i++ )
            {
                inputOptionsButtons[i][0] = CSTR_END;
            }
            strncpy( inputOptionsButtons[i++], "Player 1", sizeof( STRING ) );
            strncpy( inputOptionsButtons[i++], "Save Settings", sizeof( STRING ) );

            tipText_set_position( menuFont, "Change input settings here.", 20 );

            // Load the global icons (keyboard, mouse, etc.)
            if ( !load_all_global_icons() ) log_warning( "Could not load all global icons!\n" );

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            // Fall trough
            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
            ui_drawTextBox( menuFont, "ATK_LEFT HAND", buttonLeft, GFX_HEIGHT - 470, 0, 0, 20 );

            // Are we waiting for input?
            if ( SDLKEYDOWN( SDLK_ESCAPE ) ) waitingforinput = -1;  // Someone pressed abort

            // Grab the key/button input from the selected device
            if ( -1 != waitingforinput )
            {
                if ( NULL == pdevice || idevice < 0 || idevice >= input_device_count )
                {
                    waitingforinput = -1;
                }
                else
                {
                    if ( waitingforinput >= pdevice->count )
                    {
                        // invalid input range for this device
                        waitingforinput = -1;
                    }
                    else
                    {
                        control_t * pcontrol;
                        int         tag;

                        // make sure to update the input
                        input_read();

                        pcontrol = pdevice->control + waitingforinput;
                        if ( idevice >= INPUT_DEVICE_JOY )
                        {
                            int ijoy = idevice - INPUT_DEVICE_JOY;
                            if ( ijoy < MAXJOYSTICK )
                            {
                                for ( tag = 0; tag < scantag_count; tag++ )
                                {
                                    if ( 0 != scantag[tag].value && ( Uint32 )scantag[tag].value == joy[ijoy].b )
                                    {
                                        pcontrol->tag    = scantag[tag].value;
                                        pcontrol->is_key = bfalse;
                                        waitingforinput = -1;
                                    }
                                }

                                for ( tag = 0; tag < scantag_count; tag++ )
                                {
                                    if ( scantag[tag].value < SDLK_NUMLOCK && SDLKEYDOWN( scantag[tag].value ) )
                                    {
                                        pcontrol->tag    = scantag[tag].value;
                                        pcontrol->is_key = btrue;
                                        waitingforinput = -1;
                                    }
                                }
                            }
                        }
                        else
                        {
                            switch ( idevice )
                            {
                                case INPUT_DEVICE_KEYBOARD:
                                    {
                                        for ( tag = 0; tag < scantag_count; tag++ )
                                        {
                                            if ( scantag[tag].value < SDLK_NUMLOCK && SDLKEYDOWN( scantag[tag].value ) )
                                            {
                                                pcontrol->tag    = scantag[tag].value;
                                                pcontrol->is_key = btrue;
                                                waitingforinput = -1;
                                            }
                                        }
                                    }
                                    break;

                                case INPUT_DEVICE_MOUSE:
                                    {
                                        for ( tag = 0; tag < scantag_count; tag++ )
                                        {
                                            if ( 0 != scantag[tag].value && ( Uint32 )scantag[tag].value == mous.b )
                                            {
                                                pcontrol->tag    = scantag[tag].value;
                                                pcontrol->is_key = bfalse;
                                                waitingforinput = -1;
                                            }
                                        }

                                        for ( tag = 0; tag < scantag_count; tag++ )
                                        {
                                            if ( scantag[tag].value < SDLK_NUMLOCK && SDLKEYDOWN( scantag[tag].value ) )
                                            {
                                                pcontrol->tag    = scantag[tag].value;
                                                pcontrol->is_key = btrue;
                                                waitingforinput = -1;
                                            }
                                        }
                                    }
                                    break;
                            }
                        }

                    }
                }
            }
            if ( NULL != pdevice && -1 == waitingforinput )
            {
                // update the control names
                for ( i = CONTROL_BEGIN; i <= CONTROL_END && i < pdevice->count; i++ )
                {
                    const char * tag = scantag_get_string( pdevice->device, pdevice->control[i].tag, pdevice->control[i].is_key );

                    strncpy( inputOptionsButtons[i], tag, sizeof( STRING ) );
                }
                for ( /* nothing */; i <= CONTROL_END ; i++ )
                {
                    inputOptionsButtons[i][0] = CSTR_END;
                }
            }

            // Left hand
            if ( CSTR_END != inputOptionsButtons[CONTROL_LEFT_USE][0] )
            {
                ui_drawTextBox( menuFont, "Use:", buttonLeft, GFX_HEIGHT - 440, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 1, inputOptionsButtons[CONTROL_LEFT_USE], menuFont, buttonLeft + 100, GFX_HEIGHT - 440, 140, 30 ) )
                {
                    waitingforinput = CONTROL_LEFT_USE;
                    strncpy( inputOptionsButtons[CONTROL_LEFT_USE], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_LEFT_GET][0] )
            {
                ui_drawTextBox( menuFont, "Get/Drop:", buttonLeft, GFX_HEIGHT - 410, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 2, inputOptionsButtons[CONTROL_LEFT_GET], menuFont, buttonLeft + 100, GFX_HEIGHT - 410, 140, 30 ) )
                {
                    waitingforinput = CONTROL_LEFT_GET;
                    strncpy( inputOptionsButtons[CONTROL_LEFT_GET], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_LEFT_PACK][0] )
            {
                ui_drawTextBox( menuFont, "Inventory:", buttonLeft, GFX_HEIGHT - 380, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 3, inputOptionsButtons[CONTROL_LEFT_PACK], menuFont, buttonLeft + 100, GFX_HEIGHT - 380, 140, 30 ) )
                {
                    waitingforinput = CONTROL_LEFT_PACK;
                    strncpy( inputOptionsButtons[CONTROL_LEFT_PACK], "...", sizeof( STRING ) );
                }
            }

            // Right hand
            ui_drawTextBox( menuFont, "ATK_RIGHT HAND", buttonLeft + 300, GFX_HEIGHT - 470, 0, 0, 20 );
            if ( CSTR_END != inputOptionsButtons[CONTROL_RIGHT_USE][0] )
            {
                ui_drawTextBox( menuFont, "Use:", buttonLeft + 300, GFX_HEIGHT - 440, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 4, inputOptionsButtons[CONTROL_RIGHT_USE], menuFont, buttonLeft + 400, GFX_HEIGHT - 440, 140, 30 ) )
                {
                    waitingforinput = CONTROL_RIGHT_USE;
                    strncpy( inputOptionsButtons[CONTROL_RIGHT_USE], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_RIGHT_GET][0] )
            {
                ui_drawTextBox( menuFont, "Get/Drop:", buttonLeft + 300, GFX_HEIGHT - 410, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 5, inputOptionsButtons[CONTROL_RIGHT_GET], menuFont, buttonLeft + 400, GFX_HEIGHT - 410, 140, 30 ) )
                {
                    waitingforinput = CONTROL_RIGHT_GET;
                    strncpy( inputOptionsButtons[CONTROL_RIGHT_GET], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_RIGHT_PACK][0] )
            {
                ui_drawTextBox( menuFont, "Inventory:", buttonLeft + 300, GFX_HEIGHT - 380, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 6, inputOptionsButtons[CONTROL_RIGHT_PACK], menuFont, buttonLeft + 400, GFX_HEIGHT - 380, 140, 30 ) )
                {
                    waitingforinput = CONTROL_RIGHT_PACK;
                    strncpy( inputOptionsButtons[CONTROL_RIGHT_PACK], "...", sizeof( STRING ) );
                }
            }

            // Controls
            ui_drawTextBox( menuFont, "CONTROLS", buttonLeft, GFX_HEIGHT - 320, 0, 0, 20 );
            if ( CSTR_END != inputOptionsButtons[CONTROL_JUMP][0] )
            {
                ui_drawTextBox( menuFont, "Jump:", buttonLeft, GFX_HEIGHT - 290, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 7, inputOptionsButtons[CONTROL_JUMP], menuFont, buttonLeft + 100, GFX_HEIGHT - 290, 140, 30 ) )
                {
                    waitingforinput = CONTROL_JUMP;
                    strncpy( inputOptionsButtons[CONTROL_JUMP], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_UP][0] )
            {
                ui_drawTextBox( menuFont, "Up:", buttonLeft, GFX_HEIGHT - 260, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 8, inputOptionsButtons[CONTROL_UP], menuFont, buttonLeft + 100, GFX_HEIGHT - 260, 140, 30 ) )
                {
                    waitingforinput = CONTROL_UP;
                    strncpy( inputOptionsButtons[CONTROL_UP], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_DOWN][0] )
            {
                ui_drawTextBox( menuFont, "Down:", buttonLeft, GFX_HEIGHT - 230, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 9, inputOptionsButtons[CONTROL_DOWN], menuFont, buttonLeft + 100, GFX_HEIGHT - 230, 140, 30 ) )
                {
                    waitingforinput = CONTROL_DOWN;
                    strncpy( inputOptionsButtons[CONTROL_DOWN], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_LEFT][0] )
            {
                ui_drawTextBox( menuFont, "Left:", buttonLeft, GFX_HEIGHT - 200, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 10, inputOptionsButtons[CONTROL_LEFT], menuFont, buttonLeft + 100, GFX_HEIGHT - 200, 140, 30 ) )
                {
                    waitingforinput = CONTROL_LEFT;
                    strncpy( inputOptionsButtons[CONTROL_LEFT], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_RIGHT][0] )
            {
                ui_drawTextBox( menuFont, "Right:", buttonLeft, GFX_HEIGHT - 170, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 11, inputOptionsButtons[CONTROL_RIGHT], menuFont, buttonLeft + 100, GFX_HEIGHT - 170, 140, 30 ) )
                {
                    waitingforinput = CONTROL_RIGHT;
                    strncpy( inputOptionsButtons[CONTROL_RIGHT], "...", sizeof( STRING ) );
                }
            }

            // Controls
            ui_drawTextBox( menuFont, "CAMERA CONTROL", buttonLeft + 300, GFX_HEIGHT - 320, 0, 0, 20 );
            if ( CSTR_END != inputOptionsButtons[CONTROL_CAMERA_IN][0] )
            {
                ui_drawTextBox( menuFont, "Zoom In:", buttonLeft + 300, GFX_HEIGHT - 290, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 12, inputOptionsButtons[CONTROL_CAMERA_IN], menuFont, buttonLeft + 450, GFX_HEIGHT - 290, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA_IN;
                    strncpy( inputOptionsButtons[CONTROL_CAMERA_IN], "...", sizeof( STRING ) );
                }
            }
            else
            {
                // single button camera control
                ui_drawTextBox( menuFont, "Camera:", buttonLeft + 300, GFX_HEIGHT - 290, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 12, inputOptionsButtons[CONTROL_CAMERA], menuFont, buttonLeft + 450, GFX_HEIGHT - 290, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA;
                    strncpy( inputOptionsButtons[CONTROL_CAMERA], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_CAMERA_OUT][0] )
            {
                ui_drawTextBox( menuFont, "Zoom Out:", buttonLeft + 300, GFX_HEIGHT - 260, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 13, inputOptionsButtons[CONTROL_CAMERA_OUT], menuFont, buttonLeft + 450, GFX_HEIGHT - 260, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA_OUT;
                    strncpy( inputOptionsButtons[CONTROL_CAMERA_OUT], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_CAMERA_LEFT][0] )
            {
                ui_drawTextBox( menuFont, "Rotate Left:", buttonLeft + 300, GFX_HEIGHT - 230, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 14, inputOptionsButtons[CONTROL_CAMERA_LEFT], menuFont, buttonLeft + 450, GFX_HEIGHT - 230, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA_LEFT;
                    strncpy( inputOptionsButtons[CONTROL_CAMERA_LEFT], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != inputOptionsButtons[CONTROL_CAMERA_RIGHT][0] )
            {
                ui_drawTextBox( menuFont, "Rotate Right:", buttonLeft + 300, GFX_HEIGHT - 200, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 15, inputOptionsButtons[CONTROL_CAMERA_RIGHT], menuFont, buttonLeft + 450, GFX_HEIGHT - 200, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA_RIGHT;
                    strncpy( inputOptionsButtons[CONTROL_CAMERA_RIGHT], "...", sizeof( STRING ) );
                }
            }

            // The select player button
            if ( iicon < 0 )
            {
                if ( BUTTON_UP == ui_doButton( 16, "Select Player...", NULL, buttonLeft + 300, GFX_HEIGHT - 90, 140, 50 ) )
                {
                    player = 0;
                }
            }
            else if ( BUTTON_UP ==  ui_doImageButtonWithText( 16, TxTexture_get_ptr(( TX_REF )( ICON_KEYB + iicon ) ), inputOptionsButtons[CONTROL_COMMAND_COUNT+0], menuFont, buttonLeft + 300, GFX_HEIGHT - 90, 140, 50 ) )
            {
                if ( input_device_count > 0 )
                {
                    player++;
                    player %= input_device_count;
                }

                snprintf( inputOptionsButtons[CONTROL_COMMAND_COUNT+0], sizeof( STRING ), "Player %i", player + 1 );
            }

            // Save settings button
            if ( BUTTON_UP == ui_doButton( 17, inputOptionsButtons[CONTROL_COMMAND_COUNT+1], menuFont, buttonLeft, GFX_HEIGHT - 60, 200, 30 ) )
            {
                // save settings and go back
                player = 0;
                input_settings_save_vfs( "controls.txt" );
                menuState = MM_Leaving;
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            // Fall trough
            menuState = MM_Finish;
            break;

        case MM_Finish:
            menuState = MM_Begin;  // Make sure this all resets next time

            // reset the ui
            ui_Reset();

            // Set the next menu to load
            result = 1;
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
int doGameOptions( float deltaTime )
{
    /// @details Game options menu

    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int menuChoice = 0;

    static STRING Cdifficulty;
    static STRING Cmaxmessage;
    static const char *sz_buttons[] =
    {
        "N/A",        // Difficulty
        "N/A",        // Max messages
        "N/A",        // Message duration
        "N/A",        // Autoturn camera
        "N/A",        // Show FPS
        "N/A",        // Feedback
        "Save Settings",
        ""
    };

    char szDifficulty[4096] = EMPTY_CSTR;
    int  result = 0;

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            ego_texture_load_vfs( &background, "mp_data/menu/menu_fairy", TRANSCOLOR );

            menuChoice = 0;
            menuState = MM_Entering;

            tipText_set_position( menuFont, "Change game settings here.", 20 );

            mnu_SlidyButton_init( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            // Draw the background
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  / 2 ) + ( background.imgW / 2 ), GFX_HEIGHT - background.imgH, 0, 0, NULL );
            }

            // Load the current settings
            switch ( cfg.difficulty )
            {
                case GAME_HARD: snprintf( Cdifficulty, SDL_arraysize( Cdifficulty ), "Punishing" ); break;
                case GAME_EASY: snprintf( Cdifficulty, SDL_arraysize( Cdifficulty ), "Forgiving" ); break;
                default: cfg.difficulty = GAME_NORMAL; /* fall through */
                case GAME_NORMAL:  snprintf( Cdifficulty, SDL_arraysize( Cdifficulty ), "Challenging" ); break;
            }
            sz_buttons[0] = Cdifficulty;

            maxmessage = CLIP( maxmessage, 4, MAX_MESSAGE );
            if ( 0 == maxmessage )
            {
                snprintf( Cmaxmessage, SDL_arraysize( Cmaxmessage ), "None" );          // Set to default
            }
            else
            {
                snprintf( Cmaxmessage, SDL_arraysize( Cmaxmessage ), "%i", maxmessage );
            }
            sz_buttons[1] = Cmaxmessage;

            // Message duration
            if ( cfg.message_duration <= 100 )
            {
                sz_buttons[2] = "Short";
            }
            else if ( cfg.message_duration <= 150 )
            {
                sz_buttons[2] = "Normal";
            }
            else
            {
                sz_buttons[2] = "Long";
            }

            // Autoturn camera
            if ( CAM_TURN_GOOD == cfg.autoturncamera )        sz_buttons[3] = "Fast";
            else if ( CAM_TURN_AUTO == cfg.autoturncamera )   sz_buttons[3] = "On";
            else
            {
                sz_buttons[3] = "Off";
                cfg.autoturncamera = CAM_TURN_NONE;
            }

            // Show FPS
            sz_buttons[4] = cfg.fps_allowed ? "On" : "Off";

            //Billboard feedback
            if ( !cfg.feedback ) sz_buttons[5] = "Disabled";
            else
            {
                if ( cfg.feedback == FEEDBACK_TEXT ) sz_buttons[5] = "Enabled";
                else                              sz_buttons[5] = "Debug";
            }

            // Fall trough
            menuState = MM_Running;
            //break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 ), GFX_HEIGHT - background.imgH, 0, 0, NULL );
            }

            ui_drawTextBox( menuFont, "Game Difficulty:", buttonLeft, 50, 0, 0, 20 );

            // Buttons
            if ( !PMod->active && BUTTON_UP == ui_doButton( 1, sz_buttons[0], menuFont, buttonLeft + 150, 50, 150, 30 ) )
            {
                // Increase difficulty
                cfg.difficulty++;
                switch ( cfg.difficulty )
                {
                    case GAME_NORMAL: snprintf( Cdifficulty, SDL_arraysize( Cdifficulty ), "Challenging" ); break;
                    case GAME_HARD: snprintf( Cdifficulty, SDL_arraysize( Cdifficulty ), "Punishing" ); break;
                default: case GAME_EASY:
                        {
                            snprintf( Cdifficulty, SDL_arraysize( Cdifficulty ), "Forgiving" );
                            cfg.difficulty = GAME_EASY;
                            break;
                        }
                }
                sz_buttons[0] = Cdifficulty;
            }

            // Now do difficulty description. Currently it's handled very bad, but it works.
            switch ( cfg.difficulty )
            {
                case GAME_EASY:
                    strncpy( szDifficulty, "FORGIVING (Easy)\n - Players gain no bonus XP \n - 15%% XP loss upon death\n - Monsters take 25%% extra damage by players\n - Players take 25%% less damage by monsters\n - Halves the chance for Kursed items\n - Cannot unlock the final level in this mode\n - Life and Mana is refilled after quitting a module", SDL_arraysize( szDifficulty ) );
                    break;
                case GAME_NORMAL:
                    strncpy( szDifficulty, "CHALLENGING (Normal)\n - Players gain 10%% bonus XP \n - 15%% XP loss upon death \n - 15%% money loss upon death", SDL_arraysize( szDifficulty ) );
                    break;
                case GAME_HARD:
                    strncpy( szDifficulty, "PUNISHING (Hard)\n - Monsters award 20%% extra xp! \n - 15%% XP loss upon death\n - 15%% money loss upon death\n - No respawning\n - Channeling life can kill you\n - Players take 25%% more damage\n - Doubles the chance for Kursed items", SDL_arraysize( szDifficulty ) );
                    break;
            }
            str_add_linebreaks( szDifficulty, SDL_arraysize( szDifficulty ), 30 );
            ui_drawTextBox( menuFont, szDifficulty, buttonLeft, 100, 0, 0, 20 );

            // Text messages
            ui_drawTextBox( menuFont, "Max  Messages:", buttonLeft + 350, 50, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 2, sz_buttons[1], menuFont, buttonLeft + 515, 50, 75, 30 ) )
            {
                cfg.message_count_req++;
                if ( cfg.message_count_req > MAX_MESSAGE ) cfg.message_count_req = 0;
                if ( cfg.message_count_req < 4 && cfg.message_count_req != 0 ) cfg.message_count_req = 4;

                if ( 0 == cfg.message_count_req )
                {
                    snprintf( Cmaxmessage, SDL_arraysize( Cmaxmessage ), "None" );
                }
                else
                {
                    snprintf( Cmaxmessage, SDL_arraysize( Cmaxmessage ), "%i", cfg.message_count_req );   // Convert integer to a char we can use
                }

                sz_buttons[1] = Cmaxmessage;
            }

            // Message time
            ui_drawTextBox( menuFont, "Message Duration:", buttonLeft + 350, 100, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 3, sz_buttons[2], menuFont, buttonLeft + 515, 100, 100, 30 ) )
            {
                if ( cfg.message_duration <= 0 )
                {
                    cfg.message_duration = 100;
                }
                else
                {
                    cfg.message_duration += 50;
                }

                if ( cfg.message_duration >= 250 )
                {
                    cfg.message_duration = 100;
                }

                if ( cfg.message_duration <= 100 )
                {
                    sz_buttons[2] = "Short";
                }
                else if ( cfg.message_duration <= 150 )
                {
                    sz_buttons[2] = "Normal";
                }
                else
                {
                    sz_buttons[2] = "Long";
                }
            }

            // Autoturn camera
            ui_drawTextBox( menuFont, "Autoturn Camera:", buttonLeft + 350, 150, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 4, sz_buttons[3], menuFont, buttonLeft + 515, 150, 100, 30 ) )
            {
                if ( CAM_TURN_GOOD == cfg.autoturncamera )
                {
                    sz_buttons[3] = "Off";
                    cfg.autoturncamera = CAM_TURN_NONE;
                }
                else if ( cfg.autoturncamera )
                {
                    sz_buttons[3] = "Fast";
                    cfg.autoturncamera = CAM_TURN_GOOD;
                }
                else
                {
                    sz_buttons[3] = "On";
                    cfg.autoturncamera = CAM_TURN_AUTO;
                }
            }

            // Show the fps?
            ui_drawTextBox( menuFont, "Display FPS:", buttonLeft + 350, 200, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 5, sz_buttons[4], menuFont, buttonLeft + 515, 200, 100, 30 ) )
            {
                cfg.fps_allowed = !cfg.fps_allowed;
                if ( cfg.fps_allowed )   sz_buttons[4] = "On";
                else                     sz_buttons[4] = "Off";
            }

            // Feedback
            ui_drawTextBox( menuFont, "Floating Text:", buttonLeft + 350, 250, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 6, sz_buttons[5], menuFont, buttonLeft + 515, 250, 75, 30 ) )
            {
                if ( cfg.dev_mode )
                {
                    cfg.feedback = ( FEEDBACK_TYPE )( cfg.feedback + 1 );
                    if ( cfg.feedback > FEEDBACK_NUMBER ) cfg.feedback = FEEDBACK_OFF;
                }
                else
                {
                    if ( FEEDBACK_OFF == cfg.feedback )
                    {
                        cfg.feedback = FEEDBACK_TEXT;
                    }
                    else
                    {
                        cfg.feedback = FEEDBACK_OFF;
                    }
                }

                switch ( cfg.feedback )
                {
                    case FEEDBACK_OFF:    sz_buttons[5] = "Disabled"; break;
                    case FEEDBACK_TEXT:   sz_buttons[5] = "Enabled";  break;
                    case FEEDBACK_NUMBER: sz_buttons[5] = "Debug";    break;
                }
            }

            // Save settings
            if ( BUTTON_UP == ui_doButton( 7, sz_buttons[6], menuFont, buttonLeft, GFX_HEIGHT - 60, 200, 30 ) )
            {
                // synchronoze the config values with the various game subsystems
                setup_synch( &cfg );

                // save the setup file
                setup_upload( &cfg );
                setup_write();

                menuState = MM_Leaving;
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  / 2 ) + ( background.imgW / 2 ), GFX_HEIGHT - background.imgH, 0, 0, NULL );
            }

            // Fall trough
            menuState = MM_Finish;
            break;

        case MM_Finish:
            // Free the background texture; don't need to hold onto it
            oglx_texture_Release( &background );
            menuState = MM_Begin;  // Make sure this all resets next time

            // reset the ui
            ui_Reset();

            // Set the next menu to load
            result = 1;
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
int doAudioOptions( float deltaTime )
{
    /// @details Audio options menu

    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int menuChoice = 0;
    static STRING Cmaxsoundchannel;
    static STRING Cbuffersize;
    static STRING Csoundvolume;
    static STRING Cmusicvolume;
    static const char *sz_buttons[] =
    {
        "N/A",        // Enable sound
        "N/A",        // Sound volume
        "N/A",        // Enable music
        "N/A",        // Music volume
        "N/A",        // Sound channels
        "N/A",        // Sound buffer
        "N/A",        // Sound quality
        "N/A",        // Play footsteps
        "Save Settings",
        ""
    };

    int result = 0;

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            ego_texture_load_vfs( &background, "mp_data/menu/menu_sound", TRANSCOLOR );

            menuChoice = 0;
            menuState = MM_Entering;

            tipText_set_position( menuFont, "Change audio settings here.", 20 );

            mnu_SlidyButton_init( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            // Draw the background
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            // Load the current settings
            sz_buttons[0] = cfg.sound_allowed ? "On" : "Off";

            snprintf( Csoundvolume, SDL_arraysize( Csoundvolume ), "%i", cfg.sound_volume );
            sz_buttons[1] = Csoundvolume;

            sz_buttons[2] = cfg.music_allowed ? "On" : "Off";

            snprintf( Cmusicvolume, SDL_arraysize( Cmusicvolume ), "%i", cfg.music_volume );
            sz_buttons[3] = Cmusicvolume;

            snprintf( Cmaxsoundchannel, SDL_arraysize( Cmaxsoundchannel ), "%i", cfg.sound_channel_count );
            sz_buttons[4] = Cmaxsoundchannel;

            snprintf( Cbuffersize, SDL_arraysize( Cbuffersize ), "%i", cfg.sound_buffer_size );
            sz_buttons[5] = Cbuffersize;

            sz_buttons[6] = cfg.sound_highquality ?  "Normal" : "High";

            sz_buttons[7] = cfg.sound_footfall ? "Enabled" : "Disabled";

            // Fall trough
            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            ui_drawTextBox( menuFont, "Sound:", buttonLeft, GFX_HEIGHT - 270, 0, 0, 20 );

            // Buttons
            if ( BUTTON_UP == ui_doButton( 1, sz_buttons[0], menuFont, buttonLeft + 150, GFX_HEIGHT - 270, 100, 30 ) )
            {
                cfg.sound_allowed = !cfg.sound_allowed;
                sz_buttons[0] = cfg.sound_allowed ? "On" : "Off";
            }

            ui_drawTextBox( menuFont, "Sound Volume:", buttonLeft, GFX_HEIGHT - 235, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 2, sz_buttons[1], menuFont, buttonLeft + 150, GFX_HEIGHT - 235, 100, 30 ) )
            {
                cfg.sound_volume += 5;
                if ( cfg.sound_volume > 100 ) cfg.sound_volume = 0;

                snprintf( Csoundvolume, SDL_arraysize( Csoundvolume ), "%i", cfg.sound_volume );
                sz_buttons[1] = Csoundvolume;
            }

            ui_drawTextBox( menuFont, "Music:", buttonLeft, GFX_HEIGHT - 165, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 3, sz_buttons[2], menuFont, buttonLeft + 150, GFX_HEIGHT - 165, 100, 30 ) )
            {
                cfg.music_allowed = !cfg.music_allowed;
                sz_buttons[2] = cfg.music_allowed ? "On" : "Off";
            }

            ui_drawTextBox( menuFont, "Music Volume:", buttonLeft, GFX_HEIGHT - 130, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 4, sz_buttons[3], menuFont, buttonLeft + 150, GFX_HEIGHT - 130, 100, 30 ) )
            {
                cfg.music_volume += 5;
                if ( cfg.music_volume > 100 ) cfg.music_volume = 0;

                snprintf( Cmusicvolume, SDL_arraysize( Cmusicvolume ), "%i", cfg.music_volume );
                sz_buttons[3] = Cmusicvolume;
            }

            ui_drawTextBox( menuFont, "Sound Channels:", buttonLeft + 300, GFX_HEIGHT - 200, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 5, sz_buttons[4], menuFont, buttonLeft + 450, GFX_HEIGHT - 200, 100, 30 ) )
            {
                if ( cfg.sound_channel_count < 8 )
                {
                    cfg.sound_channel_count = 8;
                }
                else
                {
                    cfg.sound_channel_count <<= 1;
                }

                if ( cfg.sound_channel_count > 128 )
                {
                    cfg.sound_channel_count = 8;
                }

                snprintf( Cmaxsoundchannel, SDL_arraysize( Cmaxsoundchannel ), "%i", cfg.sound_channel_count );
                sz_buttons[4] = Cmaxsoundchannel;
            }

            ui_drawTextBox( menuFont, "Buffer Size:", buttonLeft + 300, GFX_HEIGHT - 165, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 6, sz_buttons[5], menuFont, buttonLeft + 450, GFX_HEIGHT - 165, 100, 30 ) )
            {
                if ( cfg.sound_buffer_size < 512 )
                {
                    cfg.sound_buffer_size = 512;
                }
                else
                {
                    cfg.sound_buffer_size <<= 1;
                }

                if ( cfg.sound_buffer_size > 8196 )
                {
                    cfg.sound_buffer_size = 512;
                }

                snprintf( Cbuffersize, SDL_arraysize( Cbuffersize ), "%i", cfg.sound_buffer_size );
                sz_buttons[5] = Cbuffersize;
            }

            ui_drawTextBox( menuFont, "Sound Quality:", buttonLeft + 300, GFX_HEIGHT - 130, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 7, sz_buttons[6], menuFont, buttonLeft + 450, GFX_HEIGHT - 130, 100, 30 ) )
            {
                cfg.sound_highquality = !cfg.sound_highquality;
                sz_buttons[6] = cfg.sound_highquality ? "Normal" : "High";
            }

            //Footfall sounds
            ui_drawTextBox( menuFont, "Footstep Sounds:", buttonLeft + 300, GFX_HEIGHT - 235, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 8, sz_buttons[7], menuFont, buttonLeft + 450, GFX_HEIGHT - 235, 100, 30 ) )
            {
                cfg.sound_footfall = !cfg.sound_footfall;
                sz_buttons[7] = cfg.sound_footfall ? "Enabled" : "Disabled";
            }

            //Save settings
            if ( BUTTON_UP == ui_doButton( 9, sz_buttons[8], menuFont, buttonLeft, GFX_HEIGHT - 60, 200, 30 ) )
            {
                // synchronoze the config values with the various game subsystems
                setup_synch( &cfg );

                // save the setup file
                setup_upload( &cfg );
                setup_write();

                // Reload the sound system
                sound_restart();

                // Do we restart the music?
                if ( cfg.music_allowed )
                {
                    load_all_music_sounds_vfs();
                    fade_in_music( musictracksloaded[songplaying] );
                }

                menuState = MM_Leaving;
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            // Fall trough
            menuState = MM_Finish;
            break;

        case MM_Finish:
            // Free the background texture; don't need to hold onto it
            oglx_texture_Release( &background );
            menuState = MM_Begin;  // Make sure this all resets next time

            // reset the ui
            ui_Reset();

            // Set the next menu to load
            result = 1;
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
bool_t doVideoOptions_coerce_aspect_ratio( int width, int height, float * pratio, STRING * psz_ratio )
{
    /// @details BB@> coerce the aspect ratio of the screen to some standard size

    float req_aspect_ratio;

    if ( 0 == height || NULL == pratio || NULL == psz_ratio ) return bfalse;

    req_aspect_ratio = ( float )width / ( float )height;

    if ( req_aspect_ratio > 0.0f && req_aspect_ratio < 0.5f*(( 5.0f / 4.0f ) + ( 4.0f / 3.0f ) ) )
    {
        *pratio = 5.0f / 4.0f;
        strncpy( *psz_ratio, "5:4", sizeof( *psz_ratio ) );
    }
    else if ( req_aspect_ratio >= 0.5f*(( 5.0f / 4.0f ) + ( 4.0f / 3.0f ) ) && req_aspect_ratio < 0.5f*(( 4.0f / 3.0f ) + ( 8.0f / 5.0f ) ) )
    {
        *pratio = 4.0f / 3.0f;
        strncpy( *psz_ratio, "4:3", sizeof( *psz_ratio ) );
    }
    else if ( req_aspect_ratio >= 0.5f*(( 4.0f / 3.0f ) + ( 8.0f / 5.0f ) ) && req_aspect_ratio < 0.5f*(( 8.0f / 5.0f ) + ( 5.0f / 3.0f ) ) )
    {
        *pratio = 8.0f / 5.0f;
        strncpy( *psz_ratio, "8:5", sizeof( *psz_ratio ) );
    }
    else if ( req_aspect_ratio >= 0.5f*(( 8.0f / 5.0f ) + ( 5.0f / 3.0f ) ) && req_aspect_ratio < 0.5f*(( 5.0f / 3.0f ) + ( 16.0f / 9.0f ) ) )
    {
        *pratio = 5.0f / 3.0f;
        strncpy( *psz_ratio, "5:3", sizeof( *psz_ratio ) );
    }
    else
    {
        *pratio = 16.0f / 9.0f;
        strncpy( *psz_ratio, "16:9", sizeof( *psz_ratio ) );
    }

    return btrue;

}

//--------------------------------------------------------------------------------------------
int doVideoOptions_fix_fullscreen_resolution( egoboo_config_t * pcfg, SDLX_screen_info_t * psdl_scr, STRING * psz_screen_size )
{
    STRING     sz_aspect_ratio = "unknown";
    float      req_screen_area  = ( float )pcfg->scrx_req * ( float )pcfg->scry_req;
    float      min_diff = 0.0f;
    SDL_Rect * found_rect = NULL, ** pprect = NULL;

    float       aspect_ratio;

    doVideoOptions_coerce_aspect_ratio( pcfg->scrx_req, pcfg->scry_req, &aspect_ratio, &sz_aspect_ratio );

    found_rect = NULL;
    pprect = psdl_scr->video_mode_list;
    while ( NULL != *pprect )
    {
        SDL_Rect * prect = *pprect;

        float sdl_aspect_ratio;
        float sdl_screen_area;
        float diff, diff1, diff2;

        sdl_aspect_ratio = ( float )prect->w / ( float )prect->h;
        sdl_screen_area  = prect->w * prect->h;

        diff1 = LOG( sdl_aspect_ratio / aspect_ratio );
        diff2 = LOG( sdl_screen_area / req_screen_area );

        diff = 2.0f * ABS( diff1 ) + ABS( diff2 );

        if ( NULL == found_rect || diff < min_diff )
        {
            found_rect = prect;
            min_diff   = diff;

            if ( 0.0f == min_diff ) break;
        }

        pprect++;
    }

    if ( NULL != found_rect )
    {
        pcfg->scrx_req = found_rect->w;
        pcfg->scry_req = found_rect->h;
    }
    else
    {
        // we cannot find an approximate screen size

        switch ( pcfg->scrx_req )
        {
                // Normal resolutions
            case 1024:
                pcfg->scry_req  = 768;
                strncpy( sz_aspect_ratio, "4:3", sizeof( sz_aspect_ratio ) );
                break;

            case 640:
                pcfg->scry_req = 480;
                strncpy( sz_aspect_ratio, "4:3", sizeof( sz_aspect_ratio ) );
                break;

            case 800:
                pcfg->scry_req = 600;
                strncpy( sz_aspect_ratio, "4:3", sizeof( sz_aspect_ratio ) );
                break;

                // 1280 can be both widescreen and normal
            case 1280:
                if ( pcfg->scry_req > 800 )
                {
                    pcfg->scry_req = 1024;
                    strncpy( sz_aspect_ratio, "5:4", sizeof( sz_aspect_ratio ) );
                }
                else
                {
                    pcfg->scry_req = 800;
                    strncpy( sz_aspect_ratio, "8:5", sizeof( sz_aspect_ratio ) );
                }
                break;

                // Widescreen resolutions
            case 1440:
                pcfg->scry_req = 900;
                strncpy( sz_aspect_ratio, "8:5", sizeof( sz_aspect_ratio ) );
                break;

            case 1680:
                pcfg->scry_req = 1050;
                strncpy( sz_aspect_ratio, "8:5", sizeof( sz_aspect_ratio ) );
                break;

            case 1920:
                pcfg->scry_req = 1200;
                strncpy( sz_aspect_ratio, "8:5", sizeof( sz_aspect_ratio ) );
                break;

                // unknown
            default:
                doVideoOptions_coerce_aspect_ratio( pcfg->scrx_req, pcfg->scry_req, &aspect_ratio, &sz_aspect_ratio );
                break;
        }
    }

    snprintf( *psz_screen_size, sizeof( *psz_screen_size ), "%dx%d - %s", pcfg->scrx_req, pcfg->scry_req, sz_aspect_ratio );

    return btrue;
}

//--------------------------------------------------------------------------------------------
int doVideoOptions( float deltaTime )
{
    /// @details Video options menu

    enum
    {
        but_antialiasing =  0,  // Antialaising
        but_unused           ,    // Unused button
        but_dither           ,    // Fast & ugly
        but_fullscreen       ,    // Fullscreen
        but_reflections      ,    // Reflections
        but_filtering        ,    // Texture filtering
        but_shadow           ,    // Shadows
        but_zbuffer          ,    // Z bit
        but_maxlights        ,    // Fog
        but_3dfx             ,    // 3D effects
        but_multiwater       ,    // Multi water layer
        but_widescreen       ,    // Widescreen
        but_screensize       ,    // Screen resolution
        but_save             ,
        but_maxparticles      ,  // Max particles
        but_end,

        but_last
    };

    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int    menuChoice = 0;
    static STRING Cantialiasing;
    static STRING Cmaxlights;
    static STRING Cscrz;
    static STRING Cmaxparticles;
    static STRING Cmaxdyna;

    static bool_t widescreen;
    static float  aspect_ratio;
    static STRING sz_screen_size;

    static const char *sz_buttons[but_last];

    int cnt, result = 0;

    switch ( menuState )
    {
        case MM_Begin:

            // set up the button text
            for ( cnt = 0; cnt < but_last; cnt++ ) sz_buttons[cnt] = "N/A";
            sz_buttons[but_end] = "";

            // set up menu variables
            ego_texture_load_vfs( &background, "mp_data/menu/menu_video", TRANSCOLOR );

            menuChoice = 0;
            menuState = MM_Entering;

            tipText_set_position( menuFont, "Change video settings here.", 20 );

            mnu_SlidyButton_init( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            // Draw the background
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            // Load all the current video settings
            if ( 0 == cfg.multisamples ) strncpy( Cantialiasing , "Off", SDL_arraysize( Cantialiasing ) );
            else snprintf( Cantialiasing, SDL_arraysize( Cantialiasing ), "X%i", cfg.multisamples );
            sz_buttons[but_antialiasing] = Cantialiasing;

            // Texture filtering
            switch ( cfg.texturefilter_req )
            {
                case TX_UNFILTERED:
                    sz_buttons[but_filtering] = "Unfiltered";
                    break;
                case TX_LINEAR:
                    sz_buttons[but_filtering] = "Linear";
                    break;
                case TX_MIPMAP:
                    sz_buttons[but_filtering] = "Mipmap";
                    break;
                case TX_BILINEAR:
                    sz_buttons[but_filtering] = "Bilinear";
                    break;
                case TX_TRILINEAR_1:
                    sz_buttons[but_filtering] = "Trilinear 1";
                    break;
                case TX_TRILINEAR_2:
                    sz_buttons[but_filtering] = "Trilinear 2";
                    break;
                case TX_ANISOTROPIC:
                    sz_buttons[but_filtering] = "Ansiotropic";
                    break;
                default:                  // Set to defaults
                    sz_buttons[but_filtering] = "Linear";
                    cfg.texturefilter_req = TX_LINEAR;
                    break;
            }

            sz_buttons[but_dither] = cfg.use_dither ? "Yes" : "No";

            sz_buttons[but_fullscreen] = cfg.fullscreen_req ? "True" : "False";

            if ( cfg.reflect_allowed )
            {
                sz_buttons[but_reflections] = "Low";
                if ( cfg.reflect_prt )
                {
                    sz_buttons[but_reflections] = "Medium";
                    if ( 0 == cfg.reflect_fade )
                    {
                        sz_buttons[but_reflections] = "High";
                    }
                }
            }
            else
            {
                sz_buttons[but_reflections] = "Off";
            }

            if ( cfg.shadow_allowed )
            {
                sz_buttons[but_shadow] = "Normal";
                if ( !cfg.shadow_sprite )
                {
                    sz_buttons[but_shadow] = "Best";
                }
            }
            else sz_buttons[but_shadow] = "Off";

#if defined(__unix__)
            //Clip linux defaults to valid values so that the game doesn't crash on startup
            if ( cfg.scrz_req == 32 ) cfg.scrz_req = 24;
            if ( cfg.scrd_req == 32 ) cfg.scrd_req = 24;
#endif

            if ( cfg.scrz_req != 32 && cfg.scrz_req != 16 && cfg.scrz_req != 24 )
            {
                cfg.scrz_req = 16;              // Set to default
            }
            snprintf( Cscrz, SDL_arraysize( Cscrz ), "%i", cfg.scrz_req );     // Convert the integer to a char we can use
            sz_buttons[but_zbuffer] = Cscrz;

            snprintf( Cmaxlights, SDL_arraysize( Cmaxlights ), "%i", cfg.dyna_count_req );
            sz_buttons[but_maxlights] = Cmaxlights;

            if ( cfg.use_phong )
            {
                sz_buttons[but_3dfx] = "Okay";
                if ( cfg.overlay_allowed && cfg.background_allowed )
                {
                    sz_buttons[but_3dfx] = "Good";
                    if ( cfg.use_perspective )
                    {
                        sz_buttons[but_3dfx] = "Superb";
                    }
                }
                else                            // Set to defaults
                {
                    cfg.use_perspective    = bfalse;
                    cfg.background_allowed = bfalse;
                    cfg.overlay_allowed    = bfalse;
                    sz_buttons[but_3dfx] = "Off";
                }
            }
            else                              // Set to defaults
            {
                cfg.use_perspective    = bfalse;
                cfg.background_allowed = bfalse;
                cfg.overlay_allowed    = bfalse;
                sz_buttons[but_3dfx] = "Off";
            }

            if ( cfg.twolayerwater_allowed ) sz_buttons[but_multiwater] = "On";
            else sz_buttons[but_multiwater] = "Off";

            snprintf( Cmaxparticles, SDL_arraysize( Cmaxparticles ), "%i", cfg.particle_count_req );     // Convert the integer to a char we can use
            sz_buttons[but_maxparticles] = Cmaxparticles;

            if ( cfg.fullscreen_req && NULL != sdl_scr.video_mode_list )
            {
                doVideoOptions_fix_fullscreen_resolution( &cfg, &sdl_scr, &sz_screen_size );
                sz_buttons[but_screensize] = sz_screen_size;

                aspect_ratio = ( float )cfg.scrx_req / ( float )cfg.scry_req;
                widescreen = ( aspect_ratio > ( 4.0f / 3.0f ) );
            }
            else
            {
                snprintf( sz_screen_size, sizeof( sz_screen_size ), "%dx%d", cfg.scrx_req, cfg.scry_req );
                sz_buttons[but_screensize] = sz_screen_size;

                aspect_ratio = ( float )cfg.scrx_req / ( float )cfg.scry_req;
                widescreen = ( aspect_ratio > ( 4.0f / 3.0f ) );
            }

            if ( widescreen ) sz_buttons[but_widescreen] = "X";
            else             sz_buttons[but_widescreen] = " ";

            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            // Antialiasing Button
            ui_drawTextBox( menuFont, "Antialiasing:", buttonLeft, GFX_HEIGHT - 215, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 1, sz_buttons[but_antialiasing], menuFont, buttonLeft + 150, GFX_HEIGHT - 215, 100, 30 ) )
            {
                // make the multi-sampling even

                if ( cfg.multisamples < 0 )
                {
                    cfg.multisamples = 0;
                }
                else
                {
                    cfg.multisamples += 1;
                }

                // set some arbitrary limit
                if ( cfg.multisamples > 4 ) cfg.multisamples = 0;

                if ( 0 == cfg.multisamples ) strncpy( Cantialiasing , "Off", SDL_arraysize( Cantialiasing ) );
                else snprintf( Cantialiasing, SDL_arraysize( Cantialiasing ), "X%i", cfg.multisamples );

                sz_buttons[but_antialiasing] = Cantialiasing;
            }

            // Dithering
            ui_drawTextBox( menuFont, "Dithering:", buttonLeft, GFX_HEIGHT - 145, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 3, sz_buttons[but_dither], menuFont, buttonLeft + 150, GFX_HEIGHT - 145, 100, 30 ) )
            {
                cfg.use_dither = !cfg.use_dither;
                sz_buttons[but_dither] = cfg.use_dither ? "Yes" : "No";
            }

            // Fullscreen
            ui_drawTextBox( menuFont, "Fullscreen:", buttonLeft, GFX_HEIGHT - 110, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 4, sz_buttons[but_fullscreen], menuFont, buttonLeft + 150, GFX_HEIGHT - 110, 100, 30 ) )
            {
                cfg.fullscreen_req = !cfg.fullscreen_req;

                sz_buttons[but_fullscreen] = cfg.fullscreen_req ? "True" : "False";
            }

            // Reflection
            ui_drawTextBox( menuFont, "Reflections:", buttonLeft, GFX_HEIGHT - 250, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 5, sz_buttons[but_reflections], menuFont, buttonLeft + 150, GFX_HEIGHT - 250, 100, 30 ) )
            {

                if ( cfg.reflect_allowed && 0 == cfg.reflect_fade && cfg.reflect_prt )
                {
                    cfg.reflect_allowed = bfalse;
                    cfg.reflect_fade = 255;
                    cfg.reflect_prt = bfalse;
                    sz_buttons[but_reflections] = "Off";
                }
                else
                {
                    if ( cfg.reflect_allowed && !cfg.reflect_prt )
                    {
                        sz_buttons[but_reflections] = "Medium";
                        cfg.reflect_fade = 255;
                        cfg.reflect_prt = btrue;
                    }
                    else
                    {
                        if ( cfg.reflect_allowed && cfg.reflect_fade == 255 && cfg.reflect_prt )
                        {
                            sz_buttons[but_reflections] = "High";
                            cfg.reflect_fade = 0;
                        }
                        else
                        {
                            cfg.reflect_allowed = btrue;
                            cfg.reflect_fade = 255;
                            sz_buttons[but_reflections] = "Low";
                            cfg.reflect_prt = bfalse;
                        }
                    }
                }
            }

            // Texture Filtering
            ui_drawTextBox( menuFont, "Texture Filtering:", buttonLeft, GFX_HEIGHT - 285, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 6, sz_buttons[but_filtering], menuFont, buttonLeft + 150, GFX_HEIGHT - 285, 130, 30 ) )
            {
                if ( cfg.texturefilter_req < TX_UNFILTERED )
                {
                    cfg.texturefilter_req = TX_UNFILTERED;
                }
                else
                {
                    cfg.texturefilter_req = ( TX_FILTERS )(( int )cfg.texturefilter_req + 1 );
                }

                if ( cfg.texturefilter_req > TX_ANISOTROPIC )
                {
                    cfg.texturefilter_req = TX_UNFILTERED;
                }

                switch ( cfg.texturefilter_req )
                {

                    case TX_UNFILTERED:
                        sz_buttons[but_filtering] = "Unfiltered";
                        break;

                    case TX_LINEAR:
                        sz_buttons[but_filtering] = "Linear";
                        break;

                    case TX_MIPMAP:
                        sz_buttons[but_filtering] = "Mipmap";
                        break;

                    case TX_BILINEAR:
                        sz_buttons[but_filtering] = "Bilinear";
                        break;

                    case TX_TRILINEAR_1:
                        sz_buttons[but_filtering] = "Trilinear 1";
                        break;

                    case TX_TRILINEAR_2:
                        sz_buttons[but_filtering] = "Trilinear 2";
                        break;

                    case TX_ANISOTROPIC:
                        sz_buttons[but_filtering] = "Anisotropic";
                        break;

                    default:
                        cfg.texturefilter_req = TX_UNFILTERED;
                        sz_buttons[but_filtering] = "Unfiltered";
                        break;
                }
            }

            // Shadows
            ui_drawTextBox( menuFont, "Shadows:", buttonLeft, GFX_HEIGHT - 320, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 7, sz_buttons[but_shadow], menuFont, buttonLeft + 150, GFX_HEIGHT - 320, 100, 30 ) )
            {
                if ( cfg.shadow_allowed && !cfg.shadow_sprite )
                {
                    cfg.shadow_allowed = bfalse;
                    cfg.shadow_sprite = bfalse;                // Just in case
                    sz_buttons[but_shadow] = "Off";
                }
                else
                {
                    if ( cfg.shadow_allowed && cfg.shadow_sprite )
                    {
                        sz_buttons[but_shadow] = "Best";
                        cfg.shadow_sprite = bfalse;
                    }
                    else
                    {
                        cfg.shadow_allowed = btrue;
                        cfg.shadow_sprite = btrue;
                        sz_buttons[but_shadow] = "Normal";
                    }
                }
            }

            // Z bit
            ui_drawTextBox( menuFont, "Z Bit:", buttonLeft + 300, GFX_HEIGHT - 320, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 8, sz_buttons[but_zbuffer], menuFont, buttonLeft + 450, GFX_HEIGHT - 320, 100, 30 ) )
            {
                if ( cfg.scrz_req < 0 )
                {
                    cfg.scrz_req = 8;
                }
                else
                {
                    cfg.scrz_req += 8;
                }

#if defined(__unix__)
                if ( cfg.scrz_req > 24 ) cfg.scrz_req = 8;          //Linux max is 24
#else
                if ( cfg.scrz_req > 32 ) cfg.scrz_req = 8;          //Others can have up to 32 bit!
#endif

                snprintf( Cscrz, SDL_arraysize( Cscrz ), "%d", cfg.scrz_req );
                sz_buttons[but_zbuffer] = Cscrz;
            }

            // Max dynamic lights
            ui_drawTextBox( menuFont, "Max Lights:", buttonLeft + 300, GFX_HEIGHT - 285, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 9, sz_buttons[but_maxlights], menuFont, buttonLeft + 450, GFX_HEIGHT - 285, 100, 30 ) )
            {
                if ( cfg.dyna_count_req < 16 )
                {
                    cfg.dyna_count_req = 16;
                }
                else
                {
                    cfg.dyna_count_req += 8;
                }

                if ( cfg.dyna_count_req > TOTAL_MAX_DYNA )
                {
                    cfg.dyna_count_req = 8;
                }

                snprintf( Cmaxdyna, SDL_arraysize( Cmaxdyna ), "%d", cfg.dyna_count_req );
                sz_buttons[but_maxlights] = Cmaxdyna;
            }

            // Perspective correction, overlay, underlay and phong mapping
            ui_drawTextBox( menuFont, "Special Effects:", buttonLeft + 300, GFX_HEIGHT - 250, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 10, sz_buttons[but_3dfx], menuFont, buttonLeft + 450, GFX_HEIGHT - 250, 100, 30 ) )
            {
                if ( cfg.use_phong && cfg.use_perspective && cfg.overlay_allowed && cfg.background_allowed )
                {
                    cfg.use_phong          = bfalse;
                    cfg.use_perspective    = bfalse;
                    cfg.overlay_allowed    = bfalse;
                    cfg.background_allowed = bfalse;
                    sz_buttons[but_3dfx] = "Off";
                }
                else
                {
                    if ( !cfg.use_phong )
                    {
                        sz_buttons[but_3dfx] = "Okay";
                        cfg.use_phong = btrue;
                    }
                    else
                    {
                        if ( !cfg.use_perspective && cfg.overlay_allowed && cfg.background_allowed )
                        {
                            sz_buttons[but_3dfx] = "Superb";
                            cfg.use_perspective = btrue;
                        }
                        else
                        {
                            cfg.overlay_allowed = btrue;
                            cfg.background_allowed = btrue;
                            sz_buttons[but_3dfx] = "Good";
                        }
                    }
                }
            }

            // Water Quality
            ui_drawTextBox( menuFont, "Good Water:", buttonLeft + 300, GFX_HEIGHT - 215, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 11, sz_buttons[but_multiwater], menuFont, buttonLeft + 450, GFX_HEIGHT - 215, 100, 30 ) )
            {
                if ( cfg.twolayerwater_allowed )
                {
                    sz_buttons[but_multiwater] = "Off";
                    cfg.twolayerwater_allowed = bfalse;
                }
                else
                {
                    sz_buttons[but_multiwater] = "On";
                    cfg.twolayerwater_allowed = btrue;
                }
            }

            // Max particles
            ui_drawTextBox( menuFont, "Max Particles:", buttonLeft + 300, GFX_HEIGHT - 180, 0, 0, 20 );

            if ( PMod->active )
            {
                snprintf( Cmaxparticles, SDL_arraysize( Cmaxparticles ), "%i (%i currently used)", maxparticles, maxparticles - prt_count_free() );
                ui_drawTextBox( menuFont, Cmaxparticles, buttonLeft + 450, GFX_HEIGHT - 180, 0, 100, 30 );
            }
            else if ( BUTTON_UP == ui_doButton( 15, sz_buttons[but_maxparticles], menuFont, buttonLeft + 450, GFX_HEIGHT - 180, 100, 30 ) )
            {
                if ( cfg.particle_count_req < 256 )
                {
                    cfg.particle_count_req = 256;
                }
                else
                {
                    cfg.particle_count_req += 128;
                }

                if ( cfg.particle_count_req > MAX_PRT ) cfg.particle_count_req = 256;

                snprintf( Cmaxparticles, SDL_arraysize( Cmaxparticles ), "%i", cfg.particle_count_req );  // Convert integer to a char we can use
                sz_buttons[but_maxparticles] =  Cmaxparticles;
            }

            // Widescreen
            ui_drawTextBox( menuFont, "Widescreen:", buttonLeft + 300, GFX_HEIGHT - 70, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 12, sz_buttons[but_widescreen], menuFont, buttonLeft + 450, GFX_HEIGHT - 70, 25, 25 ) )
            {
                bool_t old_widescreen = widescreen;

                // toggle widescreen
                widescreen = !widescreen;

                if ( old_widescreen )
                {
                    // switch the display from widescreen to non-widescreen
                    sz_buttons[but_widescreen] = " ";

                    // Set to default non-widescreen resolution
                    cfg.scrx_req = 800;
                    cfg.scry_req = 600;
                    sz_buttons[but_screensize] = "800x600";
                }
                else
                {
                    // switch the display from non-widescreen to widescreen
                    sz_buttons[but_widescreen] = "X";

                    // Set to "default" widescreen resolution
                    cfg.scrx_req = 960;
                    cfg.scry_req = 600;
                    sz_buttons[but_screensize] = "960x600";
                }
            }

            // Screen Resolution
            ui_drawTextBox( menuFont, "Resolution:", buttonLeft + 300, GFX_HEIGHT - 110, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 13, sz_buttons[but_screensize], menuFont, buttonLeft + 450, GFX_HEIGHT - 110, 125, 30 ) )
            {
                float req_area;

                cfg.scrx_req *= 1.1f;
                cfg.scry_req *= 1.1f;

                req_area = cfg.scrx_req * cfg.scry_req;

                // use 1920x1200 as a kind of max resolution
                if ( req_area > 1920 * 1200 )
                {
                    // reset the screen size to the minimum
                    if ( widescreen )
                    {
                        // "default" widescreen
                        cfg.scrx_req = 960;
                        cfg.scry_req = 600;
                    }
                    else
                    {
                        // "default"
                        cfg.scrx_req = 800;
                        cfg.scry_req = 600;
                    }
                }

                if ( cfg.fullscreen_req && NULL != sdl_scr.video_mode_list )
                {
                    // coerce the screen size to a valid fullscreen mode
                    doVideoOptions_fix_fullscreen_resolution( &cfg, &sdl_scr, &sz_screen_size );
                }
                else
                {
                    // just accept whatever we are given
                    snprintf( sz_screen_size, sizeof( sz_screen_size ), "%dx%d", cfg.scrx_req, cfg.scry_req );
                }

                sz_buttons[but_screensize] = sz_screen_size;

                aspect_ratio = ( float )cfg.scrx_req / ( float )cfg.scry_req;

                // 1.539 is "half way" between normal aspect ratio (4/3) and anamorphic (16/9)
                widescreen = ( aspect_ratio > ( 1.539f ) );

                if ( widescreen ) sz_buttons[but_widescreen] = "X";
                else              sz_buttons[but_widescreen] = " ";
            }

            // Save settings button
            if ( BUTTON_UP == ui_doButton( 14, "Save Settings", NULL, buttonLeft, GFX_HEIGHT - 60, 200, 30 ) )
            {
                menuChoice = 1;

                // synchronoze the config values with the various game subsystems
                setup_synch( &cfg );

                // save the setup file
                setup_upload( &cfg );
                setup_write();

                // Reload some of the graphics
                load_graphics();
            }
            if ( menuChoice != 0 )
            {
                menuState = MM_Leaving;
                mnu_SlidyButton_init( 0.0f, sz_buttons );
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            // Fall trough
            menuState = MM_Finish;
            break;

        case MM_Finish:
            // Free the background texture; don't need to hold onto it
            oglx_texture_Release( &background );
            menuState = MM_Begin;  // Make sure this all resets next time

            // reset the ui
            ui_Reset();

            // Set the next menu to load
            result = menuChoice;
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
int doShowResults( float deltaTime )
{
    static Font   *font;
    static int     menuState = MM_Begin;
    static int     count;
    static char*   game_hint;
    static char    buffer[1024] = EMPTY_CSTR;

    int menuResult = 0;

    switch ( menuState )
    {
        case MM_Begin:
            {
                Uint8 i;
                char * carat = buffer, * carat_end = buffer + SDL_arraysize( buffer );

                font = ui_getFont();
                count = 0;
                menuState = MM_Entering;

                // Prepeare the summary text
                for ( i = 0; i < SUMMARYLINES; i++ )
                {
                    carat += snprintf( carat, carat_end - carat - 1, "%s\n", mnu_ModList.lst[( MOD_REF )selectedModule].base.summary[i] );
                }

                // Randomize the next game hint, but only if not in hard mode
                game_hint = CSTR_END;
                if ( cfg.difficulty <= GAME_NORMAL )
                {
                    // Should be okay to randomize the seed here, the random seed isnt standarized or
                    // used elsewhere before the module is loaded.
                    srand( time( NULL ) );
                    if ( mnu_GameTip_load_local_vfs() )   game_hint = mnu_GameTip.local_hint[rand() % mnu_GameTip.local_count];
                    else if ( mnu_GameTip.count > 0 )     game_hint = mnu_GameTip.hint[rand() % mnu_GameTip.count];
                }
            }
            // pass through

        case MM_Entering:
            menuState = MM_Running;
            // pass through

        case MM_Running:
            {
                int text_h, text_w;
                ui_drawButton( UI_Nothing, 30, 30, GFX_WIDTH  - 60, GFX_HEIGHT - 65, NULL );

                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

                // the module name
                ui_drawTextBox( font, mnu_ModList.lst[( MOD_REF )selectedModule].base.longname, 50, 80, 291, 230, 20 );

                // Draw a text box
                ui_drawTextBox( menuFont, buffer, 50, 120, 291, 230, 20 );

                // Loading game... please wait
                fnt_getTextSize( font, "Loading module...", &text_w, &text_h );
                ui_drawTextBox( font, "Loading module...", ( GFX_WIDTH / 2 ) - text_w / 2, GFX_HEIGHT - 200, 0, 0, 20 );

                // Draw the game tip
                if ( VALID_CSTR( game_hint ) )
                {
                    fnt_getTextSize( menuFont, "GAME TIP", &text_w, &text_h );
                    ui_drawTextBox( font, "GAME TIP", ( GFX_WIDTH / 2 ) - ( text_w / 2 ), GFX_HEIGHT - 150, 0, 0, 20 );

                    fnt_getTextSize( menuFont, game_hint, &text_w, &text_h );       /// @todo ZF@> : this doesnt work as I intended, fnt_get_TextSize() does not take line breaks into account
                    ui_drawTextBox( menuFont, game_hint, ( GFX_WIDTH / 2 ) - ( text_w / 2 ), GFX_HEIGHT - 110, 0, 0, 10 );
                }

                // keep track of the iterations through this section for a timer
                count++;
                if ( count > UPDATE_SKIP )
                {
                    menuState  = MM_Leaving;
                }
            }
            break;

        case MM_Leaving:
            menuState = MM_Finish;
            // pass through

        case MM_Finish:
            menuResult = 1;
            menuState  = MM_Begin;
    }

    return menuResult;
}

//--------------------------------------------------------------------------------------------
int doNotImplemented( float deltaTime )
{
    int x, y;
    int w, h;
    char notImplementedMessage[] = "Not implemented yet!  Check back soon!";

    fnt_getTextSize( ui_getFont(), notImplementedMessage, &w, &h );
    w += 50; // add some space on the sides

    x = GFX_WIDTH  / 2 - w / 2;
    y = GFX_HEIGHT / 2 - 17;
    if ( BUTTON_UP == ui_doButton( 1, notImplementedMessage, NULL, x, y, w, 30 ) )
    {
        return 1;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
int doGamePaused( float deltaTime )
{
    static int menuState = MM_Begin;
    static int menuChoice = 0;

    static const char * buttons[] =
    {
        "Quit Module",
        "Restart Module",
        "Return to Module",
        "Options",
        ""
    };

    int result = 0, cnt;

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            menuChoice = 0;
            menuState = MM_Entering;

            if ( PMod->exportvalid && !local_stats.allpladead ) buttons[0] = "Save and Exit";
            else                                                buttons[0] = "Quit Module";

            mnu_SlidyButton_init( 1.0f, buttons );

        case MM_Entering:
            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( -deltaTime );

            // Let lerp wind down relative to the time elapsed
            if ( mnu_SlidyButtonState.lerp <= 0.0f )
            {
                menuState = MM_Running;
            }
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            // Buttons
            for ( cnt = 0; cnt < 4; cnt ++ )
            {
                if ( BUTTON_UP == ui_doButton( cnt + 1, buttons[cnt], NULL, buttonLeft, buttonTop + ( cnt * 35 ), 200, 30 ) )
                {
                    // audio options
                    menuChoice = cnt + 1;
                }
            }

            // Quick return to game
            if ( SDLKEYDOWN( SDLK_ESCAPE ) ) menuChoice = 3;

            if ( menuChoice != 0 )
            {
                menuState = MM_Leaving;
                mnu_SlidyButton_init( 0.0f, buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            // Buttons
            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( deltaTime );
            if ( mnu_SlidyButtonState.lerp >= 1.0f )
            {
                menuState = MM_Finish;
            }
            break;

        case MM_Finish:
            // Free the background texture; don't need to hold onto it
            menuState = MM_Begin;  // Make sure this all resets next time

            // reset the ui
            ui_Reset();

            // Set the next menu to load
            result = menuChoice;
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
int doShowEndgame( float deltaTime )
{
    static int menuState = MM_Begin;
    static int menuChoice = 0;
    static int x, y, w, h;
    static Font *font;

    static const char * buttons[] =
    {
        "BLAH",
        ""
    };

    int cnt, retval;

    retval = 0;
    switch ( menuState )
    {
        case MM_Begin:
            menuState = MM_Entering;
            font = ui_getFont();

            mnu_SlidyButton_init( 1.0f, buttons );

            if ( PMod->exportvalid )
            {
                buttons[0] = "Save and Exit";
            }
            else
            {
                buttons[0] = "Exit Game";
            }

            x = 70;
            y = 70;
            w = GFX_WIDTH  - 2 * x;
            h = GFX_HEIGHT - 2 * y;

        case MM_Entering:

            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            ui_drawTextBox( NULL, endtext, x, y, w, h, 20 );
            mnu_SlidyButton_draw_all();

            mnu_SlidyButton_update_all( -deltaTime );

            // Let lerp wind down relative to the time elapsed
            if ( mnu_SlidyButtonState.lerp <= 0.0f )
            {
                menuState = MM_Running;
            }
            break;

        case MM_Running:
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            // Buttons
            for ( cnt = 0; cnt < 1; cnt ++ )
            {
                if ( BUTTON_UP == ui_doButton( cnt + 1, buttons[cnt], NULL, buttonLeft, buttonTop + ( cnt * 35 ), 200, 30 ) )
                {
                    // audio options
                    menuChoice = cnt + 1;
                    menuState = MM_Leaving;
                }
            }

            // escape also kills this menu
            if ( SDLKEYDOWN( SDLK_ESCAPE ) )
            {
                menuChoice = 1;
                menuState = MM_Leaving;
            }

            ui_drawTextBox( NULL, endtext, x, y, w, h, 20 );

            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - mnu_SlidyButtonState.lerp );

            ui_drawTextBox( NULL, endtext, x, y, w, h, 20 );

            // Buttons
            mnu_SlidyButton_draw_all();
            mnu_SlidyButton_update_all( deltaTime );
            if ( mnu_SlidyButtonState.lerp >= 1.0f )
            {
                menuState = MM_Finish;
            }
            break;

        case MM_Finish:
            {
                bool_t reloaded = bfalse;

                // try to pop the last module off the module stack
                reloaded = link_pop_module();

                // try to go to the world map
                // if( !reloaded )
                // {
                //    reloaded = link_load_parent( mnu_ModList.lst[pickedmodule_index].base.parent_modname, mnu_ModList.lst[pickedmodule_index].base.parent_pos );
                // }

                // fix the menu that is returned when you break out of the game
                if ( PMod->beat && start_new_player )
                {
                    // we started with a new player and beat the module... yay!
                    // now we want to graduate to the ChoosePlayer menu to
                    // build our party

                    start_new_player = bfalse;

                    // if we beat a beginner module, we want to
                    // go to ChoosePlayer instead of ChooseModule.
                    if ( mnu_stack_peek() == emnu_ChooseModule )
                    {
                        mnu_stack_pop();
                        mnu_stack_push( emnu_ChoosePlayer );
                    }
                }

                // actually quit the module
                if ( !reloaded )
                {
                    game_finish_module();
                    pickedmodule_index = -1;
                    process_kill( PROC_PBASE( GProc ) );
                }

                menuState = MM_Begin;

                // Set the next menu to load
                retval = menuChoice;
            }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// place this last so that we do not have to prototype every menu function
int doMenu( float deltaTime )
{
    /// @details the global function that controls the navigation between menus

    int retval, result = 0;

    if ( mnu_whichMenu == emnu_Main )
    {
        mnu_stack_clear();
    };

    retval = MENU_NOTHING;

    switch ( mnu_whichMenu )
    {
        case emnu_Main:
            result = doMainMenu( deltaTime );
            if ( result != 0 )
            {
                if ( 1 == result )      { mnu_begin_menu( emnu_ChooseModule ); start_new_player = btrue; }
                else if ( 2 == result ) { mnu_begin_menu( emnu_ChoosePlayer ); start_new_player = bfalse; }
                else if ( 3 == result ) { mnu_begin_menu( emnu_Options ); }
                else if ( 4 == result ) retval = MENU_QUIT;  // need to request a quit somehow
            }
            break;

        case emnu_SinglePlayer:
            result = doSinglePlayerMenu( deltaTime );

            if ( result != 0 )
            {
                if ( 1 == result )
                {
                    mnu_begin_menu( emnu_ChooseModule );
                    start_new_player = btrue;
                }
                else if ( 2 == result )
                {
                    mnu_begin_menu( emnu_ChoosePlayer );
                    start_new_player = bfalse;
                }
                else if ( 3 == result )
                {
                    mnu_end_menu();
                    retval = MENU_END;
                }
                else
                {
                    mnu_begin_menu( emnu_NewPlayer );
                }
            }
            break;

        case emnu_ChooseModule:
            result = doChooseModule( deltaTime );

            if ( -1 == result )     { mnu_end_menu(); retval = MENU_END; }
            else if ( 1 == result ) mnu_begin_menu( emnu_ShowMenuResults );  // imports are not valid (starter module)
            else if ( 2 == result ) mnu_begin_menu( emnu_ShowMenuResults );  // imports are valid

            break;

        case emnu_ChoosePlayer:
            result = doChoosePlayer( deltaTime );

            if ( -1 == result )     { mnu_end_menu(); retval = MENU_END; }
            else if ( 1 == result ) mnu_begin_menu( emnu_ChooseModule );

            break;

        case emnu_Options:
            result = doOptions( deltaTime );
            if ( result != 0 )
            {
                if ( 1 == result )      mnu_begin_menu( emnu_AudioOptions );
                else if ( 2 == result ) mnu_begin_menu( emnu_InputOptions );
                else if ( 3 == result ) mnu_begin_menu( emnu_VideoOptions );
                else if ( 4 == result ) { mnu_end_menu(); retval = MENU_END; }
                else if ( 5 == result ) mnu_begin_menu( emnu_GameOptions );
            }
            break;

        case emnu_GameOptions:
            result = doGameOptions( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_AudioOptions:
            result = doAudioOptions( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_VideoOptions:
            result = doVideoOptions( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_InputOptions:
            result = doInputOptions( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_ShowMenuResults:
            result = doShowResults( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_SELECT;
            }
            break;

        case emnu_GamePaused:
            result = doGamePaused( deltaTime );
            if ( result != 0 )
            {
                if ( 1 == result )
                {
                    // "Quit Module"

                    bool_t reloaded = bfalse;

                    mnu_end_menu();

                    // try to pop the last module off the module stack
                    reloaded = link_pop_module();

                    // try to go to the world map
                    // if( !reloaded )
                    // {
                    //    reloaded = link_load_parent( mnu_ModList.lst[pickedmodule_index].base.parent_modname, mnu_ModList.lst[pickedmodule_index].base.parent_pos );
                    // }

                    if ( !reloaded )
                    {
                        game_finish_module();
                        process_kill( PROC_PBASE( GProc ) );
                    }

                    result = MENU_QUIT;
                }
                else if ( 2 == result )
                {
                    // "Restart Module"
                    mnu_end_menu();

                    //Simply quit the current module and begin it again
                    game_quit_module();
                    game_begin_module( PMod->loadname, ( Uint32 )~0 );

                    retval = MENU_END;
                }
                else if ( 3 == result )
                {
                    // "Return to Module"
                    mnu_end_menu();
                    retval = MENU_END;
                }
                else if ( 4 == result )
                {
                    // "Options"
                    mnu_begin_menu( emnu_Options );
                }
            }
            break;

        case emnu_ShowEndgame:
            result = doShowEndgame( deltaTime );
            if ( 1 == result )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
            break;

        case emnu_NotImplemented:
        default:
            result = doNotImplemented( deltaTime );
            if ( result != 0 )
            {
                mnu_end_menu();
                retval = MENU_END;
            }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// Auto formatting functions
//--------------------------------------------------------------------------------------------
void autoformat_init( gfx_config_t * pgfx )
{
    autoformat_init_slidy_buttons();
    autoformat_init_tip_text();
    autoformat_init_copyright_text();

    if ( NULL != pgfx )
    {
        ui_set_virtual_screen( pgfx->vw, pgfx->vh, GFX_WIDTH, GFX_HEIGHT );
    }
}

//--------------------------------------------------------------------------------------------
void autoformat_init_slidy_buttons()
{
    // Figure out where to draw the buttons
    buttonLeft = 40;
    buttonTop = GFX_HEIGHT - 20;
}

//--------------------------------------------------------------------------------------------
void autoformat_init_tip_text()
{
    // set the text
    tipText = NULL;

    // Draw the options text to the right of the buttons
    tipTextLeft = 280;

    // And relative to the bottom of the screen
    tipTextTop = GFX_HEIGHT;
}

//--------------------------------------------------------------------------------------------
void autoformat_init_copyright_text()
{
    // set the text
    copyrightText = "Welcome to Egoboo!\nhttp://egoboo.sourceforge.net\nVersion " VERSION "\n";

    // Draw the copyright text to the right of the buttons
    copyrightLeft = 280;

    // And relative to the bottom of the screen
    copyrightTop = GFX_HEIGHT;
}

//--------------------------------------------------------------------------------------------
// Implementation of tipText
//--------------------------------------------------------------------------------------------
void tipText_set_position( Font * font, const char * text, int spacing )
{
    int w, h;

    autoformat_init_tip_text();

    if ( NULL == text ) return;

    fnt_getTextBoxSize( font, text, spacing, &w, &h );

    // set the text
    tipText = text;

    // Draw the options text to the right of the buttons
    tipTextLeft = 280;

    // And relative to the bottom of the screen
    tipTextTop = GFX_HEIGHT - h - spacing;
}

//--------------------------------------------------------------------------------------------
// Implementation of copyrightText
//--------------------------------------------------------------------------------------------
void copyrightText_set_position( Font * font, const char * text, int spacing )
{
    int w, h;

    autoformat_init_copyright_text();

    if ( NULL == text ) return;

    copyrightLeft = 0;
    copyrightLeft = 0;

    // Figure out where to draw the copyright text
    fnt_getTextBoxSize( font, text, 20, &w, &h );

    // set the text
    copyrightText = text;

    // Draw the copyright text to the right of the buttons
    copyrightLeft = 280;

    // And relative to the bottom of the screen
    copyrightTop = GFX_HEIGHT - h - spacing;
}

//--------------------------------------------------------------------------------------------
// Asset management
//--------------------------------------------------------------------------------------------
void mnu_load_all_module_images_vfs( LoadPlayer_list_t * lp_lst )
{
    /// @details ZZ@> This function loads the title image for each module.  Modules without a
    ///     title are marked as invalid

    STRING loadname;
    MOD_REF imod;
    vfs_FILE* filesave;

    // release all allocated data from the mnu_ModList and empty the list
    mnu_ModList_release_images();

    // Log a directory list
    filesave = vfs_openWrite( "/debug/modules.txt" );
    if ( NULL != filesave )
    {
        vfs_printf( filesave, "This file logs all of the modules found\n" );
        vfs_printf( filesave, "** Denotes an invalid module\n" );
        vfs_printf( filesave, "## Denotes an unlockable module\n\n" );
    }

    // load all the title images for modules that we are going to display
    for ( imod = 0; imod < mnu_ModList.count; imod++ )
    {
        if ( !mnu_ModList.lst[imod].loaded )
        {
            vfs_printf( filesave, "**.  %s\n", mnu_ModList.lst[imod].vfs_path );
        }
        else if ( mnu_test_module_by_index( lp_lst, imod, 0, NULL ) )
        {
            // @note just because we can't load the title image DOES NOT mean that we ignore the module
            snprintf( loadname, SDL_arraysize( loadname ), "%s/gamedat/title", mnu_ModList.lst[imod].vfs_path );

            mnu_ModList.lst[imod].tex_index = TxTitleImage_load_one_vfs( loadname );

            vfs_printf( filesave, "%02d.  %s\n", REF_TO_INT( imod ), mnu_ModList.lst[imod].vfs_path );
        }
        else
        {
            vfs_printf( filesave, "##.  %s\n", mnu_ModList.lst[imod].vfs_path );
        }
    }

    if ( NULL != filesave )
    {
        vfs_close( filesave );
    }
}

//--------------------------------------------------------------------------------------------
TX_REF mnu_get_icon_ref( const CAP_REF icap, const TX_REF default_ref )
{
    /// @details BB@> This function gets the proper icon for a an object profile.
    //
    //     In the character preview section of the menu system, we do not load
    //     entire profiles, just the character definition file ("data.txt")
    //     and one icon. Sometimes, though the item is actually a spell effect which means
    //     that we need to display the book icon.

    TX_REF icon_ref = ( TX_REF )ICON_NULL;
    bool_t is_spell_fx, is_book, draw_book;

    cap_t * pitem_cap;

    if ( !LOADED_CAP( icap ) ) return icon_ref;
    pitem_cap = CapStack.lst + icap;

    // what do we need to draw?
    is_spell_fx = ( NO_SKIN_OVERRIDE != pitem_cap->spelleffect_type );
    is_book     = ( SPELLBOOK == icap );
    draw_book   = ( is_book || is_spell_fx ) && ( bookicon_count > 0 );

    if ( !draw_book )
    {
        icon_ref = default_ref;
    }
    else if ( draw_book )
    {
        int iskin = 0;

        if ( NO_SKIN_OVERRIDE != pitem_cap->spelleffect_type )
        {
            iskin = pitem_cap->spelleffect_type;
        }
        else if ( NO_SKIN_OVERRIDE != pitem_cap->skin_override )
        {
            iskin = pitem_cap->skin_override;
        }

        iskin = CLIP( iskin, 0, bookicon_count );

        icon_ref = bookicon_ref[ iskin ];
    }

    return icon_ref;
}

//--------------------------------------------------------------------------------------------
// module utilities
//--------------------------------------------------------------------------------------------
int mnu_get_mod_number( const char *szModName )
{
    /// @details ZZ@> This function returns -1 if the module does not exist locally, the module
    ///    index otherwise

    MOD_REF modnum;
    int     retval = -1;

    for ( modnum = 0; modnum < mnu_ModList.count; modnum++ )
    {
        if ( 0 == strcmp( mnu_ModList.lst[modnum].vfs_path, szModName ) )
        {
            retval = REF_TO_INT( modnum );
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_test_module_by_index( LoadPlayer_list_t * lp_lst, const MOD_REF modnumber, size_t buffer_len, char * buffer )
{
    int            cnt;
    mnu_module_t * pmod;
    bool_t         allowed;

    if ( INVALID_MOD( modnumber ) ) return bfalse;
    pmod = mnu_ModList.lst + modnumber;

    // First check if we are in developers mode or that the right module has been beaten before
    allowed = bfalse;

    if ( cfg.dev_mode )
    {
        allowed = btrue;
    }

    if ( !allowed )
    {
        if ( module_has_idsz_vfs( pmod->base.reference, pmod->base.unlockquest.id, buffer_len, buffer ) )
        {
            allowed = btrue;
        }
    }

    if ( !allowed && pmod->base.importamount > 0 )
    {
        int player_count = 0;
        int player_allowed = 0;

        // If that did not work, then check all selected players directories, but only if it isn't a starter module
        for ( cnt = 0; cnt < lp_lst->count; cnt++ )
        {
            int                    quest_level = QUEST_NONE;
            LoadPlayer_element_t * ptr         = lp_lst->lst + cnt;

            player_count++;

            quest_level = quest_get_level( ptr->quest_log, SDL_arraysize( ptr->quest_log ), pmod->base.unlockquest.id );

            // find beaten quests or quests with proper level
            if ( quest_level <= QUEST_BEATEN || pmod->base.unlockquest.level <= quest_level )
            {
                player_allowed++;
            }
        }

        allowed = ( player_allowed == player_count );
    }

    return allowed;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_test_module_by_name( LoadPlayer_list_t * lp_lst, const char *szModName )
{
    /// @details ZZ@> This function tests to see if a module can be entered by
    ///    the players

    bool_t retval;

    // find the module by name
    int modnumber = mnu_get_mod_number( szModName );

    retval = bfalse;
    if ( modnumber >= 0 )
    {
        retval = mnu_test_module_by_index( lp_lst, ( MOD_REF )modnumber, 0, NULL );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void mnu_module_init( mnu_module_t * pmod )
{
    if ( NULL == pmod ) return;

    // clear the module
    memset( pmod, 0, sizeof( *pmod ) );

    pmod->tex_index = INVALID_TITLE_TEXTURE;
}

//--------------------------------------------------------------------------------------------
void mnu_load_all_module_info()
{
    vfs_search_context_t * ctxt;

    const char *vfs_ModPath;
    STRING      loadname;

    // reset the module list
    mnu_ModList_release_all();

    // Search for all .mod directories and load the module info
    ctxt = vfs_findFirst( "mp_modules", "mod", VFS_SEARCH_DIR );
    vfs_ModPath = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( vfs_ModPath ) && mnu_ModList.count < MAX_MODULE )
    {
        mnu_module_t * pmod = mnu_ModList.lst + ( MOD_REF )mnu_ModList.count;

        // clear the module
        mnu_module_init( pmod );

        // save the filename
        snprintf( loadname, SDL_arraysize( loadname ), "%s/gamedat/menu.txt", vfs_ModPath );
        if ( NULL != module_load_info_vfs( loadname, &( pmod->base ) ) )
        {
            mnu_ModList.count++;

            // mark the module data as loaded
            pmod->loaded = btrue;

            // save the module path
            strncpy( pmod->vfs_path, vfs_ModPath, SDL_arraysize( pmod->vfs_path ) );

            // Save the user data directory version of the module path.
            // @note This is kinda a cheat since we know that the virtual paths all begin with "mp_" at the moment.
            // If that changes, this line must be changed as well.
            snprintf( pmod->dest_path, SDL_arraysize( pmod->dest_path ), "/%s", vfs_ModPath + 3 );

            // same problem as above
            strncpy( pmod->name, vfs_ModPath + 11, SDL_arraysize( pmod->name ) );
        };

        ctxt = vfs_findNext( &ctxt );
        vfs_ModPath = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );

    module_list_valid = btrue;
}

//--------------------------------------------------------------------------------------------
void mnu_release_one_module( const MOD_REF imod )
{
    mnu_module_t * pmod;

    if ( !VALID_MOD( imod ) ) return;
    pmod = mnu_ModList.lst + imod;

    TxTitleImage_release_one( pmod->tex_index );
    pmod->tex_index = INVALID_TITLE_TEXTURE;
}

//--------------------------------------------------------------------------------------------
// Implementation of the ModList struct
//--------------------------------------------------------------------------------------------
mod_file_t * mnu_ModList_get_base( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return &( mnu_ModList.lst[( MOD_REF )imod].base );
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_vfs_path( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return mnu_ModList.lst[( MOD_REF )imod].vfs_path;
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_dest_path( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return mnu_ModList.lst[( MOD_REF )imod].dest_path;
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_name( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return mnu_ModList.lst[( MOD_REF )imod].name;
}

//--------------------------------------------------------------------------------------------
void mnu_ModList_release_all()
{
    MOD_REF cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        // release any allocated data
        if ( cnt < mnu_ModList.count )
        {
            mnu_release_one_module( cnt );
        }

        memset( mnu_ModList.lst + cnt, 0, sizeof( mnu_module_t ) );
    }

    mnu_ModList.count = 0;
}

//--------------------------------------------------------------------------------------------
void mnu_ModList_release_images()
{
    MOD_REF cnt;
    int tnc;

    tnc = -1;
    for ( cnt = 0; cnt < mnu_ModList.count; cnt++ )
    {
        if ( !mnu_ModList.lst[cnt].loaded ) continue;
        tnc = REF_TO_INT( cnt );

        TxTitleImage_release_one( mnu_ModList.lst[cnt].tex_index );
        mnu_ModList.lst[cnt].tex_index = INVALID_TITLE_TEXTURE;
    }
    TxTitleImage.count = 0;

    // make sure that mnu_ModList.count is the right size, in case some modules were unloaded?
    mnu_ModList.count = tnc + 1;
}

//--------------------------------------------------------------------------------------------
// Functions for implementing the TxTitleImage array of textures
//--------------------------------------------------------------------------------------------
void TxTitleImage_clear_data()
{
    TxTitleImage.count = 0;
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_ctor()
{
    /// @details ZZ@> This function clears out all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        oglx_texture_ctor( TxTitleImage.lst + cnt );
    }

    TxTitleImage_clear_data();
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_release_one( const TX_REF index )
{
    if ( index < 0 || index >= MAX_MODULE ) return;

    oglx_texture_Release( TxTitleImage.lst + index );
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_release_all()
{
    /// @details ZZ@> This function releases all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        TxTitleImage_release_one( cnt );
    }

    TxTitleImage_clear_data();
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_dtor()
{
    /// @details ZZ@> This function clears out all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        oglx_texture_dtor( TxTitleImage.lst + cnt );
    }

    TxTitleImage_clear_data();
}

//--------------------------------------------------------------------------------------------
TX_REF TxTitleImage_load_one_vfs( const char *szLoadName )
{
    /// @details ZZ@> This function loads a title in the specified image slot, forcing it into
    ///    system memory.  Returns btrue if it worked

    TX_REF itex;

    if ( INVALID_CSTR( szLoadName ) ) return ( TX_REF )INVALID_TITLE_TEXTURE;

    if ( TxTitleImage.count >= TITLE_TEXTURE_COUNT ) return ( TX_REF )INVALID_TITLE_TEXTURE;

    itex  = ( TX_REF )TxTitleImage.count;
    if ( INVALID_GL_ID != ego_texture_load_vfs( TxTitleImage.lst + itex, szLoadName, INVALID_KEY ) )
    {
        TxTitleImage.count++;
    }
    else
    {
        itex = ( TX_REF )INVALID_TITLE_TEXTURE;
    }

    return itex;
}

//--------------------------------------------------------------------------------------------
oglx_texture_t * TxTitleImage_get_ptr( const TX_REF itex )
{
    if ( itex >= TxTitleImage.count || itex >= MAX_MODULE ) return NULL;

    return TxTitleImage.lst + itex;
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_reload_all()
{
    /// @details ZZ@> This function re-loads all the current textures back into
    ///               OpenGL texture memory using the cached SDL surfaces

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_TEXTURE_COUNT; cnt++ )
    {
        oglx_texture_t * ptex = TxTitleImage.lst + cnt;

        if ( ptex->valid )
        {
            oglx_texture_Convert( ptex, ptex->surface, INVALID_KEY );
        }
    }
}

//--------------------------------------------------------------------------------------------
// Implementation of the mnu_GameTip system
//--------------------------------------------------------------------------------------------
void mnu_GameTip_load_global_vfs()
{
    /// ZF@> This function loads all of the game hints and tips
    STRING buffer;
    vfs_FILE *fileread;
    Uint8 cnt;

    // reset the count
    mnu_GameTip.count = 0;

    // Open the file with all the tips
    fileread = vfs_openRead( "mp_data/gametips.txt" );
    if ( NULL == fileread )
    {
        log_warning( "Could not load the game tips and hints. (\"mp_data/gametips.txt\")\n" );
        return;
    }

    // Load the data
    for ( cnt = 0; cnt < MENU_MAX_GAMETIPS && !vfs_eof( fileread ); cnt++ )
    {
        if ( goto_colon( NULL, fileread, btrue ) )
        {
            //Read the line
            fget_string( fileread, buffer, SDL_arraysize( buffer ) );
            strcpy( mnu_GameTip.hint[cnt], buffer );

            //Make it look nice
            str_decode( mnu_GameTip.hint[cnt], SDL_arraysize( mnu_GameTip.hint[cnt] ), mnu_GameTip.hint[cnt] );
            //str_add_linebreaks( mnu_GameTip.hint[cnt], SDL_arraysize( mnu_GameTip.hint[cnt] ), 50 );

            //Keep track of how many we have total
            mnu_GameTip.count++;
        }
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
bool_t mnu_GameTip_load_local_vfs()
{
    /// ZF@> This function loads all module specific hints and tips. If this fails, the game will
    //       default to the global hints and tips instead

    STRING buffer;
    vfs_FILE *fileread;
    Uint8 cnt;

    // reset the count
    mnu_GameTip.local_count = 0;

    // Open all the tips
    snprintf( buffer, SDL_arraysize( buffer ), "mp_modules/%s/gamedat/gametips.txt", pickedmodule_name );
    fileread = vfs_openRead( buffer );
    if ( NULL == fileread ) return bfalse;

    // Load the data
    for ( cnt = 0; cnt < MENU_MAX_GAMETIPS && !vfs_eof( fileread ); cnt++ )
    {
        if ( goto_colon( NULL, fileread, btrue ) )
        {
            //Read the line
            fget_string( fileread, buffer, SDL_arraysize( buffer ) );
            strcpy( mnu_GameTip.local_hint[cnt], buffer );

            //Make it look nice
            str_decode( mnu_GameTip.local_hint[cnt], SDL_arraysize( mnu_GameTip.local_hint[cnt] ), mnu_GameTip.local_hint[cnt] );
            //str_add_linebreaks( mnu_GameTip.local_hint[cnt], SDL_arraysize( mnu_GameTip.local_hint[cnt] ), 50 );

            //Keep track of how many we have total
            mnu_GameTip.local_count++;
        }
    }

    vfs_close( fileread );

    return mnu_GameTip.local_count > 0;
}

//--------------------------------------------------------------------------------------------
// Implementation of the mnu_SlidyButton array
//--------------------------------------------------------------------------------------------
void mnu_SlidyButton_init( float lerp, const char *button_text[] )
{
    int i;

    autoformat_init_slidy_buttons();

    // Figure out where to draw the buttons
    for ( i = 0; button_text[i][0] != 0; i++ )
    {
        buttonTop -= 35;
    }

    mnu_SlidyButtonState.lerp = lerp;
    mnu_SlidyButtonState.buttons = ( char** )button_text;
}

//--------------------------------------------------------------------------------------------
void mnu_SlidyButton_update_all( float deltaTime )
{
    mnu_SlidyButtonState.lerp += ( deltaTime * 1.5f );
}

//--------------------------------------------------------------------------------------------
void mnu_SlidyButton_draw_all()
{
    int i;

    for ( i = 0; mnu_SlidyButtonState.buttons[i][0] != 0; i++ )
    {
        int x = buttonLeft - ( 360 - i * 35 )  * mnu_SlidyButtonState.lerp;
        int y = buttonTop + ( i * 35 );

        ui_doButton( UI_Nothing, mnu_SlidyButtonState.buttons[i], NULL, x, y, 200, 30 );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ChoosePlayer_element_t * ChoosePlayer_ctor( ChoosePlayer_element_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ChoosePlayer_init( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ChoosePlayer_element_t * ChoosePlayer_dtor( ChoosePlayer_element_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ChoosePlayer_dealloc( ptr );

    ChoosePlayer_init( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool_t ChoosePlayer_init( ChoosePlayer_element_t * ptr )
{
    if ( NULL == ptr ) return bfalse;

    memset( ptr, 0, sizeof( *ptr ) );

    ptr->cap_ref  = MAX_CAP;
    ptr->tx_ref   = INVALID_TX_TEXTURE;
    ptr->skin_ref = NO_SKIN_OVERRIDE;

    chop_definition_init( &( ptr->chop ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ChoosePlayer_dealloc( ChoosePlayer_element_t * ptr )
{
    // release all allocated resources

    if ( MAX_CAP != ptr->cap_ref )
    {
        release_one_cap( ptr->cap_ref );
    }
    ptr->cap_ref = MAX_CAP;

    if ( INVALID_TX_TEXTURE != ptr->tx_ref )
    {
        TxTexture_free_one( ptr->tx_ref );
    }
    ptr->tx_ref = INVALID_TX_TEXTURE;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ChoosePlayer_list_t * ChoosePlayer_list_dealloc( ChoosePlayer_list_t * chooseplayer )
{
    int i;
    ChoosePlayer_element_t * chooseplayer_ptr;

    if ( NULL == chooseplayer ) return chooseplayer;

    // release any data that we have accumulated
    for ( i = 0; i < chooseplayer->count; i++ )
    {
        chooseplayer_ptr = chooseplayer->lst + i;

        // don't release the first index, since we don't own that one
        if ( 0 == i )
        {
            ChoosePlayer_init( chooseplayer_ptr );
        }
        else
        {
            ChoosePlayer_dtor( chooseplayer_ptr );
        }
    }
    chooseplayer->count = 0;

    return chooseplayer;
}

//--------------------------------------------------------------------------------------------
// Implementation of the mnu_loadplayer array
//--------------------------------------------------------------------------------------------
egoboo_rv LoadPlayer_list_init( LoadPlayer_list_t * lst )
{
    if ( NULL == lst ) return rv_error;

    // restart from nothing
    LoadPlayer_list_dealloc( lst );

    chop_data_init( &chop_mem );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv LoadPlayer_list_import_one( LoadPlayer_list_t * lst, const char * foundfile )
{
    STRING  filename;
    int     slot;

    CAP_REF  icap = MAX_CAP;
    cap_t  * pcap = NULL;

    LoadPlayer_element_t * ptr = NULL;
    int                    idx = MAX_LOADPLAYER;

    // valid mnu_loadplayer list?
    if ( NULL == lst ) return rv_error;

    // is it a valid filename?
    if ( !VALID_CSTR( foundfile ) ) return rv_error;

    // does the directory exist?
    if ( lst->count >= MAX_LOADPLAYER ) return rv_fail;

    // does the directory exist?
    if ( !vfs_exists( foundfile ) ) return rv_fail;

    // offset the slots so that ChoosePlayer will have space to load the inventory objects
    slot = ( MAXIMPORTOBJECTS + 2 ) + lst->count;

    // try to load the character profile
    icap = load_one_character_profile_vfs( foundfile, slot, bfalse );
    if ( !LOADED_CAP( icap ) ) return rv_fail;
    pcap = CapStack.lst + icap;

    // get the next index
    idx = LoadPlayer_list_get_free( lst );
    if ( idx < 0 ) return rv_fail;

    // grab a valid mnu_loadplayer pointer
    ptr = lst->lst + idx;

    // set the player directory
    snprintf( ptr->dir, SDL_arraysize( ptr->dir ), "%s", str_convert_slash_net(( char* )foundfile, strlen( foundfile ) ) );

    // set the loaded character profile for this object
    ptr->cap_ref = icap;

    // read in the skin from "skin.txt"
    // We are no longer supporting skin.txt. Use the [SKIN] expansion, instead
    //snprintf( filename, SDL_arraysize( filename ), "%s/skin.txt", foundfile );
    //ptr->skin_ref = read_skin_vfs( filename );

    // get the skin from the [SKIN] expansion in the character profile
    ptr->skin_ref = pcap->skin_override % MAX_SKIN;
    ptr->skin_ref = CLIP( ptr->skin_ref, 0, MAX_SKIN - 1 );

    // don't load in the md2 at this time
    //snprintf( filename, SDL_arraysize(filename), "%s" SLASH_STR "tris.md2", foundfile );
    //md2_load_one( vfs_resolveReadFilename(filename), &(MadStack.lst[idx].md2_data) );

    // load in just the one icon
    snprintf( filename, SDL_arraysize( filename ), "%s/icon%d", foundfile, ptr->skin_ref );
    ptr->tx_ref = TxTexture_load_one_vfs( filename, ( TX_REF )INVALID_TX_TEXTURE, INVALID_KEY );

    // load the quest info from "quest.txt" so we can determine the valid modules
    snprintf( ptr->dir, SDL_arraysize( ptr->dir ), "%s", str_convert_slash_net(( char* )foundfile, strlen( foundfile ) ) );
    quest_log_download_vfs( ptr->quest_log, SDL_arraysize( ptr->quest_log ), ptr->dir );

    // load the chop data from "naming.txt" to generate the character name
    snprintf( filename, SDL_arraysize( filename ), "%s/naming.txt", foundfile );
    chop_load_vfs( &chop_mem, filename, &( ptr->chop ) );

    // generate the name from the chop
    snprintf( ptr->name, SDL_arraysize( ptr->name ), "%s", chop_create( &chop_mem, &( ptr->chop ) ) );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
int LoadPlayer_list_get_free( LoadPlayer_list_t * lst )
{
    int idx = -1;

    if ( NULL == lst ) return -1;

    // are there any loadplayers left?
    if ( lst->count >= MAX_LOADPLAYER ) return -1;

    // grab the next one
    idx = lst->count;
    lst->count++;

    return idx;
}

//--------------------------------------------------------------------------------------------
LoadPlayer_element_t * LoadPlayer_list_get_ptr( LoadPlayer_list_t * lst, int idx )
{

    if ( !VALID_LOADPLAYER_IDX( mnu_loadplayer, idx ) ) return NULL;

    return lst->lst + idx;
}

//--------------------------------------------------------------------------------------------
egoboo_rv LoadPlayer_list_dealloc( LoadPlayer_list_t * lst )
{
    int i;

    if ( NULL == lst ) return rv_error;

    if ( 0 == lst->count ) return rv_success;

    lst->count = MIN( lst->count, MAX_LOADPLAYER );
    for ( i = 0; i < lst->count; i++ )
    {
        LoadPlayer_element_dtor( lst->lst + i );
    }
    lst->count = 0;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv LoadPlayer_list_import_all( LoadPlayer_list_t * lst, const char *dirname, bool_t initialize )
{
    /// @details ZZ@> This function figures out which players may be imported, and loads basic
    ///     data for each

    vfs_search_context_t * ctxt;
    const char *foundfile;

    if ( NULL == lst ) return rv_error;

    if ( initialize )
    {
        LoadPlayer_list_init( lst );
    };

    // Search for all objects
    ctxt = vfs_findFirst( dirname, "obj", VFS_SEARCH_DIR );
    foundfile = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( foundfile ) && lst->count < MAX_LOADPLAYER )
    {
        LoadPlayer_list_import_one( lst, foundfile );

        ctxt = vfs_findNext( &ctxt );
        foundfile = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv LoadPlayer_list_from_players( LoadPlayer_list_t * lst )
{
    int ipla;
    chr_t * pchr;
    pro_t * ppro;
    player_t * ppla;

    int                    lp_idx;
    LoadPlayer_element_t * lp_ptr;

    if ( NULL == lst || 0 != lst->count ) return rv_error;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ppla = PlaStack.lst + ipla;
        if ( !ppla->valid ) continue;

        if ( !INGAME_CHR( ppla->index ) ) continue;
        pchr = ChrList.lst + ppla->index;

        if ( !LOADED_PRO( pchr->profile_ref ) )continue;
        ppro = ProList.lst + pchr->profile_ref;

        // grab a free LoadPlayer_element_t
        lp_idx = LoadPlayer_list_get_free( lst );
        if ( lp_idx < 0 ) break;

        lp_ptr = lst->lst + lp_idx;

        // fill up the data from the player's info
        strncpy( lp_ptr->name, pchr->Name, SDL_arraysize( lp_ptr->name ) );
        strncpy( lp_ptr->dir, pchr->obj_base._name, SDL_arraysize( lp_ptr->name ) );

        lp_ptr->cap_ref  = pro_get_icap( pchr->profile_ref );
        lp_ptr->skin_ref = pchr->skin;
        lp_ptr->tx_ref   = pchr->inst.texture;

        memmove( lp_ptr->quest_log, ppla->quest_log, sizeof( lp_ptr->quest_log ) );
        memmove( &( lp_ptr->chop ), &( ppro->chop ), sizeof( lp_ptr->chop ) );
    }

    return ( lst->count > 0 ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
LoadPlayer_element_t * LoadPlayer_element_ctor( LoadPlayer_element_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    LoadPlayer_element_init( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
LoadPlayer_element_t * LoadPlayer_element_dtor( LoadPlayer_element_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    LoadPlayer_element_dealloc( ptr );
    LoadPlayer_element_init( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool_t LoadPlayer_element_dealloc( LoadPlayer_element_t * ptr )
{
    if ( NULL == ptr ) return bfalse;

    // release the cap
    if ( MAX_CAP != ptr->cap_ref )
    {
        release_one_cap( ptr->cap_ref );
    }
    ptr->cap_ref = MAX_CAP;

    // release the texture
    if ( INVALID_TX_TEXTURE != ptr->tx_ref )
    {
        TxTexture_free_one( ptr->tx_ref );
    }
    ptr->tx_ref = INVALID_TX_TEXTURE;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t LoadPlayer_element_init( LoadPlayer_element_t * ptr )
{
    if ( NULL == ptr ) return bfalse;

    memset( ptr, 0, sizeof( *ptr ) );

    // set the non-zero, non-null values
    ptr->cap_ref = MAX_CAP;
    ptr->tx_ref = INVALID_TX_TEXTURE;

    idsz_map_init( ptr->quest_log, MAX_IDSZ_MAP_SIZE );
    chop_definition_init( &( ptr->chop ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv mnu_set_selected_list( LoadPlayer_list_t * dst, LoadPlayer_list_t * src, SelectedPlayer_list_t * sp_lst )
{
    int                        src_idx = -1;
    LoadPlayer_element_t     * src_ptr = NULL;
    LoadPlayer_element_t     * dst_ptr = NULL;

    int                        selectedplayer_idx;
    SelectedPlayer_element_t * selectedplayer_ptr = NULL;

    if ( NULL == src || NULL == dst || NULL == sp_lst ) return rv_error;

    // blank out any existing data
    LoadPlayer_list_init( dst );

    if ( 0 == src->count || 0 == sp_lst->count ) return rv_success;

    // loop through the selected players and store all the valid data in the list of imported players
    for ( selectedplayer_idx = 0; selectedplayer_idx < sp_lst->count; selectedplayer_idx++ )
    {
        // grab a pointer to the selectedplayer data
        selectedplayer_ptr = sp_lst->lst + selectedplayer_idx;

        // does the loadplayer exist?
        src_idx = selectedplayer_ptr->player;
        if ( !VALID_LOADPLAYER_IDX( *src, src_idx ) ) continue;

        // grab the loadplayer info
        src_ptr = src->lst + src_idx;

        // get a new import data pointer
        dst_ptr = dst->lst + dst->count;
        dst->count++;

        // copy the data over
        memcpy( dst_ptr, src_ptr, sizeof( *dst_ptr ) );
    }

    return ( dst->count > 0 ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv mnu_set_local_import_list( Import_list_t * imp_lst, SelectedPlayer_list_t * sp_lst )
{
    int                import_idx;
    Import_element_t * import_ptr = NULL;

    int                        loadplayer_idx = -1;
    LoadPlayer_element_t     * loadplayer_ptr = NULL;

    int                        selectedplayer_idx;
    SelectedPlayer_element_t * selectedplayer_ptr = NULL;

    if ( NULL == imp_lst || NULL == sp_lst ) return rv_error;

    // blank out any existing data
    Import_list_init( imp_lst );

    // loop through the selected players and store all the valid data in the list of imported players
    for ( selectedplayer_idx = 0; selectedplayer_idx < sp_lst->count; selectedplayer_idx++ )
    {
        // grab a pointer to the selectedplayer data
        selectedplayer_ptr = sp_lst->lst + selectedplayer_idx;

        // does the loadplayer exist?
        loadplayer_idx = selectedplayer_ptr->player;
        if ( !VALID_LOADPLAYER_IDX( mnu_loadplayer, loadplayer_idx ) ) continue;

        // grab the loadplayer info
        loadplayer_ptr = mnu_loadplayer.lst + loadplayer_idx;

        // get a new import data pointer
        import_idx = imp_lst->count;
        import_ptr = imp_lst->lst + imp_lst->count;
        imp_lst->count++;

        // set the import info
        import_ptr->bits   = selectedplayer_ptr->input;
        import_ptr->slot   = selectedplayer_idx * MAXIMPORTPERPLAYER;
        import_ptr->player = selectedplayer_idx;

        strncpy( import_ptr->srcDir, loadplayer_ptr->dir, SDL_arraysize( import_ptr->srcDir ) );
        import_ptr->dstDir[0] = CSTR_END;
    }

    return ( imp_lst->count > 0 ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv SelectedPlayer_element_init( SelectedPlayer_element_t * ptr )
{
    if ( NULL == ptr ) return rv_error;

    memset( ptr, 0, sizeof( *ptr ) );

    // the non-zero, non-null values
    ptr->player = MAX_PLAYER;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
// implementation of SelectedPlayer_list_t
//--------------------------------------------------------------------------------------------
egoboo_rv SelectedPlayer_list_init( SelectedPlayer_list_t * sp_lst )
{
    int cnt;

    if ( NULL == sp_lst ) return rv_error;

    for ( cnt = 0; cnt < MAX_PLAYER; cnt++ )
    {
        SelectedPlayer_element_init( sp_lst->lst + cnt );
    }
    sp_lst->count = 0;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool_t SelectedPlayer_list_check_loadplayer( SelectedPlayer_list_t * sp_lst, int loadplayer_idx )
{
    int i;

    if ( !VALID_LOADPLAYER_IDX( mnu_loadplayer, loadplayer_idx ) ) return bfalse;

    for ( i = 0; i < MAX_PLAYER && i < sp_lst->count; i++ )
    {
        if ( sp_lst->lst[i].player == loadplayer_idx ) return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
int SelectedPlayer_list_index_from_loadplayer( SelectedPlayer_list_t * sp_lst,  int loadplayer_idx )
{
    int cnt, selected_index;

    if ( !VALID_LOADPLAYER_IDX( mnu_loadplayer, loadplayer_idx ) ) return INVALID_PLAYER;

    selected_index = INVALID_PLAYER;
    for ( cnt = 0; cnt < MAX_PLAYER && cnt < sp_lst->count; cnt++ )
    {
        if ( sp_lst->lst[ cnt ].player == loadplayer_idx )
        {
            selected_index = cnt;
            break;
        }
    }

    return selected_index;
}

//--------------------------------------------------------------------------------------------
bool_t SelectedPlayer_list_add( SelectedPlayer_list_t * sp_lst, int loadplayer_idx )
{
    if ( !VALID_LOADPLAYER_IDX( mnu_loadplayer, loadplayer_idx ) || sp_lst->count >= MAX_PLAYER ) return bfalse;
    if ( SelectedPlayer_list_check_loadplayer( sp_lst,  loadplayer_idx ) ) return bfalse;

    sp_lst->lst[sp_lst->count].player = loadplayer_idx;
    sp_lst->lst[sp_lst->count].input  = INPUT_BITS_NONE;
    sp_lst->count++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t SelectedPlayer_list_remove( SelectedPlayer_list_t * sp_lst, int loadplayer_idx )
{
    int i;
    bool_t found = bfalse;

    if ( !VALID_LOADPLAYER_IDX( mnu_loadplayer, loadplayer_idx ) || sp_lst->count <= 0 ) return bfalse;

    if ( 1 == sp_lst->count )
    {
        if ( sp_lst->lst[0].player == loadplayer_idx )
        {
            sp_lst->count = 0;
        }
    }
    else
    {
        for ( i = 0; i < MAX_PLAYER && i < sp_lst->count; i++ )
        {
            if ( sp_lst->lst[i].player == loadplayer_idx )
            {
                found = btrue;
                break;
            }
        }

        if ( found )
        {
            i++;
            for ( /* nothing */; i < MAX_PLAYER && i < sp_lst->count; i++ )
            {
                sp_lst->lst[i-1].player = sp_lst->lst[i].player;
                sp_lst->lst[i-1].input  = sp_lst->lst[i].input;
            }

            sp_lst->count--;
        }
    };

    return found;
}

//--------------------------------------------------------------------------------------------
bool_t SelectedPlayer_list_add_input( SelectedPlayer_list_t * sp_lst, int loadplayer_idx, BIT_FIELD input_bits )
{
    int i;
    bool_t done, retval = bfalse;

    int selected_index = -1;

    for ( i = 0; i < sp_lst->count; i++ )
    {
        if ( sp_lst->lst[i].player == loadplayer_idx )
        {
            selected_index = i;
            break;
        }
    }

    if ( -1 == selected_index )
    {
        SelectedPlayer_list_add( sp_lst,  loadplayer_idx );
    }

    if ( selected_index >= 0 && selected_index < sp_lst->count )
    {
        for ( i = 0; i < sp_lst->count; i++ )
        {
            if ( i == selected_index )
            {
                // add in the selected bits for the selected loadplayer_idx
                SET_BIT( sp_lst->lst[i].input, input_bits );
                retval = btrue;
            }
            else
            {
                // remove the selectd bits from all other players
                UNSET_BIT( sp_lst->lst[i].input, input_bits );
            }
        }
    }

    // Do the tricky part of removing all players with invalid inputs from the list
    // It is tricky because removing a loadplayer_idx changes the value of the loop control
    // value sp_lst->count within the loop.
    done = bfalse;
    while ( !done )
    {
        // assume the best
        done = btrue;

        for ( i = 0; i < sp_lst->count; i++ )
        {
            if ( INPUT_BITS_NONE == sp_lst->lst[i].input )
            {
                // we found one
                done = bfalse;
                SelectedPlayer_list_remove( sp_lst,  sp_lst->lst[i].player );
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t SelectedPlayer_list_remove_input( SelectedPlayer_list_t * sp_lst, int loadplayer_idx, Uint32 input_bits )
{
    int i;
    bool_t retval = bfalse;

    for ( i = 0; i < MAX_PLAYER && i < sp_lst->count; i++ )
    {
        if ( sp_lst->lst[i].player == loadplayer_idx )
        {
            UNSET_BIT( sp_lst->lst[i].input, input_bits );

            // This part is not so tricky as in SelectedPlayer_list_add_input.
            // Even though we are modding the loop control variable, it is never
            // tested in the loop because we are using the break command to
            // break out of the loop immediately

            if ( INPUT_BITS_NONE == sp_lst->lst[i].input )
            {
                SelectedPlayer_list_remove( sp_lst,  loadplayer_idx );
            }

            retval = btrue;

            break;
        }
    }

    return retval;
}
