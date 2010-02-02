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

#include "SDL_extensions.h"

#include "egoboo_vfs.h"
#include "egoboo_typedef.h"
#include "egoboo_fileutil.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MNU_INVALID_PLA MAX_PLAYER

/// The possible states of the menu state machine
enum e_menu_states
{
    MM_Begin,
    MM_Entering,
    MM_Running,
    MM_Leaving,
    MM_Finish
};

#define INVALID_TITLEIMAGE MAX_MODULE
#define MENU_STACK_COUNT   256
#define MAXWIDGET          100
#define MENU_MAX_GAMETIPS  100

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// "Slidy" buttons used in some of the menus.  They're shiny.
struct
{
    float lerp;
    int top;
    int left;
    char **buttons;
} SlidyButtonState;

//--------------------------------------------------------------------------------------------
/// the data to display a chosen player in the load player menu
struct s_ChoosePlayer_element
{
    int cap_ref;              ///< the index of the cap_t
    int tx_ref;               ///< the index of the icon texture
    chop_definition_t chop;   ///< put this here so we can generate a name without loading an entire profile
};
typedef struct s_ChoosePlayer_element ChoosePlayer_element_t;

/// The data that menu.c uses to store the users' choice of players
struct s_ChoosePlayer_profiles
{
    int count;                                                 ///< the profiles that have been loaded
    ChoosePlayer_element_t pro_data[MAXIMPORTPERPLAYER + 1];   ///< the profile data
};
typedef struct s_ChoosePlayer_profiles ChoosePlayer_profiles_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int          menu_stack_index = 0;
static which_menu_t menu_stack[MENU_STACK_COUNT];

static which_menu_t mnu_whichMenu = emnu_Main;

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

static int selectedPlayer = 0;           // Which player is currently selected to play

static menu_process_t    _mproc;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
menu_process_t    * MProc   = &_mproc;

Uint32            TxTitleImage_count = 0;
oglx_texture      TxTitleImage[MAX_MODULE];    // OpenGL title image surfaces

bool_t startNewPlayer = bfalse;

/* The font used for drawing text.  It's smaller than the button font */
Font *menuFont = NULL;

bool_t mnu_draw_background = btrue;

int              loadplayer_count = 0;
LOAD_PLAYER_INFO loadplayer[MAXLOADPLAYER];

int     mnu_selectedPlayerCount = 0;
Uint32  mnu_selectedInput[MAX_PLAYER] = {0};
PLA_REF mnu_selectedPlayer[MAX_PLAYER] = {0};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void load_all_menu_images();

static bool_t       menu_stack_push( which_menu_t menu );
static which_menu_t menu_stack_pop();
static which_menu_t menu_stack_peek();
static void         menu_stack_clear();

static void initSlidyButtons( float lerp, const char *button_text[] );
static void updateSlidyButtons( float deltaTime );
static void drawSlidyButtons();

static bool_t mnu_checkSelectedPlayer( PLA_REF player );
static REF_T  mnu_getSelectedPlayer( PLA_REF player );
static bool_t mnu_addSelectedPlayer( PLA_REF player );
static bool_t mnu_removeSelectedPlayer( PLA_REF player );
static bool_t mnu_addSelectedPlayerInput( PLA_REF player, Uint32 input );
static bool_t mnu_removeSelectedPlayerInput( PLA_REF player, Uint32 input );

static void TxTitleImage_clear_data();

static void mnu_release_one_module( int imod );

// "process" management
static int do_menu_proc_begin( menu_process_t * mproc );
static int do_menu_proc_running( menu_process_t * mproc );
static int do_menu_proc_leaving( menu_process_t * mproc );

// the hint system
void   load_global_game_hints();
bool_t load_local_game_hints();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// the module data that the menu system needs
struct s_mnu_module
{
    EGO_PROFILE_STUFF;

    mod_file_t base;

    // extended data
    Uint32  tex_index;                              // the index of the tile image
};
typedef struct s_mnu_module mnu_module_t;

DEFINE_STACK_STATIC( mnu_module_t, mnu_ModList, MAX_MODULE );

#define VALID_MOD_RANGE( IMOD ) ( ((IMOD) >= 0) && ((IMOD) < MAX_MODULE) )
#define VALID_MOD( IMOD )       ( VALID_MOD_RANGE( IMOD ) && IMOD < mnu_ModList.count && mnu_ModList.lst[IMOD].loaded )
#define INVALID_MOD( IMOD )     ( !VALID_MOD_RANGE( IMOD ) || IMOD >= mnu_ModList.count || !mnu_ModList.lst[IMOD].loaded )

DECLARE_STACK( ACCESS_TYPE_NONE, mnu_module_t, mnu_ModList );

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

