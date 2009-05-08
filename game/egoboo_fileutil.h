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

//Read/write values from/to files

#include <stdio.h>
#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern const char *parse_filename;          // For debuggin' goto_colon

// For damage/stat pair reads/writes
extern int pairbase, pairrand;
extern float pairfrom, pairto;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// function prototypes

void   make_newloadname( const char *modname, const char *appendname, char *newloadname );

void   goto_colon( FILE* fileread );
bool_t goto_colon_yesno( FILE* fileread );

bool_t fget_name( FILE* fileread,  char *szName, size_t max_len );
Sint32 fget_int( FILE* fileread );
char   fget_first_letter( FILE* fileread );
IDSZ   fget_idsz( FILE* fileread );

void ftruthf( FILE* filewrite, const char* text, Uint8 truth );
void fdamagf( FILE* filewrite, const char* text, Uint8 damagetype );
void factiof( FILE* filewrite, const char* text, Uint8 action );
void fgendef( FILE* filewrite, const char* text, Uint8 gender );
void fpairof( FILE* filewrite, const char* text, int base, int rand );
void funderf( FILE* filewrite, const char* text, const char* usename );

bool_t fcopy_line(FILE * fileread, FILE * filewrite);

void read_pair( FILE* fileread );
void undo_pair( int base, int rand );

