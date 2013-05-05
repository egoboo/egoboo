#pragma once

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

/// @file menu.h
/// @details Implements the main menu tree, using the code in Ui.*.

#include "egoboo_typedef.h"

#include "../egolib/IDSZ_map.h"
#include "../egolib/process.h"
#include "../egolib/timer.h"
#include "../egolib/extensions/ogl_texture.h"
#include "../egolib/file_formats/module_file.h"        // for MAX_MODULE

#include "network.h"
#include "profile.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_mod_file;
struct s_gfx_config;
struct s_Font;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_menu_process;
typedef struct s_menu_process menu_process_t;

struct s_LoadPlayer_element;
typedef struct s_LoadPlayer_element LoadPlayer_element_t;

struct s_LoadPlayer_list;
typedef struct s_LoadPlayer_list LoadPlayer_list_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

enum e_menu_textures
{
    MENU_TX_FONT_BMP,
    MENU_TX_ICON_NULL,
    MENU_TX_ICON_KEYB,
    MENU_TX_ICON_MOUS,
    MENU_TX_ICON_JOYA,
    MENU_TX_ICON_JOYB,
    MENU_TX_CURSOR,
    MENU_TX_LAST_SPECIAL
};

#define MENU_TX_COUNT (MENU_TX_LAST_SPECIAL + MAX_MODULE)

#define INVALID_MENU_TX_IDX MENU_TX_COUNT
#define INVALID_MNU_TX_REF ((TX_REF)INVALID_MENU_TX_IDX)

#define VALID_MENU_TX_RANGE(VAL) ( ((VAL)>=0) && ((VAL)<MENU_TX_COUNT) )

enum e_menu_retvals
{
    MENU_SELECT   =  1,
    MENU_NOTHING  =  0,
    MENU_END      = -1,
    MENU_QUIT     = -2
};

/// All the possible Egoboo menus.  yay!
enum e_which_menu
{
    emnu_Main,
    emnu_SinglePlayer,
    emnu_MultiPlayer,
    emnu_ChooseModule,
    emnu_ChoosePlayer,
    emnu_ChoosePlayerCharacter,
    emnu_ShowMenuResults,
    emnu_Options,
    emnu_GameOptions,
    emnu_VideoOptions,
    emnu_AudioOptions,
    emnu_InputOptions,
    emnu_NewPlayer,
    emnu_LoadPlayer,
    emnu_HostGame,
    emnu_JoinGame,
    emnu_GamePaused,
    emnu_ShowEndgame,
    emnu_NotImplemented
};

// this typedef must be after the enum definition or gcc has a fit
typedef enum e_which_menu which_menu_t;

/// the maxumum number of player items that can be loaded
#define MAX_LOADPLAYER     100

//--------------------------------------------------------------------------------------------
// menu_process
//--------------------------------------------------------------------------------------------

/// a process that controls the menu system
struct s_menu_process
{
    process_t base;

    bool_t was_active;
    bool_t escape_requested, escape_latch;

    egolib_timer_t gui_timer;
};

//--------------------------------------------------------------------------------------------
// LoadPlayer_element
//--------------------------------------------------------------------------------------------

/// data for caching the which players may be loaded
struct s_LoadPlayer_element
{
    STRING name;              ///< the object's name
    STRING dir;               ///< the object's full path

    CAP_REF cap_ref;          ///< the character profile index
    SKIN_T  skin_ref;         ///< the index of the object's skin
    TX_REF  tx_ref;           ///< the index of the texture

    IDSZ_node_t       quest_log[MAX_IDSZ_MAP_SIZE]; ///< all the quests this player has
    chop_definition_t chop;                         ///< put this here so we can generate a name without loading an entire profile
};

LoadPlayer_element_t * LoadPlayer_element_ctor( LoadPlayer_element_t * );
LoadPlayer_element_t * LoadPlayer_element_dtor( LoadPlayer_element_t * );

bool_t LoadPlayer_element_dealloc( LoadPlayer_element_t * );
bool_t LoadPlayer_element_init( LoadPlayer_element_t * );

//--------------------------------------------------------------------------------------------
// LoadPlayer_list
//--------------------------------------------------------------------------------------------
struct s_LoadPlayer_list
{
    int                  count;
    LoadPlayer_element_t lst[MAX_LOADPLAYER];
};

egolib_rv              LoadPlayer_list_init( LoadPlayer_list_t * lst );
egolib_rv              LoadPlayer_list_import_one( LoadPlayer_list_t * lst, const char * foundfile );
LoadPlayer_element_t * LoadPlayer_list_get_ptr( LoadPlayer_list_t * lst, int idx );
egolib_rv              LoadPlayer_list_dealloc( LoadPlayer_list_t * lst );
int                    LoadPlayer_list_get_free( LoadPlayer_list_t * lst );
egolib_rv              LoadPlayer_list_import_all( LoadPlayer_list_t * lst, const char *dirname, bool_t initialize );
egolib_rv              LoadPlayer_list_from_players( LoadPlayer_list_t * lst );

#define LOADPLAYER_LIST_INIT { 0 }
#define VALID_LOADPLAYER_IDX(LST, IDX) ( ((IDX) >= 0) && ((IDX)<(LST).count) && ((IDX)<MAX_LOADPLAYER) )

//--------------------------------------------------------------------------------------------
// "public" mnu_TxList functions
//--------------------------------------------------------------------------------------------

oglx_texture_t * mnu_TxList_get_valid_ptr( const TX_REF itex );
void             mnu_TxList_reload_all( void );
TX_REF mnu_TxList_load_one_vfs( const char *filename, const TX_REF  itex_src, Uint32 key );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern bool_t mnu_draw_background;

extern menu_process_t * MProc;

extern bool_t module_list_valid;

/// the default menu font
extern struct s_Font *menuFont;

/// declare special arrays of textures
DECLARE_LIST_EXTERN( oglx_texture_t, mnu_TxList, MENU_TX_COUNT );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// code for initializing and deinitializing the menu system
int  menu_system_begin( void );
void menu_system_end( void );

// global function to control navigation of the game menus
int doMenu( float deltaTime );

// code to start and stop menus
bool_t mnu_begin_menu( which_menu_t which );
void   mnu_end_menu( void );
int    mnu_get_menu_depth( void );

// "public" implmentation of the TxTitleImage array
//void   TxTitleImage_reload_all( void );
//TX_REF TxTitleImage_load_one_vfs( const char *szLoadName );

extern bool_t start_new_player;

// "public" implementation of mnu_ModList
struct s_mod_file * mnu_ModList_get_base( int imod );
const char *        mnu_ModList_get_vfs_path( int imod );
const char *        mnu_ModList_get_dest_path( int imod );
const char *        mnu_ModList_get_name( int imod );

// "public" module utilities
int    mnu_get_mod_number( const char *szModName );
bool_t mnu_test_module_by_name( LoadPlayer_list_t * lp_lst, const char *szModName );
bool_t mnu_test_module_by_index( LoadPlayer_list_t * lp_lst, const MOD_REF modnumber, size_t buffer_len, char * buffer );

// "public" menu process hooks
int                  menu_process_run( menu_process_t * mproc, double frameDuration );
menu_process_t     * menu_process_init( menu_process_t * mproc );

// "public" reset of the autoformatting
void autoformat_init( struct s_gfx_config * pgfx );

bool_t mnu_load_cursor( void );
bool_t mnu_load_all_global_icons( void );

#define egoboo_Menu_h