static GameTips_t mnu_GameTip = { 0 };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void TxTitleImage_clear_data()
{
    TxTitleImage_count = 0;
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_ctor()
{
    /// @details ZZ@> This function clears out all of the textures

    int cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        oglx_texture_ctor( TxTitleImage + cnt );
    }

    TxTitleImage_clear_data();
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_release_one( int index )
{
    if ( index < 0 || index >= MAX_MODULE ) return;

    oglx_texture_Release( TxTitleImage + index );
}

//--------------------------------------------------------------------------------------------
void TxTitleImage_release_all()
{
    /// @details ZZ@> This function releases all of the textures

    int cnt;

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

    int cnt;

    for ( cnt = 0; cnt < MAX_MODULE; cnt++ )
    {
        oglx_texture_dtor( TxTitleImage + cnt );
    }

    TxTitleImage_clear_data();
}

//--------------------------------------------------------------------------------------------
int TxTitleImage_load_one( const char *szLoadName )
{
    /// @details ZZ@> This function loads a title in the specified image slot, forcing it into
    ///    system memory.  Returns btrue if it worked

    int    index;

    if ( INVALID_CSTR( szLoadName ) ) return MAX_MODULE;

    if ( TxTitleImage_count >= MAX_MODULE ) return MAX_MODULE;

    index = MAX_MODULE;
    if ( INVALID_TX_ID != ego_texture_load( TxTitleImage + TxTitleImage_count, szLoadName, INVALID_KEY ) )
    {
        index = TxTitleImage_count;
        TxTitleImage_count++;
    }

    return index;
}

//--------------------------------------------------------------------------------------------
oglx_texture * TxTitleImage_get_ptr( Uint32 itex )
{
    if ( itex >= TxTitleImage_count || itex >= MAX_MODULE ) return NULL;

    return TxTitleImage + itex;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mod_file_t * mnu_ModList_get_base( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return &( mnu_ModList.lst[imod].base );
}

//--------------------------------------------------------------------------------------------
const char * mnu_ModList_get_name( int imod )
{
    if ( imod < 0 || imod >= MAX_MODULE ) return NULL;

    return mnu_ModList.lst[imod].name;
}

//--------------------------------------------------------------------------------------------
void mnu_ModList_release_all()
{
    int cnt;

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
    int cnt, tnc;

    tnc = -1;
    for ( cnt = 0; cnt < mnu_ModList.count; cnt++ )
    {
        if ( !mnu_ModList.lst[cnt].loaded ) continue;
        tnc = cnt;

        TxTitleImage_release_one( mnu_ModList.lst[cnt].tex_index );
        mnu_ModList.lst[cnt].tex_index = INVALID_TITLEIMAGE;
    }
    TxTitleImage_count = 0;

    // make sure that mnu_ModList.count is the right size, in case some modules were unloaded?
    mnu_ModList.count = tnc + 1;

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void initSlidyButtons( float lerp, const char *button_text[] )
{
    int i;

    // Figure out where to draw the buttons
    buttonLeft = 40;
    buttonTop = GFX_HEIGHT - 20;

    for ( i = 0; button_text[i][0] != 0; i++ )
    {
        buttonTop -= 35;
    }

    SlidyButtonState.lerp = lerp;
    SlidyButtonState.buttons = ( char** )button_text;
}

//--------------------------------------------------------------------------------------------
void updateSlidyButtons( float deltaTime )
{
    SlidyButtonState.lerp += ( deltaTime * 1.5f );
}

//--------------------------------------------------------------------------------------------
void drawSlidyButtons()
{
    int i;

    for ( i = 0; SlidyButtonState.buttons[i][0] != 0; i++ )
    {
        int x = buttonLeft - ( 360 - i * 35 )  * SlidyButtonState.lerp;
        int y = buttonTop + ( i * 35 );

        ui_doButton( UI_Nothing, SlidyButtonState.buttons[i], NULL, x, y, 200, 30 );
    }
}

//--------------------------------------------------------------------------------------------
void set_tip_position( Font * font, const char * text, int spacing )
{
    int w, h;

    if ( NULL == text ) return;

    tipTextLeft = 0;
    tipTextTop  = 0;

    fnt_getTextBoxSize( font, text, spacing, &w, &h );

    // set the text
    tipText = text;

    // Draw the options text to the right of the buttons
    tipTextLeft = 280;

    // And relative to the bottom of the screen
    tipTextTop = GFX_HEIGHT - h - spacing;

}

//--------------------------------------------------------------------------------------------
void set_copyright_position( Font * font, const char * text, int spacing )
{
    int w, h;

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
int menu_system_begin()
{
    // initializes the menu system
    //
    // Loads resources for the menus, and figures out where things should
    // be positioned.  If we ever allow changing resolution on the fly, this
    // function will have to be updated/called more than once.

    ui_set_virtual_screen( gfx.vw, gfx.vh, GFX_WIDTH, GFX_HEIGHT );

    menuFont = ui_loadFont( "basicdat" SLASH_STR "Negatori.ttf", 18 );
    if ( NULL == menuFont )
    {
        log_error( "Could not load the menu font! (Negatori.ttf)\n" );
        return 0;
    }

    // Figure out where to draw the copyright text
    set_copyright_position( menuFont, copyrightText, 20 );

    // Figure out where to draw the options text
    set_tip_position( menuFont, tipText, 20 );

    // construct the TxTitleImage array
    TxTitleImage_ctor();

    // Load game hints
    load_global_game_hints();

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

    if ( NULL != menuFont )
    {
        fnt_freeFont( menuFont );
        menuFont = NULL;
    }

    // destruct the TxTitleImage array
    TxTitleImage_dtor();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int doMainMenu( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture background;
    static oglx_texture logo;

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

            // load the menu image
            ego_texture_load( &background, "basicdat" SLASH_STR "menu" SLASH_STR "menu_main", INVALID_KEY );

            // load the logo image
            ego_texture_load( &logo,       "basicdat" SLASH_STR "menu" SLASH_STR "menu_logo", INVALID_KEY );

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

            initSlidyButtons( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, bg_rect.x,   bg_rect.y,   bg_rect.w,   bg_rect.h );
                ui_drawImage( 0, &logo,       logo_rect.x, logo_rect.y, logo_rect.w, logo_rect.h );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            drawSlidyButtons();
            updateSlidyButtons( -deltaTime );

            // Let lerp wind down relative to the time elapsed
            if ( SlidyButtonState.lerp <= 0.0f )
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
                ui_drawImage( 0, &background, bg_rect.x,   bg_rect.y,   bg_rect.w,   bg_rect.h );
                ui_drawImage( 0, &logo,       logo_rect.x, logo_rect.y, logo_rect.w, logo_rect.h );
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
                initSlidyButtons( 0.0f, sz_buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, bg_rect.x,   bg_rect.y,   bg_rect.w,   bg_rect.h );
                ui_drawImage( 0, &logo,       logo_rect.x, logo_rect.y, logo_rect.w, logo_rect.h );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            // Buttons
            drawSlidyButtons();
            updateSlidyButtons( deltaTime );
            if ( SlidyButtonState.lerp >= 1.0f )
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
    static oglx_texture background;
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
            ego_texture_load( &background, "basicdat" SLASH_STR "menu" SLASH_STR "menu_advent", TRANSCOLOR );
            menuChoice = 0;

            menuState = MM_Entering;

            initSlidyButtons( 1.0f, sz_buttons );

            // Let this fall through

        case MM_Entering:
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            // Draw the background image
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0 );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            drawSlidyButtons();
            updateSlidyButtons( -deltaTime );
            if ( SlidyButtonState.lerp <= 0.0f )
                menuState = MM_Running;

            break;

        case MM_Running:

            // Draw the background image
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0 );
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
                initSlidyButtons( 0.0f, sz_buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, GFX_WIDTH  - background.imgW, 0, 0, 0 );
            }

            // "Copyright" text
            ui_drawTextBox( menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20 );

            drawSlidyButtons();
            updateSlidyButtons( deltaTime );
            if ( SlidyButtonState.lerp >= 1.0f )
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
// Choose the module
int doChooseModule( float deltaTime )
{
    static oglx_texture background;
    static int menuState = MM_Begin;
    static int startIndex;
    static int validModules[MAX_MODULE];
    static int numValidModules;
    static Uint8 keycooldown;

    static int moduleMenuOffsetX;
    static int moduleMenuOffsetY;

    int result = 0;
    int i, j, x, y;

    switch ( menuState )
    {
        case MM_Begin:
            // Reload all modules, something might be unlocked
            load_all_menu_images();

            // Load font & background
            ego_texture_load( &background, "basicdat" SLASH_STR "menu" SLASH_STR "menu_sleepy", TRANSCOLOR );
            startIndex = 0;
            selectedModule = -1;

            // Find the module's that we want to allow loading for.  If startNewPlayer
            // is true, we want ones that don't allow imports (e.g. starter modules).
            // Otherwise, we want modules that allow imports
            memset( validModules, 0, sizeof( int ) * MAX_MODULE );
            numValidModules = 0;
            for ( i = 0; i < mnu_ModList.count; i++ )
            {
                // if this module is not valid given the game options and the
                // selected players, skip it
                if ( !mnu_test_by_index( i ) ) continue;

                if ( startNewPlayer && 0 == mnu_ModList.lst[i].base.importamount )
                {
                    // starter module
                    validModules[numValidModules] = i;
                    numValidModules++;
                }
                else
                {
                    if ( mnu_selectedPlayerCount > mnu_ModList.lst[i].base.importamount ) continue;
                    if ( mnu_selectedPlayerCount < mnu_ModList.lst[i].base.minplayers ) continue;
                    if ( mnu_selectedPlayerCount > mnu_ModList.lst[i].base.maxplayers ) continue;

                    // regular module
                    validModules[numValidModules] = i;
                    numValidModules++;
                }
            }

            // Figure out at what offset we want to draw the module menu.
            moduleMenuOffsetX = ( GFX_WIDTH  - 640 ) / 2;
            moduleMenuOffsetY = ( GFX_HEIGHT - 480 ) / 2;

            if ( 0 == numValidModules )
            {
                set_tip_position( menuFont, "Sorry, there are no valid games!\n Please press the \"Back\" button.", 20 );
            }
            else if ( numValidModules <= 3 )
            {
                set_tip_position( menuFont, "Press an icon to select a game.", 20 );
            }
            else
            {
                set_tip_position( menuFont, "Press an icon to select a game.\nUse the mouse wheel or the \"<-\" and \"->\" buttons to scroll.", 20 );
            }

            menuState = MM_Entering;

            // fall through...

        case MM_Entering:
            menuState = MM_Running;

            // fall through for now...

        case MM_Running:
            // Draw the background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
            x = ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 );
            y = GFX_HEIGHT - background.imgH;

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, x, y, 0, 0 );
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
                if ( keycooldown == 0 )
                {
                    startIndex++;
                    keycooldown = 5;
                }
            }
            else if ( SDLKEYDOWN( SDLK_LEFT ) )
            {
                if ( keycooldown == 0 )
                {
                    startIndex--;
                    keycooldown = 5;
                }
            }
            else keycooldown = 0;
            if ( keycooldown > 0 ) keycooldown--;

            // Draw the arrows to pick modules
            if ( BUTTON_UP == ui_doButton( 1051, "<-", NULL, moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, 30, 30 ) )
            {
                startIndex--;
            }
            if ( BUTTON_UP == ui_doButton( 1052, "->", NULL, moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30 ) )
            {
                startIndex++;
            }

            // restrict the range to valid values
            startIndex = CLIP( startIndex, 0, numValidModules - 3 );

            // Draw buttons for the modules that can be selected
            x = 93;
            y = 20;
            for ( i = startIndex, j = 0; i < ( startIndex + 3 ) && j < numValidModules; i++ )
            {
                // fix the menu images in case one or more of them are undefined
                int         imod       = validModules[i];
                Uint32      tex_offset = mnu_ModList.lst[imod].tex_index;
                oglx_texture * ptex    = TxTitleImage_get_ptr( tex_offset );

                if ( ui_doImageButton( i, ptex, moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138 ) )
                {
                    selectedModule = i;
                }

                x += 138 + 20;  // Width of the button, and the spacing between buttons
            }

            // Draw an empty button as the backdrop for the module text
            ui_drawButton( UI_Nothing, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 250, NULL );

            // Draw the text description of the selected module
            if ( selectedModule > -1 && selectedModule < MAX_MODULE && validModules[selectedModule] >= 0 )
            {
                char buffer[1024]  = EMPTY_CSTR;
                char * carat = buffer, * carat_end = buffer + SDL_arraysize( buffer );
                int imodule = validModules[selectedModule];

                GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

                carat += snprintf( carat, carat_end - carat - 1, "%s\n", mnu_ModList.lst[imodule].base.longname );

                carat += snprintf( carat, carat_end - carat - 1, "Difficulty: %s\n", mnu_ModList.lst[imodule].base.rank );

                if ( mnu_ModList.lst[imodule].base.maxplayers > 1 )
                {
                    if ( mnu_ModList.lst[imodule].base.minplayers == mnu_ModList.lst[imodule].base.maxplayers )
                    {
                        carat += snprintf( carat, carat_end - carat - 1, "%d Players\n", mnu_ModList.lst[imodule].base.minplayers );
                    }
                    else
                    {
                        carat += snprintf( carat, carat_end - carat - 1, "%d - %d Players\n", mnu_ModList.lst[imodule].base.minplayers, mnu_ModList.lst[imodule].base.maxplayers );
                    }
                }
                else
                {
                    if ( 0 != mnu_ModList.lst[imodule].base.importamount )
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
                    carat += snprintf( carat, carat_end - carat - 1, "%s\n", mnu_ModList.lst[imodule].base.summary[i] );
                }

                // Draw a text box
                ui_drawTextBox( menuFont, buffer, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230, 20 );
            }

            // And draw the next & back buttons
            if ( selectedModule > -1 )
            {
                if ( SDLKEYDOWN( SDLK_RETURN ) || BUTTON_UP == ui_doButton( 53, "Select Module", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30 ) )
                {
                    // go to the next menu with this module selected
                    selectedModule = validModules[selectedModule];
                    menuState = MM_Leaving;
                }
            }

            if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 54, "Back", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30 ) )
            {
                // Signal doMenu to go back to the previous menu
                selectedModule = -1;
                menuState = MM_Leaving;
            }

            // the tool-tip text
            glColor4f( 1, 1, 1, 1 );
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            menuState = MM_Finish;
            // fall through for now

        case MM_Finish:
            oglx_texture_Release( &background );

            menuState = MM_Begin;
            if ( selectedModule == -1 )
            {
                result = -1;
            }
            else
            {
                // Save the name of the module that we've picked
                pickedmodule_index = selectedModule;
                strncpy( pickedmodule_name, mnu_ModList.lst[selectedModule].name, SDL_arraysize( pickedmodule_name ) );

                if ( !game_choose_module( selectedModule, -1 ) )
                {
                    log_warning( "Tried to select an invalid module. index == %d\n", selectedModule );
                    result = -1;
                }
                else
                {
                    pickedmodule_ready = btrue;
                    result = ( PMod->importamount > 0 ) ? 1 : 2;
                }
            }

            // reset the ui
            ui_Reset();

            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
