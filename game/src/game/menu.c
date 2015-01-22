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

/// @file game/menu.c
/// @brief Implements the main menu tree, using the code in Ui.*
/// @details
#include <list>

#include "game/menu.h"
#include "game/mad.h"
#include "game/player.h"
#include "game/game.h"
#include "game/graphic_texture.h"
#include "game/renderer_2d.h"
#include "game/ui.h"
#include "game/link.h"
#include "game/game.h"
#include "game/input.h"
#include "game/egoboo.h"
#include "game/particle.h"
#include "game/char.h"
#include "game/profiles/Profile.hpp"
#include "game/profiles/ProfileSystem.hpp"

#include "game/audio/AudioSystem.hpp"

#include "game/ChrList.h"
#include "game/PrtList.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_SlidyButtonState;
typedef struct s_SlidyButtonState mnu_SlidyButtonState_t;

struct s_mnu_module;
typedef struct s_mnu_module mnu_module_t;

struct s_GameTips;
typedef struct s_GameTips GameTips_t;

#define SCANTAG_KEYDOWN(VAL)          (((VAL) < SDLK_NUMLOCK) && SDL_KEYDOWN(keyb, VAL ))
#define SCANTAG_KEYMODDOWN(VAL)       (((VAL) < SDLK_LAST) && SDL_KEYDOWN(keyb, VAL ))
#define SCANTAG_MOUSBUTTON(VAL)       ((0 != (VAL)) && (mous.b == (VAL)) )
#define SCANTAG_JOYBUTTON(PJOY, VAL)  ((NULL != (PJOY)) && (0 != (VAL)) && ((PJOY)->b == (VAL)) )

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

#define TITLE_TEXTURE_COUNT   MAX_MODULE
#define INVALID_TITLE_TEXTURE TITLE_TEXTURE_COUNT

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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

class LoadPlayerElement
{
public:
    LoadPlayerElement(std::shared_ptr<ObjectProfile> profile) :
        _name("*NONE*"),
        _profile(profile),
        _skinRef(profile->getSkinOverride()),
        _questLog(),
        _selectedByPlayer(-1),
        _inputDevice(INPUT_DEVICE_UNKNOWN)
    {
        // load the quest info from "quest.txt" so we can determine the valid modules
        quest_log_download_vfs(_questLog, SDL_arraysize(_questLog), profile->getFolderPath().c_str());

        // load the chop data from "naming.txt" to generate the character name (kinda silly how it's done currently)
        RandomName randomName;
        randomName.loadFromFile(profile->getFolderPath() + "/naming.txt");
        
        // generate the name from the chop
        _name = randomName.generateRandomName();
    }

    inline const std::string& getName() const {return _name;}
    inline TX_REF getIcon() const {return _profile->getIcon(_skinRef);}
    inline const std::shared_ptr<ObjectProfile>& getProfile() const {return _profile;}

    /**
    * @return which player number has selected this character (-1 for nobody)
    **/
    int getSelectedByPlayer() const {return _selectedByPlayer;}

    /**
    * @return true if the player meets the specified requirements for this quest
    **/
    bool hasQuest(const IDSZ idsz, const int requiredLevel)
    {
        int quest_level = quest_log_get_level(_questLog, SDL_arraysize(_questLog), idsz);

        // find beaten quests or quests with proper level
        if ( quest_level <= QUEST_BEATEN || requiredLevel <= quest_level )
        {
            return true;
        }

        return false;
    }

    void setSelectedByPlayer(int selected) {_selectedByPlayer = selected;}

private:
    std::string                     _name;
    TX_REF                          _icon;
    std::shared_ptr<ObjectProfile>  _profile;
    uint16_t                        _skinRef;
    IDSZ_node_t                     _questLog[MAX_IDSZ_MAP_SIZE]; ///< all the quests this player has
    int                             _selectedByPlayer;                    ///< ID of player who has selected this character (-1 for none)
    int                             _inputDevice;
};

static std::vector<std::shared_ptr<LoadPlayerElement>> _loadPlayerList;     ///< Possible character we can load
static std::list<std::shared_ptr<LoadPlayerElement>> _selectedPlayerList;   ///< List of characters who are actually going to play

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the module data that the menu system needs
struct s_mnu_module
{
    EGO_PROFILE_STUFF                           ///< the "base class" of a profile object

    mod_file_t base;                            ///< the data for the "base class" of the module

    // extended data
    MNU_TX_REF tex_index;                           ///< the index of the module's tile image

    STRING vfs_path;                            ///< the virtual pathname of the module
    STRING dest_path;                           ///< the path that module data can be written into
};

mnu_module_t * mnu_module__init( mnu_module_t * );

