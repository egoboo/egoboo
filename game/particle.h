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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PRTLEVELFIX         20                      // Fix for shooting over cliffs

// Particle template
#define DYNAOFF   0
#define DYNAON    1
#define DYNALOCAL 2
#define MAXFALLOFF 1400


//--------------------------------------------------------------------------------------------
// function prototypes

void free_one_particle_no_sound( Uint16 particle );
void free_one_particle( Uint16 particle );


void move_particles( void );
void free_all_particles();

void setup_particles();

void play_particle_sound( Uint16 particle, Sint8 sound );
int get_free_particle( int force );
Uint16 spawn_one_particle( float x, float y, float z,
                           Uint16 facing, Uint16 model, Uint16 pip,
                           Uint16 characterattach, Uint16 grip, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget );

int prt_count_free();

int load_one_particle(  const char *szLoadName, Uint16 object, Uint16 pip );
void reset_particles(  const char* modname );

Uint8 __prthitawall( Uint16 particle );

int    prt_is_over_water( Uint16 cnt );