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

#include "egoboo_typedef.h"

//------------------------------------
// Module constants
//------------------------------------

#define RANKSIZE 12
#define SUMMARYLINES 8
#define SUMMARYSIZE  80
#define MAX_MODULE           100                     ///< Number of modules


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// All the possible filters for modules
enum e_module_filter
{
    FILTER_OFF,						///< Display all modules
	FILTER_MAIN,					///< Only main quest modules
	FILTER_SIDE,					///< Only alternate sidequest modules
	FILTER_TOWN,					///< Only display Town modules
	FILTER_FUN,						///< Only fun modules (bumpercars!)
	FILTER_COUNT
};
typedef enum e_module_filter module_filter_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The internal representation of the *.mod file
struct s_mod_file
{
    // data from menu.txt
    char    rank[RANKSIZE];               ///< Number of stars
    STRING  longname;                     ///< Module names
    STRING  reference;                    ///< the module reference string
    Uint8   importamount;                 ///< # of import characters
    bool_t  allowexport;                  ///< Export characters?
    Uint8   minplayers;                   ///< Number of players
    Uint8   maxplayers;
    bool_t  monstersonly;                           ///< Only allow monsters
    Uint8   respawnvalid;                           ///< Allow respawn
    Uint8   rtscontrol;                             ///< !! keep this in the file, even though it is not used in the game !!
    module_filter_t moduletype;						///< Main quest, town, sidequest or whatever
	int     numlines;                               ///< Lines in summary
    char    summary[SUMMARYLINES][SUMMARYSIZE];     ///< Quest description

    IDSZ    quest_idsz;                             ///< the quest required to unlock this module
    int     quest_level;                            ///< the quest level required to unlock this module
};
typedef struct s_mod_file mod_file_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int    module_has_idsz( const char *szLoadName, IDSZ idsz );
void   module_add_idsz_vfs( const char *szLoadName, IDSZ idsz );

mod_file_t * module_load_info( const char * szLoadName, mod_file_t * pmod );