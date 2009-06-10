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

/* Egoboo - proto.h
 * Function prototypes for a huge portion of the game code.
 */

#include "egoboo_typedef.h"

struct s_script_state;
struct s_chr_instance;

///////////////////////////////
// INTERNAL FUNCTIONS
///////////////////////////////
// internal game functions that would never be called by a script


// object initialization
void  prime_names( void );
void  free_all_objects( void );


///////////////////////////////
// EXTERNAL FUNCTIONS
///////////////////////////////
// functions that might be called by a script

// Action
int  action_which( char cTmp );
void chr_play_action( Uint16 character, Uint16 action, Uint8 actionready );
void chr_set_frame( Uint16 character, Uint16 action, int frame, Uint16 lip );

// Module
bool_t load_blip_bitmap();
void   quit_module();

// Model
bool_t chr_instance_update_vertices( struct s_chr_instance * pinst, int vmin, int vmax );

//---------------------------------------------------------------------------------------------

#define _PROTO_H_
