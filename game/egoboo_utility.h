#pragma once

#include "egoboo_types.inl"
#include "egoboo_config.h"

typedef enum slot_e SLOT;
typedef enum grip_e GRIP;
typedef enum action_e ACTION;
typedef enum lip_transition_e LIPT;
typedef enum damage_e DAMAGE;
typedef enum experience_e EXPERIENCE;
typedef enum team_e TEAM;
typedef enum gender_e GENDER;
typedef enum dyna_mode_e DYNA_MODE;
typedef enum particle_type PRTTYPE;
typedef enum blud_level_e BLUD_LEVEL;
typedef enum respawn_mode_e RESPAWN_MODE;
typedef enum idsz_index_e IDSZ_INDEX;
typedef enum color_e COLR;

bool_t undo_pair_fp8( PAIR * ppair, RANGE * prange );
bool_t fget_pair_fp8( FILE* fileread, PAIR * ppair );
bool_t fget_next_pair_fp8( FILE* fileread, PAIR * ppair );

bool_t undo_pair( PAIR * ppair, RANGE * prange );
bool_t fget_pair( FILE* fileread, PAIR * ppair );
bool_t fget_next_pair( FILE* fileread, PAIR * ppair );

char * undo_idsz( IDSZ idsz );
IDSZ fget_idsz( FILE* fileread );
IDSZ fget_next_idsz( FILE* fileread );

int   fget_int( FILE* fileread );
int   fget_next_int( FILE* fileread );

float fget_float( FILE* fileread );
float fget_next_float( FILE* fileread );

Uint16 fget_fixed( FILE* fileread );
Uint16 fget_next_fixed( FILE* fileread );

bool_t fget_bool( FILE* fileread );
bool_t fget_next_bool( FILE* fileread );

GENDER fget_gender( FILE* fileread );
GENDER fget_next_gender( FILE* fileread );

DAMAGE fget_damage( FILE *fileread );
DAMAGE fget_next_damage( FILE *fileread );

BLUD_LEVEL fget_blud( FILE *fileread );
BLUD_LEVEL fget_next_blud( FILE *fileread );

RESPAWN_MODE fget_respawn( FILE *fileread );
RESPAWN_MODE fget_next_respawn( FILE *fileread );

DYNA_MODE fget_dynamode( FILE *fileread );
DYNA_MODE fget_next_dynamode( FILE *fileread );

bool_t fget_name( FILE* fileread, char *szName, size_t lnName );
bool_t fget_next_name( FILE* fileread, char *szName, size_t lnName );

bool_t fget_string( FILE* fileread, char *szLine, size_t lnLine );
bool_t fget_next_string( FILE* fileread, char *szLine, size_t lnLine );

PRTTYPE fget_prttype( FILE * fileread );
PRTTYPE fget_next_prttype( FILE * fileread );

ACTION fget_action( FILE* fileread );
ACTION fget_next_action( FILE* fileread );

void ftruthf( FILE* filewrite, char* text, bool_t truth );
void fdamagf( FILE* filewrite, char* text, DAMAGE damagetype );
void factiof( FILE* filewrite, char* text, ACTION action );
void fgendef( FILE* filewrite, char* text, GENDER gender );
void fpairof( FILE* filewrite, char* text, PAIR * ppair );
void funderf( FILE* filewrite, char* text, char* usename );

bool_t fget_message( FILE* fileread );
bool_t fget_next_message( FILE* fileread );

void   fgoto_colon( FILE* fileread );
bool_t fgoto_colon_yesno( FILE* fileread );
char   fget_first_letter( FILE* fileread );


//FILE * inherit_fopen(char * szObjPath, char * szObject, char *szFname, char * mode);
const char * inherit_fname(char * szObjPath, char * szObject, char *szFname );