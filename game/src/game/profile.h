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

/// @file game/profile.h
#pragma once

#include "game/egoboo_typedef.h"
#include "game/egoboo.h"
#include "game/script.h"     //for script_info_t
#include "game/char.h"
#include "game/mad.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct chr_t;
struct s_prt;
struct s_cap;
struct mad_t;
struct eve_t;
struct s_pip;

struct s_mpd_BSP;
struct s_prt_bundle;

typedef int SoundID;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_pro_import;
typedef struct s_pro_import pro_import_t;

struct s_chop_data;
typedef struct s_chop_data chop_data_t;

struct s_chop_section;
typedef struct s_chop_section chop_section_t;

struct s_chop_definition;
typedef struct s_chop_definition chop_definition_t;

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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// This is for random naming

#define CHOPPERMODEL                    32
#define MAXCHOP                         (MAX_PROFILE*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)
#define MAXSECTION                      4              ///< T-wi-n-k...  Most of 4 sections

/// The buffer for the random naming data
struct s_chop_data
{
    size_t  chop_count;             ///< The global number of name parts

    Uint32  carat;                  ///< The data pointer
    char    buffer[CHOPDATACHUNK];  ///< The name parts
    int     start[MAXCHOP];         ///< The first character of each part
};

chop_data_t * chop_data_init( chop_data_t * pdata );

bool        chop_export_vfs( const char *szSaveName, const char * szChop );

//--------------------------------------------------------------------------------------------

/// Defintion of a single chop section
struct s_chop_section
{
    int size;     ///< Number of choices, 0
    int start;    ///< A reference to a specific offset in the chop_data_t buffer
};

//--------------------------------------------------------------------------------------------

/// Defintion of the chop info needed to create a name
struct s_chop_definition
{
    chop_section_t  section[MAXSECTION];
};

chop_definition_t * chop_definition_init( chop_definition_t * pdefinition );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a wrapper for all the datafiles in the *.obj dir
class pro_t
{
public:
    pro_t();

    /// @author ZF
    /// @details This adds one string to the list of messages associated with a profile. The function will
    //              dynamically allocate more memory if there are more messages than array size
    //  @param filterDuplicates don't add it if it already exists
    void addMessage(const std::string &message, const bool filterDuplicates = false);

    /**
    * @return a string loaded into the specified index, or an empty string if the index is not valid
    **/
    const std::string& getMessage(size_t index) const;

    SoundID getSoundID(int index) const;

public:
    bool loaded;                    /** Was the data read in? */
    STRING name;                    /** Usually the source filename */
    int    request_count;           /** the number of attempted spawnx */
    int    create_count;            /** the number of successful spawns */

    // the sub-profiles
    CAP_REF icap;                             ///< the cap for this profile
    MAD_REF imad;                             ///< the mad for this profile
    EVE_REF ieve;                             ///< the eve for this profile

    script_info_t ai_script;                    ///< the AI script for this profile

    PIP_REF prtpip[MAX_PIP_PER_PROFILE];      ///< Local particles

    // the profile skins
    size_t  skin_gfx_cnt;                     ///< Number of loaded skin graphics
    TX_REF  tex_ref[MAX_SKIN];                ///< References to the icon textures
    TX_REF  ico_ref[MAX_SKIN];                ///< References to the skin textures

    /// the random naming info
    chop_definition_t chop;

    // the profile message info
    std::vector<std::string> _messageList;   ///< Dynamic array of messages

    // sounds
    std::array<SoundID, 30> _soundList;             ///< sounds in a object
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the profile list

DECLARE_LIST_EXTERN( pro_t, ProList, MAX_PROFILE );

int          pro_get_slot_vfs( const char * tmploadname, int slot_override );
const char * pro_create_chop( const PRO_REF profile_ref );
bool       pro_load_chop_vfs( const PRO_REF profile_ref, const char *szLoadname );

void    ProList_init();
size_t  ProList_get_free_ref( const PRO_REF override_ref );
bool  ProList_free_one( const PRO_REF object_ref );

// utility macros
#define VALID_PRO_RANGE( IPRO ) ( ((IPRO) >= 0) && ((IPRO) < MAX_PROFILE) )
#define LOADED_PRO( IPRO )       ( VALID_PRO_RANGE( IPRO ) && LOADED_PRO_RAW( IPRO ) )
#define IS_VALID_MESSAGE_PRO( IPRO, MESSAGE ) ( LOADED_PRO(IPRO) && MESSAGE >= 0 && MESSAGE < ProList.lst[IPRO]._messageList.size() )

// utility macros without range checking
#define LOADED_PRO_RAW( IPRO )   ( ProList.lst[IPRO].loaded )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern size_t  bookicon_count;
extern TX_REF  bookicon_ref[MAX_SKIN];                      ///< The first book icon

extern pro_import_t import_data;
extern chop_data_t chop_mem;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void profile_system_begin();
void profile_system_end();

void   init_all_profiles();
int    load_profile_skins_vfs( const char * tmploadname, const PRO_REF object_ref );
void   release_all_pro_data();
void   release_all_profiles();
void   release_all_pro();
void   release_all_local_pips();
bool release_one_pro( const PRO_REF object_ref );
bool release_one_local_pips( const PRO_REF object_ref );

int load_one_profile_vfs( const char* tmploadname, int slot_override );

const char *  chop_create( chop_data_t * pdata, chop_definition_t * pdef );
bool        chop_load_vfs( chop_data_t * pchop_data, const char *szLoadname, chop_definition_t * pchop_definition );

//inline
CAP_REF pro_get_icap( const PRO_REF iobj );
MAD_REF pro_get_imad( const PRO_REF iobj );
EVE_REF pro_get_ieve( const PRO_REF iobj );
PIP_REF pro_get_ipip( const PRO_REF iobj, int ipip );
IDSZ    pro_get_idsz( const PRO_REF iobj, int type );

cap_t *     pro_get_pcap( const PRO_REF iobj );
mad_t *     pro_get_pmad( const PRO_REF iobj );
eve_t *     pro_get_peve( const PRO_REF iobj );
pip_t *     pro_get_ppip( const PRO_REF iobj, int pip_index );