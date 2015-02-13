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

/// @file game/menu.h
/// @details Implements the main menu tree, using the code in Ui.*.

#pragma once

#include "game/egoboo_typedef.h"

#include "game/network.h"
#include "game/profiles/_Include.hpp"
#include "game/egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Forward declarations
struct mod_file_t;
struct gfx_config_t;
struct Font;

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

#define MENU_TX_COUNT (MENU_TX_LAST_SPECIAL)

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
enum which_menu_t
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

//--------------------------------------------------------------------------------------------
// menu_process
//--------------------------------------------------------------------------------------------

/// a process that controls the menu system
struct menu_process_t
{
    process_t base;

    bool was_active;
    bool escape_requested, escape_latch;

    egolib_timer_t gui_timer;
};

//--------------------------------------------------------------------------------------------
// "public" mnu_TxList functions
//--------------------------------------------------------------------------------------------

oglx_texture_t * mnu_TxList_get_valid_ptr( const TX_REF itex );
TX_REF mnu_TxList_load_one_vfs( const char *filename, const TX_REF  itex_src, Uint32 key );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//extern menu_process_t * MProc;

//extern bool module_list_valid;

/// the default menu font
//extern struct Font *menuFont;

//extern bool start_new_player;

// "public" implementation of mnu_ModList
mod_file_t * mnu_ModList_get_base( int imod );
const char * mnu_ModList_get_vfs_path( int imod );
const char * mnu_ModList_get_dest_path( int imod );
const char * mnu_ModList_get_name( int imod );

// "public" module utilities
int    mnu_get_mod_number( const char *szModName );

// "public" reset of the autoformatting
void autoformat_init(gfx_config_t * pgfx);

