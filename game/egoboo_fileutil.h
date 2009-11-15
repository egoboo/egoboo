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

/// Read/write values from/to files

#include "egoboo_typedef.h"
#include "egoboo_vfs.h"

#include <stdio.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_oglx_texture;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern const char *parse_filename;          ///< For debuggin' goto_colon

extern  STRING     TxFormatSupported[20]; ///< OpenGL icon surfaces
extern  Uint8      maxformattypes;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// function prototypes

void   make_newloadname( const char *modname, const char *appendname, char *newloadname );

bool_t goto_delimiter( char * buffer, vfs_FILE* fileread, char delim, bool_t optional );
char   goto_delimiter_list( char * buffer, vfs_FILE* fileread, const char * delim_list, bool_t optional );
bool_t goto_colon( char * buffer, vfs_FILE* fileread, bool_t optional );
char * goto_colon_mem( char * buffer, char * pmem, char * pmem_end, bool_t optional );

bool_t fcopy_line( vfs_FILE * fileread, vfs_FILE * filewrite );

int    fget_version( vfs_FILE* fileread );
bool_t fput_version( vfs_FILE* filewrite, int version );

bool_t copy_file_to_delimiter( vfs_FILE * fileread, vfs_FILE * filewrite, int delim, char * buffer, size_t bufflen );
char * copy_mem_to_delimiter( char * pmem, char * pmem_end, vfs_FILE * filewrite, int delim, char * user_buffer, size_t user_buffer_len );

char   fget_next_char( vfs_FILE * fileread );
int    fget_next_int( vfs_FILE * fileread );
bool_t fget_next_string( vfs_FILE * fileread, char * str, size_t str_len );
float  fget_next_float( vfs_FILE * fileread );
bool_t fget_next_name( vfs_FILE * fileread, char * name, size_t name_len );
bool_t fget_next_range( vfs_FILE* fileread, FRange * prange );
bool_t fget_next_pair( vfs_FILE * fileread, IPair * ppair );
IDSZ   fget_next_idsz( vfs_FILE * fileread );
bool_t fget_next_bool( vfs_FILE * fileread );

Sint32 fget_int( vfs_FILE* fileread );
char   fget_first_letter( vfs_FILE* fileread );
IDSZ   fget_idsz( vfs_FILE* fileread );
int    fget_int( vfs_FILE * fileread );
float  fget_float( vfs_FILE * fileread );
int    fget_damage_type( vfs_FILE * fileread );
int    fget_next_damage_type( vfs_FILE * fileread );
bool_t fget_bool( vfs_FILE * fileread );
Uint8  fget_damage_modifier( vfs_FILE * fileread );
bool_t fget_name( vfs_FILE* fileread,  char *szName, size_t max_len );
bool_t fget_string( vfs_FILE * fileread, char * str, size_t str_len );
bool_t fget_range( vfs_FILE* fileread, FRange * prange );
bool_t fget_pair( vfs_FILE* fileread, IPair * ppair );

void fput_range_raw( vfs_FILE* filewrite, FRange val );

void fput_int( vfs_FILE* filewrite, const char* text, int ival );
void fput_float( vfs_FILE* filewrite, const char* text, float fval );
void fput_bool( vfs_FILE* filewrite, const char* text, bool_t truth );
void fput_damage_type( vfs_FILE* filewrite, const char* text, Uint8 damagetype );
void fput_action( vfs_FILE* filewrite, const char* text, Uint8 action );
void fput_gender( vfs_FILE* filewrite, const char* text, Uint8 gender );
void fput_range( vfs_FILE* filewrite, const char* text, FRange val );
void fput_pair( vfs_FILE* filewrite, const char* text, IPair val );
void fput_string_under( vfs_FILE* filewrite, const char* text, const char* usename );
void fput_idsz( vfs_FILE* filewrite, const char* text, IDSZ idsz );
void fput_expansion( vfs_FILE* filewrite, const char* text, IDSZ idsz, int value );

void    GLSetup_SupportedFormats();
Uint32  ego_texture_load( struct s_oglx_texture *texture, const char *filename, Uint32 key );

int read_skin( const char *filename );