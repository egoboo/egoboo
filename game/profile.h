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

#include "egoboo_typedef.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_cap;
struct s_mad;
struct s_eve;
struct s_pip;
struct Mix_Chunk;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Message files

DEFINE_STACK_EXTERN( int, MessageOffset, MAXTOTALMESSAGE );

extern Uint32          message_buffer_carat;                                  ///< Where to put letter
extern char            message_buffer[MESSAGEBUFFERSIZE];                     ///< The text buffer

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Placeholders used while importing profiles
struct s_pro_import
{
    int   slot;
    int   player;
    int   slot_lst[MAX_PROFILE];
    int   max_slot;
};
typedef struct s_pro_import pro_import_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// This is for random naming

#define CHOPPERMODEL                    32
#define MAXCHOP                         (MAX_PROFILE*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)
#define MAXSECTION                      4              ///< T-wi-n-k...  Most of 4 sections

/// The buffer for the random naming data
struct s_chop_data
{
    Uint16  chop_count;             ///< The global number of name parts

    Uint32  carat;                  ///< The data pointer
    char    buffer[CHOPDATACHUNK];  ///< The name parts
    Uint16  start[MAXCHOP];         ///< The first character of each part
};
typedef struct s_chop_data chop_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Defintion of a single chop secttion
struct s_chop_section
{
    Uint16       size;     ///< Number of choices, 0
    Uint16       start;    ///< A reference to a specific offset in the chop_data_t buffer
};
typedef struct s_chop_section chop_section_t;

/// Defintion of the chop info needed to create a name
struct s_chop_definition
{
    chop_section_t  section[MAXSECTION];
};
typedef struct s_chop_definition chop_definition_t;

chop_definition_t * chop_definition_init( chop_definition_t * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a wrapper for all the datafiles in the *.obj dir
struct s_object_profile
{
    EGO_PROFILE_STUFF;

    // the sub-profiles
    Uint16  iai;                              ///< the AI  for this profile
    Uint16  icap;                             ///< the cap for this profile
    Uint16  imad;                             ///< the mad for this profile
    Uint16  ieve;                             ///< the eve for this profile

    Uint16  prtpip[MAX_PIP_PER_PROFILE];      ///< Local particles

    // the profile skins
    Uint16  skins;                            ///< Number of skins
    int     tex_ref[MAX_SKIN];                 ///< references to the icon textures
    int     ico_ref[MAX_SKIN];                 ///< references to the skin textures

    // the profile message info
    Uint16  message_start;                    ///< The first message

    /// the random naming info
    chop_definition_t chop;

    // sounds
    struct Mix_Chunk *  wavelist[MAX_WAVE];             ///< sounds in a object
};

typedef struct s_object_profile object_profile_t;
typedef struct s_object_profile pro_t;

#define VALID_PRO_RANGE( IOBJ ) ( ((IOBJ) >= 0) && ((IOBJ) < MAX_PROFILE) )
#define LOADED_PRO( IOBJ )       ( VALID_PRO_RANGE( IOBJ ) && ProList.lst[IOBJ].loaded )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern Uint16  bookicon_count;
extern Uint16  bookicon_ref[MAX_SKIN];                      ///< The first book icon

extern pro_import_t import_data;

chop_data_t chop_mem;

DEFINE_LIST_EXTERN( pro_t, ProList, MAX_PROFILE );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_profile_system();

void   ProList_init();
//void   ProList_free_all();
Uint16 ProList_get_free( Uint16 override );
bool_t ProList_free_one( Uint16 iobj );

Uint16 pro_get_icap( Uint16 iobj );
Uint16 pro_get_imad( Uint16 iobj );
Uint16 pro_get_ieve( Uint16 iobj );
Uint16 pro_get_ipip( Uint16 iobj, Uint16 ipip );

struct s_cap * pro_get_pcap( Uint16 iobj );
struct s_mad * pro_get_pmad( Uint16 iobj );
struct s_eve * pro_get_peve( Uint16 iobj );
struct s_pip * pro_get_ppip( Uint16 iobj, Uint16 ipip );

IDSZ               pro_get_idsz( Uint16 iobj, int type );
struct Mix_Chunk * pro_get_chunk( Uint16 iobj, int index );

int    pro_get_slot( const char * tmploadname, int slot_override );

int    load_one_profile( const char* tmploadname, int slot_override );
void   release_all_pro();
bool_t release_one_pro( Uint16 override );

int    load_profile_skins( const char * tmploadname, Uint16 object );
void   load_all_messages( const char *loadname, Uint16 object );
bool_t release_one_local_pips( Uint16 iobj );
void   release_all_local_pips();

void   release_all_pro_data();

void init_all_profiles();
void release_all_profiles();

void reset_messages();

const char * pro_create_chop( Uint16 iprofile );
bool_t       pro_load_chop( Uint16 profile, const char *szLoadname );

chop_data_t * chop_data_init( chop_data_t * pdata );
chop_definition_t * chop_definition_init( chop_definition_t * pdefinition );

const char *  chop_create( chop_data_t * pdata, chop_definition_t * pdef );
bool_t        chop_load( chop_data_t * pchop_data, const char *szLoadname, chop_definition_t * pchop_definition );
bool_t        chop_export( const char *szSaveName, const char * szChop );