#define VALID_MOD_RANGE( IMOD ) ( ((IMOD) >= 0) && ((IMOD) < MAX_MODULE) )
#define VALID_MOD( IMOD )       ( VALID_MOD_RANGE( IMOD ) && IMOD < mnu_ModList.count && mnu_ModList.lst[IMOD].loaded )
#define INVALID_MOD( IMOD )     ( !VALID_MOD_RANGE( IMOD ) || IMOD >= mnu_ModList.count || !mnu_ModList.lst[IMOD].loaded )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data that menu.c uses to store the users' choice of players
struct s_GameTips
{
    //These are loaded only once
    Uint8  count;                         //< Number of global tips loaded
    STRING hint[MENU_MAX_GAMETIPS];       //< The global hints/tips
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void loadAllImportPlayers(const std::string &directoryName);
static int currentSelectingPlayer = 0;

//--------------------------------------------------------------------------------------------
// declaration of "private" variables
//--------------------------------------------------------------------------------------------

static int mnu_stack_index = 0;
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

static GameTips_t mnu_Tips_global = { 0 };
static GameTips_t mnu_Tips_local  = { 0 };

static mnu_SlidyButtonState_t mnu_SlidyButtonState = { NULL };

//--------------------------------------------------------------------------------------------
// declaration of public variables
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST( ACCESS_TYPE_NONE, oglx_texture_t, mnu_TxList, MENU_TX_COUNT );

static Stack<mnu_module_t, MAX_MODULE> mnu_ModList;
//INSTANTIATE_STACK_STATIC( mnu_module_t, mnu_ModList, MAX_MODULE );

#define INVALID_MOD_IDX MAX_MODULE
#define INVALID_MOD_REF ((MOD_REF)INVALID_MOD_IDX)

menu_process_t * MProc             = &_mproc;
bool           start_new_player  = false;
bool           module_list_valid = false;

/* The font used for drawing text.  It's smaller than the button font */
Font *menuFont = NULL;

bool mnu_draw_background = true;

//--------------------------------------------------------------------------------------------
// "private" function prototypes
//--------------------------------------------------------------------------------------------

// Implementation of the mnu_stack
static bool mnu_stack_push( which_menu_t menu );
static which_menu_t mnu_stack_pop();
static which_menu_t mnu_stack_peek();
static void mnu_stack_clear();

// Implementation of the mnu_SlidyButton array
static void mnu_SlidyButton_init( float lerp, const char *button_text[] );
static void mnu_SlidyButton_update_all( float deltaTime );
static void mnu_SlidyButton_draw_all();

// implementation of "private" TxTitleImage functions
//static void TxTitleImage_clear_data();
//static void TxTitleImage_release_one( const MNU_TX_REF index );
//static void TxTitleImage_ctor();
//static void TxTitleImage_release_all();
//static void TxTitleImage_dtor();

// tipText functions
static void tipText_set_position( Font * font, const char * text, int spacing );

// copyrightText functions
static void copyrightText_set_position( Font * font, const char * text, int spacing );

// implementation of "private" ModList functions
static void mnu_ModList_release_all();

// "process" management
static int menu_process_do_begin( menu_process_t * mproc );
static int menu_process_do_running( menu_process_t * mproc );
static int menu_process_do_leaving( menu_process_t * mproc );

// the hint system
static bool       mnu_Tips_global_load_vfs( GameTips_t * );
static bool mnu_Tips_local_load_vfs( GameTips_t * );
static const char *mnu_Tips_get_hint( GameTips_t * pglobal, GameTips_t * plocal );

// "private" module utility
static void mnu_load_all_module_info();

// "private" asset function
static MNU_TX_REF mnu_get_txtexture_ref( const std::shared_ptr<ObjectProfile> &profile, const MNU_TX_REF default_ref );

// implementation of the autoformatting
static void autoformat_init_slidy_buttons();
static void autoformat_init_tip_text();
static void autoformat_init_copyright_text();

// misc other stuff
static void mnu_release_one_module( const MOD_REF imod );
static void mnu_load_all_module_images_vfs();
static egolib_rv mnu_set_local_import_list( import_list_t * imp_lst);

static void mnu_ModList_release_images();
static void mnu_module_init( mnu_module_t * pmod );
static bool mnu_test_module_by_index(const MOD_REF modnumber, size_t buffer_len, char * buffer);

static int doShowEndgame( float deltaTime );
static int doGamePaused( float deltaTime );
static int doNotImplemented( float deltaTime );
static int doShowLoadingScreen( float deltaTime );

static int doVideoOptions( float deltaTime );
static int doVideoOptions_fix_fullscreen_resolution( egoboo_config_t * pcfg, SDLX_screen_info_t * psdl_scr, STRING * psz_screen_size );
static bool doVideoOptions_coerce_aspect_ratio( int width, int height, float * pratio, STRING * psz_ratio );

static int doAudioOptions( float deltaTime );
static int doGameOptions( float deltaTime );
static int doInputOptions( float deltaTime );
static int doOptions( float deltaTime );

static int doChooseCharacter( float deltaTime );
static bool doChooseCharacter_show_stats( std::shared_ptr<LoadPlayerElement> loadPlayer, bool loadItems, const int x, const int y, const int width, const int height );

static int doChoosePlayer( float deltaTime );

static int doChooseModule( float deltaTime );
static int doSinglePlayerMenu( float deltaTime );
static int doMainMenu( float deltaTime );

static int cmp_mod_ref( const void * vref1, const void * vref2 );

// declaration of mnu_TxList functions not used outside this module
static void   mnu_TxList_init_all();
static void   mnu_TxList_delete_all();
static MNU_TX_REF mnu_TxList_get_free( const MNU_TX_REF itex );
static bool mnu_TxList_free_one( const MNU_TX_REF  itex );

// implementation of "private" mnu_TxList functions
static void   mnu_TxList_reset_freelist();
static void   mnu_TxList_release_one( const MNU_TX_REF index );
static void   mnu_TxList_release_all();

//--------------------------------------------------------------------------------------------
// implementation of the menu stack
//--------------------------------------------------------------------------------------------
bool mnu_stack_push( which_menu_t menu )
{
    mnu_stack_index = CLIP( mnu_stack_index, 0, MENU_STACK_COUNT ) ;

    if ( mnu_stack_index >= MENU_STACK_COUNT ) return false;

    mnu_stack[mnu_stack_index] = menu;
    mnu_stack_index++;

    return true;
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
int menu_process_do_begin( menu_process_t * mproc )
{
    // reset the fps counter
    menu_fps_clock        = 0;
    menu_fps_loops        = 0;

    stabilized_menu_fps        = TARGET_FPS;
    stabilized_menu_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_menu_fps_weight = STABILIZED_COVER;

    // play some music
    _audioSystem.playMusic(AudioSystem::MENU_SONG);

    // initialize all these structures
    menu_system_begin();        // start the menu menu

    // load all module info at menu initialization
    // this will not change unless a new module is downloaded for a network menu?
    mnu_load_all_module_info();

    // initialize the process state
    mproc->base.valid = true;

    return 1;
}

//--------------------------------------------------------------------------------------------
int menu_process_do_running( menu_process_t * mproc )
{
    int menuResult;

    if ( !process_validate( PROC_PBASE( mproc ) ) ) return -1;

    mproc->was_active = TO_C_BOOL( mproc->base.valid );

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
        GProc->escape_latch = false;

        // We have exited the menu and restarted the game
        GProc->mod_paused = false;
        process_pause( PROC_PBASE( MProc ) );
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
int menu_process_do_leaving( menu_process_t * mproc )
{
    if ( !process_validate( PROC_PBASE( mproc ) ) ) return -1;

    // terminate the menu system
    menu_system_end();

    // finish the menu song
    _audioSystem.stopMusic();

    // reset the fps counter
    menu_fps_clock        = 0;
    menu_fps_loops        = 0;

    stabilized_menu_fps        = TARGET_FPS;
    stabilized_menu_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_menu_fps_weight = STABILIZED_COVER;

    return 1;
}

//--------------------------------------------------------------------------------------------
int menu_process_run( menu_process_t * mproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_validate( PROC_PBASE( mproc ) ) ) return -1;
    mproc->base.frameDuration = frameDuration;

    if ( mproc->base.paused ) return 0;

    if ( mproc->base.killme )
    {
        mproc->base.state = proc_leaving;
    }

    switch ( mproc->base.state )
    {
        case proc_begin:
            proc_result = menu_process_do_begin( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            // proc_result = menu_process_do_entering( mproc );

            mproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = menu_process_do_running( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = menu_process_do_leaving( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state  = proc_finish;
                mproc->base.killme = false;
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
menu_process_t *menu_process_init( menu_process_t * mproc )
{
    if ( NULL == mproc ) return NULL;

    BLANK_STRUCT_PTR( mproc )

    process_init( PROC_PBASE( mproc ) );

    return mproc;
}

//--------------------------------------------------------------------------------------------
// Code for global initialization/deinitialization of the menu system
//--------------------------------------------------------------------------------------------
static bool _menu_system_constructed = false;
static bool _menu_system_atexit_registered = false;

static void menu_system_atexit();
static bool menu_system_ctor();
static bool menu_system_dtor();
static bool menu_system_init();
static bool menu_system_deinit();

//--------------------------------------------------------------------------------------------
void menu_system_atexit()
{
    menu_system_dtor();
}

//--------------------------------------------------------------------------------------------
bool menu_system_ctor()
{
    if ( !_menu_system_constructed )
    {
        // construct the TxTitleImage array
        //TxTitleImage_ctor();

        // construct the mnu_TxList array
        mnu_TxList_ctor();

        _menu_system_constructed = true;
    }

    return _menu_system_constructed;
}

//--------------------------------------------------------------------------------------------
bool menu_system_dtor()
{
    if ( _menu_system_constructed )
    {
        // release the font
        if ( NULL != menuFont )
        {
            fnt_freeFont( menuFont );
            menuFont = NULL;
        }

        // destruct the TxTitleImage array
        //TxTitleImage_dtor();

        // destruct the mnu_TxList array
        mnu_TxList_dtor();

        _menu_system_constructed = false;
    }

    return !_menu_system_constructed;
}

//--------------------------------------------------------------------------------------------
bool menu_system_init()
{
    bool retval = true;

    // load the bitmapped font
    font_bmp_load_vfs( mnu_TxList_get_valid_ptr(( MNU_TX_REF )MENU_TX_FONT_BMP ), "mp_data/font_new_shadow", "mp_data/font.txt" );  // must be done after gfx_system_init_all_graphics()

    // load the ttf font
    menuFont = ui_loadFont("mp_data/Bo_Chen.ttf", 18);
    if ( NULL == menuFont )
    {
        log_error( "Could not load the menu font! (\"mp_data/Bo_Chen.ttf\")\n" );
        retval = false;
    }

    autoformat_init( &gfx );

    // Figure out where to draw the copyright text
    copyrightText_set_position( menuFont, copyrightText, 20 );

    // Figure out where to draw the options text
    tipText_set_position( menuFont, tipText, 20 );

    // ready the mouse cursor
    if ( !mnu_load_cursor() )
    {
        log_warning( "Could not load mouse cursor (basicdat" SLASH_STR "cursor.png)\n" );
        retval = false;
    }

    // ready the global icons used in the menu
    if ( !mnu_load_all_global_icons() )
    {
        log_warning( "Could not load all global icons!\n" );
        retval = false;
    }

    // Load game hints
    mnu_Tips_global_load_vfs( &mnu_Tips_global );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool menu_system_deinit()
{

    // release the font
    if ( NULL != menuFont )
    {
        fnt_freeFont( menuFont );
        menuFont = NULL;
    }

    // if this has not been done before yet, do it now
    _loadPlayerList.clear();

    return true;
}

//--------------------------------------------------------------------------------------------
int menu_system_begin()
{
    // initializes the menu system
    //
    // Loads resources for the menus, and figures out where things should
    // be positioned.  If we ever allow changing resolution on the fly, this
    // function will have to be updated/called more than once.

    if ( !_menu_system_atexit_registered )
    {
        atexit( menu_system_atexit );
        _menu_system_atexit_registered = true;
    }

    // Should be okay to randomize the seed here
    srand(( unsigned int )time( NULL ) );

    menu_system_ctor();

    menu_system_init();

    return 1;
}

//--------------------------------------------------------------------------------------------
void menu_system_end()
{
    // deinitializes the menu system

    menu_system_deinit();

    menu_system_dtor();
}

//--------------------------------------------------------------------------------------------
// Interface for starting and stopping menus
//--------------------------------------------------------------------------------------------
bool mnu_begin_menu( which_menu_t which )
{
    if ( !mnu_stack_push( mnu_whichMenu ) ) return false;
    mnu_whichMenu = which;

    return true;
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
            fminw = ( float ) std::min( GFX_WIDTH , background.imgW ) / ( float ) background.imgW;
            fminh = ( float ) std::min( GFX_HEIGHT, background.imgH ) / ( float ) background.imgW;
            fmin  = std::min( fminw, fminh );

            bg_rect.w = background.imgW * fmin;
            bg_rect.h = background.imgH * fmin;
            bg_rect.x = ( GFX_WIDTH  - bg_rect.w ) * 0.5f;
            bg_rect.y = ( GFX_HEIGHT - bg_rect.h ) * 0.5f;

            // calculate the position of the logo
            fmin  = std::min( bg_rect.w * 0.5f / logo.imgW, bg_rect.h * 0.5f / logo.imgH );

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

            GL_DEBUG( glColor4fv )( white_vec );

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
            if ( SDL_KEYDOWN( keyb, SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 4, sz_buttons[3], NULL, buttonLeft, buttonTop + 35 * 3, 200, 30 ) )
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
            if ( SDL_KEYDOWN( keyb, SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 3, sz_buttons[2], NULL, buttonLeft, buttonTop + 35 * 2, 200, 30 ) )
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
    /// @author BB
    /// @details Sort MOD REF values based on the rank of the module that they point to.
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
    static const char * filterText = "All Modules";

    static std::vector<MOD_REF> validModules;

    static int moduleMenuOffsetX;
    static int moduleMenuOffsetY;

    static size_t startIndex = 0;
    static int ext_module = -1;

    int result = 0;
    int x, y;
    MOD_REF imod;

    switch ( menuState )
    {
        case MM_Begin:

            // Reset which module we are selecting
            startIndex = 0;
            ext_module = -1;

            // reset the global module selection index
            selectedModule = -1;

            // blank out the valid modules
            validModules.clear();

            // Figure out at what offset we want to draw the module menu.
            moduleMenuOffsetX = ( GFX_WIDTH  - 640 ) / 2;
            moduleMenuOffsetX = std::max( 0, moduleMenuOffsetX );

            moduleMenuOffsetY = ( GFX_HEIGHT - 480 ) / 2;
            moduleMenuOffsetY = std::max( 0, moduleMenuOffsetY );

            menuState = MM_Entering;

            // fall through...

        case MM_Entering:
            menuState = MM_Running;

            if ( !module_list_valid )
            {
                mnu_load_all_module_info();
                mnu_load_all_module_images_vfs();
            }

            // Find the modules that we want to allow loading for.  If start_new_player
            // is true, we want ones that don't allow imports (e.g. starter modules).
            // Otherwise, we want modules that allow imports
            for ( imod = 0; imod < mnu_ModList.count; imod++ )
            {
                // if this module is not valid given the game options and the
                // selected players, skip it
                if ( !mnu_test_module_by_index( imod, 0, NULL ) ) continue;

                if ( start_new_player && 0 == mnu_ModList.lst[imod].base.importamount )
                {
                    // starter module
                    validModules.push_back(imod);
                }
                else
                {
                    if ( FILTER_OFF != mnu_moduleFilter && mnu_ModList.lst[imod].base.moduletype != mnu_moduleFilter ) continue;
                    if ( _selectedPlayerList.size() > mnu_ModList.lst[imod].base.importamount ) continue;
                    if ( _selectedPlayerList.size() < mnu_ModList.lst[imod].base.minplayers ) continue;
                    if ( _selectedPlayerList.size() > mnu_ModList.lst[imod].base.maxplayers ) continue;

                    // regular module
                    validModules.push_back(imod);
                }
            }

            // sort the modules by difficulty. easiest to hardeest for starting a new character
            // hardest to easiest for loading a module
            cmp_mod_ref_mult = start_new_player ? 1 : -1;
            qsort( validModules.data(), validModules.size(), sizeof(MOD_REF), cmp_mod_ref );

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
            if ( validModules.empty() )
            {
                tipText_set_position( menuFont, "Sorry, there are no valid games!\n Please press the \"Back\" button.", 20 );
            }
            else if ( validModules.size() <= 3 )
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
                    mnu_load_all_module_images_vfs();
                }

                // Draw the background
                GL_DEBUG( glColor4fv )( white_vec );
                x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
                y = GFX_HEIGHT - background.imgH;

                if ( mnu_draw_background )
                {
                    ui_drawImage( 0, &background, x, y, 0, 0, NULL );
                }

                // use the mouse wheel to scan the modules
                if ( input_cursor_wheel_event_pending() )
                {
                    if ( input_cursor.z > 0 )
                    {
                        startIndex++;
                    }
                    else if ( input_cursor.z < 0 )
                    {
                        startIndex--;
                    }

                    input_cursor_finish_wheel_event();
                }

                //Allow arrow keys to scroll as well
                if ( SDL_KEYDOWN( keyb, SDLK_RIGHT ) )
                {
                    if ( 0 == keycooldown )
                    {
                        startIndex++;
                        keycooldown = 5;
                    }
                }
                else if ( SDL_KEYDOWN( keyb, SDLK_LEFT ) )
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
                if ( validModules.size() > 3 )
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
                startIndex = CLIP<int>( startIndex, 0, validModules.size() - 3 );

                // Draw buttons for the modules that can be selected
                x = 93;
                y = 20;
				for (size_t i = startIndex; i < std::min(startIndex + 3, validModules.size()); i++)
                {
                    // fix the menu images in case one or more of them are undefined
                    MOD_REF          loc_imod;
                    mnu_module_t    *loc_pmod;
                    MNU_TX_REF           tex_offset;
                    oglx_texture_t * ptex;
                    GLfloat        * img_tint = normal_tint;

                    loc_imod   = validModules[i];
                    if ( !VALID_MOD_RANGE( loc_imod ) ) continue;

                    loc_pmod = mnu_ModList.lst + loc_imod;
                    tex_offset = loc_pmod->tex_index;
                    ptex       = mnu_TxList_get_ptr( tex_offset );

                    // only do modules that are valid
                    if ( i >= 0 && i <= validModules.size() )
                    {
                        if ( loc_pmod->base.beaten )
                        {
                            img_tint = beat_tint;
                        }

                        if ( ui_doImageButton( i, ptex, moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138, img_tint ) )
                        {
                            ext_module = REF_TO_INT( validModules[i] );
                        }

                        //Draw a text over the image explaining what it means
                        if ( loc_pmod->base.beaten )
                        {
                            ui_drawTextBox( NULL, "BEATEN", moduleMenuOffsetX + x + 32, moduleMenuOffsetY + y + 64, 64, 30, 20 );
                        }
                    }

                    x += 138 + 20;  // Width of the button, and the spacing between buttons
                }

                // Draw an empty button as the backdrop for the module text
                ui_drawButton( UI_Nothing, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 330, 250, NULL );

                // Draw the text description of the selected module
                if ( ext_module > -1 && ext_module < mnu_ModList.count )
                {
                    char    buffer[1024]  = EMPTY_CSTR;
                    const char * rank_string, * name_string;
                    char  * carat = buffer, * carat_end = buffer + SDL_arraysize( buffer );

                    mod_file_t * pmod = &( mnu_ModList.lst[ext_module].base );

                    GL_DEBUG( glColor4fv )( white_vec );

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

                    for (size_t i = 0; i < SUMMARYLINES; i++ )
                    {
                        carat += snprintf( carat, carat_end - carat - 1, "%s\n", pmod->summary[i] );
                    }

                    // Draw a text box
                    ui_drawTextBox( menuFont, buffer, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230, 20 );
                }

                // And draw the next & back buttons
                if ( ext_module > -1 )
                {
                    if ( SDL_KEYDOWN( keyb, SDLK_RETURN ) || BUTTON_UP == ui_doButton( 53, "Select Module", NULL, moduleMenuOffsetX + 377, moduleMenuOffsetY + 173, 200, 30 ) )
                    {
                        // go to the next menu with this module selected
                        menuState = MM_Leaving;
                    }
                }

                if ( SDL_KEYDOWN( keyb, SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 54, "Back", NULL, moduleMenuOffsetX + 377, moduleMenuOffsetY + 208, 200, 30 ) )
                {
                    // Signal doMenu to go back to the previous menu
                    ext_module = -1;
                    menuState = MM_Leaving;
                }

                //Do the module filter button
                if ( !start_new_player )
                {
                    bool click_button;

                    GL_DEBUG( glColor4fv )( white_vec );
                    ui_drawTextBox( menuFont, "Module Filter:", moduleMenuOffsetX + 257, moduleMenuOffsetY - 27, 0, 0, 20 );

                    // unly display the filter name
                    ui_doButton( 55, filterText, NULL, moduleMenuOffsetX + 377, moduleMenuOffsetY - 27 , 200, 30 );

                    // use the ">" button to change since we are already using arrows to indicate "spin control"-like widgets
                    click_button = ( BUTTON_UP == ui_doButton( 56, ">", NULL, moduleMenuOffsetX + 580, moduleMenuOffsetY - 27, 30, 30 ) );

                    if ( click_button )
                    {
                        //Reload the modules with the new filter
                        menuState = MM_Entering;

                        // Reset which module we are selecting
                        startIndex = 0;
                        ext_module = -1;

                        // reset the global module selection index
                        selectedModule = -1;

                        //Swap to the next filter
                        mnu_moduleFilter = CLIP( mnu_moduleFilter, FILTER_NORMAL_BEGIN, FILTER_NORMAL_END );

                        mnu_moduleFilter = ( module_filter_t )( mnu_moduleFilter + 1 );

                        if ( mnu_moduleFilter > FILTER_NORMAL_END ) mnu_moduleFilter = FILTER_NORMAL_BEGIN;

                        switch ( mnu_moduleFilter )
                        {
                            case FILTER_MAIN:      filterText = "Main Quest";       break;
                            case FILTER_SIDE:      filterText = "Sidequests";       break;
                            case FILTER_TOWN:      filterText = "Towns and Cities"; break;
                            case FILTER_FUN:       filterText = "Fun Modules";      break;
                            case FILTER_STARTER:   filterText = "Starter Modules";  break;
                            default:
                            case FILTER_OFF:       filterText = "All Modules";      break;
                        }
                    }
                }

                // the tool-tip text
                GL_DEBUG( glColor4fv )( white_vec );
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
                    pickedmodule_ready = true;
                    result = ( PMod->importamount > 0 ) ? 1 : 2;
                }
            }

            // post the selected module
            selectedModule = ext_module;

            // reset the ui
            ui_Reset();

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
bool doChooseCharacter_load_inventory(std::shared_ptr<LoadPlayerElement> loadPlayer, std::vector<std::shared_ptr<ObjectProfile>> &items)
{
    int    i;
    STRING  szFilename;

    PRO_REF   pro_ref;

    if (!loadPlayer) {
        return false;
    }

    // grab the inventory data
    for ( i = 0; i < MAX_IMPORT_OBJECTS; i++ )
    {
        int slot = i + 1;

        const std::string itemFolder = loadPlayer->getProfile()->getFolderPath() + "/" + std::to_string(i) + ".obj";

        if(!vfs_exists(itemFolder.c_str())) {
            continue;
        }

        // load the lightweight profile
        std::shared_ptr<ObjectProfile> profile = ObjectProfile::loadFromFile(itemFolder, slot, true);
        if ( profile )
        {
            items.push_back(profile);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool doChooseCharacter_show_stats( std::shared_ptr<LoadPlayerElement> loadPlayer, bool loadItems, const int x, const int y, const int width, const int height )
{
    int   i;
    float x1, y1;

    static std::vector<std::shared_ptr<ObjectProfile>> objects;

    if (!loadPlayer) {
        // release all of the temporary profiles
        objects.clear();
        return false;
    }

    if(loadItems) {
        objects.clear();
        doChooseCharacter_load_inventory(loadPlayer, objects);
    }

    // do the actual display
    x1 = x + 25;
    y1 = y + 10;

    const int icon_hgt = 32;
    const int text_hgt = 20;
    const int text_vert_centering = ( icon_hgt - text_hgt ) / 2;
    const int section_spacing = 10;

    const std::shared_ptr<ObjectProfile> &profile = loadPlayer->getProfile();

    STRING  temp_string;
    SKIN_T  skin = profile->getSkinOverride();

    ui_drawButton( UI_Nothing, x, y, width, height, NULL );

    //Character level and class
    GL_DEBUG( glColor4fv )( white_vec );
    y1 = ui_drawTextBox( NULL, loadPlayer->getName().c_str(), x1, y1, 0, 0, text_hgt );
    y1 += section_spacing;

    //Character level and class
    GL_DEBUG( glColor4fv )( white_vec );
    snprintf( temp_string, SDL_arraysize( temp_string ), "A level %d %s", profile->getStartingLevel() + 1, profile->getClassName().c_str() );
    y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );

    // Armor
    const SkinInfo &skinInfo = profile->getSkinInfo(skin);
    GL_DEBUG( glColor4fv )( white_vec );
    snprintf( temp_string, SDL_arraysize( temp_string ), "Wearing %s %s",
              !skinInfo.name.empty() ? skinInfo.name.c_str() : "UNKNOWN",
              skinInfo.dressy ? "(Light)" : "(Heavy)" );
    y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );
    y1 += section_spacing;

    // Life and mana (can be less than maximum if not in easy mode)
    if ( cfg.difficulty >= GAME_NORMAL )
    {
        snprintf( temp_string, SDL_arraysize( temp_string ), "Life: %d/%d",
			      std::min( (int)UFP8_TO_UINT( profile->getSpawnLife() ), ( int )profile->getBaseLife().from ), ( int )profile->getBaseLife().from );
        y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );

        y1 = ui_drawBar( x1, y1, UFP8_TO_UINT( profile->getSpawnLife() ), ( int )profile->getBaseLife().from, profile->getLifeColor() );

        if ( profile->getBaseMana().from > 0 )
        {
            snprintf( temp_string, SDL_arraysize( temp_string ), "Mana: %d/%d",
				      std::min( (int)UFP8_TO_UINT( profile->getSpawnLife() ), ( int )profile->getBaseMana().from ), ( int )profile->getBaseMana().from );
            y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );

            y1 = ui_drawBar( x1, y1, UFP8_TO_UINT( profile->getSpawnMana() ), ( int )profile->getBaseMana().from, profile->getManaColor() );
        }
    }
    else
    {
        snprintf( temp_string, SDL_arraysize( temp_string ), "Life: %d", ( int )profile->getBaseLife().from );
        y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );
        y1 = ui_drawBar( x1, y1, ( int )profile->getBaseLife().from, ( int )profile->getBaseLife().from, profile->getLifeColor() );

        if ( profile->getBaseMana().from > 0 )
        {
            snprintf( temp_string, SDL_arraysize( temp_string ), "Mana: %d", ( int )profile->getBaseMana().from );
            y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );
            y1 = ui_drawBar( x1, y1, ( int )profile->getBaseMana().from, ( int )profile->getBaseMana().from, profile->getManaColor() );
        }
    }
    y1 += section_spacing;

    //SWID
    y1 = ui_drawTextBox( menuFont, "Stats", x1, y1, 0, 0, text_hgt );

    snprintf( temp_string, SDL_arraysize( temp_string ), "  Str: %s (%d)", describe_value( profile->getBaseStrength().from,     60, NULL ), ( int )profile->getBaseStrength().from );
    y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );

    snprintf( temp_string, SDL_arraysize( temp_string ), "  Wis: %s (%d)", describe_value( profile->getBaseWisdom().from,       60, NULL ), ( int )profile->getBaseWisdom().from );
    y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );

    snprintf( temp_string, SDL_arraysize( temp_string ), "  Int: %s (%d)", describe_value( profile->getBaseIntelligence().from, 60, NULL ), ( int )profile->getBaseIntelligence().from );
    y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );

    snprintf( temp_string, SDL_arraysize( temp_string ), "  Dex: %s (%d)", describe_value( profile->getBaseDexterity().from,    60, NULL ), ( int )profile->getBaseDexterity().from );
    y1 = ui_drawTextBox( menuFont, temp_string, x1, y1, 0, 0, text_hgt );
    y1 += section_spacing;

    //Inventory
    if (!objects.empty())
    {
        y1 = ui_drawTextBox( menuFont, "Inventory", x1, y1, 0, 0, text_hgt );

        for(const std::shared_ptr<ObjectProfile> &item : objects)
        {
            //Name depending on if it is identified or not
            std::string itemName = item->isNameKnown() ? item->generateRandomName() : item->getClassName();

            //draw the icon for this item
            MNU_TX_REF icon_ref = mnu_get_txtexture_ref(item, item->getIcon(item->getSkinOverride()) );
            ui_drawImage( 0, TxList_get_valid_ptr( icon_ref ), x1, y1, icon_hgt, icon_hgt, NULL );

            if ( item->getSlotNumber() == SLOT_LEFT + 1 )
            {
                itemName = "  Left: " + itemName;
                ui_drawTextBox( menuFont, itemName.c_str(), x1 + icon_hgt, y1 + text_vert_centering, 0, 0, text_hgt );
                y1 += icon_hgt;
            }
            else if ( item->getSlotNumber() == SLOT_RIGHT + 1 )
            {
                itemName = "  Right: " + itemName;
                ui_drawTextBox( menuFont, itemName.c_str(), x1 + icon_hgt, y1 + text_vert_centering, 0, 0, text_hgt );
                y1 += icon_hgt;
            }
            else
            {
                itemName = "  Item: " + itemName;
                ui_drawTextBox( menuFont, itemName.c_str(), x1 + icon_hgt, y1 + text_vert_centering, 0, 0, text_hgt );
                y1 += icon_hgt;
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
int doChoosePlayer( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static const char * button_text[] = { "Continue", "Back", ""};
    static int sparkle_counter = 0;

    int result = 0;
    int i, x, y;

    const int text_hgt = 20;
    const int icon_hgt = 32;

    const int butt_wid = 200;
    const int butt_hgt = icon_hgt + 10;
    const int butt_spc = 50;

    const int icon_vert_centering = ( butt_hgt - icon_hgt ) / 2;
    const int text_vert_centering = ( butt_hgt - text_hgt ) / 2;

    //This makes sparkles move
    sparkle_counter++;

    switch ( menuState )
    {
        case MM_Begin:

            ego_texture_load_vfs( &background, "mp_data/menu/menu_selectplayers", TRANSCOLOR );

            // make sure we have the proper resources loaded
            if ( gfx_success != gfx_load_blips() )
            {
                log_warning( "Could not load blips!\n" );
            }

            if ( !mnu_load_all_global_icons() )
            {
                log_warning( "Could not load all global icons!\n" );
            }

            // load information for all the players that could be imported
            loadAllImportPlayers("mp_players");

            mnu_SlidyButton_init( 1.0f, button_text );

            menuState = MM_Entering;
            // fall through

        case MM_Entering:
            tipText_set_position( menuFont, "Select characters for each player that is going to play.", 20 );

            menuState = MM_Running;
            break;

        case MM_Running:

            // Draw the background
            if ( mnu_draw_background )
            {
                x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
                y = GFX_HEIGHT - background.imgH;
                ui_drawImage( 0, &background, x, y, 0, 0, NULL );
            }

            y = GFX_HEIGHT / ( MAX_LOCAL_PLAYERS * 2 );

            ui_drawTextBox( menuFont, "PLAYER",    buttonLeft, 10, 0, 0, 20 );
            ui_drawTextBox( menuFont, "CHARACTER", buttonLeft + butt_wid + butt_spc, 10, 0, 0, 20 );

            // Draw the player selection buttons
            for ( size_t i = 0; i < MAX_LOCAL_PLAYERS; i++ )
            {
                std::shared_ptr<LoadPlayerElement> player = nullptr;

                float y1 = y + i * butt_spc;

                //Figure out if there is a valid player selected
                for(const std::shared_ptr<LoadPlayerElement> &checkPlayer : _selectedPlayerList)
                {
                    if ( i == checkPlayer->getSelectedByPlayer() )
                    {
                        player = checkPlayer;
                    }
                }

                // reset the base color each time
                GL_DEBUG( glColor4fv )( white_vec );

                const std::string text = "Player " + std::to_string(i + 1);
                ui_drawTextBox( menuFont, text.c_str(), buttonLeft + butt_spc, y1 + text_vert_centering, 0, 0, icon_hgt );

                // character button
                if ( BUTTON_UP == ui_doButton( 10 + i, ( NULL == player ) ? "Click to select" : player->getName().c_str(), NULL, buttonLeft + butt_wid + butt_spc, y1, butt_wid, butt_hgt ) )
                {
                    currentSelectingPlayer = i;
                    menuState = MM_Entering;
                    result = 1;
                }

                //character icon
                if ( player != NULL )
                {
                    MNU_TX_REF device_icon = MENU_TX_ICON_NULL;

                    ui_drawButton( UI_Nothing, buttonLeft + 2 *( butt_wid + butt_spc ), y1, butt_hgt, butt_hgt, NULL );
                    draw_icon_texture(TxList_get_valid_ptr(player->getIcon()), buttonLeft + 2 *( butt_wid + butt_spc ) + icon_vert_centering, y1 + icon_vert_centering, i, sparkle_counter, icon_hgt);


                    if ( i < MAX_LOCAL_PLAYERS )
                    {
                        int device_type = InputDevices.lst[i].device_type;

                        if ( INPUT_DEVICE_KEYBOARD == device_type )
                        {
                            device_icon = MENU_TX_ICON_KEYB;
                        }
                        else if ( INPUT_DEVICE_MOUSE == device_type )
                        {
                            device_icon = MENU_TX_ICON_MOUS;
                        }
                        else if ( IS_VALID_JOYSTICK( device_type ) )
                        {
                            int ijoy = device_type - ( int )INPUT_DEVICE_JOY;

                            // alternate the joystick icons in case we have a lot of them
                            device_icon = ( 0 == ( ijoy & 1 ) ) ? MENU_TX_ICON_JOYA : MENU_TX_ICON_JOYB;
                        }
                        else
                        {
                            // out of range value
                            device_icon = MENU_TX_ICON_NULL;
                        }

                        ui_drawButton( UI_Nothing, buttonLeft + 2 *( butt_wid + butt_spc ) + butt_spc, y1, butt_hgt, butt_hgt, NULL );
                        ui_drawIcon( device_icon, buttonLeft + 2 *( butt_wid + butt_spc ) + butt_spc + icon_vert_centering, y1 + icon_vert_centering, i, sparkle_counter );
                    }
                }
            }

            // Continue
            if ( !_selectedPlayerList.empty() )
            {
                if ( SDL_KEYDOWN( keyb, SDLK_RETURN ) || BUTTON_UP == ui_doButton( 100, button_text[0], NULL, buttonLeft, buttonTop, 200, 30 ) )
                {
                    menuState = MM_Leaving;
                }
            }

            // Back
            if ( SDL_KEYDOWN( keyb, SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 101, button_text[1], NULL, buttonLeft, buttonTop + 35, 200, 30 ) )
            {
                _selectedPlayerList.clear();
                menuState = MM_Leaving;
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            //fall through

        case MM_Finish:

            oglx_texture_Release( &background );

            menuState = MM_Begin;

            if (_selectedPlayerList.empty())
            {
                result = -1;
                _loadPlayerList.clear();
            }
            else
            {
                // set up the slots and the import stuff for the selected players
                if ( rv_success == mnu_set_local_import_list( &ImportList ) )
                {
                    game_copy_imports( &ImportList );
                }
                else
                {
                    // erase the data in the import folder
                    vfs_removeDirectoryAndContents( "import", VFS_TRUE );
                }

                //free allocated data
                _loadPlayerList.clear();

                result = 2;
            }

            // reset the ui
            ui_Reset();

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
int doChooseCharacter( float deltaTime )
{
    const int x0 = 20, y0 = 20, icon_hgt = 42, text_width = 175, button_repeat = 47;

    static int menuState = MM_Begin;
    static oglx_texture_t background;
    static int    startIndex = 0;
    static int    selectedCharacter = -1;
    static bool loadPlayerInventory = false;
    static int numVertical, numHorizontal;
    static const char * button_text[] = { "Select Character", "None", "" };


    int result = 0;
    int i, x, y;

    switch ( menuState )
    {
        case MM_Begin:
            TxList_free_one(( TX_REF )TX_BARS );
            loadPlayerInventory = true;

            //Figure out if we have already selected something
            selectedCharacter = -1;
            for(int i = 0; i < _loadPlayerList.size(); ++i)
            {
                if(_loadPlayerList[i]->getSelectedByPlayer() == currentSelectingPlayer) 
                {
                    selectedCharacter = i;
                    break;
                }
            }

            ego_texture_load_vfs( &background, "mp_data/menu/menu_selectplayers", TRANSCOLOR );

            TxList_load_one_vfs( "mp_data/bars", ( TX_REF )TX_BARS, INVALID_KEY );

            mnu_SlidyButton_init( 1.0f, button_text );

            numVertical   = ( buttonTop - y0 ) / button_repeat - 1;
            numHorizontal = 1;

            x = x0;
            y = y0;
            for ( i = 0; i < numVertical; i++ )
            {
                int m = i * 5;

                ui_initWidget( mnu_widgetList + m, m, NULL, NULL, NULL, x, y, text_width, icon_hgt );
                ui_widgetAddMask( mnu_widgetList + m, UI_BITS_CLICKED );

                y += button_repeat;
            };

            if ( _loadPlayerList.size() < 10 )
            {
                tipText_set_position( menuFont, "Select your character", 20 );
            }
            else
            {
                tipText_set_position( menuFont, "Select your character\nUse the mouse wheel to scroll.", 20 );
            }

            menuState = MM_Entering;
            // fall through

        case MM_Entering:
            menuState = MM_Running;
            // Simply fall through

        case MM_Running:
            // Figure out how many players we can show without scrolling

            // Draw the background
            x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
            y = GFX_HEIGHT - background.imgH;

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, x, y, 0, 0, NULL );
            }

            // use the mouse wheel to scan the characters
            if ( input_cursor_wheel_event_pending() )
            {
                if ( input_cursor.z > 0 )
                {
                    if ( startIndex + numVertical < _loadPlayerList.size() )
                    {
                        startIndex++;
                    }
                }
                else if ( input_cursor.z < 0 )
                {
                    if ( startIndex > 0 )
                    {
                        startIndex--;
                    }
                }

                input_cursor_finish_wheel_event();
            }

            // Draw the player selection buttons
            x = x0;
            y = y0;
            for ( i = 0; i < numVertical; i++ )
            {
                int m = i * 5;

                if (startIndex + i >= _loadPlayerList.size()) {
                    break;
                }

                const std::shared_ptr<LoadPlayerElement> &loadPlayer = _loadPlayerList[startIndex+i];

                //Skip already selected characters
                if(loadPlayer->getSelectedByPlayer() != -1 && loadPlayer->getSelectedByPlayer() != currentSelectingPlayer) {
                    continue;
                }

                // do the character button
                mnu_widgetList[m].img  = TxList_get_valid_ptr( loadPlayer->getIcon() );
                mnu_widgetList[m].text = loadPlayer->getName().c_str();
                if ( loadPlayer->getSelectedByPlayer() == currentSelectingPlayer )
                {
                    SET_BIT( mnu_widgetList[m].state, UI_BITS_CLICKED );
                }
                else
                {
                    UNSET_BIT( mnu_widgetList[m].state, UI_BITS_CLICKED );
                }

                if ( BUTTON_DOWN == ui_doWidget( mnu_widgetList + m ) )
                {
                    if ( HAS_SOME_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) && startIndex + i != selectedCharacter )
                    {
                        // button has become cursor_clicked
                        size_t newSelection = startIndex + i;
                        loadPlayerInventory  = true;
                        SET_BIT( mnu_widgetList[m].state, UI_BITS_CLICKED );

                        //Unselect previous
                        if(selectedCharacter != -1) {
                            _selectedPlayerList.remove(_loadPlayerList[selectedCharacter]);
                            _loadPlayerList[selectedCharacter]->setSelectedByPlayer(-1);                            
                        }

                        //Select new
                        _selectedPlayerList.push_back(_loadPlayerList[newSelection]);
                        _loadPlayerList[newSelection]->setSelectedByPlayer(newSelection);
                        selectedCharacter = newSelection;
                    }
                    else if ( HAS_NO_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) )
                    {
                        // button has become unclicked
                        UNSET_BIT( mnu_widgetList[m].state, UI_BITS_CLICKED );
                        _selectedPlayerList.remove(_loadPlayerList[selectedCharacter]);
                        _loadPlayerList[selectedCharacter]->setSelectedByPlayer(-1);
                        selectedCharacter = -1;
                    }
                }
            }

            // Buttons for going ahead

            // Select character
            if ( selectedCharacter != -1 )
            {
                if ( SDL_KEYDOWN( keyb, SDLK_RETURN ) || BUTTON_UP == ui_doButton( 100, button_text[0], NULL, buttonLeft, buttonTop, 200, 30 ) )
                {
                    menuState = MM_Leaving;
                }

                // show the stats
                doChooseCharacter_show_stats(_loadPlayerList[selectedCharacter], loadPlayerInventory, GFX_WIDTH - 400, 5, 350, GFX_HEIGHT - 50);
                loadPlayerInventory = false;
            }
            else {
                //Don't draw stats
                doChooseCharacter_show_stats( nullptr, false, GFX_WIDTH - 100, 5, 100, GFX_HEIGHT - 50 );
            }

            // I am not playing!
            if ( SDL_KEYDOWN( keyb, SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 101, button_text[1], NULL, buttonLeft, buttonTop + 35, 200, 30 ) )
            {
                //Unselect any player that might have been selected
                if ( selectedCharacter != -1 )
                {
                    _selectedPlayerList.remove(_loadPlayerList[selectedCharacter]);
                    _loadPlayerList[selectedCharacter]->setSelectedByPlayer(-1);
                }
                menuState = MM_Leaving;
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            // Simply fall through

        case MM_Finish:

            // release all of the temporary profiles
            doChooseCharacter_show_stats( nullptr, false, 0, 0, 0, 0 );

            oglx_texture_Release( &background );
            TxList_free_one(( TX_REF )TX_BARS );

            menuState = MM_Begin;
            result = -1;

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
            GL_DEBUG( glColor4fv )( white_vec );

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
            if ( SDL_KEYDOWN( keyb, SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 5, sz_buttons[4], NULL, buttonLeft, buttonTop + 35 * 4, 200, 30 ) )
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
int doInputOptions_get_input( int waitingforinput, input_device_t * pdevice )
{
    // get a key/button combo for the given device

    control_t * pcontrol = NULL;
    scantag_t * ptag     = NULL;
    int         cnt;
    size_t      max_tag  = 0;
    size_t      tag_count = 0;

    // handle several special conditions for resetting waitingforinput
    if ( SDL_KEYDOWN( keyb, SDLK_ESCAPE ) )
    {
        waitingforinput = -1;
    }
    else if ( NULL == pdevice )
    {
        waitingforinput = -1;
    }
    else if ( waitingforinput < 0 )
    {
        waitingforinput = -1;
    }
    else if ( waitingforinput >= CONTROL_COMMAND_COUNT )
    {
        waitingforinput = -1;
    }

    // exit if there is a dumb value
    if ( waitingforinput < 0 ) return waitingforinput;

    // grab the control
    pcontrol = pdevice->control_lst + waitingforinput;

    // clear out all the old control data
    control_init( pcontrol );

    // how many scantags are there?
    max_tag = scantag_get_count();

    // make sure to update the input
    input_read_all_devices();

    if ( IS_VALID_JOYSTICK( pdevice->device_type ) )
    {
        int ijoy = pdevice->device_type - INPUT_DEVICE_JOY;

        for ( size_t tag_idx = 0; tag_idx < max_tag; tag_idx++ )
        {
            ptag = scantag_get_tag( tag_idx );
            if ( NULL == ptag ) continue;

            if ( SCANTAG_JOYBUTTON( joy_lst + ijoy, ptag->value ) )
            {
                pcontrol->loaded = true;
                pcontrol->tag_bits |= ptag->value;

                // count the valid tags
                tag_count++;
            }
        }
    }
    else if ( INPUT_DEVICE_MOUSE  == pdevice->device_type )
    {
        for ( size_t tag_idx = 0; tag_idx < max_tag; tag_idx++ )
        {
            ptag = scantag_get_tag( tag_idx );
            if ( NULL == ptag ) continue;

            if ( SCANTAG_MOUSBUTTON( ptag->value ) )
            {
                pcontrol->loaded = true;
                pcontrol->tag_bits |= ptag->value;

                // count the valid tags
                tag_count++;
            }
        }
    }

    // grab any key combinations
    for ( cnt = 0; cnt < SDLK_LAST && pcontrol->tag_key_count < MAXCONTROLKEYS; cnt++ )
    {
        if ( !SCANTAG_KEYMODDOWN( cnt ) ) continue;

        ptag = scantag_find_value( NULL, 'K', cnt );
        if ( NULL == ptag ) continue;

        pcontrol->loaded = true;
        pcontrol->tag_key_lst[pcontrol->tag_key_count] = ptag->value;
        pcontrol->tag_key_count++;

        // add in any tag keymods
        pcontrol->tag_key_mods |= scancode_get_kmod( ptag->value );

        // count the valid tags
        tag_count++;
    }

    // are we done?
    if ( tag_count > 0 )
    {
        waitingforinput = -1;
    }

    return waitingforinput;
}

//--------------------------------------------------------------------------------------------
int doInputOptions( float deltaTime )
{
    static int menuState = MM_Begin;
    static int menuChoice = 0;
    static int waitingforinput = -1;
    static bool device_found = true;
    static bool update_input_type;

    enum extra_input_strings
    {
        string_device = CONTROL_COMMAND_COUNT,
        string_player,
        string_save,
        string_count
    };

    static STRING button_text[string_count];

    Sint8  result = 0;
    static Uint8 player = 0;

    Uint32              i;
    input_device_t      * pdevice = NULL;
    control_t           * pcontrol = NULL;

    pdevice = NULL;
    if ( player < MAX_LOCAL_PLAYERS )
    {
        pdevice = InputDevices.lst + player;
    };

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            menuChoice = 0;
            menuState = MM_Entering;
            // let this fall through into MM_Entering

            waitingforinput = -1;
            player = 0;
            update_input_type = true;

            //Clip to valid value
            if ( INPUT_DEVICE_UNKNOWN == InputDevices.lst[player].device_type )
            {
                InputDevices.lst[player].device_type = INPUT_DEVICE_BEGIN;
            }

            //Prepare all buttons
            for ( i = 0; i < string_device; i++ )
            {
                button_text[i][0] = CSTR_END;
            }
            strncpy( button_text[string_device], translate_input_type_to_string( InputDevices.lst[player].device_type ), sizeof( STRING ) );
            strncpy( button_text[string_player], "Player 1", sizeof( STRING ) );
            strncpy( button_text[string_save],   "Save Settings", sizeof( STRING ) );

            tipText_set_position( menuFont, "Change input settings here.", 20 );

            // Load the global icons (keyboard, mouse, etc.)
            if ( !mnu_load_all_global_icons() )
            {
                log_warning( "Could not load all global icons!\n" );
            }

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
            GL_DEBUG( glColor4fv )( white_vec );

            //Detect if input is availible and update the input type button accordingly
            if ( update_input_type )
            {
                update_input_type = false;
                device_found = input_device_is_enabled( pdevice );
                snprintf( button_text[string_device], sizeof( STRING ), "%s", translate_input_type_to_string( pdevice->device_type ) );
            }

            // Handle waiting for input
            waitingforinput = doInputOptions_get_input( waitingforinput, pdevice );

            //Draw the buttons
            if ( NULL != pdevice && -1 == waitingforinput )
            {
                // update the control names
                for ( i = 0; i < string_device; i++ )
                {
                    pcontrol = pdevice->control_lst + i;

                    scantag_get_string( pdevice->device_type, pcontrol, button_text[i], SDL_arraysize( button_text[i] ) );
                }
                //for ( /* nothing */; i < string_device; i++ )
                //{
                //    button_text[i][0] = CSTR_END;
                //}
            }

            // Left hand
            ui_drawTextBox( menuFont, "LEFT HAND", buttonLeft, GFX_HEIGHT - 470, 0, 0, 20 );
            if ( CSTR_END != button_text[CONTROL_LEFT_USE][0] )
            {
                ui_drawTextBox( menuFont, "Use:", buttonLeft, GFX_HEIGHT - 440, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 1, button_text[CONTROL_LEFT_USE], menuFont, buttonLeft + 100, GFX_HEIGHT - 440, 140, 30 ) )
                {
                    waitingforinput = CONTROL_LEFT_USE;
                    strncpy( button_text[CONTROL_LEFT_USE], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != button_text[CONTROL_LEFT_GET][0] )
            {
                ui_drawTextBox( menuFont, "Get/Drop:", buttonLeft, GFX_HEIGHT - 410, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 2, button_text[CONTROL_LEFT_GET], menuFont, buttonLeft + 100, GFX_HEIGHT - 410, 140, 30 ) )
                {
                    waitingforinput = CONTROL_LEFT_GET;
                    strncpy( button_text[CONTROL_LEFT_GET], "...", sizeof( STRING ) );
                }
            }

            // Right hand
            ui_drawTextBox( menuFont, "RIGHT HAND", buttonLeft + 300, GFX_HEIGHT - 470, 0, 0, 20 );
            if ( CSTR_END != button_text[CONTROL_RIGHT_USE][0] )
            {
                ui_drawTextBox( menuFont, "Use:", buttonLeft + 300, GFX_HEIGHT - 440, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 4, button_text[CONTROL_RIGHT_USE], menuFont, buttonLeft + 400, GFX_HEIGHT - 440, 140, 30 ) )
                {
                    waitingforinput = CONTROL_RIGHT_USE;
                    strncpy( button_text[CONTROL_RIGHT_USE], "...", sizeof( STRING ) );
                }
            }
            if ( CSTR_END != button_text[CONTROL_RIGHT_GET][0] )
            {
                ui_drawTextBox( menuFont, "Get/Drop:", buttonLeft + 300, GFX_HEIGHT - 410, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 5, button_text[CONTROL_RIGHT_GET], menuFont, buttonLeft + 400, GFX_HEIGHT - 410, 140, 30 ) )
                {
                    waitingforinput = CONTROL_RIGHT_GET;
                    strncpy( button_text[CONTROL_RIGHT_GET], "...", sizeof( STRING ) );
                }
            }

            // Controls
            ui_drawTextBox( menuFont, "CONTROLS", buttonLeft, GFX_HEIGHT - 350, 0, 0, 20 );
            if ( CSTR_END != button_text[CONTROL_JUMP][0] )
            {
                ui_drawTextBox( menuFont, "Jump:", buttonLeft, GFX_HEIGHT - 320, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 7, button_text[CONTROL_JUMP], menuFont, buttonLeft + 100, GFX_HEIGHT - 320, 140, 30 ) )
                {
                    waitingforinput = CONTROL_JUMP;
                    strncpy( button_text[CONTROL_JUMP], "...", sizeof( STRING ) );
                }
            }

            ui_drawTextBox( menuFont, "Sneak:", buttonLeft, GFX_HEIGHT - 290, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 8, button_text[CONTROL_SNEAK], menuFont, buttonLeft + 100, GFX_HEIGHT - 290, 140, 30 ) )
            {
                waitingforinput = CONTROL_SNEAK;
                strncpy( button_text[CONTROL_SNEAK], "...", sizeof( STRING ) );
            }

            ui_drawTextBox( menuFont, "Inventory:", buttonLeft, GFX_HEIGHT - 260, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 3, button_text[CONTROL_INVENTORY], menuFont, buttonLeft + 100, GFX_HEIGHT - 260, 140, 30 ) )
            {
                waitingforinput = CONTROL_INVENTORY;
                strncpy( button_text[CONTROL_INVENTORY], "...", sizeof( STRING ) );
            }

            //Only keyboard has the UP, DOWN, LEFT and RIGHT buttons
            if ( pdevice->device_type == INPUT_DEVICE_KEYBOARD )
            {
                if ( CSTR_END != button_text[CONTROL_UP][0] )
                {
                    ui_drawTextBox( menuFont, "Up:", buttonLeft, GFX_HEIGHT - 230, 0, 0, 20 );
                    if ( BUTTON_UP == ui_doButton( 9, button_text[CONTROL_UP], menuFont, buttonLeft + 100, GFX_HEIGHT - 230, 140, 30 ) )
                    {
                        waitingforinput = CONTROL_UP;
                        strncpy( button_text[CONTROL_UP], "...", sizeof( STRING ) );
                    }
                }
                if ( CSTR_END != button_text[CONTROL_DOWN][0] )
                {
                    ui_drawTextBox( menuFont, "Down:", buttonLeft, GFX_HEIGHT - 200, 0, 0, 20 );
                    if ( BUTTON_UP == ui_doButton( 10, button_text[CONTROL_DOWN], menuFont, buttonLeft + 100, GFX_HEIGHT - 200, 140, 30 ) )
                    {
                        waitingforinput = CONTROL_DOWN;
                        strncpy( button_text[CONTROL_DOWN], "...", sizeof( STRING ) );
                    }
                }
                if ( CSTR_END != button_text[CONTROL_LEFT][0] )
                {
                    ui_drawTextBox( menuFont, "Left:", buttonLeft, GFX_HEIGHT - 170, 0, 0, 20 );
                    if ( BUTTON_UP == ui_doButton( 11, button_text[CONTROL_LEFT], menuFont, buttonLeft + 100, GFX_HEIGHT - 170, 140, 30 ) )
                    {
                        waitingforinput = CONTROL_LEFT;
                        strncpy( button_text[CONTROL_LEFT], "...", sizeof( STRING ) );
                    }
                }
                if ( CSTR_END != button_text[CONTROL_RIGHT][0] )
                {
                    ui_drawTextBox( menuFont, "Right:", buttonLeft, GFX_HEIGHT - 140, 0, 0, 20 );
                    if ( BUTTON_UP == ui_doButton( 12, button_text[CONTROL_RIGHT], menuFont, buttonLeft + 100, GFX_HEIGHT - 140, 140, 30 ) )
                    {
                        waitingforinput = CONTROL_RIGHT;
                        strncpy( button_text[CONTROL_RIGHT], "...", sizeof( STRING ) );
                    }
                }
            }

            // Camera
            ui_drawTextBox( menuFont, "CAMERA CONTROL", buttonLeft + 300, GFX_HEIGHT - 320, 0, 0, 20 );
            if ( pdevice->device_type != INPUT_DEVICE_KEYBOARD )
            {
                // single button camera control
                ui_drawTextBox( menuFont, "Camera:", buttonLeft + 300, GFX_HEIGHT - 290, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 13, button_text[CONTROL_CAMERA], menuFont, buttonLeft + 450, GFX_HEIGHT - 290, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA;
                    strncpy( button_text[CONTROL_CAMERA], "...", sizeof( STRING ) );
                }
            }

            else
            {
                ui_drawTextBox( menuFont, "Zoom In:", buttonLeft + 300, GFX_HEIGHT - 290, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 14, button_text[CONTROL_CAMERA_IN], menuFont, buttonLeft + 450, GFX_HEIGHT - 290, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA_IN;
                    strncpy( button_text[CONTROL_CAMERA_IN], "...", sizeof( STRING ) );
                }

                ui_drawTextBox( menuFont, "Zoom Out:", buttonLeft + 300, GFX_HEIGHT - 260, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 15, button_text[CONTROL_CAMERA_OUT], menuFont, buttonLeft + 450, GFX_HEIGHT - 260, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA_OUT;
                    strncpy( button_text[CONTROL_CAMERA_OUT], "...", sizeof( STRING ) );
                }

                ui_drawTextBox( menuFont, "Rotate Left:", buttonLeft + 300, GFX_HEIGHT - 230, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 16, button_text[CONTROL_CAMERA_LEFT], menuFont, buttonLeft + 450, GFX_HEIGHT - 230, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA_LEFT;
                    strncpy( button_text[CONTROL_CAMERA_LEFT], "...", sizeof( STRING ) );
                }

                ui_drawTextBox( menuFont, "Rotate Right:", buttonLeft + 300, GFX_HEIGHT - 200, 0, 0, 20 );
                if ( BUTTON_UP == ui_doButton( 17, button_text[CONTROL_CAMERA_RIGHT], menuFont, buttonLeft + 450, GFX_HEIGHT - 200, 140, 30 ) )
                {
                    waitingforinput = CONTROL_CAMERA_RIGHT;
                    strncpy( button_text[CONTROL_CAMERA_RIGHT], "...", sizeof( STRING ) );
                }
            }

            // The select controller button
            ui_drawTextBox( menuFont, "INPUT DEVICE:", buttonLeft + 300, 55, 0, 0, 20 );
            if ( BUTTON_UP ==  ui_doImageButtonWithText( 18, mnu_TxList_get_valid_ptr(( MNU_TX_REF )( MENU_TX_ICON_KEYB + pdevice->device_type ) ), button_text[string_device], menuFont, buttonLeft + 450, 50, 200, 40 ) )
            {
                // switch to next controller type
                int old_device_type = pdevice->device_type;
                int new_device_type = pdevice->device_type;

                new_device_type++;

                // clamp out of range values
                if ( new_device_type < 0 )
                {
                    // fix a negative value
                    new_device_type = INPUT_DEVICE_JOY + MAX_JOYSTICK - 1;
                }
                else if ( new_device_type >= INPUT_DEVICE_JOY + MAX_JOYSTICK )
                {
                    // make all large values wrap around
                    new_device_type = 0;
                }

                pdevice->device_type = new_device_type;

                update_input_type = ( new_device_type != old_device_type );
            }

            //Display warning if input device was not found
            if ( !device_found ) ui_drawTextBox( menuFont, "WARNING: Input device not found!", buttonLeft + 300, 95, 0, 0, 20 );

            // The select player button
            ui_drawTextBox( menuFont, "SELECT PLAYER:", buttonLeft, 55, 0, 0, 20 );
            if ( BUTTON_UP ==  ui_doButton( 19, button_text[string_player], menuFont, buttonLeft + 150, 50, 100, 30 ) )
            {
                player++;
                player %= MAX_LOCAL_PLAYERS;

                snprintf( button_text[string_player], sizeof( STRING ), "Player %i", player + 1 );
                update_input_type = true;
            }

            // Save settings button
            if ( BUTTON_UP == ui_doButton( 20, button_text[string_save], menuFont, buttonLeft, GFX_HEIGHT - 60, 200, 30 ) )
            {
                // save settings and go back
                player = 0;
                input_settings_save_vfs( "controls.txt", -1 );
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

            cfg.message_count_req = CLIP( cfg.message_count_req, 0, EGO_MESSAGE_MAX );
            if ( 0 == cfg.message_count_req )
            {
                snprintf( Cmaxmessage, SDL_arraysize( Cmaxmessage ), "None" );          // Set to default
            }
            else
            {
                snprintf( Cmaxmessage, SDL_arraysize( Cmaxmessage ), "%i", cfg.message_count_req );
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
                if ( cfg.feedback == EGO_FEEDBACK_TYPE_TEXT ) sz_buttons[5] = "Enabled";
                else                                          sz_buttons[5] = "Debug";
            }

            // Fall trough
            menuState = MM_Running;
            //break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4fv )( white_vec );

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
                    strncpy( szDifficulty, "FORGIVING (Easy)\n - Players gain no bonus XP \n - 15%% XP loss upon death\n - Monsters take 25%% extra damage from players\n - Players take 50%% less damage from monsters\n - Halves the chance for Kursed items\n - Cannot unlock the final level in this mode\n - Life and Mana is refilled after quitting a module", SDL_arraysize( szDifficulty ) );
                    break;
                case GAME_NORMAL:
                    strncpy( szDifficulty, "CHALLENGING (Normal)\n - Players gain 10%% bonus XP \n - 15%% XP loss upon death \n - 15%% money loss upon death \n - Players take 30%% less damage from monsters", SDL_arraysize( szDifficulty ) );
                    break;
                case GAME_HARD:
                    strncpy( szDifficulty, "PUNISHING (Hard)\n - Monsters award 20%% extra xp! \n - 15%% XP loss upon death\n - 15%% money loss upon death\n - No respawning\n - Channeling life can kill you\n - Players take full damage from monsters\n - Doubles the chance for Kursed items", SDL_arraysize( szDifficulty ) );
                    break;
            }
            str_add_linebreaks( szDifficulty, SDL_arraysize( szDifficulty ), 30 );
            ui_drawTextBox( menuFont, szDifficulty, buttonLeft, 100, 0, 0, 20 );

            // Text messages
            ui_drawTextBox( menuFont, "Max  Messages:", buttonLeft + 350, 50, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 2, sz_buttons[1], menuFont, buttonLeft + 515, 50, 75, 30 ) )
            {
                cfg.message_count_req++;
                if ( cfg.message_count_req > EGO_MESSAGE_MAX ) cfg.message_count_req = 0;
                if ( cfg.message_count_req < EGO_MESSAGE_STD && cfg.message_count_req != 0 ) cfg.message_count_req = EGO_MESSAGE_STD;

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
                cfg.fps_allowed = TO_C_BOOL( !cfg.fps_allowed );
                if ( cfg.fps_allowed )   sz_buttons[4] = "On";
                else                     sz_buttons[4] = "Off";
            }

            // Feedback
            ui_drawTextBox( menuFont, "Floating Text:", buttonLeft + 350, 250, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 6, sz_buttons[5], menuFont, buttonLeft + 515, 250, 75, 30 ) )
            {
                if ( cfg.dev_mode )
                {
                    cfg.feedback = ( EGO_FEEDBACK_TYPE )( cfg.feedback + 1 );
                    if ( cfg.feedback > EGO_FEEDBACK_TYPE_NUMBER ) cfg.feedback = EGO_FEEDBACK_TYPE_OFF;
                }
                else
                {
                    if ( EGO_FEEDBACK_TYPE_OFF == cfg.feedback )
                    {
                        cfg.feedback = EGO_FEEDBACK_TYPE_TEXT;
                    }
                    else
                    {
                        cfg.feedback = EGO_FEEDBACK_TYPE_OFF;
                    }
                }

                switch ( cfg.feedback )
                {
                    case EGO_FEEDBACK_TYPE_OFF:    sz_buttons[5] = "Disabled"; break;
                    case EGO_FEEDBACK_TYPE_TEXT:   sz_buttons[5] = "Enabled";  break;
                    case EGO_FEEDBACK_TYPE_NUMBER: sz_buttons[5] = "Debug";    break;
                }
            }

