#pragma once

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

void   make_newloadname(  const char *modname,  const char *appendname, char *newloadname );

void   goto_colon( FILE* fileread );
bool_t goto_colon_yesno( FILE* fileread );

bool_t fget_name( FILE* fileread,  char *szName, size_t max_len );
Sint32 fget_int( FILE* fileread );
char   fget_first_letter( FILE* fileread );
IDSZ   fget_idsz( FILE* fileread );

void ftruthf( FILE* filewrite,  const char* text, Uint8 truth );
void fdamagf( FILE* filewrite,  const char* text, Uint8 damagetype );
void factiof( FILE* filewrite,  const char* text, Uint8 action );
void fgendef( FILE* filewrite,  const char* text, Uint8 gender );
void fpairof( FILE* filewrite,  const char* text, int base, int rand );
void funderf( FILE* filewrite,  const char* text,  const char* usename );

bool_t fcopy_line(FILE * fileread, FILE * filewrite);

void read_pair( FILE* fileread );
void undo_pair( int base, int rand );

