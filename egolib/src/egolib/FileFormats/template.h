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

/// @file egolib/FileFormats/template.h
/// @brief Write formatted output using a pre-formatted template file

#pragma once

#include "egolib/typedef.h"
#include "egolib/vfs.h"
#include "egolib/Profiles/_Include.hpp"
#include "egolib/IDSZ.hpp"

vfs_FILE* template_open_vfs( const char * filename );
int template_close_vfs( vfs_FILE* filetemp );

bool template_seek_free(vfs_FILE* filetemp, vfs_FILE* filewrite);
void template_flush( vfs_FILE* filetemp, vfs_FILE* filewrite );

void template_put_char( vfs_FILE* filetemp, vfs_FILE* filewrite, char cval );
void template_put_int( vfs_FILE* filetemp, vfs_FILE* filewrite, int ival );
void template_put_float( vfs_FILE* filetemp, vfs_FILE* filewrite, float fval );
void template_put_ufp8( vfs_FILE* filetemp, vfs_FILE* filewrite, UFP8_T ival );
void template_put_sfp8( vfs_FILE* filetemp, vfs_FILE* filewrite, SFP8_T ival );
void template_put_bool( vfs_FILE* filetemp, vfs_FILE* filewrite, bool truth );
void template_put_damage_type( vfs_FILE* filetemp, vfs_FILE* filewrite, uint8_t damagetype );
void template_put_local_particle_profile_ref(vfs_FILE *filetetemp, vfs_FILE *filewrite, const LocalParticleProfileRef& lppref);
void template_put_action( vfs_FILE* filetemp, vfs_FILE* filewrite, uint8_t action );
void template_put_gender(vfs_FILE* filetemp, vfs_FILE* filewrite, Gender gender);
void template_put_gender_profile( vfs_FILE* filetemp, vfs_FILE* filewrite, GenderProfile gender );
void template_put_pair( vfs_FILE* filetemp, vfs_FILE* filewrite, IPair val );
void template_put_range( vfs_FILE* filetemp, vfs_FILE* filewrite, id::interval<float> val );
void template_put_string_under( vfs_FILE* filetemp, vfs_FILE* filewrite, const char* usename );
void template_put_idsz( vfs_FILE* filetemp, vfs_FILE* filewrite, const IDSZ2 &idsz );
void template_put_damage_modifier( vfs_FILE* filetemp, vfs_FILE* filewrite, uint8_t mod );