            // Save settings
            if ( BUTTON_UP == ui_doButton( 7, sz_buttons[6], menuFont, buttonLeft, GFX_HEIGHT - 60, 200, 30 ) )
            {
                //apply changes
                config_synch( &cfg, false );

                // save the setup file
                setup_write_vfs();

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
            GL_DEBUG( glColor4fv )( white_vec );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0, NULL );
            }

            ui_drawTextBox( menuFont, "Sound:", buttonLeft, GFX_HEIGHT - 270, 0, 0, 20 );

            // Buttons
            if ( BUTTON_UP == ui_doButton( 1, sz_buttons[0], menuFont, buttonLeft + 150, GFX_HEIGHT - 270, 100, 30 ) )
            {
                cfg.sound_allowed = TO_C_BOOL( !cfg.sound_allowed );
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
                cfg.music_allowed = TO_C_BOOL( !cfg.music_allowed );
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
                cfg.sound_highquality = TO_C_BOOL( !cfg.sound_highquality );
                sz_buttons[6] = cfg.sound_highquality ? "Normal" : "High";
            }

            //Footfall sounds
            ui_drawTextBox( menuFont, "Footstep Sounds:", buttonLeft + 300, GFX_HEIGHT - 235, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 8, sz_buttons[7], menuFont, buttonLeft + 450, GFX_HEIGHT - 235, 100, 30 ) )
            {
                cfg.sound_footfall = TO_C_BOOL( !cfg.sound_footfall );
                sz_buttons[7] = cfg.sound_footfall ? "Enabled" : "Disabled";
            }

            //Save settings
            if ( BUTTON_UP == ui_doButton( 9, sz_buttons[8], menuFont, buttonLeft, GFX_HEIGHT - 60, 200, 30 ) )
            {
                //apply changes
                config_synch( &cfg, false );

                // save the setup file
                setup_write_vfs();

                // Reload the sound system
                _audioSystem.reset();

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
bool doVideoOptions_coerce_aspect_ratio( int width, int height, float * pratio, STRING * psz_ratio )
{
    /// @author BB
    /// @details coerce the aspect ratio of the screen to some standard size

    float req_aspect_ratio;

    if ( 0 == height || NULL == pratio || NULL == psz_ratio ) return false;

    req_aspect_ratio = ( float )width / ( float )height;

    if ( req_aspect_ratio > 0.0f && req_aspect_ratio < 0.5f*(( 5.0f / 4.0f ) + ( 4.0f / 3.0f ) ) )
    {
        *pratio = 5.0f / 4.0f;
        strncpy( *psz_ratio, "5:4", SDL_arraysize( *psz_ratio ) );
    }
    else if ( req_aspect_ratio >= 0.5f*(( 5.0f / 4.0f ) + ( 4.0f / 3.0f ) ) && req_aspect_ratio < 0.5f*(( 4.0f / 3.0f ) + ( 8.0f / 5.0f ) ) )
    {
        *pratio = 4.0f / 3.0f;
        strncpy( *psz_ratio, "4:3", SDL_arraysize( *psz_ratio ) );
    }
    else if ( req_aspect_ratio >= 0.5f*(( 4.0f / 3.0f ) + ( 8.0f / 5.0f ) ) && req_aspect_ratio < 0.5f*(( 8.0f / 5.0f ) + ( 5.0f / 3.0f ) ) )
    {
        *pratio = 8.0f / 5.0f;
        strncpy( *psz_ratio, "8:5", SDL_arraysize( *psz_ratio ) );
    }
    else if ( req_aspect_ratio >= 0.5f*(( 8.0f / 5.0f ) + ( 5.0f / 3.0f ) ) && req_aspect_ratio < 0.5f*(( 5.0f / 3.0f ) + ( 16.0f / 9.0f ) ) )
    {
        *pratio = 5.0f / 3.0f;
        strncpy( *psz_ratio, "5:3", SDL_arraysize( *psz_ratio ) );
    }
    else
    {
        *pratio = 16.0f / 9.0f;
        strncpy( *psz_ratio, "16:9", SDL_arraysize( *psz_ratio ) );
    }

    return true;

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
                strncpy( sz_aspect_ratio, "4:3", SDL_arraysize( sz_aspect_ratio ) );
                break;

            case 640:
                pcfg->scry_req = 480;
                strncpy( sz_aspect_ratio, "4:3", SDL_arraysize( sz_aspect_ratio ) );
                break;

            case 800:
                pcfg->scry_req = 600;
                strncpy( sz_aspect_ratio, "4:3", SDL_arraysize( sz_aspect_ratio ) );
                break;

                // 1280 can be both widescreen and normal
            case 1280:
                if ( pcfg->scry_req > 800 )
                {
                    pcfg->scry_req = 1024;
                    strncpy( sz_aspect_ratio, "5:4", SDL_arraysize( sz_aspect_ratio ) );
                }
                else
                {
                    pcfg->scry_req = 800;
                    strncpy( sz_aspect_ratio, "8:5", SDL_arraysize( sz_aspect_ratio ) );
                }
                break;

                // Widescreen resolutions
            case 1440:
                pcfg->scry_req = 900;
                strncpy( sz_aspect_ratio, "8:5", SDL_arraysize( sz_aspect_ratio ) );
                break;

            case 1680:
                pcfg->scry_req = 1050;
                strncpy( sz_aspect_ratio, "8:5", SDL_arraysize( sz_aspect_ratio ) );
                break;

            case 1920:
                pcfg->scry_req = 1200;
                strncpy( sz_aspect_ratio, "8:5", SDL_arraysize( sz_aspect_ratio ) );
                break;

                // unknown
            default:
                doVideoOptions_coerce_aspect_ratio( pcfg->scrx_req, pcfg->scry_req, &aspect_ratio, &sz_aspect_ratio );
                break;
        }
    }

    snprintf( *psz_screen_size, SDL_arraysize( *psz_screen_size ), "%dx%d - %s", pcfg->scrx_req, pcfg->scry_req, sz_aspect_ratio );

    return true;
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

    static bool widescreen;
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
                    cfg.use_perspective    = false;
                    cfg.background_allowed = false;
                    cfg.overlay_allowed    = false;
                    sz_buttons[but_3dfx] = "Off";
                }
            }
            else                              // Set to defaults
            {
                cfg.use_perspective    = false;
                cfg.background_allowed = false;
                cfg.overlay_allowed    = false;
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
                snprintf( sz_screen_size, SDL_arraysize( sz_screen_size ), "%dx%d", cfg.scrx_req, cfg.scry_req );
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
            GL_DEBUG( glColor4fv )( white_vec );

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
                cfg.use_dither = TO_C_BOOL( !cfg.use_dither );
                sz_buttons[but_dither] = cfg.use_dither ? "Yes" : "No";
            }

            // Fullscreen
            ui_drawTextBox( menuFont, "Fullscreen:", buttonLeft, GFX_HEIGHT - 110, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 4, sz_buttons[but_fullscreen], menuFont, buttonLeft + 150, GFX_HEIGHT - 110, 100, 30 ) )
            {
                cfg.fullscreen_req = TO_C_BOOL( !cfg.fullscreen_req );

                sz_buttons[but_fullscreen] = cfg.fullscreen_req ? "True" : "False";
            }

            // Reflection
            ui_drawTextBox( menuFont, "Reflections:", buttonLeft, GFX_HEIGHT - 250, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 5, sz_buttons[but_reflections], menuFont, buttonLeft + 150, GFX_HEIGHT - 250, 100, 30 ) )
            {

                if ( cfg.reflect_allowed && 0 == cfg.reflect_fade && cfg.reflect_prt )
                {
                    cfg.reflect_allowed = false;
                    cfg.reflect_fade = 255;
                    cfg.reflect_prt = false;
                    sz_buttons[but_reflections] = "Off";
                }
                else
                {
                    if ( cfg.reflect_allowed && !cfg.reflect_prt )
                    {
                        sz_buttons[but_reflections] = "Medium";
                        cfg.reflect_fade = 255;
                        cfg.reflect_prt = true;
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
                            cfg.reflect_allowed = true;
                            cfg.reflect_fade = 255;
                            sz_buttons[but_reflections] = "Low";
                            cfg.reflect_prt = false;
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
                    cfg.texturefilter_req = ( tx_filter_t )(( int )cfg.texturefilter_req + 1 );
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
                    cfg.shadow_allowed = false;
                    cfg.shadow_sprite = false;                // Just in case
                    sz_buttons[but_shadow] = "Off";
                }
                else
                {
                    if ( cfg.shadow_allowed && cfg.shadow_sprite )
                    {
                        sz_buttons[but_shadow] = "Best";
                        cfg.shadow_sprite = false;
                    }
                    else
                    {
                        cfg.shadow_allowed = true;
                        cfg.shadow_sprite = true;
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
                    cfg.use_phong          = false;
                    cfg.use_perspective    = false;
                    cfg.overlay_allowed    = false;
                    cfg.background_allowed = false;
                    sz_buttons[but_3dfx] = "Off";
                }
                else
                {
                    if ( !cfg.use_phong )
                    {
                        sz_buttons[but_3dfx] = "Okay";
                        cfg.use_phong = true;
                    }
                    else
                    {
                        if ( !cfg.use_perspective && cfg.overlay_allowed && cfg.background_allowed )
                        {
                            sz_buttons[but_3dfx] = "Superb";
                            cfg.use_perspective = true;
                        }
                        else
                        {
                            cfg.overlay_allowed = true;
                            cfg.background_allowed = true;
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
                    cfg.twolayerwater_allowed = false;
                }
                else
                {
                    sz_buttons[but_multiwater] = "On";
                    cfg.twolayerwater_allowed = true;
                }
            }

            // Max particles
            ui_drawTextBox( menuFont, "Max Particles:", buttonLeft + 300, GFX_HEIGHT - 180, 0, 0, 20 );

            if ( PMod->active )
            {
                snprintf( Cmaxparticles, SDL_arraysize( Cmaxparticles ), "%i (%i currently used)", maxparticles, maxparticles - PrtList_count_free() );
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
                bool old_widescreen = widescreen;

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

                //apply changes
                config_synch( &cfg, false );

                // save the setup file
                setup_write_vfs();

                // Reload some of the graphics
                gfx_system_load_assets();
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
int doShowLoadingScreen( float deltaTime )
{
    static Font   *font;
    static int     menuState = MM_Begin;
    static int     count;
    static char    buffer[1024] = EMPTY_CSTR;
    static STRING  game_tip;

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
                game_tip[0] = CSTR_END;
                if ( cfg.difficulty <= GAME_NORMAL )
                {
                    const char * hint_ptr = NULL;

                    if ( 0 == mnu_Tips_local.count )
                    {
                        mnu_Tips_local_load_vfs( &mnu_Tips_local );
                    }

                    hint_ptr = mnu_Tips_get_hint( &mnu_Tips_global, &mnu_Tips_local );
                    if ( VALID_CSTR( hint_ptr ) )
                    {
                        strncpy( game_tip, hint_ptr, SDL_arraysize( game_tip ) );
                        str_add_linebreaks( game_tip, SDL_arraysize( game_tip ), 50 );
                    }
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

                GL_DEBUG( glColor4fv )( white_vec );

                // the module name
                ui_drawTextBox( font, mnu_ModList.lst[( MOD_REF )selectedModule].base.longname, 50, 80, 291, 230, 20 );

                // Draw a text box
                ui_drawTextBox( menuFont, buffer, 50, 120, 291, 230, 20 );

                // Loading game... please wait
                fnt_getTextSize( font, "Loading module...", &text_w, &text_h );
                ui_drawTextBox( font, "Loading module...", ( GFX_WIDTH / 2 ) - text_w / 2, GFX_HEIGHT - 200, 0, 0, 20 );

                // Draw the game tip
                if ( VALID_CSTR( game_tip ) )
                {
                    fnt_getTextSize( menuFont, "GAME TIP", &text_w, &text_h );
                    ui_drawTextBox( font, "GAME TIP", ( GFX_WIDTH / 2 ) - ( text_w / 2 ), GFX_HEIGHT - 150, 0, 0, 20 );

                    /// @note ZF@> : this doesnt work as I intended, fnt_get_TextSize() does not take line breaks into account
                    /// @note BB@> : fixed by changing the function to fnt_getTextBoxSize()
                    fnt_getTextBoxSize( menuFont, 20, game_tip, &text_w, &text_h );
                    ui_drawTextBox( menuFont, game_tip, ( GFX_WIDTH / 2 ) - ( text_w / 2 ), GFX_HEIGHT - 110, 0, 0, 20 );
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
            GL_DEBUG( glColor4fv )( white_vec );

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
            if ( SDL_KEYDOWN( keyb, SDLK_ESCAPE ) ) menuChoice = 3;

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
            GL_DEBUG( glColor4fv )( white_vec );

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
            if ( SDL_KEYDOWN( keyb, SDLK_ESCAPE ) )
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
                bool reloaded = false;

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

                    start_new_player = false;

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

                    /// @note ZF@> Reload all players since their quest logs may have changed and new modules unlocked
                    loadAllImportPlayers("mp_players");
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
                if ( 1 == result )      { mnu_begin_menu( emnu_ChooseModule ); start_new_player = true; }
                else if ( 2 == result ) { mnu_begin_menu( emnu_ChoosePlayer ); start_new_player = false; }
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
                    start_new_player = true;
                }
                else if ( 2 == result )
                {
                    mnu_begin_menu( emnu_ChoosePlayer );
                    start_new_player = false;
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
            else if ( 1 == result ) mnu_begin_menu( emnu_ChoosePlayerCharacter );
            else if ( 2 == result ) mnu_begin_menu( emnu_ChooseModule );

            break;

        case emnu_ChoosePlayerCharacter:
            result = doChooseCharacter( deltaTime );

            if ( -1 == result )     { mnu_end_menu(); retval = MENU_END; }

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
            result = doShowLoadingScreen( deltaTime );
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

                    bool reloaded = false;

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
                    process_terminate( PROC_PBASE( GProc ) );
                    process_start( PROC_PBASE( GProc ) );

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

    fnt_getTextBoxSize( font, spacing, text, &w, &h );

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
    fnt_getTextBoxSize( font, 20, text, &w, &h );

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
void mnu_load_all_module_images_vfs()
{
    /// @author ZZ
    /// @details This function loads the title image for each module.  Modules without a
    ///     title are marked as invalid

    MOD_REF imod;
    vfs_FILE* filesave;

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
        else if ( mnu_test_module_by_index( imod, 0, NULL ) )
        {
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
MNU_TX_REF mnu_get_txtexture_ref( const std::shared_ptr<ObjectProfile> &profile, const MNU_TX_REF default_ref )
{
    /// @author BB
    /// @details This function gets the proper icon for a an object profile.
    //
    //     In the character preview section of the menu system, we do not load
    //     entire profiles, just the character definition file ("data.txt")
    //     and one icon. Sometimes, though the item is actually a spell effect which means
    //     that we need to display the book icon.

    MNU_TX_REF icon_ref = ( MNU_TX_REF )TX_ICON_NULL;
    bool is_spell_fx, is_book, draw_book;


    // what do we need to draw?
    is_spell_fx = ( profile->getSpellEffectType() >= 0 );
    is_book     = ( SPELLBOOK == profile->getSlotNumber() );
    draw_book   = ( is_book || is_spell_fx );

    if ( !draw_book )
    {
        icon_ref = default_ref;
    }
    else if ( draw_book )
    {
        SKIN_T iskin = profile->getSkinOverride();

        icon_ref = _profileSystem.getSpellBookIcon(iskin);
    }

    return icon_ref;
}

//--------------------------------------------------------------------------------------------
// module utilities
//--------------------------------------------------------------------------------------------
int mnu_get_mod_number( const char *szModName )
{
    /// @author ZZ
    /// @details This function returns -1 if the module does not exist locally, the module
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
bool mnu_test_module_by_index( const MOD_REF modnumber, size_t buffer_len, char * buffer )
{
    mnu_module_t * pmod;

    if ( INVALID_MOD( modnumber ) ) return false;
    pmod = mnu_ModList.get_ptr(modnumber);

    // First check if we are in developers mode or that the right module has been beaten before
    if ( cfg.dev_mode )
    {
        return true;
    }

    if ( module_has_idsz_vfs( pmod->base.reference, pmod->base.unlockquest.id, buffer_len, buffer ) )
    {
        return true;
    }

    if ( pmod->base.importamount > 0 )
    {
        // If that did not work, then check all selected players directories, but only if it isn't a starter module
        for(const std::shared_ptr<LoadPlayerElement> &player : _selectedPlayerList)
        {
            // find beaten quests or quests with proper level
            if(!player->hasQuest(pmod->base.unlockquest.id, pmod->base.unlockquest.level)) {
                return false;
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool mnu_test_module_by_name( const char *szModName )
{
    /// @author ZZ
    /// @details This function tests to see if a module can be entered by
    ///    the players

    bool retval;

    // find the module by name
    int modnumber = mnu_get_mod_number( szModName );

    retval = false;
    if ( modnumber >= 0 )
    {
        retval = mnu_test_module_by_index( ( MOD_REF )modnumber, 0, NULL );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void mnu_module_init( mnu_module_t * pmod )
{
    if ( NULL == pmod ) return;

    // clear the module
    BLANK_STRUCT_PTR( pmod )

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

    // reset the texture cache
    mnu_TxList_release_all();

    // Search for all .mod directories and load the module info
    ctxt = vfs_findFirst( "mp_modules", "mod", VFS_SEARCH_DIR );
    vfs_ModPath = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( vfs_ModPath ) && mnu_ModList.count < MAX_MODULE )
    {
        mnu_module_t * pmod = mnu_ModList.get_ptr(mnu_ModList.count);

        // clear the module
        mnu_module_init( pmod );

        // save the filename
        snprintf( loadname, SDL_arraysize( loadname ), "%s/gamedat/menu.txt", vfs_ModPath );
        if ( NULL != module_load_info_vfs( loadname, &( pmod->base ) ) )
        {
            mnu_ModList.count++;

            // mark the module data as loaded
            pmod->loaded = true;

            // save the module path
            strncpy( pmod->vfs_path, vfs_ModPath, SDL_arraysize( pmod->vfs_path ) );

            /// @note just because we can't load the title image DOES NOT mean that we ignore the module
            // load title image
            snprintf( loadname, SDL_arraysize( loadname ), "%s/gamedat/title", pmod->vfs_path );
            pmod->tex_index = mnu_TxList_load_one_vfs( loadname, INVALID_TX_REF, INVALID_KEY );

            /// @note This is kinda a cheat since we know that the virtual paths all begin with "mp_" at the moment.
            // If that changes, this line must be changed as well.
            // Save the user data directory version of the module path.
            snprintf( pmod->dest_path, SDL_arraysize( pmod->dest_path ), "/%s", vfs_ModPath + 3 );

            // same problem as above
            strncpy( pmod->name, vfs_ModPath + 11, SDL_arraysize( pmod->name ) );
        };

        ctxt = vfs_findNext( &ctxt );
        vfs_ModPath = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );

    module_list_valid = true;
}

//--------------------------------------------------------------------------------------------
void mnu_release_one_module( const MOD_REF imod )
{
    mnu_module_t * pmod;

    if ( !VALID_MOD( imod ) ) return;
    pmod = mnu_ModList.get_ptr( imod );

    mnu_TxList_release_one( pmod->tex_index );
    pmod->tex_index = INVALID_TITLE_TEXTURE;
}

//--------------------------------------------------------------------------------------------
// Implementation of the ModList struct
//--------------------------------------------------------------------------------------------
mod_file_t * mnu_ModList_get_base( int imod )
{
    if ( !VALID_MOD_RANGE( imod ) ) return NULL;

    return &( mnu_ModList.lst[( MOD_REF )imod].base );
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_vfs_path( int imod )
{
    if ( !VALID_MOD_RANGE( imod ) ) return NULL;

    return mnu_ModList.lst[( MOD_REF )imod].vfs_path;
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_dest_path( int imod )
{
    if ( !VALID_MOD_RANGE( imod ) ) return NULL;

    return mnu_ModList.lst[( MOD_REF )imod].dest_path;
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_name( int imod )
{
    if ( !VALID_MOD_RANGE( imod ) ) return NULL;

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

        mnu_module__init( mnu_ModList.lst + cnt );
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

        mnu_TxList_release_one( mnu_ModList.lst[cnt].tex_index );
        mnu_ModList.lst[cnt].tex_index = INVALID_TITLE_TEXTURE;
    }

    // make sure that mnu_ModList.count is the right size, in case some modules were unloaded?
    mnu_ModList.count = tnc + 1;
}

//--------------------------------------------------------------------------------------------
// Implementation of the GameTips_t system
//--------------------------------------------------------------------------------------------
bool mnu_Tips_global_load_vfs( GameTips_t * pglobal )
{
    /// @author ZF
    /// @details This function loads all of the game hints and tips
    STRING buffer;
    vfs_FILE *fileread;
    Uint8 cnt;

    if ( NULL == pglobal ) return false;

    // reset the count
    pglobal->count = 0;

    // Open the file with all the tips
    fileread = vfs_openRead( "mp_data/gametips.txt" );
    if ( NULL == fileread )
    {
        log_warning( "Could not load the game tips and hints. (\"mp_data/gametips.txt\")\n" );
        return false;
    }

    // Load the data
    for ( cnt = 0; cnt < MENU_MAX_GAMETIPS && !vfs_eof( fileread ); cnt++ )
    {
        if ( goto_colon_vfs( NULL, fileread, true ) )
        {
            //Read the line
            vfs_get_string( fileread, buffer, SDL_arraysize( buffer ) );
            strcpy( pglobal->hint[cnt], buffer );

            //Make it look nice
            str_decode( pglobal->hint[cnt], SDL_arraysize( pglobal->hint[cnt] ), pglobal->hint[cnt] );
            //str_add_linebreaks( pglobal->hint[cnt], SDL_arraysize( pglobal->hint[cnt] ), 50 );

            //Keep track of how many we have total
            pglobal->count++;
        }
    }

    vfs_close( fileread );

    return pglobal->count > 0;
}

//--------------------------------------------------------------------------------------------
bool mnu_Tips_local_load_vfs( GameTips_t * plocal )
{
    /// @author ZF
    /// @details This function loads all module specific hints and tips. If this fails, the game will
    ///       default to the global hints and tips instead

    STRING buffer;
    vfs_FILE *fileread;
    Uint8 cnt;

    // reset the count
    plocal->count = 0;

    // Open all the tips
    snprintf( buffer, SDL_arraysize( buffer ), "mp_modules/%s/gamedat/gametips.txt", pickedmodule_name );
    fileread = vfs_openRead( buffer );
    if ( NULL == fileread ) return false;

    // Load the data
    for ( cnt = 0; cnt < MENU_MAX_GAMETIPS && !vfs_eof( fileread ); cnt++ )
    {
        if ( goto_colon_vfs( NULL, fileread, true ) )
        {
            //Read the line
            vfs_get_string( fileread, buffer, SDL_arraysize( buffer ) );
            strcpy( plocal->hint[cnt], buffer );

            //Make it look nice
            str_decode( plocal->hint[cnt], SDL_arraysize( plocal->hint[cnt] ), plocal->hint[cnt] );

            //Keep track of how many we have total
            plocal->count++;
        }
    }

    vfs_close( fileread );

    return plocal->count > 0;
}

//--------------------------------------------------------------------------------------------
const char * mnu_Tips_get_hint( GameTips_t * pglobal, GameTips_t * plocal )
{
    const char * retval = "Don't die...\n";
    int          randval = 0;

    bool valid_global = ( NULL != pglobal ) && ( 0 != pglobal->count );
    bool valid_local  = ( NULL != plocal )  && ( 0 != plocal->count );

    randval = rand();

    if ( !valid_global && !valid_local )
    {
        // no hints loaded, use the default hint
    }
    else if ( valid_global && !valid_local )
    {
        // only global hints
        int randval_global = randval % pglobal->count;
        retval = pglobal->hint[randval_global];
    }
    else if ( !valid_global && valid_local )
    {
        // only local hints
        int randval_local = randval % plocal->count;
        retval = plocal->hint[randval_local];
    }
    else
    {
        // both hints
        int randval_total = randval % ( pglobal->count + plocal->count );

        if ( randval_total < pglobal->count )
        {
            retval = pglobal->hint[randval_total];
        }
        else
        {
            retval = plocal->hint[randval_total - pglobal->count];
        }
    }

    return retval;
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
void loadAllImportPlayers(const std::string &saveGameDirectory)
{
    /// @author ZZ
    /// @details This function figures out which players may be imported, and loads basic
    ///     data for each

    //Clear any old imports
    //_loadPlayerList.clear();

    // Search for all objects
    vfs_search_context_t *ctxt = vfs_findFirst( saveGameDirectory.c_str(), "obj", VFS_SEARCH_DIR );
    const char *foundfile = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR(foundfile) )
    {
        std::string folderPath = foundfile;

        // is it a valid filename?
        if ( folderPath.empty() ) {
            continue;
        }

        // does the directory exist?
        if ( !vfs_exists( folderPath.c_str() ) ) {
            continue;
        }

        // offset the slots so that ChoosePlayer will have space to load the inventory objects
        int slot = ( MAX_IMPORT_OBJECTS + 2 ) * _loadPlayerList.size();

        // try to load the character profile
        std::shared_ptr<ObjectProfile> profile = ObjectProfile::loadFromFile(folderPath, slot, true);
        if(!profile) {
            continue;
        }

        //Loaded!
        _loadPlayerList.push_back( std::make_shared<LoadPlayerElement>(profile) );

        //Get next player object
        ctxt = vfs_findNext( &ctxt );
        foundfile = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
egolib_rv mnu_set_local_import_list( import_list_t * imp_lst )
{
    int                import_idx, i;
    import_element_t * import_ptr = NULL;

    if ( NULL == imp_lst ) return rv_error;

    // blank out any existing data
    import_list_init( imp_lst );

    // loop through the selected players and store all the valid data in the list of imported players
    for(const std::shared_ptr<LoadPlayerElement> &player : _selectedPlayerList)
    {
        // get a new import data pointer
        import_idx = imp_lst->count;
        import_ptr = imp_lst->lst + imp_lst->count;
        imp_lst->count++;

        //figure out which player we are (1, 2, 3 or 4)
        import_ptr->local_player_num = player->getSelectedByPlayer();

        // set the import info
        import_ptr->slot            = (import_ptr->local_player_num) * MAX_IMPORT_PER_PLAYER;
        import_ptr->player          = (import_ptr->local_player_num) ;

        strncpy( import_ptr->srcDir, player->getProfile()->getFolderPath().c_str(), SDL_arraysize( import_ptr->srcDir ) );
        import_ptr->dstDir[0] = CSTR_END;
    }

    return ( imp_lst->count > 0 ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
// mnu_TxList IMPLEMENTATION
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( oglx_texture_t, mnu_TxList, MENU_TX_COUNT );

//--------------------------------------------------------------------------------------------
void mnu_TxList_reset_freelist()
{
    /// @author BB
    /// @details reset the free texture list. Start at MENU_TX_LAST_SPECIAL so that the global textures/icons are
    ///     can't be allocated by mistake

    int cnt, tnc;

    for ( cnt = MENU_TX_LAST_SPECIAL, tnc = 0; cnt < MENU_TX_COUNT; cnt++, tnc++ )
    {
        mnu_TxList.free_ref[tnc] = cnt;
    }
    mnu_TxList.free_count = tnc;
}

//--------------------------------------------------------------------------------------------
void mnu_TxList_ctor()
{
    /// @author ZZ
    /// @details This function clears out all of the textures

    MNU_TX_REF cnt;

    for ( cnt = 0; cnt < MENU_TX_COUNT; cnt++ )
    {
        oglx_texture_ctor( mnu_TxList.lst + cnt );
    }

    mnu_TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void mnu_TxList_release_one( const MNU_TX_REF index )
{
    oglx_texture_t * ptr = mnu_TxList_get_ptr( index );
    if ( NULL == ptr ) return;

    oglx_texture_Release( ptr );
}

//--------------------------------------------------------------------------------------------
void mnu_TxList_dtor()
{
    /// @author ZZ
    /// @details This function clears out all of the textures

    MNU_TX_REF cnt;

    for ( cnt = 0; cnt < MENU_TX_COUNT; cnt++ )
    {
        oglx_texture_dtor( mnu_TxList.lst + cnt );
    }

    mnu_TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void mnu_TxList_init_all()
{
    /// @author ZZ
    /// @details This function clears out all of the textures

    MNU_TX_REF cnt;

    for ( cnt = 0; cnt < MENU_TX_COUNT; cnt++ )
    {
        oglx_texture_ctor( mnu_TxList.lst + cnt );
    }

    mnu_TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void mnu_TxList_release_all()
{
    /// @author ZZ
    /// @details This function releases all of the textures

    MNU_TX_REF cnt;

    for ( cnt = MENU_TX_LAST_SPECIAL; cnt < MENU_TX_COUNT; cnt++ )
    {
        oglx_texture_Release( mnu_TxList.lst + cnt );
    }

    mnu_TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void mnu_TxList_delete_all()
{
    /// @author ZZ
    /// @details This function clears out all of the textures

    MNU_TX_REF cnt;

    for ( cnt = MENU_TX_LAST_SPECIAL; cnt < MENU_TX_COUNT; cnt++ )
    {
        oglx_texture_dtor( mnu_TxList.lst + cnt );
    }

    mnu_TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void mnu_TxList_reload_all()
{
    /// @author BB
    /// @details This function re-loads all the current textures back into
    ///               OpenGL texture memory using the cached SDL surfaces

    MNU_TX_REF cnt;

    for ( cnt = 0; cnt < MENU_TX_COUNT; cnt++ )
    {
        oglx_texture_t * ptex = mnu_TxList.lst + cnt;

        if ( oglx_texture_Valid( ptex ) )
        {
            oglx_texture_Convert( ptex, ptex->surface, INVALID_KEY );
        }
    }
}

//--------------------------------------------------------------------------------------------
MNU_TX_REF mnu_TxList_get_free( const MNU_TX_REF itex )
{
    MNU_TX_REF retval = INVALID_MNU_TX_REF;

    if ( itex >= 0 && itex < MENU_TX_LAST_SPECIAL )
    {
        retval = itex;
        oglx_texture_Release( mnu_TxList.lst + itex );
    }
    else if ( !VALID_MENU_TX_RANGE( itex ) )
    {
        if ( mnu_TxList.free_count > 0 )
        {
            mnu_TxList.free_count--;
            mnu_TxList.update_guid++;

            retval = ( MNU_TX_REF )mnu_TxList.free_ref[mnu_TxList.free_count];
        }
        else
        {
            retval = INVALID_MNU_TX_REF;
        }
    }
    else
    {
        int i;

        // grab the specified index
        oglx_texture_Release( mnu_TxList.lst + ( MNU_TX_REF )itex );

        // if this index is on the free stack, remove it
        for ( i = 0; i < mnu_TxList.free_count; i++ )
        {
            if ( mnu_TxList.free_ref[i] == itex )
            {
                if ( mnu_TxList.free_count > 0 )
                {
                    mnu_TxList.free_count--;
                    mnu_TxList.update_guid++;

                    SWAP( size_t, mnu_TxList.free_ref[i], mnu_TxList.free_ref[mnu_TxList.free_count] );
                }
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool mnu_TxList_free_one( const MNU_TX_REF itex )
{
    if ( !VALID_MENU_TX_RANGE( itex ) ) return false;

    // release the texture
    oglx_texture_Release( mnu_TxList.lst + itex );

#if defined(_DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < mnu_TxList.free_count; cnt++ )
        {
            if ( itex == mnu_TxList.free_ref[cnt] ) return false;
        }
    }
#endif

    if ( mnu_TxList.free_count >= MENU_TX_COUNT )
        return false;

    // do not put anything below MENU_TX_LAST_SPECIAL back onto the free stack
    if ( itex >= MENU_TX_LAST_SPECIAL )
    {
        mnu_TxList.free_ref[mnu_TxList.free_count] = REF_TO_INT( itex );

        mnu_TxList.free_count++;
        mnu_TxList.update_guid++;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
MNU_TX_REF mnu_TxList_load_one_vfs( const char *filename, const MNU_TX_REF itex_src, Uint32 key )
{
    /// @author BB
    /// @details load a texture into mnu_TxList.
    ///     If !VALID_TX_RANGE(itex_src), then we just get the next free index

    MNU_TX_REF retval;

    // get a texture index.
    retval = mnu_TxList_get_free( itex_src );

    // handle an error
    if ( VALID_MENU_TX_RANGE( retval ) )
    {
        Uint32 txid = ego_texture_load_vfs( mnu_TxList.lst + retval, filename, key );
        if ( INVALID_GL_ID == txid )
        {
            mnu_TxList_free_one( retval );
            retval = INVALID_MNU_TX_REF;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
oglx_texture_t * mnu_TxList_get_valid_ptr( const MNU_TX_REF itex )
{
    oglx_texture_t * ptex = mnu_TxList_get_ptr( itex );

    if ( !oglx_texture_Valid( ptex ) )
    {
        return NULL;
    }

    return ptex;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool mnu_load_cursor()
{
    /// @author ZF
    /// @details Load the mouse cursor

    MNU_TX_REF load_rv = INVALID_MNU_TX_REF;

    load_rv = mnu_TxList_load_one_vfs( "mp_data/cursor", MENU_TX_CURSOR, TRANSCOLOR );

    return VALID_MENU_TX_RANGE( load_rv ) ? true : false;
}

//--------------------------------------------------------------------------------------------
bool mnu_load_all_global_icons()
{
    /// @author ZF
    /// @details Load all the global icons used in all modules

    // Setup
    MNU_TX_REF load_rv;
    bool result = gfx_success;

    // Now load every icon
    load_rv = mnu_TxList_load_one_vfs( "mp_data/nullicon", ( MNU_TX_REF )MENU_TX_ICON_NULL, INVALID_KEY );
    result = !VALID_MENU_TX_RANGE( load_rv ) ? false : result;

    load_rv = mnu_TxList_load_one_vfs( "mp_data/keybicon", ( MNU_TX_REF )MENU_TX_ICON_KEYB, INVALID_KEY );
    result = !VALID_MENU_TX_RANGE( load_rv ) ? false : result;

    load_rv = mnu_TxList_load_one_vfs( "mp_data/mousicon", ( MNU_TX_REF )MENU_TX_ICON_MOUS, INVALID_KEY );
    result = !VALID_MENU_TX_RANGE( load_rv ) ? false : result;

    load_rv = mnu_TxList_load_one_vfs( "mp_data/joyaicon", ( MNU_TX_REF )MENU_TX_ICON_JOYA, INVALID_KEY );
    result = !VALID_MENU_TX_RANGE( load_rv ) ? false : result;

    load_rv = mnu_TxList_load_one_vfs( "mp_data/joybicon", ( MNU_TX_REF )MENU_TX_ICON_JOYB, INVALID_KEY );
    result = !VALID_MENU_TX_RANGE( load_rv ) ? false : result;

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

mnu_module_t * mnu_module__init( mnu_module_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr->request_count = 0;
    ptr->create_count = 0;
    ptr->loaded = false;
    ptr->name[0] = CSTR_END;

    mod_file__init( &( ptr->base ) );

    // extended data
    ptr->tex_index = INVALID_TX_REF;
    ptr->vfs_path[0] = CSTR_END;
    ptr->dest_path[0] = CSTR_END;

    return ptr;
}
