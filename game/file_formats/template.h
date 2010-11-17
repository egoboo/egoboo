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

/// @file file_formats/template.h
/// @brief Write formatted output using a pre-formatted template file

#include "egoboo_typedef.h"
#include "egoboo_vfs.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    vfs_FILE* template_open_vfs( const char * filename );
    int       template_close_vfs( vfs_FILE* filetemp );

    bool_t    template_seek_free( vfs_FILE* filetemp, vfs_FILE* filewrite );
    void      template_flush( vfs_FILE* filetemp, vfs_FILE* filewrite );

    void template_put_char( vfs_FILE* filetemp, vfs_FILE* filewrite, char cval );
    void template_put_int( vfs_FILE* filetemp, vfs_FILE* filewrite, int ival );
    void template_put_float( vfs_FILE* filetemp, vfs_FILE* filewrite, float fval );
    void template_put_bool( vfs_FILE* filetemp, vfs_FILE* filewrite, bool_t truth );
    void template_put_damage_type( vfs_FILE* filetemp, vfs_FILE* filewrite, Uint8 damagetype );
    void template_put_action( vfs_FILE* filetemp, vfs_FILE* filewrite, Uint8 action );
    void template_put_gender( vfs_FILE* filetemp, vfs_FILE* filewrite, Uint8 gender );
    void template_put_pair( vfs_FILE* filetemp, vfs_FILE* filewrite, IPair val );
    void template_put_range( vfs_FILE* filetemp, vfs_FILE* filewrite, FRange val );
    void template_put_string_under( vfs_FILE* filetemp, vfs_FILE* filewrite, const char* usename );
    void template_put_idsz( vfs_FILE* filetemp, vfs_FILE* filewrite, IDSZ idsz );
    void template_put_damage_modifier( vfs_FILE* filetemp, vfs_FILE* filewrite, Uint8 mod );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _template_file_h