bool_t doChoosePlayer_load_profiles( int player, ChoosePlayer_profiles_t * pro_list )
{
    int    i, ref_temp;
    STRING szFilename;
    ChoosePlayer_element_t * pdata;

    // release all of the temporary profiles
    release_all_profiles();
    overrideslots = btrue;

    if ( 0 == bookicon_count )
    {
        load_one_profile( "basicdat" SLASH_STR "globalobjects" SLASH_STR "book.obj", SPELLBOOK );
    }

    // release any data that we have accumulated
    for ( i = 0; i < pro_list->count; i++ )
    {
        pdata = pro_list->pro_data + i;

        TxTexture_free_one( pdata->tx_ref );

        // initialize the data
        pdata->cap_ref = MAX_CAP;
        pdata->tx_ref  = INVALID_TEXTURE;
        chop_definition_init( &( pdata->chop ) );
    }
    pro_list->count = 0;

    if ( player < 0 || player >= MAXLOADPLAYER || player >= loadplayer_count ) return bfalse;

    // grab the player data
    ref_temp = load_one_character_profile( loadplayer[player].dir, 0, bfalse );
    if ( !LOADED_CAP( ref_temp ) )
    {
        return bfalse;
    }

    // go to the next element in the list
    pdata = pro_list->pro_data + pro_list->count;
    pro_list->count++;

    // set the index of this object
    pdata->cap_ref = ref_temp;

    // grab the inventory data
    for ( i = 0; i < MAXIMPORTOBJECTS; i++ )
    {
        int slot = i + 1;

        snprintf( szFilename, SDL_arraysize( szFilename ), "%s" SLASH_STR "%d.obj", loadplayer[player].dir, i );

        // load the profile
        ref_temp = load_one_character_profile( szFilename, slot, bfalse );
        if ( LOADED_CAP( ref_temp ) )
        {
            cap_t * pcap = CapList + ref_temp;

            // go to the next element in the list
            pdata = pro_list->pro_data + pro_list->count;
            pro_list->count++;

            pdata->cap_ref = ref_temp;

            // load the icon
            snprintf( szFilename, SDL_arraysize( szFilename ), "%s" SLASH_STR "%d.obj" SLASH_STR "icon%d", loadplayer[player].dir, i, MAX( 0, pcap->skinoverride ) );
            pdata->tx_ref = TxTexture_load_one( szFilename, INVALID_TEXTURE, INVALID_KEY );

            // load the naming
            snprintf( szFilename, SDL_arraysize( szFilename ), "%s" SLASH_STR "%d.obj" SLASH_STR "naming.txt", loadplayer[player].dir, i );
            chop_load( &chop_mem, szFilename, &( pdata->chop ) );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t doChoosePlayer_show_stats( int player, int mode, int x, int y, int width, int height )
{
    int i, x1, y1;

    static ChoosePlayer_profiles_t objects = { 0 };

    if ( player < 0 ) mode = 1;

    // handle the profile data
    switch ( mode )
    {
        case 0: // load new player data

            if ( !doChoosePlayer_load_profiles( player, &objects ) )
            {
                player = -1;
            }
            break;

        case 1: // unload player data

            player = -1;
            objects.count = 0;

            // release all of the temporary profiles
            release_all_profiles();

            break;
    }

    // do the actual display
    x1 = x + 25;
    y1 = y + 25;
    if ( player >= 0 && objects.count > 0 )
    {
        CAP_REF icap = objects.pro_data[0].cap_ref;

        if ( LOADED_CAP( icap ) )
        {
            cap_t * pcap = CapList + icap;
            Uint8 skin = MAX( 0, pcap->skinoverride );

            ui_drawButton( UI_Nothing, x, y, width, height, NULL );

            //Character level and class
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            // fix class name capitalization
            pcap->classname[0] = toupper( pcap->classname[0] );
            fnt_drawText( menuFont, NULL, x1, y1, "A level %d %s", pcap->leveloverride + 1, pcap->classname );
            y1 += 20;

            // Armor
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
            fnt_drawText( menuFont, NULL, x1, y1, "Wearing %s %s", pcap->skinname[skin], HAS_SOME_BITS( pcap->skindressy, 1 << skin ) ? "(Light)" : "(Heavy)" );
            y1 += 40;

            // Life and mana (can be less than maximum if not in easy mode)
            if ( cfg.difficulty >= GAME_NORMAL )
            {
                fnt_drawText( menuFont, NULL, x1, y1, "Life: %d/%d", MIN( FP8_TO_INT( pcap->life_spawn ), ( int )pcap->life_stat.val.from ), ( int )pcap->life_stat.val.from ); y1 += 20;
                y1 = draw_one_bar( pcap->lifecolor, x1, y1, FP8_TO_INT( pcap->life_spawn ), ( int )pcap->life_stat.val.from );

                if ( pcap->mana_stat.val.from > 0 )
                {
                    fnt_drawText( menuFont, NULL, x1, y1, "Mana: %d/%d", MIN( FP8_TO_INT( pcap->mana_spawn ), ( int )pcap->mana_stat.val.from ), ( int )pcap->mana_stat.val.from ); y1 += 20;
                    y1 = draw_one_bar( pcap->manacolor, x1, y1, FP8_TO_INT( pcap->mana_spawn ), ( int )pcap->mana_stat.val.from );
                }
            }
            else
            {
                fnt_drawText( menuFont, NULL, x1, y1, "Life: %d", ( int )pcap->life_stat.val.from ); y1 += 20;
                y1 = draw_one_bar( pcap->lifecolor, x1, y1, ( int )pcap->life_stat.val.from, ( int )pcap->life_stat.val.from );

                if ( pcap->mana_stat.val.from > 0 )
                {
                    fnt_drawText( menuFont, NULL, x1, y1, "Mana: %d", ( int )pcap->mana_stat.val.from ); y1 += 20;
                    y1 = draw_one_bar( pcap->manacolor, x1, y1, ( int )pcap->mana_stat.val.from, ( int )pcap->mana_stat.val.from );
                }
            }
            y1 += 20;

            //SWID
            fnt_drawText( menuFont, NULL, x1, y1, "Stats" ); y1 += 20;
            fnt_drawText( menuFont, NULL, x1, y1, "  Str: %s (%d)", describe_value( pcap->strength_stat.val.from,     60, NULL ), ( int )pcap->strength_stat.val.from ); y1 += 20;
            fnt_drawText( menuFont, NULL, x1, y1, "  Wis: %s (%d)", describe_value( pcap->wisdom_stat.val.from,       60, NULL ), ( int )pcap->wisdom_stat.val.from ); y1 += 20;
            fnt_drawText( menuFont, NULL, x1, y1, "  Int: %s (%d)", describe_value( pcap->intelligence_stat.val.from, 60, NULL ), ( int )pcap->intelligence_stat.val.from ); y1 += 20;
            fnt_drawText( menuFont, NULL, x1, y1, "  Dex: %s (%d)", describe_value( pcap->dexterity_stat.val.from,    60, NULL ), ( int )pcap->dexterity_stat.val.from ); y1 += 20;
            y1 += 20;

            if ( objects.count > 1 )
            {
                ChoosePlayer_element_t * pdata;

                fnt_drawText( menuFont, NULL, x1, y1, "Inventory" ); y1 += 20;

                for ( i = 1; i < objects.count; i++ )
                {
                    pdata = objects.pro_data + i;

                    icap = pdata->cap_ref;
                    if ( LOADED_CAP( icap ) )
                    {
                        TX_REF  icon_ref;
                        cap_t * pcap = CapList + icap;

                        STRING itemname;
                        if ( pcap->nameknown ) strncpy( itemname, chop_create( &chop_mem, &( pdata->chop ) ), SDL_arraysize( itemname ) );
                        else                   strncpy( itemname, pcap->classname,   SDL_arraysize( itemname ) );

                        icon_ref = mnu_get_icon_ref( icap, pdata->tx_ref );

                        draw_one_icon( icon_ref, x1, y1, NOSPARKLE );

                        if ( icap == SLOT_LEFT + 1 )
                        {
                            fnt_drawText( menuFont, NULL, x1 + 32, y1 + 6, "  Left: %s", itemname ); y1 += 32;
                        }
                        else if ( icap == SLOT_RIGHT + 1 )
                        {
                            fnt_drawText( menuFont, NULL, x1 + 32, y1 + 6, "  Right: %s", itemname ); y1 += 32;
                        }
                        else
                        {
                            fnt_drawText( menuFont, NULL, x1 + 32, y1 + 6, "  Item: %s", itemname ); y1 += 32;
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
    static int menuState = MM_Begin;
    static oglx_texture background;
    int result = 0;
    int i, j, x, y;
    STRING srcDir, destDir;
    static int    startIndex = 0;
    static int    last_player = -1;
    static bool_t new_player = bfalse;

    const int x0 = 20, y0 = 20, icon_size = 42, text_width = 175, button_repeat = 47;

    static int numVertical, numHorizontal;
    static Uint32 BitsInput[4];
    static bool_t device_on[4];

    static const char * button_text[] = { "N/A", "Back", ""};

    switch ( menuState )
    {
        case MM_Begin:
            TxTexture_free_one( TX_BARS );

            mnu_selectedPlayerCount = 0;
            mnu_selectedPlayer[0] = 0;

            TxTexture_load_one( "basicdat" SLASH_STR "nullicon", ICON_NULL, INVALID_KEY );

            TxTexture_load_one( "basicdat" SLASH_STR "keybicon", ICON_KEYB, INVALID_KEY );
            BitsInput[0] = INPUT_BITS_KEYBOARD;
            device_on[0] = keyb.on;

            TxTexture_load_one( "basicdat" SLASH_STR "mousicon", ICON_MOUS, INVALID_KEY );
            BitsInput[1] = INPUT_BITS_MOUSE;
            device_on[1] = mous.on;

            TxTexture_load_one( "basicdat" SLASH_STR "joyaicon", ICON_JOYA, INVALID_KEY );
            BitsInput[2] = INPUT_BITS_JOYA;
            device_on[2] = joy[0].on;

            TxTexture_load_one( "basicdat" SLASH_STR "joybicon", ICON_JOYB, INVALID_KEY );
            BitsInput[3] = INPUT_BITS_JOYB;
            device_on[3] = joy[1].on;

            ego_texture_load( &background, "basicdat" SLASH_STR "menu" SLASH_STR "menu_sleepy", TRANSCOLOR );

            TxTexture_load_one( "basicdat" SLASH_STR "bars", TX_BARS, INVALID_KEY );

            // load information for all the players that could be imported
            check_player_import( "players", btrue );

            // reset button 0, or it will mess up the menu.
            // must do it before initSlidyButtons()
            button_text[0] = "N/A";

            initSlidyButtons( 1.0f, button_text );

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
                    ui_initWidget( mnu_widgetList + m, m, menuFont, NULL, TxTexture_get_ptr( ICON_KEYB + j ), x + text_width + j*icon_size, y, icon_size, icon_size );
                    ui_widgetAddMask( mnu_widgetList + m, UI_BITS_CLICKED );
                };

                y += button_repeat;
            };

            if ( loadplayer_count < 10 )
            {
                set_tip_position( menuFont, "Choose an input device to select your player(s)", 20 );
            }
            else
            {
                set_tip_position( menuFont, "Choose an input device to select your player(s)\nUse the mouse wheel to scroll.", 20 );
            }

            menuState = MM_Entering;
            // fall through

        case MM_Entering:

            /*GL_DEBUG(glColor4f)(1, 1, 1, 1 - SlidyButtonState.lerp );
            drawSlidyButtons();
            updateSlidyButtons( -deltaTime );
            // Let lerp wind down relative to the time elapsed
            if ( SlidyButtonState.lerp <= 0.0f )
            {
                menuState = MM_Running;
            }*/

            // Simply fall through
            // menuState = MM_Running;
            // break;

        case MM_Running:
            // Figure out how many players we can show without scrolling

            if ( 0 == mnu_selectedPlayerCount )
            {
                button_text[0] = "";
            }
            else if ( 1 == mnu_selectedPlayerCount )
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
                ui_drawImage( 0, &background, x, y, 0, 0 );
            }

            // use the mouse wheel to scan the characters
            if ( cursor_wheel_event_pending() )
            {
                if ( cursor.z > 0 )
                {
                    if ( startIndex + numVertical < loadplayer_count )
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

            // Draw the player selection buttons
            x = x0;
            y = y0;
            for ( i = 0; i < numVertical; i++ )
            {
                PLA_REF player, splayer;
                int m = i * 5;

                player = i + startIndex;
                if ( player >= loadplayer_count ) continue;

                splayer = mnu_getSelectedPlayer( player );

                // do the character button
                mnu_widgetList[m].img  = TxTexture_get_ptr( loadplayer[player].tx_ref );
                mnu_widgetList[m].text = loadplayer[player].name;
                if ( MNU_INVALID_PLA != splayer )
                {
                    mnu_widgetList[m].state |= UI_BITS_CLICKED;
                }
                else
                {
                    mnu_widgetList[m].state &= ~UI_BITS_CLICKED;
                }

                if ( BUTTON_DOWN == ui_doWidget( mnu_widgetList + m ) )
                {
                    if ( HAS_SOME_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) && !mnu_checkSelectedPlayer( player ) )
                    {
                        // button has become cursor_clicked
                        // mnu_addSelectedPlayer(player);
                        last_player = player;
                        new_player  = btrue;
                    }
                    else if ( HAS_NO_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) && mnu_checkSelectedPlayer( player ) )
                    {
                        // button has become unclicked
                        if ( mnu_removeSelectedPlayer( player ) )
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
                        case INPUT_DEVICE_JOY + 0:  device_on[j] = joy[0].on; break;
                        case INPUT_DEVICE_JOY + 1:  device_on[j] = joy[1].on; break;
                        default: device_on[j] = bfalse;
                    }

                    // replace any not on device with a null icon
                    if ( !device_on[j] )
                    {
                        mnu_widgetList[m].img = TxTexture_get_ptr( ICON_NULL );

                        // draw the widget, even though it will not do anything
                        // the menu would looks funny if odd columns missing
                        ui_doWidget( mnu_widgetList + m );
                    }

                    // make the button states reflect the chosen input devices
                    if ( MNU_INVALID_PLA == splayer || HAS_NO_BITS( mnu_selectedInput[ splayer ], BitsInput[j] ) )
                    {
                        mnu_widgetList[m].state &= ~UI_BITS_CLICKED;
                    }
                    else if ( HAS_SOME_BITS( mnu_selectedInput[splayer], BitsInput[j] ) )
                    {
                        mnu_widgetList[m].state |= UI_BITS_CLICKED;
                    }

                    if ( BUTTON_DOWN == ui_doWidget( mnu_widgetList + m ) )
                    {
                        if ( HAS_SOME_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) )
                        {
                            // button has become cursor_clicked
                            if ( MNU_INVALID_PLA == splayer )
                            {
                                if ( mnu_addSelectedPlayer( player ) )
                                {
                                    last_player = player;
                                    new_player  = btrue;
                                }
                            }
                            if ( mnu_addSelectedPlayerInput( player, BitsInput[j] ) )
                            {
                                last_player = player;
                                new_player  = btrue;
                            }
                        }
                        else if ( MNU_INVALID_PLA != splayer && HAS_NO_BITS( mnu_widgetList[m].state, UI_BITS_CLICKED ) )
                        {
                            // button has become unclicked
                            if ( mnu_removeSelectedPlayerInput( player, BitsInput[j] ) )
                            {
                                last_player = -1;
                            }
                        }
                    }
                }
            }

            // Buttons for going ahead

            // Continue
            if ( mnu_selectedPlayerCount != 0 )
            {
                if ( SDLKEYDOWN( SDLK_RETURN ) || BUTTON_UP == ui_doButton( 100, button_text[0], NULL, buttonLeft, buttonTop, 200, 30 ) )
                {
                    menuState = MM_Leaving;
                }
            }

            // Back
            if ( SDLKEYDOWN( SDLK_ESCAPE ) || BUTTON_UP == ui_doButton( 101, button_text[1], NULL, buttonLeft, buttonTop + 35, 200, 30 ) )
            {
                mnu_selectedPlayerCount = 0;
                menuState = MM_Leaving;
            }

            // show the stats
            if ( -1 != last_player )
            {
                if ( new_player )
                {
                    // load and display the new player data
                    new_player = bfalse;
                    doChoosePlayer_show_stats( last_player, 0, GFX_WIDTH - 400, 10, 350, GFX_HEIGHT - 60 );
                }
                else
                {
                    // just display the new player data
                    doChoosePlayer_show_stats( last_player, 2, GFX_WIDTH - 400, 10, 350, GFX_HEIGHT - 60 );
                }
            }
            else
            {
                doChoosePlayer_show_stats( last_player, 1, GFX_WIDTH - 100, 10, 100, GFX_HEIGHT - 60 );
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            /*
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG(glColor4f)(1, 1, 1, 1 - SlidyButtonState.lerp );
            // Buttons
            drawSlidyButtons();
            updateSlidyButtons( deltaTime );
            if ( SlidyButtonState.lerp >= 1.0f )
            {
                menuState = MM_Finish;
            }
            */

            // Simply fall through
            //  menuState = MM_Finish;
            //  break;

        case MM_Finish:

            // release all of the temporary profiles
            doChoosePlayer_show_stats( -1, 1, 0, 0, 0, 0 );

            oglx_texture_Release( &background );
            TxTexture_free_one( TX_BARS );

            menuState = MM_Begin;
            if ( 0 == mnu_selectedPlayerCount )
            {
                result = -1;
            }
            else
            {
                // Build the import directory
                vfs_empty_import_directory();
                if ( !vfs_mkdir( "import" ) )
                {
                    log_warning( "game_update_imports() - Could not create the import folder. (%s)\n", vfs_getError() );
                }

                // set up the slots and the import stuff for the selected players
                local_import_count = mnu_selectedPlayerCount;
                for ( i = 0; i < local_import_count; i++ )
                {
                    selectedPlayer = mnu_selectedPlayer[i];

                    local_import_control[i] = mnu_selectedInput[i];
                    local_import_slot[i]    = i * MAXIMPORTPERPLAYER;

                    // Copy the character to the import directory
                    strncpy( srcDir, loadplayer[selectedPlayer].dir, SDL_arraysize( srcDir ) );
                    snprintf( destDir, SDL_arraysize( destDir ), "import" SLASH_STR "temp%04d.obj", local_import_slot[i] );
                    vfs_copyDirectory( srcDir, destDir );

                    // Copy all of the character's items to the import directory
                    for ( j = 0; j < MAXIMPORTOBJECTS; j++ )
                    {
                        snprintf( srcDir, SDL_arraysize( srcDir ), "%s" SLASH_STR "%d.obj", loadplayer[selectedPlayer].dir, j );

                        // make sure the source directory exists
                        if ( vfs_isDirectory( srcDir ) )
                        {
                            snprintf( destDir, SDL_arraysize( destDir ), "import" SLASH_STR "temp%04d.obj", local_import_slot[i] + j + 1 );
                            vfs_copyDirectory( srcDir, destDir );
                        }
                    }
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
    static oglx_texture background;
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
            ego_texture_load( &background, "basicdat" SLASH_STR "menu" SLASH_STR "menu_gnome", TRANSCOLOR );
            menuChoice = 0;
            menuState = MM_Entering;

            set_tip_position( menuFont, "Change your audio, input and video\nsettings here.", 20 );

            initSlidyButtons( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            // Draw the background
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0 );
            }

            drawSlidyButtons();
            updateSlidyButtons( -deltaTime );

            // Let lerp wind down relative to the time elapsed
            if ( SlidyButtonState.lerp <= 0.0f )
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
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0 );
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
                initSlidyButtons( 0.0f, sz_buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0 );
            }

            // Buttons
            drawSlidyButtons();
            updateSlidyButtons( deltaTime );
            if ( SlidyButtonState.lerp >= 1.0f )
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

            set_tip_position( menuFont, "Change input settings here.", 20 );

            // Load the global icons (keyboard, mouse, etc.)
            if ( !load_all_global_icons() ) log_warning( "Could not load all global icons!\n" );

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            // Fall trough
            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
            ui_drawTextBox( menuFont, "ATK_LEFT HAND", buttonLeft, GFX_HEIGHT - 470, 0, 0, 20 );

            // Are we waiting for input?
            if ( SDLKEYDOWN( SDLK_ESCAPE ) ) waitingforinput = -1;  // Someone cursor_pressed abort

            // Grab the key/button input from the selected device
            if ( waitingforinput != -1 )
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
            if ( NULL != pdevice && waitingforinput == -1 )
            {
                // update the control names
                for ( i = CONTROL_BEGIN; i <= CONTROL_END && i < pdevice->count; i++ )
                {
                    char * tag = scantag_get_string( pdevice->device, pdevice->control[i].tag, pdevice->control[i].is_key );

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
            else if ( BUTTON_UP ==  ui_doImageButtonWithText( 16, TxTexture_get_ptr( ICON_KEYB + iicon ), inputOptionsButtons[CONTROL_COMMAND_COUNT+0], menuFont, buttonLeft + 300, GFX_HEIGHT - 90, 140, 50 ) )
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
                input_settings_save( "controls.txt" );
                menuState = MM_Leaving;
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

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
// Game options menu
int doGameOptions( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture background;
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
            ego_texture_load( &background, "basicdat" SLASH_STR "menu" SLASH_STR "menu_fairy", TRANSCOLOR );

            menuChoice = 0;
            menuState = MM_Entering;

            set_tip_position( menuFont, "Change game settings here.", 20 );

            initSlidyButtons( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            // Draw the background
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  / 2 ) + ( background.imgW / 2 ), GFX_HEIGHT - background.imgH, 0, 0 );
            }

            // Load the current settings
            switch ( cfg.difficulty )
            {
                case GAME_HARD: snprintf( Cdifficulty, SDL_arraysize( Cdifficulty ), "Punishing" ); break;
                case GAME_EASY: snprintf( Cdifficulty, SDL_arraysize( Cdifficulty ), "Forgiving" ); break;
            default: case GAME_NORMAL:
                    {
                        snprintf( Cdifficulty, SDL_arraysize( Cdifficulty ), "Challenging" );
                        cfg.difficulty = GAME_NORMAL;
                        break;
                    }
            }
            sz_buttons[0] = Cdifficulty;

            maxmessage = CLIP( maxmessage, 4, MAX_MESSAGE );
            if ( maxmessage == 0 )
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
            if ( cfg.autoturncamera == CAMTURN_GOOD )        sz_buttons[3] = "Fast";
            else if ( cfg.autoturncamera == CAMTURN_AUTO )   sz_buttons[3] = "On";
            else
            {
                sz_buttons[3] = "Off";
                cfg.autoturncamera = CAMTURN_NONE;
            }

            // Show FPS
            if ( cfg.fps_allowed ) sz_buttons[4] = "On";
            else                   sz_buttons[4] = "Off";

            //Billboard feedback
            if ( !cfg.feedback ) sz_buttons[5] = "Disabled";
            else
            {
                if ( cfg.feedback == FEEDBACK_TEXT ) sz_buttons[5] = "Enabled";
                else                              sz_buttons[5] = "Debug";
            }

            // Fall trough
            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  / 2 ) - ( background.imgW / 2 ), GFX_HEIGHT - background.imgH, 0, 0 );
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
                if ( cfg.autoturncamera == CAMTURN_GOOD )
                {
                    sz_buttons[3] = "Off";
                    cfg.autoturncamera = CAMTURN_NONE;
                }
                else if ( cfg.autoturncamera )
                {
                    sz_buttons[3] = "Fast";
                    cfg.autoturncamera = CAMTURN_GOOD;
                }
                else
                {
                    sz_buttons[3] = "On";
                    cfg.autoturncamera = CAMTURN_AUTO;
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
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  / 2 ) + ( background.imgW / 2 ), GFX_HEIGHT - background.imgH, 0, 0 );
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
// Audio options menu
int doAudioOptions( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture background;
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
        "Save Settings",
        ""
    };

    int result = 0;

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            ego_texture_load( &background, "basicdat" SLASH_STR "menu" SLASH_STR "menu_draco", TRANSCOLOR );

            menuChoice = 0;
            menuState = MM_Entering;

            set_tip_position( menuFont, "Change audio settings here.", 20 );

            initSlidyButtons( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            // Draw the background
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0 );
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

            // Fall trough
            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0 );
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
                if ( cfg.music_volume <= 0 )
                {
                    cfg.music_volume = 0;
                }
                else
                {
                    cfg.music_volume += 5;
                }

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

            //Save settings
            if ( BUTTON_UP == ui_doButton( 8, sz_buttons[7], menuFont, buttonLeft, GFX_HEIGHT - 60, 200, 30 ) )
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
                    load_all_music_sounds();
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
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0 );
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
int doVideoOptions( float deltaTime )
{
    static int menuState = MM_Begin;
    static oglx_texture background;
    static int    menuChoice = 0;
    static STRING Cantialiasing;
    static STRING Cmaxlights;
    static STRING Cscrz;
    static STRING Cmaxparticles;
    static STRING Cmaxdyna;
    static bool_t widescreen;
    static const char *sz_buttons[] =
    {
        "N/A",    // Antialaising
        "NOT_USED",    // Unused button
        "N/A",    // Fast & ugly
        "N/A",    // Fullscreen
        "N/A",    // Reflections
        "N/A",    // Texture filtering
        "N/A",    // Shadows
        "N/A",    // Z bit
        "N/A",    // Fog
        "N/A",    // 3D effects
        "N/A",    // Multi water layer
        "N/A",    // Widescreen
        "N/A",    // Screen resolution
        "Save Settings",
        "N/A",    // Max particles
        ""
    };

    int result = 0;

    switch ( menuState )
    {
        case MM_Begin:
            // set up menu variables
            ego_texture_load( &background, "basicdat" SLASH_STR "menu" SLASH_STR "menu_video", TRANSCOLOR );

            menuChoice = 0;
            menuState = MM_Entering;

            set_tip_position( menuFont, "Change video settings here.", 20 );

            initSlidyButtons( 1.0f, sz_buttons );
            // let this fall through into MM_Entering

        case MM_Entering:
            // do buttons sliding in animation, and background fading in
            // background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            // Draw the background
            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0 );
            }

            // Load all the current video settings
            if ( cfg.multisamples == 0 ) strncpy( Cantialiasing , "Off", SDL_arraysize( Cantialiasing ) );
            else snprintf( Cantialiasing, SDL_arraysize( Cantialiasing ), "X%i", cfg.multisamples );
            sz_buttons[0] = Cantialiasing;

            // Texture filtering
            switch ( cfg.texturefilter_req )
            {
                case TX_UNFILTERED:
                    sz_buttons[5] = "Unfiltered";
                    break;
                case TX_LINEAR:
                    sz_buttons[5] = "Linear";
                    break;
                case TX_MIPMAP:
                    sz_buttons[5] = "Mipmap";
                    break;
                case TX_BILINEAR:
                    sz_buttons[5] = "Bilinear";
                    break;
                case TX_TRILINEAR_1:
                    sz_buttons[5] = "Trilinear 1";
                    break;
                case TX_TRILINEAR_2:
                    sz_buttons[5] = "Trilinear 2";
                    break;
                case TX_ANISOTROPIC:
                    sz_buttons[5] = "Ansiotropic";
                    break;
                default:                  // Set to defaults
                    sz_buttons[5] = "Linear";
                    cfg.texturefilter_req = TX_LINEAR;
                    break;
            }

            sz_buttons[2] = cfg.use_dither ? "Yes" : "No";

            sz_buttons[3] = cfg.fullscreen_req ? "True" : "False";

            if ( cfg.reflect_allowed )
            {
                sz_buttons[4] = "Low";
                if ( cfg.reflect_prt )
                {
                    sz_buttons[4] = "Medium";
                    if ( cfg.reflect_fade == 0 )
                    {
                        sz_buttons[4] = "High";
                    }
                }
            }
            else
            {
                sz_buttons[4] = "Off";
            }

            if ( cfg.shadow_allowed )
            {
                sz_buttons[6] = "Normal";
                if ( !cfg.shadow_sprite )
                {
                    sz_buttons[6] = "Best";
                }
            }
            else sz_buttons[6] = "Off";

            if ( cfg.scrz_req != 32 && cfg.scrz_req != 16 && cfg.scrz_req != 24 )
            {
                cfg.scrz_req = 16;              // Set to default
            }
            snprintf( Cscrz, SDL_arraysize( Cscrz ), "%i", cfg.scrz_req );     // Convert the integer to a char we can use
            sz_buttons[7] = Cscrz;

            snprintf( Cmaxlights, SDL_arraysize( Cmaxlights ), "%i", cfg.dyna_count_req );
            sz_buttons[8] = Cmaxlights;

            if ( cfg.use_phong )
            {
                sz_buttons[9] = "Okay";
                if ( cfg.overlay_allowed && cfg.background_allowed )
                {
                    sz_buttons[9] = "Good";
                    if ( cfg.use_perspective )
                    {
                        sz_buttons[9] = "Superb";
                    }
                }
                else                            // Set to defaults
                {
                    cfg.use_perspective    = bfalse;
                    cfg.background_allowed = bfalse;
                    cfg.overlay_allowed    = bfalse;
                    sz_buttons[9] = "Off";
                }
            }
            else                              // Set to defaults
            {
                cfg.use_perspective    = bfalse;
                cfg.background_allowed = bfalse;
                cfg.overlay_allowed    = bfalse;
                sz_buttons[9] = "Off";
            }

            if ( cfg.twolayerwater_allowed ) sz_buttons[10] = "On";
            else sz_buttons[10] = "Off";

            snprintf( Cmaxparticles, SDL_arraysize( Cmaxparticles ), "%i", cfg.particle_count_req );     // Convert the integer to a char we can use
            sz_buttons[14] = Cmaxparticles;

            switch ( cfg.scrx_req )
            {
                    // Normal resolutions
                case 1024: sz_buttons[12] = "1024X768";
                    widescreen = bfalse;
                    break;
                case 640: sz_buttons[12] = "640X480";
                    widescreen = bfalse;
                    break;
                case 800: sz_buttons[12] = "800X600";
                    widescreen = bfalse;
                    break;

                    // 1280 can be both widescreen and normal
                case 1280:
                    if ( cfg.scry_req == 1280 )
                    {
                        sz_buttons[12] = "1280X1024";
                        widescreen = bfalse;
                    }
                    if ( cfg.scry_req == 800 )
                    {
                        sz_buttons[12] = "1280X800";
                        widescreen = btrue;
                    }
                    break;

                    // Widescreen resolutions
                case 1440:
                    sz_buttons[12] = "1440X900";
                    widescreen = btrue;
                    break;
                case 1680:
                    sz_buttons[12] = "1680X1050";
                    widescreen = btrue;
                    break;
                case 1920:
                    sz_buttons[12] = "1920X1200";
                    widescreen = btrue;
                    break;

                    // unknown
                default:
                    sz_buttons[12] = "Custom";
                    break;
            }

            if ( widescreen ) sz_buttons[11] = "X";
            else             sz_buttons[11] = " ";

            menuState = MM_Running;
            break;

        case MM_Running:
            // Do normal run
            // Background
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0 );
            }

            // Antialiasing Button
            ui_drawTextBox( menuFont, "Antialiasing:", buttonLeft, GFX_HEIGHT - 215, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 1, sz_buttons[0], menuFont, buttonLeft + 150, GFX_HEIGHT - 215, 100, 30 ) )
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

                if ( cfg.multisamples == 0 ) strncpy( Cantialiasing , "Off", SDL_arraysize( Cantialiasing ) );
                else snprintf( Cantialiasing, SDL_arraysize( Cantialiasing ), "X%i", cfg.multisamples );

                sz_buttons[0] = Cantialiasing;
            }

            // Dithering
            ui_drawTextBox( menuFont, "Dithering:", buttonLeft, GFX_HEIGHT - 145, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 3, sz_buttons[2], menuFont, buttonLeft + 150, GFX_HEIGHT - 145, 100, 30 ) )
            {
                cfg.use_dither = !cfg.use_dither;
                sz_buttons[2] = cfg.use_dither ? "Yes" : "No";
            }

            // Fullscreen
            ui_drawTextBox( menuFont, "Fullscreen:", buttonLeft, GFX_HEIGHT - 110, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 4, sz_buttons[3], menuFont, buttonLeft + 150, GFX_HEIGHT - 110, 100, 30 ) )
            {
                cfg.fullscreen_req = !cfg.fullscreen_req;

                sz_buttons[3] = cfg.fullscreen_req ? "True" : "False";
            }

            // Reflection
            ui_drawTextBox( menuFont, "Reflections:", buttonLeft, GFX_HEIGHT - 250, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 5, sz_buttons[4], menuFont, buttonLeft + 150, GFX_HEIGHT - 250, 100, 30 ) )
            {

                if ( cfg.reflect_allowed && cfg.reflect_fade == 0 && cfg.reflect_prt )
                {
                    cfg.reflect_allowed = bfalse;
                    cfg.reflect_fade = 255;
                    cfg.reflect_prt = bfalse;
                    sz_buttons[4] = "Off";
                }
                else
                {
                    if ( cfg.reflect_allowed && !cfg.reflect_prt )
                    {
                        sz_buttons[4] = "Medium";
                        cfg.reflect_fade = 255;
                        cfg.reflect_prt = btrue;
                    }
                    else
                    {
                        if ( cfg.reflect_allowed && cfg.reflect_fade == 255 && cfg.reflect_prt )
                        {
                            sz_buttons[4] = "High";
                            cfg.reflect_fade = 0;
                        }
                        else
                        {
                            cfg.reflect_allowed = btrue;
                            cfg.reflect_fade = 255;
                            sz_buttons[4] = "Low";
                            cfg.reflect_prt = bfalse;
                        }
                    }
                }
            }

            // Texture Filtering
            ui_drawTextBox( menuFont, "Texture Filtering:", buttonLeft, GFX_HEIGHT - 285, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 6, sz_buttons[5], menuFont, buttonLeft + 150, GFX_HEIGHT - 285, 130, 30 ) )
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
                        sz_buttons[5] = "Unfiltered";
                        break;

                    case TX_LINEAR:
                        sz_buttons[5] = "Linear";
                        break;

                    case TX_MIPMAP:
                        sz_buttons[5] = "Mipmap";
                        break;

                    case TX_BILINEAR:
                        sz_buttons[5] = "Bilinear";
                        break;

                    case TX_TRILINEAR_1:
                        sz_buttons[5] = "Trilinear 1";
                        break;

                    case TX_TRILINEAR_2:
                        sz_buttons[5] = "Trilinear 2";
                        break;

                    case TX_ANISOTROPIC:
                        sz_buttons[5] = "Anisotropic";
                        break;

                    default:
                        cfg.texturefilter_req = TX_UNFILTERED;
                        sz_buttons[5] = "Unfiltered";
                        break;
                }
            }

            // Shadows
            ui_drawTextBox( menuFont, "Shadows:", buttonLeft, GFX_HEIGHT - 320, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 7, sz_buttons[6], menuFont, buttonLeft + 150, GFX_HEIGHT - 320, 100, 30 ) )
            {
                if ( cfg.shadow_allowed && !cfg.shadow_sprite )
                {
                    cfg.shadow_allowed = bfalse;
                    cfg.shadow_sprite = bfalse;                // Just in case
                    sz_buttons[6] = "Off";
                }
                else
                {
                    if ( cfg.shadow_allowed && cfg.shadow_sprite )
                    {
                        sz_buttons[6] = "Best";
                        cfg.shadow_sprite = bfalse;
                    }
                    else
                    {
                        cfg.shadow_allowed = btrue;
                        cfg.shadow_sprite = btrue;
                        sz_buttons[6] = "Normal";
                    }
                }
            }

            // Z bit
            ui_drawTextBox( menuFont, "Z Bit:", buttonLeft + 300, GFX_HEIGHT - 320, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 8, sz_buttons[7], menuFont, buttonLeft + 450, GFX_HEIGHT - 320, 100, 30 ) )
            {
                if ( cfg.scrz_req < 0 )
                {
                    cfg.scrz_req = 8;
                }
                else
                {
                    cfg.scrz_req += 8;
                }

                if ( cfg.scrz_req > 32 ) cfg.scrz_req = 8;

                snprintf( Cscrz, SDL_arraysize( Cscrz ), "%d", cfg.scrz_req );
                sz_buttons[7] = Cscrz;
            }

            // Max dynamic lights
            ui_drawTextBox( menuFont, "Max Lights:", buttonLeft + 300, GFX_HEIGHT - 285, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 9, sz_buttons[8], menuFont, buttonLeft + 450, GFX_HEIGHT - 285, 100, 30 ) )
            {
                if ( cfg.dyna_count_req < 8 )
                {
                    cfg.dyna_count_req = 8;
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
                sz_buttons[8] = Cmaxdyna;
            }

            // Perspective correction, overlay, underlay and phong mapping
            ui_drawTextBox( menuFont, "3D Effects:", buttonLeft + 300, GFX_HEIGHT - 250, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 10, sz_buttons[9], menuFont, buttonLeft + 450, GFX_HEIGHT - 250, 100, 30 ) )
            {
                if ( cfg.use_phong && cfg.use_perspective && cfg.overlay_allowed && cfg.background_allowed )
                {
                    cfg.use_phong          = bfalse;
                    cfg.use_perspective    = bfalse;
                    cfg.overlay_allowed    = bfalse;
                    cfg.background_allowed = bfalse;
                    sz_buttons[9] = "Off";
                }
                else
                {
                    if ( !cfg.use_phong )
                    {
                        sz_buttons[9] = "Okay";
                        cfg.use_phong = btrue;
                    }
                    else
                    {
                        if ( !cfg.use_perspective && cfg.overlay_allowed && cfg.background_allowed )
                        {
                            sz_buttons[9] = "Superb";
                            cfg.use_perspective = btrue;
                        }
                        else
                        {
                            cfg.overlay_allowed = btrue;
                            cfg.background_allowed = btrue;
                            sz_buttons[9] = "Good";
                        }
                    }
                }
            }

            // Water Quality
            ui_drawTextBox( menuFont, "Good Water:", buttonLeft + 300, GFX_HEIGHT - 215, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 11, sz_buttons[10], menuFont, buttonLeft + 450, GFX_HEIGHT - 215, 100, 30 ) )
            {
                if ( cfg.twolayerwater_allowed )
                {
                    sz_buttons[10] = "Off";
                    cfg.twolayerwater_allowed = bfalse;
                }
                else
                {
                    sz_buttons[10] = "On";
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
            else if ( BUTTON_UP == ui_doButton( 15, sz_buttons[14], menuFont, buttonLeft + 450, GFX_HEIGHT - 180, 100, 30 ) )
            {
                if ( cfg.particle_count_req < 256 )
                {
                    cfg.particle_count_req = 256;
                }
                else
                {
                    cfg.particle_count_req += 128;
                }

                if ( cfg.particle_count_req > TOTAL_MAX_PRT ) cfg.particle_count_req = 256;

                snprintf( Cmaxparticles, SDL_arraysize( Cmaxparticles ), "%i", cfg.particle_count_req );  // Convert integer to a char we can use
                sz_buttons[14] =  Cmaxparticles;
            }

            // Widescreen
            ui_drawTextBox( menuFont, "Widescreen:", buttonLeft + 300, GFX_HEIGHT - 70, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 12, sz_buttons[11], menuFont, buttonLeft + 450, GFX_HEIGHT - 70, 25, 25 ) )
            {
                widescreen = !widescreen;
                if ( !widescreen )
                {
                    sz_buttons[11] = " ";

                    // Set to default non-widescreen resolution
                    cfg.scrx_req = 640;
                    cfg.scry_req = 480;
                    sz_buttons[12] = "640x480";
                }
                else
                {
                    sz_buttons[11] = "X";

                    // Set to default widescreen resolution
                    cfg.scrx_req = 1280;
                    cfg.scry_req = 800;
                    sz_buttons[12] = "1280x800";
                }
            }

            // Screen Resolution
            ui_drawTextBox( menuFont, "Resolution:", buttonLeft + 300, GFX_HEIGHT - 110, 0, 0, 20 );
            if ( BUTTON_UP == ui_doButton( 13, sz_buttons[12], menuFont, buttonLeft + 450, GFX_HEIGHT - 110, 125, 30 ) )
            {

                // Do normal resolutions
                if ( !widescreen )
                {
                    switch ( cfg.scrx_req )
                    {
                        case 640:
                            cfg.scrx_req = 800;
                            cfg.scry_req = 600;
                            sz_buttons[12] = "800x600";
                            break;

                        case 800:
                            cfg.scrx_req = 1024;
                            cfg.scry_req = 768;
                            sz_buttons[12] = "1024x768";
                            break;

                        case 1024:
                            cfg.scrx_req = 1280;
                            cfg.scry_req = 1024;
                            sz_buttons[12] = "1280x1024";
                            break;

                    default: case 1280:
                            cfg.scrx_req = 640;
                            cfg.scry_req = 480;
                            sz_buttons[12] = "640x480";
                            break;
                    }
                }

                // Do widescreen resolutions
                else
                {
                    switch ( cfg.scrx_req )
                    {
                    default: case 1920:
                            cfg.scrx_req = 1280;
                            cfg.scry_req = 800;
                            sz_buttons[12] = "1280x800";
                            break;

                        case 1280:
                            cfg.scrx_req = 1440;
                            cfg.scry_req = 900;
                            sz_buttons[12] = "1440x900";
                            break;

                        case 1440:
                            cfg.scrx_req = 1680;
                            cfg.scry_req = 1050;
                            sz_buttons[12] = "1680x1050";
                            break;

                        case 1680:
                            cfg.scrx_req = 1920;
                            cfg.scry_req = 1200;
                            sz_buttons[12] = "1920x1200";
                            break;
                    }
                }
            }

            // Save settings button
            if ( BUTTON_UP == ui_doButton( 14, sz_buttons[13], NULL, buttonLeft, GFX_HEIGHT - 60, 200, 30 ) )
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
                initSlidyButtons( 0.0f, sz_buttons );
            }

            // tool-tip text
            ui_drawTextBox( menuFont, tipText, tipTextLeft, tipTextTop, 0, 0, 20 );

            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            if ( mnu_draw_background )
            {
                ui_drawImage( 0, &background, ( GFX_WIDTH  - background.imgW ), 0, 0, 0 );
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
					carat += snprintf( carat, carat_end - carat - 1, "%s\n", mnu_ModList.lst[selectedModule].base.summary[i] );
				}
				
				// Randomize the next game hint, but only if not in hard mode
				game_hint = CSTR_END;
				if( cfg.difficulty <= GAME_NORMAL )
				{
					// Should be okay to randomize the seed here, the random seed isnt standarized or 
					// used elsewhere before the module is loaded.
					srand( time( NULL ) );
            		if( load_local_game_hints() )		game_hint = mnu_GameTip.local_hint[rand() % mnu_GameTip.local_count];
					else if ( mnu_GameTip.count > 0 )	game_hint = mnu_GameTip.hint[rand() % mnu_GameTip.count];
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
                ui_drawTextBox( font, mnu_ModList.lst[selectedModule].base.longname, 50, 80, 291, 230, 20 );

                // Draw a text box
                ui_drawTextBox( menuFont, buffer, 50, 120, 291, 230, 20 );

				// Loading game... please wait
				fnt_getTextSize( font, "Loading module...", &text_w, &text_h );
				fnt_drawText( font, NULL, ( GFX_WIDTH / 2 ) - text_w / 2, GFX_HEIGHT - 200, "Loading module..." );

                // Draw the game tip
                if ( VALID_CSTR( game_hint ) )
                {
                    fnt_getTextSize( menuFont, "GAME TIP", &text_w, &text_h );
                    fnt_drawText( menuFont, NULL, ( GFX_WIDTH / 2 )  - text_w / 2, GFX_HEIGHT - 150, "GAME TIP" );

                    fnt_getTextSize( menuFont, game_hint, &text_w, &text_h );		//ZF> @todo: this doesnt work as I intended, fnt_get_TextSize() does not take line breaks into account
                    ui_drawTextBox( menuFont, game_hint, ( GFX_WIDTH / 2 ) - text_w / 2, GFX_HEIGHT - 125, GFX_WIDTH + 150, GFX_HEIGHT, 20 );
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

            if ( PMod->exportvalid && !local_allpladead ) buttons[0] = "Save and Exit";
            else                                          buttons[0] = "Quit Module";

            initSlidyButtons( 1.0f, buttons );

        case MM_Entering:
            drawSlidyButtons();
            updateSlidyButtons( -deltaTime );

            // Let lerp wind down relative to the time elapsed
            if ( SlidyButtonState.lerp <= 0.0f )
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
                initSlidyButtons( 0.0f, buttons );
            }
            break;

        case MM_Leaving:
            // Do buttons sliding out and background fading
            // Do the same stuff as in MM_Entering, but backwards
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            // Buttons
            drawSlidyButtons();
            updateSlidyButtons( deltaTime );
            if ( SlidyButtonState.lerp >= 1.0f )
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

            initSlidyButtons( 1.0f, buttons );

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

            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            ui_drawTextBox( NULL, endtext, x, y, w, h, 20 );
            drawSlidyButtons();

            updateSlidyButtons( -deltaTime );

            // Let lerp wind down relative to the time elapsed
            if ( SlidyButtonState.lerp <= 0.0f )
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
            GL_DEBUG( glColor4f )( 1, 1, 1, 1 - SlidyButtonState.lerp );

            ui_drawTextBox( NULL, endtext, x, y, w, h, 20 );

            // Buttons
            drawSlidyButtons();
            updateSlidyButtons( deltaTime );
            if ( SlidyButtonState.lerp >= 1.0f )
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
                if ( PMod->beat && startNewPlayer )
                {
                    // we started with a new player and beat the module... yay!
                    // now we want to graduate to the ChoosePlayer menu to
                    // build our party

                    startNewPlayer = bfalse;

                    // if we beat a beginner module, we want to
                    // go to ChoosePlayer instead of ChooseModule.
                    if ( menu_stack_peek() == emnu_ChooseModule )
                    {
                        menu_stack_pop();
                        menu_stack_push( emnu_ChoosePlayer );
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
int doMenu( float deltaTime )
{
    int retval, result = 0;

    if ( mnu_whichMenu == emnu_Main )
    {
        menu_stack_clear();
    };

    retval = MENU_NOTHING;

    switch ( mnu_whichMenu )
    {
        case emnu_Main:
            result = doMainMenu( deltaTime );
            if ( result != 0 )
            {
                if ( result == 1 )      { mnu_begin_menu( emnu_ChooseModule ); startNewPlayer = btrue; }
                else if ( result == 2 ) { mnu_begin_menu( emnu_ChoosePlayer ); startNewPlayer = bfalse; }
                else if ( result == 3 ) { mnu_begin_menu( emnu_Options ); }
                else if ( result == 4 ) retval = MENU_QUIT;  // need to request a quit somehow
            }
            break;

        case emnu_SinglePlayer:
            result = doSinglePlayerMenu( deltaTime );

            if ( result != 0 )
            {
                if ( result == 1 )
                {
                    mnu_begin_menu( emnu_ChooseModule );
                    startNewPlayer = btrue;
                }
                else if ( result == 2 )
                {
                    mnu_begin_menu( emnu_ChoosePlayer );
                    startNewPlayer = bfalse;
                }
                else if ( result == 3 )
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

            if ( result == -1 )     { mnu_end_menu(); retval = MENU_END; }
            else if ( result == 1 ) mnu_begin_menu( emnu_ShowMenuResults );  // imports are not valid (starter module)
            else if ( result == 2 ) mnu_begin_menu( emnu_ShowMenuResults );  // imports are valid

            break;

        case emnu_ChoosePlayer:
            result = doChoosePlayer( deltaTime );

            if ( result == -1 )     { mnu_end_menu(); retval = MENU_END; }
            else if ( result == 1 ) mnu_begin_menu( emnu_ChooseModule );

            break;

        case emnu_Options:
            result = doOptions( deltaTime );
            if ( result != 0 )
            {
                if ( result == 1 )      mnu_begin_menu( emnu_AudioOptions );
                else if ( result == 2 ) mnu_begin_menu( emnu_InputOptions );
                else if ( result == 3 ) mnu_begin_menu( emnu_VideoOptions );
                else if ( result == 4 ) { mnu_end_menu(); retval = MENU_END; }
                else if ( result == 5 ) mnu_begin_menu( emnu_GameOptions );
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
                if ( result == 1 )
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
                else if ( result == 2 )
                {
					// "Restart Module"
                    mnu_end_menu();
                    game_begin_module( PMod->loadname, ( Uint32 )~0 );
                    retval = MENU_END;
                }
                else if ( result == 3 )
                {
                    // "Return to Module"
                    mnu_end_menu();
                    retval = MENU_END;
                }
                else if ( result == 4 )
                {
                    // "Options"
                    mnu_begin_menu( emnu_Options );
                }
            }
            break;

        case emnu_ShowEndgame:
            result = doShowEndgame( deltaTime );
            if ( result == 1 )
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
//--------------------------------------------------------------------------------------------
bool_t mnu_checkSelectedPlayer( PLA_REF player )
{
    int i;
    if ( player > loadplayer_count ) return bfalse;

    for ( i = 0; i < MAX_PLAYER && i < mnu_selectedPlayerCount; i++ )
    {
        if ( mnu_selectedPlayer[i] == player ) return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
REF_T mnu_getSelectedPlayer( PLA_REF player )
{
    REF_T ipla;
    if ( player > loadplayer_count ) return MNU_INVALID_PLA;

    for ( ipla = 0; ipla < MAX_PLAYER && ipla < mnu_selectedPlayerCount; ipla++ )
    {
        if ( mnu_selectedPlayer[ ipla ] == player ) return ipla;
    }

    return MNU_INVALID_PLA;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_addSelectedPlayer( PLA_REF player )
{
    if ( player > loadplayer_count || mnu_selectedPlayerCount >= MAX_PLAYER ) return bfalse;
    if ( mnu_checkSelectedPlayer( player ) ) return bfalse;

    mnu_selectedPlayer[mnu_selectedPlayerCount] = player;
    mnu_selectedInput[mnu_selectedPlayerCount]  = INPUT_BITS_NONE;
    mnu_selectedPlayerCount++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_removeSelectedPlayer( PLA_REF player )
{
    int i;
    bool_t found = bfalse;

    if ( player > loadplayer_count || mnu_selectedPlayerCount <= 0 ) return bfalse;

    if ( mnu_selectedPlayerCount == 1 )
    {
        if ( mnu_selectedPlayer[0] == player )
        {
            mnu_selectedPlayerCount = 0;
        };
    }
    else
    {
        for ( i = 0; i < MAX_PLAYER && i < mnu_selectedPlayerCount; i++ )
        {
            if ( mnu_selectedPlayer[i] == player )
            {
                found = btrue;
                break;
            }
        }

        if ( found )
        {
            i++;
            for ( /* nothing */; i < MAX_PLAYER && i < mnu_selectedPlayerCount; i++ )
            {
                mnu_selectedPlayer[i-1] = mnu_selectedPlayer[i];
                mnu_selectedInput[i-1]  = mnu_selectedInput[i];
            }

            mnu_selectedPlayerCount--;
        }
    };

    return found;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_addSelectedPlayerInput( PLA_REF player, Uint32 input )
{
    int i;
    bool_t done, retval = bfalse;

    int selected_index = -1;

    for ( i = 0; i < mnu_selectedPlayerCount; i++ )
    {
        if ( mnu_selectedPlayer[i] == player )
        {
            selected_index = i;
            break;
        }
    }

    if ( -1 == selected_index )
    {
        mnu_addSelectedPlayer( player );
    }

    if ( selected_index >= 0 && selected_index < mnu_selectedPlayerCount )
    {
        for ( i = 0; i < mnu_selectedPlayerCount; i++ )
        {
            if ( i == selected_index )
            {
                // add in the selected bits for the selected player
                mnu_selectedInput[i] |= input;
                retval = btrue;
            }
            else
            {
                // remove the selectd bits from all other players
                mnu_selectedInput[i] &= ~input;
            }
        }
    }

    // Do the tricky part of removing all players with invalid inputs from the list
    // It is tricky because removing a player changes the value of the loop control
    // value mnu_selectedPlayerCount within the loop.
    done = bfalse;
    while ( !done )
    {
        // assume the best
        done = btrue;

        for ( i = 0; i < mnu_selectedPlayerCount; i++ )
        {
            if ( INPUT_BITS_NONE == mnu_selectedInput[i] )
            {
                // we found one
                done = bfalse;
                mnu_removeSelectedPlayer( mnu_selectedPlayer[i] );
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_removeSelectedPlayerInput( PLA_REF player, Uint32 input )
{
    int i;
    bool_t retval = bfalse;

    for ( i = 0; i < MAX_PLAYER && i < mnu_selectedPlayerCount; i++ )
    {
        if ( mnu_selectedPlayer[i] == player )
        {
            mnu_selectedInput[i] &= ~input;

            // This part is not so tricky as in mnu_addSelectedPlayerInput.
            // Even though we are modding the loop control variable, it is never
            // tested in the loop because we are using the break command to
            // break out of the loop immediately

            if ( INPUT_BITS_NONE == mnu_selectedInput[i] )
            {
                mnu_removeSelectedPlayer( player );
            }

            retval = btrue;

            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void check_player_import( const char *dirname, bool_t initialize )
{
    /// @details ZZ@> This function figures out which players may be imported, and loads basic
    ///     data for each

    STRING filename;
    const char *foundfile;
    int skin = 0;

    LOAD_PLAYER_INFO * pinfo;

    if ( initialize )
    {
        // restart from nothing
        loadplayer_count = 0;
        release_all_profile_textures();

        chop_data_init( &chop_mem );
    };

    // Search for all objects
    foundfile = vfs_findFirst( dirname, "obj", VFS_SEARCH_DIR );
    while ( VALID_CSTR( foundfile ) && loadplayer_count < MAXLOADPLAYER )
    {
        pinfo = loadplayer + loadplayer_count;

        snprintf( pinfo->dir, SDL_arraysize( pinfo->dir ), "%s", str_convert_slash_sys(( char* )foundfile, strlen( foundfile ) ) );

        snprintf( filename, SDL_arraysize( filename ), "%s" SLASH_STR "skin.txt", foundfile );
        skin = read_skin( filename );

        //snprintf( filename, SDL_arraysize(filename), "%s" SLASH_STR "tris.md2", foundfile );
        //md2_load_one( vfs_resolveReadFilename(filename), &(MadList[loadplayer_count].md2_data) );

        snprintf( filename, SDL_arraysize( filename ), "%s" SLASH_STR "icon%d", foundfile, skin );
        pinfo->tx_ref = TxTexture_load_one( filename, INVALID_TEXTURE, INVALID_KEY );

        // load the chop data
        snprintf( filename, SDL_arraysize( filename ), "%s" SLASH_STR "naming.txt", foundfile );
        chop_load( &chop_mem, filename, &( pinfo->chop ) );

        // generate the name from the chop
        snprintf( pinfo->name, SDL_arraysize( pinfo->name ), "%s", chop_create( &chop_mem, &( pinfo->chop ) ) );

        loadplayer_count++;

        foundfile = vfs_findNext();
    }
    vfs_findClose();
}

//--------------------------------------------------------------------------------------------
void load_all_menu_images()
{
    /// @details ZZ@> This function loads the title image for each module.  Modules without a
    ///     title are marked as invalid

    STRING loadname;
    int cnt;
    vfs_FILE* filesave;

    // release all allocated data from the mnu_ModList and empty the list
    mnu_ModList_release_images();

    // Log a directory list
    filesave = vfs_openWrite( "debug" SLASH_STR "modules.txt" );
    if ( NULL != filesave )
    {
        vfs_printf( filesave, "This file logs all of the modules found\n" );
        vfs_printf( filesave, "** Denotes an invalid module\n" );
        vfs_printf( filesave, "## Denotes an unlockable module\n\n" );
    }

    // load all the title images for modules that we are going to display
    for ( cnt = 0; cnt < mnu_ModList.count; cnt++ )
    {
        if ( !mnu_ModList.lst[cnt].loaded )
        {
            vfs_printf( filesave, "**.  %s\n", mnu_ModList.lst[cnt].name );
        }
        else if ( mnu_test_by_index( cnt ) )
        {
            // @note just because we can't load the title image DOES NOT mean that we ignore the module
            snprintf( loadname, SDL_arraysize( loadname ), "%s" SLASH_STR "gamedat" SLASH_STR "title", mnu_ModList.lst[cnt].name );

            mnu_ModList.lst[cnt].tex_index = TxTitleImage_load_one( loadname );

            vfs_printf( filesave, "%02d.  %s\n", cnt, mnu_ModList.lst[cnt].name );
        }
        else
        {
            vfs_printf( filesave, "##.  %s\n", mnu_ModList.lst[cnt].name );
        }
    }

    if ( filesave != NULL )
    {
        vfs_close( filesave );
    }
}

//--------------------------------------------------------------------------------------------
bool_t mnu_begin_menu( which_menu_t which )
{
    if ( !menu_stack_push( mnu_whichMenu ) ) return bfalse;
    mnu_whichMenu = which;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void   mnu_end_menu()
{
    mnu_whichMenu = menu_stack_pop();
}

//--------------------------------------------------------------------------------------------
int mnu_get_menu_depth()
{
    return menu_stack_index;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t menu_stack_push( which_menu_t menu )
{
    menu_stack_index = CLIP( menu_stack_index, 0, MENU_STACK_COUNT ) ;

    if ( menu_stack_index >= MENU_STACK_COUNT ) return bfalse;

    menu_stack[menu_stack_index] = menu;
    menu_stack_index++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
which_menu_t menu_stack_pop()
{
    if ( menu_stack_index < 0 )
    {
        menu_stack_index = 0;
        return emnu_Main;
    }
    if ( menu_stack_index > MENU_STACK_COUNT )
    {
        menu_stack_index = MENU_STACK_COUNT;
    }

    if ( menu_stack_index == 0 ) return emnu_Main;

    menu_stack_index--;
    return menu_stack[menu_stack_index];
}

//--------------------------------------------------------------------------------------------
which_menu_t menu_stack_peek()
{
    which_menu_t return_menu = emnu_Main;

    if ( menu_stack_index > 0 )
    {
        return_menu = menu_stack[menu_stack_index-1];
    }

    return return_menu;
}

//--------------------------------------------------------------------------------------------
void menu_stack_clear()
{
    menu_stack_index = 0;
    menu_stack[0] = emnu_Main;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void mnu_release_one_module( int imod )
{
    mnu_module_t * pmod;

    if ( !VALID_MOD( imod ) ) return;
    pmod = mnu_ModList.lst + imod;

    TxTitleImage_release_one( pmod->tex_index );
    pmod->tex_index = INVALID_TITLEIMAGE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int mnu_get_mod_number( const char *szModName )
{
    /// @details ZZ@> This function returns -1 if the module does not exist locally, the module
    ///    index otherwise

    int modnum, retval = -1;

    for ( modnum = 0; modnum < mnu_ModList.count; modnum++ )
    {
        if ( 0 == strcmp( mnu_ModList.lst[modnum].name, szModName ) )
        {
            retval = modnum;
            break;
        }
    }

    return modnum;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_test_by_index( int modnumber )
{
    int     cnt;
    mnu_module_t * pmod;
    bool_t  allowed;

    if ( INVALID_MOD( modnumber ) ) return bfalse;
    pmod = mnu_ModList.lst + modnumber;

    // First check if we are in developers mode or that the right module has been beaten before
    allowed = bfalse;
    if ( cfg.dev_mode || module_has_idsz( pmod->base.reference, pmod->base.quest_idsz ) )
    {
        allowed = btrue;
    }
    else
    {
        // If that did not work, then check all selected players directories
        for ( cnt = 0; cnt < mnu_selectedPlayerCount; cnt++ )
        {
            if ( pmod->base.quest_level <= quest_check( loadplayer[mnu_selectedPlayer[cnt]].dir, pmod->base.quest_idsz ) )
            {
                allowed = btrue;
                break;
            }
        }

    }

    return allowed;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_test_by_name( const char *szModName )
{
    /// @details ZZ@> This function tests to see if a module can be entered by
    ///    the players

    // find the module by name
    int modnumber = mnu_get_mod_number( szModName );

    return mnu_test_by_index( modnumber );
}

//--------------------------------------------------------------------------------------------
void mnu_module_init( mnu_module_t * pmod )
{
    if ( NULL == pmod ) return;

    // clear the module
    memset( pmod, 0, sizeof( *pmod ) );

    pmod->tex_index = INVALID_TITLEIMAGE;
}

//--------------------------------------------------------------------------------------------
void mnu_load_all_module_info()
{
    STRING loadname;
    const char *FileName;

    // reset the module list
    mnu_ModList_release_all();

    // Search for all .mod directories and load the module info
    FileName = vfs_findFirst( "modules", "mod", VFS_SEARCH_DIR );
    while ( VALID_CSTR( FileName ) && mnu_ModList.count < MAX_MODULE )
    {
        mnu_module_t * pmod = mnu_ModList.lst + mnu_ModList.count;

        // clear the module
        mnu_module_init( pmod );

        // save the filename
        snprintf( loadname, SDL_arraysize( loadname ), "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", FileName );

        if ( NULL != module_load_info( loadname, &( pmod->base ) ) )
        {
            pmod->loaded = btrue;
            strncpy( pmod->name, vfs_convert_fname_sys( FileName ), SDL_arraysize( pmod->name ) );
            mnu_ModList.count++;
        };

        FileName = vfs_findNext();
    }
    vfs_findClose();
}

//--------------------------------------------------------------------------------------------
REF_T mnu_get_icon_ref( REF_T icap, Uint32 default_ref )
{
    /// @details BB@> This function gets the proper icon for a an object profile.
    //
    //     In the character preview section of the menu system, we do not load
    //     entire profiles, just the character definition file ("data.txt")
    //     and one icon. Sometimes, though the item is actually a spell effect which means
    //     that we need to display the book icon.

    TX_REF icon_ref = ICON_NULL;
    bool_t is_spell_fx, is_book, draw_book;

    cap_t * pitem_cap;

    if ( !LOADED_CAP( icap ) ) return icon_ref;
    pitem_cap = CapList + icap;

    // what do we need to draw?
    is_spell_fx = pitem_cap->spelleffect_type != NOSKINOVERRIDE;
    is_book     = ( SPELLBOOK == icap );
    draw_book   = ( is_book || is_spell_fx ) && ( bookicon_count > 0 );

    if ( !draw_book )
    {
        icon_ref = default_ref;
    }
    else if ( draw_book )
    {
        int iskin = 0;

        if ( pitem_cap->spelleffect_type != 0 )
        {
            iskin = pitem_cap->spelleffect_type;
        }
        else if ( pitem_cap->skinoverride != 0 )
        {
            iskin = pitem_cap->skinoverride;
        }

        iskin = CLIP( iskin, 0, bookicon_count );

        icon_ref = bookicon_ref[ iskin ];
    }

    return icon_ref;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int do_menu_proc_begin( menu_process_t * mproc )
{
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
//--------------------------------------------------------------------------------------------
void load_global_game_hints()
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
        log_warning( "Could not load the game tips and hints. (basicdat" SLASH_STR "gametips.txt)\n" );
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
            str_add_linebreaks( mnu_GameTip.hint[cnt], SDL_arraysize( mnu_GameTip.hint[cnt] ), 50 );

            //Keep track of how many we have total
            mnu_GameTip.count++;
        }
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t load_local_game_hints()
{
    /// ZF@> This function loads all module specific hints and tips. If this fails, the game will
	//       default to the global hints and tips instead

    STRING buffer;
    vfs_FILE *fileread;
    Uint8 cnt;

    // reset the count
    mnu_GameTip.local_count = 0;

    // Open all the tips
    fileread = vfs_openRead( "mp_data/gametips.txt" );
    if ( NULL == fileread )	return bfalse;

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
            str_add_linebreaks( mnu_GameTip.local_hint[cnt], SDL_arraysize( mnu_GameTip.local_hint[cnt] ), 50 );

            //Keep track of how many we have total
            mnu_GameTip.local_count++;
        }
    }

    vfs_close( fileread );

	return mnu_GameTip.local_count > 0;
}
