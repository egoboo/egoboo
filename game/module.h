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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egoboo_typedef.h"

//------------------------------------
// Module variables
//------------------------------------
#define RANKSIZE 12
#define SUMMARYLINES 8
#define SUMMARYSIZE  80
#define MAX_MODULE           100                     // Number of modules

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_mod_data
{
    bool_t  loaded;

	// data from menu.txt
    char    rank[RANKSIZE];               // Number of stars
    STRING  longname;                     // Module names
    STRING  loadname;                     // Module load names
    STRING  reference;                    // the module reference string
    Uint8   importamount;                 // # of import characters
    bool_t  allowexport;                  // Export characters?
    Uint8   minplayers;                   // Number of players
    Uint8   maxplayers;
    bool_t  monstersonly;                           // Only allow monsters
    Uint8   respawnvalid;                           // Allow respawn
    int     numlines;                               // Lines in summary
    char    summary[SUMMARYLINES][SUMMARYSIZE];     // Quest description

    IDSZ    quest_idsz;                             // the quest required to unlock this module
    int     quest_level;                            // the quest level required to unlock this module

	// extended data
    Uint32  tex_index;                              // the index of the tile image
};
typedef struct s_mod_data mod_data_t;

DEFINE_STACK_EXTERN(mod_data_t, ModList, MAX_MODULE );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_module_instance
{
    Uint8   importamount;               // Number of imports for this module
    bool_t  exportvalid;                // Can it export?
    Uint8   playeramount;               // How many players?
    bool_t  importvalid;                // Can it import?
    bool_t  respawnvalid;               // Can players respawn with Spacebar?
    bool_t  respawnanytime;             // True if it's a small level...
    STRING  loadname;                     // Module load names

    bool_t  active;                     // Is the control loop still going?
    bool_t  beat;                       // Show Module Ended text?
    Uint32  seed;                       // The module seed
    Uint32  randsave;
};

typedef struct s_module_instance module_instance_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void   modlist_load_all_info();
int    modlist_get_mod_number( const char *szModName );
bool_t modlist_test_by_name( const char *szModName );
bool_t modlist_test_by_index( int modnumber );

int    module_reference_matches( const char *szLoadName, IDSZ idsz );
void   module_add_idsz( const char *szLoadName, IDSZ idsz );

bool_t module_instance_init( module_instance_t * pgmod );
bool_t module_upload( module_instance_t * pinst, int selectedModule, Uint32 seed );
bool_t module_reset( module_instance_t * pinst, Uint32 seed  );
bool_t module_start( module_instance_t * pinst );
bool_t module_stop( module_instance_t * pinst );
