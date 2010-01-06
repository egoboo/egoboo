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

/// @file profile.c
/// @brief Implementation of functions for controlling and accessing object profiles
/// @details

#include "profile.h"

#include "char.h"
#include "particle.h"
#include "mad.h"
#include "enchant.h"

#include "texture.h"
#include "log.h"
#include "script_compile.h"
#include "game.h"
#include "mesh.h"
#include "bsp.h"

#include "egoboo_setup.h"
#include "egoboo_mem.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t _profile_initialized = bfalse;

chop_data_t  chop_mem = {0, 0};
pro_import_t import_data;

Uint16  bookicon_count   = 0;
Uint16  bookicon_ref[MAX_SKIN];                      // The first book icon

DECLARE_LIST( ACCESS_TYPE_NONE, pro_t,  ProList );

DECLARE_STACK( ACCESS_TYPE_NONE, int, MessageOffset );

Uint32  message_buffer_carat = 0;                           // Where to put letter
char    message_buffer[MESSAGEBUFFERSIZE] = EMPTY_CSTR;     // The text buffer

obj_BSP_t obj_BSP_root = {0, NULL, 0, NULL};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void get_message( vfs_FILE* fileread );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_profiles()
{
    /// @details ZZ@> This function initializes all of the model profiles

    int tnc;

    // initialize all the sub-profile lists
    init_all_pip();
    init_all_eve();
    init_all_cap();
    init_all_mad();
    init_all_ai_scripts();

    // initialize the profile list
    ProList_init();

    // fix the book icon list
    for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
    {
        bookicon_ref[tnc] = INVALID_TEXTURE;
    }
    bookicon_count = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_profiles()
{
    /// @details ZZ@> This function clears out all of the model data

    // release the allocated data in all profiles (sounds, textures, etc.)
    release_all_pro_data();

    // relese every type of sub-profile and re-initalize the lists
    release_all_pip();
    release_all_eve();
    release_all_cap();
    release_all_mad();
    release_all_ai_scripts();

    // re-initialize the profile list
    ProList_init();
}

//---------------------------------------------------------------------------------------------
void init_profile_system()
{
    /// @details BB@> initialize the profile list and load up some intialization files
    ///     necessary for the the profile loading code to work

    if ( _profile_initialized )
    {
        // release all profile data and reinitialize the profile list
        release_all_profiles();

        _profile_initialized = bfalse;
    }

    // initialize all the profile lists
    init_all_profiles();

    // initialize the script compiler
    script_compiler_init();

    // necessary for loading up the copy.txt file
    load_action_names( "basicdat" SLASH_STR "actions.txt" );

    // necessary for properly reading the "message.txt"
    reset_messages();

    // necessary for reading "naming.txt" properly
    chop_data_init( &chop_mem );

    // something that is used in the game that is somewhat related to the profile stuff
    init_slot_idsz();

    // let the code know that everything is initialized
    _profile_initialized = btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t pro_init( pro_t * pobj )
{
    int cnt;

    if ( NULL == pobj ) return bfalse;

    if ( pobj->loaded )
    {
        log_warning( "pro_init() - trying to init an object in use" );
    }

    //---- reset everything to safe values
    memset( pobj, 0, sizeof( *pobj ) );

    pobj->iai  = 0;
    pobj->icap = MAX_CAP;
    pobj->imad = MAX_MAD;
    pobj->ieve = MAX_EVE;

    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        pobj->prtpip[cnt] = MAX_PIP;
    }

    chop_definition_init( &( pobj->chop ) );

    // do the final invalidation
    pobj->loaded   = bfalse;
    strncpy( pobj->name, "*NONE*", SDL_arraysize( pobj->name ) );

    // clear out the textures
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        pobj->tex_ref[cnt] = INVALID_TEXTURE;
        pobj->ico_ref[cnt] = INVALID_TEXTURE;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// The "private" ProList management functions
//--------------------------------------------------------------------------------------------
int ProList_search_free( Uint16 iobj )
{
    /// @details BB@> if an object of index iobj exists on the free list, return the free list index
    ///     otherwise return -1

    int cnt, retval;

    // determine whether this character is already in the list of free textures
    // that is an error
    retval = -1;
    for ( cnt = 0; cnt < ProList.free_count; cnt++ )
    {
        if ( iobj == ProList.free_ref[cnt] )
        {
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int ProList_pop_free( int idx )
{
    /// @details BB@> pop off whatever object exists at the free list index idx

    int retval;

    if ( idx >= 0 && idx < ProList.free_count )
    {
        // move the index idx to the top
        int idx_top, idx_bottom;

        idx_bottom = idx;
        idx_top    = ProList.free_count - 1;

        // make sure this is a valid case
        if ( idx_top > idx_bottom && idx_top > 1 )
        {
            SWAP( int, ProList.free_ref[idx_top], ProList.free_ref[idx_bottom] );
        }
    }

    // just pop off the top index
    retval = -1;
    if ( ProList.free_count > 0 )
    {
        ProList.free_count--;
        retval = ProList.free_ref[ProList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ProList_push_free( Uint16 iobj )
{
    /// @details BB@> push an object onto the free stack

    int retval;

#if defined(USE_DEBUG)
    // determine whether this character is already in the list of free objects
    // that is an error
    if ( -1 != ProList_search_free( iobj ) ) return bfalse;
#endif

    // push it on the free stack
    retval = bfalse;
    if ( ProList.free_count < MAX_PROFILE )
    {
        ProList.free_ref[ProList.free_count] = iobj;
        ProList.free_count++;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// The "public" ProList management functions
//--------------------------------------------------------------------------------------------
void ProList_init()
{
    /// @details BB@> initialize all the objects and the object free list.
    ///     call before ever using the object list.

    int cnt;

    ProList.free_count = 0;
    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        // make sure we don't get a stupid warning
        ProList.lst[cnt].loaded = bfalse;
        pro_init( ProList.lst + cnt );

        ProList_push_free( cnt );
    }
}

//--------------------------------------------------------------------------------------------
Uint16 ProList_get_free( Uint16 override )
{
    /// @details ZZ@> This function returns the next free character or MAX_PROFILE if there are none

    Uint16 retval = MAX_PROFILE;

    if ( VALID_PRO_RANGE( override ) )
    {
        // grab the object that is specified.
        int free_idx;

        // if the object is in use, make sure to free everything associated with it
        if ( LOADED_PRO( override ) )
        {
            release_one_pro( override );
        }

        // grab the specified index
        free_idx = ProList_search_free( override );
        retval   = ProList_pop_free( free_idx );
    }
    else
    {
        // grab the next free index
        retval = ProList_pop_free( -1 );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ProList_free_one( Uint16 iobj )
{
    /// @details ZZ@> This function sticks an object back on the free object stack

    if ( !VALID_PRO_RANGE( iobj ) ) return bfalse;

    // object "destructor"
    // inilializes an object to safe values
    pro_init( ProList.lst + iobj );

    return ProList_push_free( iobj );
}

//--------------------------------------------------------------------------------------------
// object functions
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t release_one_profile_textures( Uint16 iobj )
{
    int tnc;
    pro_t  * pobj;

    if ( !LOADED_PRO( iobj ) ) return bfalse;
    pobj = ProList.lst + iobj;

    for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
    {
        int itex;

        itex = pobj->tex_ref[tnc] ;
        if ( itex > TX_LAST )
        {
            TxTexture_free_one( itex );
        }

        itex = pobj->ico_ref[tnc] ;
        if ( itex > TX_LAST )
        {
            TxTexture_free_one( itex );
        }
    }

    // reset the bookicon stuff if this object is a book
    if ( SPELLBOOK == iobj )
    {
        for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
        {
            bookicon_ref[tnc] = INVALID_TEXTURE;
        }
        bookicon_count = 0;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void release_all_profile_textures()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_profile_textures( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_pro_data( Uint16 iobj )
{
    int cnt;
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return bfalse;
    pobj = ProList.lst + iobj;

    // free all sounds
    for ( cnt = 0; cnt < MAX_WAVE; cnt++ )
    {
        sound_free_chunk( pobj->wavelist[cnt] );
        pobj->wavelist[cnt] = NULL;
    }

    // release whatever textures are being used
    release_one_profile_textures( iobj );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t release_one_pro( Uint16 iobj )
{
    pro_t * pobj;

    if ( !VALID_PRO_RANGE( iobj ) ) return bfalse;

    if ( !LOADED_PRO( iobj ) ) return btrue;
    pobj = ProList.lst + iobj;

    // release all of the sub-profiles
    //release_one_ai ( pobj->iai  );
    release_one_cap( pobj->icap );
    release_one_mad( pobj->imad );
    //release_one_eve( pobj->ieve );

    release_one_local_pips( iobj );

    // release the allocated data
    release_one_pro_data( iobj );

    pro_init( pobj );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void release_all_pro()
{
    /// @details BB@> release the allocated data for all objects
    int cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_pro( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_pro_data()
{
    /// @details BB@> release the allocated data for all objects
    int cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_pro_data( cnt );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16 pro_get_icap( Uint16 iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return MAX_CAP;
    pobj = ProList.lst + iobj;

    return LOADED_CAP( pobj->icap ) ? pobj->icap : MAX_CAP;
}

//--------------------------------------------------------------------------------------------
Uint16 pro_get_imad( Uint16 iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return MAX_MAD;
    pobj = ProList.lst + iobj;

    return LOADED_MAD( pobj->imad ) ? pobj->imad : MAX_MAD;
}

//--------------------------------------------------------------------------------------------
Uint16 pro_get_ieve( Uint16 iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return MAX_EVE;
    pobj = ProList.lst + iobj;

    return LOADED_EVE( pobj->ieve ) ? pobj->ieve : MAX_EVE;
}

//--------------------------------------------------------------------------------------------
Uint16 pro_get_ipip( Uint16 iobj, Uint16 ipip )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return MAX_PIP;
    pobj = ProList.lst + iobj;

    // find the local pip if it exists
    if ( ipip < MAX_PIP_PER_PROFILE )
    {
        ipip = pobj->prtpip[ipip];
    }

    return LOADED_PIP( ipip ) ? ipip : MAX_PIP;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
cap_t * pro_get_pcap( Uint16 iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    if ( !LOADED_CAP( pobj->icap ) ) return NULL;

    return CapList + pobj->icap;
}

//--------------------------------------------------------------------------------------------
mad_t * pro_get_pmad( Uint16 iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    if ( !LOADED_MAD( pobj->imad ) ) return NULL;

    return MadList + pobj->imad;
}

//--------------------------------------------------------------------------------------------
eve_t * pro_get_peve( Uint16 iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    if ( !LOADED_EVE( pobj->ieve ) ) return NULL;

    return EveStack.lst + pobj->ieve;
}

//--------------------------------------------------------------------------------------------
pip_t * pro_get_ppip( Uint16 iobj, Uint16 ipip )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    // find the local pip if it exists
    if ( ipip < MAX_PIP_PER_PROFILE )
    {
        ipip = pobj->prtpip[ipip];
    }

    if ( !LOADED_PIP( ipip ) ) return NULL;

    return PipStack.lst + ipip;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
IDSZ pro_get_idsz( Uint16 iobj, int type )
{
    cap_t * pcap;

    if ( type >= IDSZ_COUNT ) return IDSZ_NONE;

    pcap = pro_get_pcap( iobj );
    if ( NULL == pcap ) return IDSZ_NONE;

    return pcap->idsz[type];
}

//--------------------------------------------------------------------------------------------
Mix_Chunk * pro_get_chunk( Uint16 iobj, int index )
{
    pro_t * pobj;

    if ( !VALID_SND( index ) ) return NULL;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    return pobj->wavelist[index];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int load_profile_skins( const char * tmploadname, Uint16 object )
{
    int min_skin_tx, min_icon_tx;
    int max_skin, max_icon, max_tex;
    int iskin, iicon;
    int cnt;

    STRING newloadname;

    pro_t * pobj;

    if ( !VALID_PRO_RANGE( object ) ) return 0;
    pobj = ProList.lst + object;

    // Load the skins and icons
    max_skin    = max_icon    = -1;
    min_skin_tx = min_icon_tx = INVALID_TEXTURE;
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s" SLASH_STR "tris%d", tmploadname, cnt );

        pobj->tex_ref[cnt] = TxTexture_load_one( newloadname, INVALID_TEXTURE, TRANSCOLOR );
        if ( INVALID_TEXTURE != pobj->tex_ref[cnt] )
        {
            max_skin = cnt;
            if ( INVALID_TEXTURE == min_skin_tx )
            {
                min_skin_tx = pobj->tex_ref[cnt];
            }
        }

        snprintf( newloadname, SDL_arraysize( newloadname ), "%s" SLASH_STR "icon%d", tmploadname, cnt );
        pobj->ico_ref[cnt] = TxTexture_load_one( newloadname, INVALID_TEXTURE, INVALID_KEY );

        if ( INVALID_TEXTURE != pobj->ico_ref[cnt] )
        {
            max_icon = cnt;

            if ( INVALID_TEXTURE == min_icon_tx )
            {
                min_icon_tx = pobj->ico_ref[cnt];
            }

            if ( SPELLBOOK == object )
            {
                if ( bookicon_count < MAX_SKIN )
                {
                    bookicon_ref[bookicon_count] = pobj->ico_ref[cnt];
                    bookicon_count++;
                }
            }
        }
    }

    if ( max_skin < 0 )
    {
        // If we didn't get a skin, set it to the water texture
        max_skin = 0;
        pobj->tex_ref[cnt] = TX_WATER_TOP;

        log_debug( "Object is missing a skin (%s)!\n", tmploadname );
    }

    max_tex = MAX( max_skin, max_icon );

    // fill in any missing textures
    iskin = min_skin_tx;
    iicon = min_icon_tx;
    for ( cnt = 0; cnt <= max_tex; cnt++ )
    {
        if ( INVALID_TEXTURE != pobj->tex_ref[cnt] && iskin != pobj->tex_ref[cnt] )
        {
            iskin = pobj->tex_ref[cnt];
        }

        if ( INVALID_TEXTURE != pobj->ico_ref[cnt] && iicon != pobj->ico_ref[cnt] )
        {
            iicon = pobj->ico_ref[cnt];
        }

        pobj->tex_ref[cnt] = iskin;
        pobj->ico_ref[cnt] = iicon;
    }

    return max_tex + 1;
}

//--------------------------------------------------------------------------------------------
void get_message( vfs_FILE* fileread )
{
    /// @details ZZ@> This function loads a string into the message buffer, making sure it
    ///    is null terminated.

    int cnt;
    char cTmp;
    STRING szTmp;

    if ( message_buffer_carat >= MESSAGEBUFFERSIZE )
    {
        message_buffer_carat = MESSAGEBUFFERSIZE - 1;
        message_buffer[message_buffer_carat] = CSTR_END;
        return;
    }

    if ( MessageOffset.count >= MAXTOTALMESSAGE )
    {
        return;
    }

    MessageOffset.lst[MessageOffset.count] = message_buffer_carat;
    fget_string( fileread, szTmp, SDL_arraysize( szTmp ) );
    szTmp[255] = CSTR_END;

    cTmp = szTmp[0];
    cnt = 1;
    while ( CSTR_END != cTmp && message_buffer_carat < MESSAGEBUFFERSIZE - 1 )
    {
        if ( '_' == cTmp )  cTmp = ' ';

        message_buffer[message_buffer_carat] = cTmp;
        message_buffer_carat++;
        cTmp = szTmp[cnt];
        cnt++;
    }

    message_buffer[message_buffer_carat] = CSTR_END;
    message_buffer_carat++;
    MessageOffset.count++;
}

//--------------------------------------------------------------------------------------------
void load_all_messages( const char *loadname, Uint16 object )
{
    /// @details ZZ@> This function loads all of an objects messages
    vfs_FILE *fileread;

    ProList.lst[object].message_start = 0;
    fileread = vfs_openRead( loadname );
    if ( fileread )
    {
        ProList.lst[object].message_start = MessageOffset.count;

        while ( goto_colon( NULL, fileread, btrue ) )
        {
            get_message( fileread );
        }

        vfs_close( fileread );
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_local_pips( Uint16 iobj )
{
    int cnt;
    pro_t * pobj;

    if ( !VALID_PRO_RANGE( iobj ) ) return bfalse;

    if ( !LOADED_PRO( iobj ) ) return btrue;
    pobj = ProList.lst + iobj;

    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        release_one_pip( pobj->prtpip[cnt] );
        pobj->prtpip[cnt] = MAX_PIP;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void release_all_local_pips()
{
    // clear out the local pips

    int object, cnt;

    for ( object = 0; object < MAX_PROFILE; object++ )
    {
        pro_t * pobj;

        if ( !ProList.lst[object].loaded ) continue;
        pobj = ProList.lst + object;

        for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
        {
            release_one_pip( pobj->prtpip[cnt] );
            pobj->prtpip[cnt] = MAX_PIP;
        }
    }
}

//--------------------------------------------------------------------------------------------
int obj_read_slot( const char * tmploadname )
{
    vfs_FILE* fileread;
    int slot;
    STRING szLoadName;

    make_newloadname( tmploadname, SLASH_STR "data.txt", szLoadName );

    // Open the file
    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread ) return -1;

    // load the slot's slot no matter what
    slot = fget_next_int( fileread );

    vfs_close( fileread );

    return slot;
}

//--------------------------------------------------------------------------------------------
bool_t obj_verify_file( const char * tmploadname )
{
    STRING szLoadName;

    make_newloadname( tmploadname, SLASH_STR "data.txt", szLoadName );

    // Open the file
    return vfs_exists( szLoadName );
}

//--------------------------------------------------------------------------------------------
int pro_get_slot( const char * tmploadname, int slot_override )
{
    int slot;

    slot = -1;
    if ( VALID_PRO_RANGE( slot_override ) )
    {
        // just use the slot that was provided
        slot = slot_override;
    }
    else
    {
        // grab the slot from the file
        int tmp_slot = obj_read_slot( tmploadname );

        // set the slot slot
        if ( tmp_slot >= 0 )
        {
            slot = tmp_slot;
        }
        else if ( import_data.slot >= 0 )
        {
            slot = import_data.slot;
        }
    }

    // return an error value if the file does not exist
    if ( !obj_verify_file( tmploadname ) )
    {
        slot = -1;
    }

    return slot;
}

//--------------------------------------------------------------------------------------------
int load_one_profile( const char* tmploadname, int slot_override )
{
    /// @details ZZ@> This function loads one object and returns the object slot

    int cnt;
    STRING newloadname;
    bool_t required;

    int     iobj;
    pro_t * pobj;

    required = !VALID_CAP_RANGE( slot_override );

    // get a slot value
    iobj = pro_get_slot( tmploadname, slot_override );

    // throw an error code if the slot is invalid of if the file doesn't exist
    if ( !VALID_PRO_RANGE( iobj ) )
    {
        // The data file wasn't found
        if ( required )
        {
            log_warning( "load_one_profile() - \"%s\" was not found. Overriding a global object?\n", tmploadname );
        }
        else if ( VALID_CAP_RANGE( slot_override ) && slot_override > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "load_one_profile() - Not able to open file \"%s\"\n", tmploadname );
        }

        return MAX_PROFILE;
    }

    // throw an error code if we are trying to load over an existing profile
    // without permission
    if ( LOADED_PRO( iobj ) )
    {
        pro_t * pobj = ProList.lst + iobj;

        // Make sure global objects don't load over existing models
        if ( required && SPELLBOOK == iobj )
        {
            log_error( "load_one_profile() - object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, tmploadname );
        }
        else if ( required && overrideslots )
        {
            log_error( "load_one_profile() - object slot %i used twice (%s, %s)\n", iobj, pobj->name, tmploadname );
        }
        else
        {
            // Stop, we don't want to override it
            return MAX_PROFILE;
        }
    }

    // allocate/reallocate this slot
    iobj = ProList_get_free( iobj );
    if ( !VALID_PRO_RANGE( iobj ) )
    {
        log_warning( "load_one_profile() - Cannot allocate object %d (\"%s\")\n", iobj, tmploadname );
        return MAX_PROFILE;
    }

    // grab a pointer to the object
    pobj = ProList.lst + iobj;

    // load the character profile
    pobj->icap = load_one_character_profile( tmploadname, iobj, bfalse );

    // Load the model for this iobj
    pobj->imad = load_one_model_profile( tmploadname, iobj );

    // Load the enchantment for this iobj
    make_newloadname( tmploadname, SLASH_STR "enchant.txt", newloadname );
    pobj->ieve = load_one_enchant_profile( newloadname, iobj );

    // Load the AI script for this iobj
    make_newloadname( tmploadname, SLASH_STR "script.txt", newloadname );
    pobj->iai = load_ai_script( newloadname );

    // Load the messages for this iobj
    make_newloadname( tmploadname, SLASH_STR "message.txt", newloadname );
    load_all_messages( newloadname, iobj );

    // Load the particles for this iobj
    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s" SLASH_STR "part%d.txt", tmploadname, cnt );

        // Make sure it's referenced properly
        pobj->prtpip[cnt] = load_one_particle_profile( newloadname, MAX_PIP );
    }

    pobj->skins = load_profile_skins( tmploadname, iobj );

    // Load the waves for this iobj
    for ( cnt = 0; cnt < MAX_WAVE; cnt++ )
    {
        STRING  szLoadName, wavename;

        snprintf( wavename, SDL_arraysize( wavename ), SLASH_STR "sound%d", cnt );
        make_newloadname( tmploadname, wavename, szLoadName );
        pobj->wavelist[cnt] = sound_load_chunk( szLoadName );
    }

    // Load the random naming table for this icap
    make_newloadname( tmploadname, SLASH_STR "naming.txt", newloadname );
    pro_load_chop( iobj, newloadname );

    // Fix lighting if need be
    if ( CapList[pobj->icap].uniformlit )
    {
        mad_make_equally_lit( pobj->imad );
    }

    // mark the profile as loaded
    strncpy( pobj->name, tmploadname, SDL_arraysize( pobj->name ) );
    pobj->loaded = btrue;

    return iobj;
}

//--------------------------------------------------------------------------------------------
void reset_messages()
{
    /// @details ZZ@> This makes messages safe to use
    int cnt;

    MessageOffset.count = 0;
    message_buffer_carat = 0;
    msgtimechange = 0;
    DisplayMsg.count = 0;

    for ( cnt = 0; cnt < MAX_MESSAGE; cnt++ )
    {
        DisplayMsg.lst[cnt].time = 0;
    }

    for ( cnt = 0; cnt < MAXTOTALMESSAGE; cnt++ )
    {
        MessageOffset.lst[cnt] = 0;
    }

    message_buffer[0] = CSTR_END;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
const char * pro_create_chop( Uint16 iprofile )
{
    /// BB@> use the profile's chop to generate a name. Return "*NONE*" on a falure.

    pro_t * ppro;
    cap_t * pcap;
    const char * szTmp;

    // The name returned by the function
    static char buffer[MAXCAPNAMESIZE] = EMPTY_CSTR;

    // the default "bad" name
    strncpy( buffer, "*NONE*", SDL_arraysize( buffer ) );

    if ( !LOADED_PRO( iprofile ) ) return buffer;
    ppro = ProList.lst + iprofile;

    if ( !LOADED_CAP( ppro->icap ) ) return buffer;
    pcap = CapList + ppro->icap;

    if ( 0 == ppro->chop.section[0].size )
    {
        strncpy( buffer, pcap->classname, SDL_arraysize( buffer ) );
    }
    else
    {
        szTmp = chop_create( &chop_mem, &( ppro->chop ) );

        if ( VALID_CSTR( szTmp ) )
        {
            strncpy( buffer, szTmp, SDL_arraysize( buffer ) );
        }
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------
bool_t pro_load_chop( Uint16 iprofile, const char *szLoadname )
{
    /// BB@> load the chop for the given profile
    pro_t * ppro;

    if ( !VALID_PRO_RANGE( iprofile ) ) return bfalse;
    ppro = ProList.lst + iprofile;

    // clear out any current definition
    chop_definition_init( &( ppro->chop ) );

    return chop_load( &chop_mem, szLoadname, &( ppro->chop ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
chop_definition_t * chop_definition_init( chop_definition_t * pdefinition )
{
    int cnt;

    if ( NULL == pdefinition ) return pdefinition;

    for ( cnt = 0; cnt < MAXSECTION; cnt++ )
    {
        pdefinition->section[cnt].start = MAXCHOP;
        pdefinition->section[cnt].size  = 0;
    }

    return pdefinition;
}

//--------------------------------------------------------------------------------------------
chop_data_t * chop_data_init( chop_data_t * pdata )
{
    /// @details ZZ@> This function prepares the name chopper for use
    ///          BB@> It may actually be useful to blank the chop buffer

    if ( NULL == pdata ) return pdata;

    pdata->chop_count = 0;
    pdata->carat      = 0;

    return pdata;
}

//--------------------------------------------------------------------------------------------
const char * chop_create( chop_data_t * pdata, chop_definition_t * pdefinition )
{
    /// @details ZZ@> This function generates a random name.  Return "Blah" on a falure.

    int read, write, section, mychop;
    char cTmp;

    // The name returned by the function
    static char buffer[MAXCAPNAMESIZE] = EMPTY_CSTR;

    strncpy( buffer, "Blah", SDL_arraysize( buffer ) );

    if ( NULL == pdata || NULL == pdefinition ) return buffer;

    write = 0;
    for ( section = 0; section < MAXSECTION; section++ )
    {
        if ( 0 != pdefinition->section[section].size )
        {
            int irand = RANDIE;

            mychop = pdefinition->section[section].start + ( irand % pdefinition->section[section].size );

            if ( mychop < MAXCHOP )
            {
                read = pdata->start[mychop];
                cTmp = pdata->buffer[read];
                while ( CSTR_END != cTmp && write < MAXCAPNAMESIZE - 1 )
                {
                    buffer[write] = cTmp;
                    write++;
                    read++;
                    cTmp = pdata->buffer[read];
                }
                buffer[write] = CSTR_END;
            }
        }
    }
    if ( write >= MAXCAPNAMESIZE ) write = MAXCAPNAMESIZE - 1;

    buffer[write] = CSTR_END;

    return buffer;
}

//--------------------------------------------------------------------------------------------
bool_t chop_load( chop_data_t * pdata, const char *szLoadname, chop_definition_t * pdefinition )
{
    /// @details ZZ@> This function reads a naming.txt file into the chop data buffer and sets the
    ///               values of a chop definition

    int       which_section, section_count;
    STRING    tmp_buffer = EMPTY_CSTR;
    vfs_FILE *fileread;

    chop_definition_t local_definition;

    if ( NULL == pdata || pdata->carat >= CHOPDATACHUNK ) return bfalse;

    fileread = vfs_openRead( szLoadname );
    if ( NULL == fileread ) return bfalse;

    // in case we get a stupid value.
    // we could create a dynamically allocated struct in this case...
    if ( NULL == pdefinition ) pdefinition = &local_definition;

    // clear out any old definition
    chop_definition_init( pdefinition );

    which_section = 0;
    section_count = 0;
    while ( which_section < MAXSECTION && pdata->carat < CHOPDATACHUNK && goto_colon( NULL, fileread, btrue ) )
    {
        fget_string( fileread, tmp_buffer, SDL_arraysize( tmp_buffer ) );

        // convert all the '_' and junk in the string
        str_decode( tmp_buffer, SDL_arraysize( tmp_buffer ), tmp_buffer );

        if ( 0 == strcmp( tmp_buffer, "STOP" ) )
        {
            if ( which_section < MAXSECTION )
            {
                pdefinition->section[which_section].size  = section_count;
                pdefinition->section[which_section].start = pdata->chop_count - section_count;
            }

            which_section++;
            section_count = 0;
            tmp_buffer[0] = CSTR_END;
        }
        else
        {
            int chop_len;

            // fill in the chop data
            pdata->start[pdata->chop_count] = pdata->carat;
            chop_len = snprintf( pdata->buffer + pdata->carat, CHOPDATACHUNK - pdata->carat - 1, "%s", tmp_buffer );

            pdata->carat += chop_len + 1;
            pdata->chop_count++;
            section_count++;
            tmp_buffer[0] = CSTR_END;
        }
    }

    // handle the case where the chop buffer has overflowed
    // pretend the last command was "STOP"
    if ( CSTR_END != tmp_buffer[0] && which_section < MAXSECTION )
    {
        pdefinition->section[which_section].size  = section_count;
        pdefinition->section[which_section].start = pdata->chop_count - section_count;
    }

    vfs_close( fileread );

    return section_count > 0;
}

//--------------------------------------------------------------------------------------------
bool_t chop_export( const char *szSaveName, const char * szChop )
{
    /// @details ZZ@> This function exports a simple string to the naming.txt file

    vfs_FILE* filewrite;
    char cTmp;
    int cnt, tnc;

    if ( !VALID_CSTR( szChop ) ) return bfalse;

    // Can it export?
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return bfalse;

    cnt = 0;
    cTmp = szChop[0];
    cnt++;
    while ( cnt < MAXCAPNAMESIZE && cTmp != 0 )
    {
        vfs_printf( filewrite, ":" );

        for ( tnc = 0; tnc < 8 && cTmp != 0; tnc++ )
        {
            if ( ' ' == cTmp )
            {
                vfs_printf( filewrite, "_" );
            }
            else
            {
                vfs_printf( filewrite, "%c", cTmp );
            }

            cTmp = szChop[cnt];
            cnt++;
        }

        vfs_printf( filewrite, "\n" );
        vfs_printf( filewrite, ":STOP\n\n" );
    }

    vfs_close( filewrite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t obj_BSP_start( obj_BSP_t * pbsp, mpd_BSP_t * pmesh_bsp, int chr_count, int prt_count )
{
    // BB> Create a new BSP tree for the mesh.
    //     These parameters duplicate the max resolution of the old system.

    float size;

    if ( NULL == pbsp ) return bfalse;

    // copy the volume from the mesh
    pbsp->volume = pmesh_bsp->volume;

    // make some extra space in the z direction
    size = MAX( ABS( pmesh_bsp->volume.mins[OCT_X] ), ABS( pmesh_bsp->volume.maxs[OCT_X] ) );
    size = MAX( size, MAX( ABS( pmesh_bsp->volume.mins[OCT_Y] ), ABS( pmesh_bsp->volume.maxs[OCT_Y] ) ) );
    size = MAX( size, MAX( ABS( pmesh_bsp->volume.mins[OCT_Z] ), ABS( pmesh_bsp->volume.maxs[OCT_Z] ) ) );

    pbsp->volume.mins[OCT_Z] = -size * 2;
    pbsp->volume.maxs[OCT_Z] =  size * 2;

    // make a 3D BSP tree, depth copied from the mesh depth
    BSP_tree_init_1( &( pbsp->tree ), 3, pmesh_bsp->tree.depth );

    // allocate nodes for all of the tiles
    if ( chr_count < 0 ) chr_count = MAX_CHR;
    pbsp->chr_nodes = EGOBOO_NEW_ARY( BSP_node_t, chr_count );
    pbsp->chr_node_count = chr_count;

    if ( prt_count < 0 ) prt_count = MAX_CHR;
    pbsp->prt_nodes = EGOBOO_NEW_ARY( BSP_node_t, prt_count );
    pbsp->prt_node_count = prt_count;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_end( obj_BSP_t * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    BSP_tree_clear( &( pbsp->tree ) );

    EGOBOO_DELETE( pbsp->chr_nodes );
    pbsp->chr_node_count = 0;

    EGOBOO_DELETE( pbsp->prt_nodes );
    pbsp->prt_node_count = 0;

    // make the volume zero
    pbsp->volume.mins[OCT_X] = pbsp->volume.maxs[OCT_X] = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_init( obj_BSP_t * pbsp )
{
    int i;
    BSP_node_t * pnode;

    if ( NULL == pbsp ) return bfalse;

    // Create a new BSP tree for this mesh
    // use the known sizes for the pre-allocated nodes
    obj_BSP_start( pbsp, &mesh_BSP_root, MAX_CHR, maxparticles );

    for ( i = 0; i < ChrList.used_count; i++ )
    {
        Uint16 chr_ref = ChrList.used_ref[i];
        if ( !ACTIVE_CHR( chr_ref ) ) continue;

        pnode = pbsp->chr_nodes + REF_TO_INT( chr_ref );

        // let data type 1 stand for a character, -1 is uninitialized
        BSP_node_ctor( pnode, ChrList.lst + i, 1 );
        pnode->index = i;
    }

    for ( i = 0; i < PrtList.used_count; i++ )
    {
        Uint16 prt_ref = PrtList.used_ref[i];
        pnode = pbsp->prt_nodes + REF_TO_INT( prt_ref );

        // let data type 2 stand for a particle, -1 is uninitialized
        BSP_node_ctor( pnode, PrtList.lst + i, 2 );
        pnode->index = i;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_insert_node( obj_BSP_t * pbsp, BSP_node_t * pnode, int depth, int address_x[], int address_y[], int address_z[] )
{
    int i;
    bool_t retval;
    Uint32 index;
    BSP_leaf_t * pleaf, * pnew_leaf;
    BSP_tree_t * ptree = &( pbsp->tree );

    retval = bfalse;
    if ( depth < 0 )
    {
        // this can only happen if the node does not intersect the BSP bounding box
        pnode->next = ptree->infinite;
        ptree->infinite = pnode;
        retval = btrue;
    }
    else if ( 0 == depth )
    {
        // this can only happen if the object should be in the root node list
        pnode->next = ptree->root->nodes;
        ptree->root->nodes = pnode;
        retval = btrue;
    }
    else
    {
        // insert the node into the tree at this point
        pleaf = ptree->root;
        for ( i = 0; i < depth; i++ )
        {
            index = (( Uint32 )address_x[i] ) + ((( Uint32 )address_y[i] ) << 1 ) + ((( Uint32 )address_z[i] ) << 2 ) ;

            pnew_leaf = BSP_tree_ensure_leaf( ptree, pleaf, index );
            if ( NULL == pnew_leaf ) break;

            pleaf = pnew_leaf;
        };

        // insert the node in this leaf
        retval = BSP_tree_insert( ptree, pleaf, pnode, -1 );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_insert_obj_node( obj_BSP_t * pbsp, BSP_node_t * pnode, size_t depth, int address_x[], int address_y[], int address_z[] )
{
    // BB> insert a tile wrapped in a BSP_node_t into the BSP_tree_t

    int i;
    bool_t retval;
    chr_t * pchr = ( chr_t * )( pnode->data );
    prt_t * pprt = ( prt_t * )( pnode->data );

    retval = bfalse;
    if ( 1 == pnode->data_type )
    {
        if ( ACTIVE_PCHR( pchr ) && !oct_bb_empty( pchr->chr_prt_cv ) )
        {
            i = BSP_find_address_3d( pbsp->volume, pchr->chr_prt_cv, depth, address_x, address_y, address_z );
            retval = obj_BSP_insert_node( pbsp, pnode, i, address_x, address_y, address_z );
        }
    }
    else if ( 2 == pnode->data_type )
    {
        if ( ACTIVE_PPRT( pprt ) && !oct_bb_empty( pprt->chr_prt_cv ) )
        {
            i = BSP_find_address_3d( pbsp->volume, pprt->chr_prt_cv, depth, address_x, address_y, address_z );
            retval = obj_BSP_insert_node( pbsp, pnode, i, address_x, address_y, address_z );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_fill( obj_BSP_t * pbsp )
{
    size_t i;
    int * address_x, * address_y, * address_z;

    BSP_tree_t * ptree = &( pbsp->tree );

    address_x = EGOBOO_NEW_ARY( int, ptree->depth );
    address_y = EGOBOO_NEW_ARY( int, ptree->depth );
    address_z = EGOBOO_NEW_ARY( int, ptree->depth );

    for ( i = 0; i < pbsp->chr_node_count; i++ )
    {
        obj_BSP_insert_obj_node( pbsp, pbsp->chr_nodes + i, ptree->depth, address_x, address_y, address_z );
    }

    for ( i = 0; i < pbsp->prt_node_count; i++ )
    {
        obj_BSP_insert_obj_node( pbsp, pbsp->prt_nodes + i, ptree->depth, address_x, address_y, address_z );
    }

    EGOBOO_DELETE( address_x );
    EGOBOO_DELETE( address_y );
    EGOBOO_DELETE( address_z );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_empty( obj_BSP_t * pbsp )
{
    int i;

    if ( NULL == pbsp ) return bfalse;

    // free all the leaves
    BSP_tree_init_0( &( pbsp->tree ) );

    // unlink all the nodes
    for ( i = 0; i < MAX_CHR; i++ )
    {
        pbsp->chr_nodes[i].next = NULL;
    }

    // unlink all the nodes
    for ( i = 0; i < maxparticles; i++ )
    {
        pbsp->prt_nodes[i].next = NULL;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_collide_nodes( BSP_node_t node_lst[], oct_bb_t * pvobj, int colst[], size_t colist_size, int * pcolst_index )
{
    // BB > check for collisions with the given node list

    BSP_node_t * pnode;
    oct_bb_t    int_ov, * pnodevol;

    if ( NULL == node_lst || NULL == pvobj || NULL == pcolst_index ) return bfalse;
    if ( *pcolst_index >= colist_size || 0 == colist_size ) return bfalse;

    // check for collisions with any of the nodes of this leaf
    for ( pnode = node_lst; NULL != pnode; pnode = pnode->next )
    {
        if ( NULL == pnode ) assert( bfalse );

        // get the volume of the node
        pnodevol = NULL;
        if ( 1 == pnode->data_type )
        {
            pnodevol = &((( chr_t* )pnode->data )->chr_prt_cv );
        }
        else if ( 2 == pnode->data_type )
        {
            pnodevol = &((( prt_t* )pnode->data )->chr_prt_cv );
        }
        else
        {
            continue;
        }

        if ( oct_bb_intersection( *pvobj, *pnodevol, &int_ov ) )
        {
            // we have a possible intersection
            colst[*pcolst_index] = pnode->index * (( 1 == pnode->data_type ) ? 1 : -1 );
            ( *pcolst_index )++;

            if (( *pcolst_index ) >= colist_size )
            {
                // too many nodes. break out of the search.
                return bfalse;
            };
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_collide_leaf( BSP_leaf_t * pleaf, oct_bb_t * pvleaf, oct_bb_t * pvobj, int colst[], size_t colist_size, int * pcolst_index )
{
    // BB > Recursively search the BSP tree for collisions with the pvobj
    //      Return bfalse if we need to break out of the recursive search for any reson.

    Uint32 i;
    oct_bb_t    int_ov, tmp_ov;
    float x_mid, y_mid, z_mid;
    int address_x, address_y, address_z;

    if ( NULL == pvleaf || oct_bb_empty( *pvleaf ) ) return bfalse;
    if ( NULL == pvobj  || oct_bb_empty( *pvobj ) ) return bfalse;

    // return if the object does not intersect the leaf
    if ( !oct_bb_intersection( *pvobj, *pvleaf, &int_ov ) )
    {
        return bfalse;
    }

    if ( !obj_BSP_collide_nodes( pleaf->nodes, pvobj, colst, colist_size, pcolst_index ) )
    {
        return bfalse;
    };

    // check for collisions with any of the children
    x_mid = ( pvleaf->maxs[OCT_X] + pvleaf->mins[OCT_X] ) * 0.5f;
    y_mid = ( pvleaf->maxs[OCT_Y] + pvleaf->mins[OCT_Y] ) * 0.5f;
    z_mid = ( pvleaf->maxs[OCT_Z] + pvleaf->mins[OCT_Z] ) * 0.5f;
    for ( i = 0; i < pleaf->child_count; i++ )
    {
        // scan all the children
        if ( NULL == pleaf->children[i] ) continue;

        // create the volume of this node
        address_x = i & ( 1 << 0 );
        address_y = i & ( 1 << 1 );
        address_z = i & ( 1 << 2 );

        tmp_ov = *( pvleaf );

        if ( 0 == address_x )
        {
            tmp_ov.maxs[OCT_X] = x_mid;
        }
        else
        {
            tmp_ov.mins[OCT_X] = x_mid;
        }

        if ( 0 == address_y )
        {
            tmp_ov.maxs[OCT_Y] = y_mid;
        }
        else
        {
            tmp_ov.mins[OCT_X] = y_mid;
        }

        if ( 0 == address_z )
        {
            tmp_ov.maxs[OCT_Z] = z_mid;
        }
        else
        {
            tmp_ov.mins[OCT_Z] = z_mid;
        }

        if ( oct_bb_intersection( *pvobj, tmp_ov, &int_ov ) )
        {
            // potential interaction with the child. go recursive!
            bool_t ret = obj_BSP_collide_leaf( pleaf->children[i], &( tmp_ov ), pvobj, colst, colist_size, pcolst_index );
            if ( !ret ) return ret;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int obj_BSP_collide( obj_BSP_t * pbsp, oct_bb_t * pvobj, int colst[], size_t colist_size )
{
    // BB > fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    int collision_count;

    if ( NULL == pbsp || NULL == colst || 0 == colist_size ) return 0;

    if ( NULL == pvobj || oct_bb_empty( *pvobj ) ) return 0;

    // do actual collision
    collision_count = 0;

    // infinite nodes
    obj_BSP_collide_nodes( pbsp->tree.infinite, pvobj, colst, colist_size, &collision_count );

    // nodes inside the tree
    obj_BSP_collide_leaf( pbsp->tree.root, &( pbsp->volume ), pvobj, colst, colist_size, &collision_count );

    return collision_count;
}
