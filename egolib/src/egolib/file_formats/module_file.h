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

#include "egolib/typedef.h"
#include "egolib/IDSZ_map.h"

#pragma once

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct mod_file_t;

//--------------------------------------------------------------------------------------------
// Module constants
//--------------------------------------------------------------------------------------------

#define RANKSIZE              12
#define SUMMARYLINES           8
#define SUMMARYSIZE           80
#define MAX_MODULE           100                     ///< Number of modules
#define RESPAWN_ANYTIME     0xFF                     ///< Code for respawnvalid...

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// All the possible filters for modules
    enum e_module_filter
    {
        FILTER_OFF,                     ///< Display all modules
        FILTER_MAIN,                    ///< Only main quest modules
        FILTER_SIDE,                    ///< Only alternate sidequest modules
        FILTER_TOWN,                    ///< Only display Town modules
        FILTER_FUN,                     ///< Only fun modules (bumpercars!)

        FILTER_STARTER,                 ///< An extra filter for the starter modules

        // aliases
        FILTER_NORMAL_BEGIN = FILTER_OFF,
        FILTER_NORMAL_END   = FILTER_FUN
    };

    // this typedef must be after the enum definition or gcc has a fit
    typedef enum e_module_filter module_filter_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The internal representation of the *.mod file
    struct mod_file_t
    {
        // data from menu.txt
        char    rank[RANKSIZE];               ///< Number of stars
        STRING  longname;                     ///< Module names
        STRING  reference;                    ///< the module reference string
        Uint8   importamount;                 ///< # of import characters
        bool    allowexport;                  ///< Export characters?
        Uint8   minplayers;                   ///< Number of players
        Uint8   maxplayers;
        bool    monstersonly;                           ///< Only allow monsters
        Uint8   respawnvalid;                           ///< Allow respawn
        int     numlines;                               ///< Lines in summary
        char    summary[SUMMARYLINES][SUMMARYSIZE];     ///< Quest description

        IDSZ_node_t     unlockquest;                    ///< the quest required to unlock this module
        module_filter_t moduletype;                     ///< Main quest, town, sidequest or whatever
        bool            beaten;                         ///< The module has been marked with the [BEAT] eapansion
    };

    mod_file_t * mod_file__init( mod_file_t * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    bool module_has_idsz_vfs( const char *szModName, IDSZ idsz, size_t buffer_len, char * buffer );
    bool module_add_idsz_vfs( const char *szModName, IDSZ idsz, size_t buffer_len, const char * buffer );

    mod_file_t * module_load_info_vfs( const char * szLoadName, mod_file_t * pmod );
