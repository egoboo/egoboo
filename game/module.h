#pragma once

#include "egoboo_typedef.h"

//------------------------------------
// Module variables
//------------------------------------
#define RANKSIZE 12
#define SUMMARYLINES 8
#define SUMMARYSIZE  80
#define MAXMODULE           100                     // Number of modules


struct s_mod
{
    bool_t  loaded;

    char    rank[RANKSIZE];               // Number of stars
    STRING  longname;                     // Module names
    STRING  loadname;                     // Module load names
    STRING  reference;                    // the module reference string
    Uint8   importamount;                 // # of import characters
    Uint8   allowexport;                  // Export characters?
    Uint8   minplayers;                   // Number of players
    Uint8   maxplayers;                   //
    bool_t  monstersonly;                 // Only allow monsters
    bool_t  rtscontrol;                   // Real Time Stragedy?
    Uint8   respawnvalid;                 // Allow respawn
    int     numlines;                                   // Lines in summary
    char    summary[SUMMARYLINES][SUMMARYSIZE];      // Quest description
};
typedef struct s_mod mod_t;

extern int   ModList_count;                            // Number of modules
extern mod_t ModList[MAXMODULE];