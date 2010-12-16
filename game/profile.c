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

#include "profile.inl"

#include "ChrList.h"
#include "PrtList.h"

#include "texture.h"
#include "log.h"
#include "script_compile.h"
#include "game.h"

#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo_mem.h"

#include "mesh.inl"
#include "bsp.inl"
#include "particle.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t _profile_system_initialized = bfalse;

chop_data_t  chop_mem = {0, 0};
pro_import_t import_data;

size_t bookicon_count   = 0;
TX_REF bookicon_ref[MAX_SKIN];                      // The first book icon

INSTANTIATE_LIST( ACCESS_TYPE_NONE, pro_t, ProList, MAX_PROFILE );
INSTANTIATE_STATIC_ARY( MessageOffsetAry, MessageOffset );

Uint32  message_buffer_carat = 0;                           // Where to put letter
char    message_buffer[MESSAGEBUFFERSIZE] = EMPTY_CSTR;     // The text buffer

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
        bookicon_ref[tnc] = INVALID_TX_TEXTURE;
    }
    bookicon_count = 0;
}

//--------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------
void profile_system_begin()
{
    /// @details BB@> initialize the profile list and load up some intialization files
    ///     necessary for the the profile loading code to work

    if ( _profile_system_initialized )
    {
        // release all profile data and reinitialize the profile list
        release_all_profiles();

        _profile_system_initialized = bfalse;
    }

    // initialize all the profile lists
    init_all_profiles();

    // initialize the script compiler
    script_compiler_init();

    // necessary for loading up the copy.txt file
    load_action_names_vfs( "mp_data/actions.txt" );

    // necessary for properly reading the "message.txt"
    reset_messages();

    // necessary for reading "naming.txt" properly
    chop_data_init( &chop_mem );

    // something that is used in the game that is somewhat related to the profile stuff
    init_slot_idsz();

    // let the code know that everything is initialized
    _profile_system_initialized = btrue;
}

//--------------------------------------------------------------------------------------------
void profile_system_end()
{
    /// @details BB@> initialize the profile list and load up some intialization files
    ///     necessary for the the profile loading code to work

    if ( _profile_system_initialized )
    {
        // release all profile data and reinitialize the profile list
        release_all_profiles();

        _profile_system_initialized = bfalse;
    }
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

    pobj->icap = ( CAP_REF ) MAX_CAP;
    pobj->imad = ( MAD_REF ) MAX_MAD;
    pobj->ieve = ( EVE_REF ) MAX_EVE;

    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        pobj->prtpip[cnt] = ( PIP_REF ) MAX_PIP;
    }

    chop_definition_init( &( pobj->chop ) );

    // do the final invalidation
    pobj->loaded   = bfalse;
    strncpy( pobj->name, "*NONE*", SDL_arraysize( pobj->name ) );

    // clear out the textures
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        pobj->tex_ref[cnt] = INVALID_TX_TEXTURE;
        pobj->ico_ref[cnt] = INVALID_TX_TEXTURE;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// The "private" ProList management functions
//--------------------------------------------------------------------------------------------
int ProList_search_free( const PRO_REF iobj )
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
size_t ProList_pop_free( int idx )
{
    /// @details BB@> pop off whatever object exists at the free list index idx

    size_t retval;

    if ( idx >= 0 && idx < ProList.free_count )
    {
        // move the index idx to the top
        int idx_top, idx_bottom;

        idx_bottom = idx;
        idx_top    = ProList.free_count - 1;

        // make sure this is a valid case
        if ( idx_top > idx_bottom && idx_top > 1 )
        {
            SWAP( size_t, ProList.free_ref[idx_top], ProList.free_ref[idx_bottom] );
        }
    }

    // just pop off the top index
    retval = MAX_PROFILE;
    if ( ProList.free_count > 0 )
    {
        ProList.free_count--;
        ProList.update_guid++;

        retval = ProList.free_ref[ProList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ProList_push_free( const PRO_REF iobj )
{
    /// @details BB@> push an object onto the free stack

    bool_t retval;

#if defined(_DEBUG)
    // determine whether this character is already in the list of free objects
    // that is an error
    if ( -1 != ProList_search_free( iobj ) ) return bfalse;
#endif

    // push it on the free stack
    retval = bfalse;
    if ( ProList.free_count < MAX_PROFILE )
    {
        ProList.free_ref[ProList.free_count] = REF_TO_INT( iobj );

        ProList.free_count++;
        ProList.update_guid++;

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

    PRO_REF cnt;

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
size_t ProList_get_free( const PRO_REF override )
{
    /// @details ZZ@> This function returns the next free character or MAX_PROFILE if there are none

    size_t retval = MAX_PROFILE;

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
bool_t ProList_free_one( const PRO_REF iobj )
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
bool_t release_one_profile_textures( const PRO_REF iobj )
{
    int tnc;
    pro_t  * pobj;

    if ( !LOADED_PRO( iobj ) ) return bfalse;
    pobj = ProList.lst + iobj;

    for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
    {
        TX_REF itex;

        itex = pobj->tex_ref[tnc];
        if ( itex > TX_LAST )
        {
            TxTexture_free_one( itex );
        }
        pobj->tex_ref[tnc] = INVALID_TX_TEXTURE;

        itex = pobj->ico_ref[tnc] ;
        if ( itex > TX_LAST )
        {
            TxTexture_free_one( itex );
        }
        pobj->ico_ref[tnc] = INVALID_TX_TEXTURE;
    }

    // reset the bookicon stuff if this object is a book
    if ( SPELLBOOK == iobj )
    {
        for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
        {
            bookicon_ref[tnc] = INVALID_TX_TEXTURE;
        }
        bookicon_count = 0;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void release_all_profile_textures()
{
    PRO_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_profile_textures( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_pro_data( const PRO_REF iobj )
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
bool_t release_one_pro( const PRO_REF iobj )
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
    PRO_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_pro( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_pro_data()
{
    /// @details BB@> release the allocated data for all objects
    PRO_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_pro_data( cnt );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int load_profile_skins_vfs( const char * tmploadname, const PRO_REF object )
{
    TX_REF min_skin_tx, min_icon_tx;
    int    max_skin, max_icon, max_tex;
    TX_REF iskin, iicon;
    int    cnt;

    STRING newloadname;

    pro_t * pobj;

    if ( !VALID_PRO_RANGE( object ) ) return 0;
    pobj = ProList.lst + object;

    // Load the skins and icons
    max_skin    = max_icon    = -1;
    min_skin_tx = min_icon_tx = INVALID_TX_TEXTURE;
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/tris%d", tmploadname, cnt );

        pobj->tex_ref[cnt] = TxTexture_load_one_vfs( newloadname, ( TX_REF )INVALID_TX_TEXTURE, TRANSCOLOR );
        if ( INVALID_TX_TEXTURE != pobj->tex_ref[cnt] )
        {
            max_skin = cnt;
            if ( INVALID_TX_TEXTURE == min_skin_tx )
            {
                min_skin_tx = pobj->tex_ref[cnt];
            }
        }

        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/icon%d", tmploadname, cnt );
        pobj->ico_ref[cnt] = TxTexture_load_one_vfs( newloadname, ( TX_REF )INVALID_TX_TEXTURE, INVALID_KEY );

        if ( INVALID_TX_TEXTURE != pobj->ico_ref[cnt] )
        {
            max_icon = cnt;

            if ( INVALID_TX_TEXTURE == min_icon_tx )
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
        if ( INVALID_TX_TEXTURE != pobj->tex_ref[cnt] && iskin != pobj->tex_ref[cnt] )
        {
            iskin = pobj->tex_ref[cnt];
        }

        if ( INVALID_TX_TEXTURE != pobj->ico_ref[cnt] && iicon != pobj->ico_ref[cnt] )
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

    MessageOffset.ary[MessageOffset.count] = message_buffer_carat;
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
void load_all_messages_vfs( const char *loadname, const PRO_REF object )
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
bool_t release_one_local_pips( const PRO_REF iobj )
{
    int cnt;
    pro_t * pobj;

    if ( !VALID_PRO_RANGE( iobj ) ) return bfalse;

    if ( !LOADED_PRO( iobj ) ) return btrue;
    pobj = ProList.lst + iobj;

    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        release_one_pip( pobj->prtpip[cnt] );
        pobj->prtpip[cnt] = ( PIP_REF ) MAX_PIP;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void release_all_local_pips()
{
    // clear out the local pips

    PRO_REF object;
    int cnt;

    for ( object = 0; object < MAX_PROFILE; object++ )
    {
        pro_t * pobj;

        if ( !ProList.lst[object].loaded ) continue;
        pobj = ProList.lst + object;

        for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
        {
            release_one_pip( pobj->prtpip[cnt] );
            pobj->prtpip[cnt] = ( PIP_REF ) MAX_PIP;
        }
    }
}

//--------------------------------------------------------------------------------------------
int obj_read_slot_vfs( const char * tmploadname )
{
    vfs_FILE* fileread;
    int slot;
    STRING szLoadName;

    make_newloadname( tmploadname, "/data.txt", szLoadName );

    // Open the file
    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread ) return -1;

    // load the slot's slot no matter what
    slot = fget_next_int( fileread );

    vfs_close( fileread );

    return slot;
}

//--------------------------------------------------------------------------------------------
bool_t obj_verify_file_vfs( const char * tmploadname )
{
    STRING szLoadName;

    make_newloadname( tmploadname, "/data.txt", szLoadName );

    // Open the file
    return ( 0 != vfs_exists( szLoadName ) );
}

//--------------------------------------------------------------------------------------------
int pro_get_slot_vfs( const char * tmploadname, int slot_override )
{
    int slot;

    slot = -1;
    if ( VALID_PRO_RANGE( slot_override ) )
    {
        // just use the slot that was provided
        slot = slot_override;
    }

    // grab the slot from the file
    else
    {
        int tmp_slot = obj_read_slot_vfs( tmploadname );

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
    if ( !obj_verify_file_vfs( tmploadname ) )
    {
        slot = -1;
    }

    return slot;
}

//--------------------------------------------------------------------------------------------
int load_one_profile_vfs( const char* tmploadname, int slot_override )
{
    /// @details ZZ@> This function loads one object and returns the object slot

    int cnt;
    STRING newloadname;
    bool_t required;

    int islot;     // this has to be a signed value for this function to work properly

    PRO_REF iobj;
    pro_t * pobj;

    required = !VALID_CAP_RANGE( slot_override );

    // get a slot value
    islot = pro_get_slot_vfs( tmploadname, slot_override );

    // throw an error code if the slot is invalid of if the file doesn't exist
    if ( islot < 0 || islot > MAX_PROFILE )
    {
        // The data file wasn't found
        if ( required )
        {
            log_debug( "load_one_profile_vfs() - \"%s\" was not found. Overriding a global object?\n", tmploadname );
        }
        else if ( VALID_CAP_RANGE( slot_override ) && slot_override > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_debug( "load_one_profile_vfs() - Not able to open file \"%s\"\n", tmploadname );
        }

        return MAX_PROFILE;
    }

    // convert the slot to a profile reference
    iobj = islot;

    // throw an error code if we are trying to load over an existing profile
    // without permission
    if ( LOADED_PRO( iobj ) )
    {
        pro_t * pobj = ProList.lst + iobj;

        // Make sure global objects don't load over existing models
        if ( required && SPELLBOOK == iobj )
        {
            log_error( "load_one_profile_vfs() - object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, tmploadname );
        }
        else if ( required && overrideslots )
        {
            log_error( "load_one_profile_vfs() - object slot %i used twice (%s, %s)\n", REF_TO_INT( iobj ), pobj->name, tmploadname );
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
        log_warning( "load_one_profile_vfs() - Cannot allocate object %d (\"%s\")\n", REF_TO_INT( iobj ), tmploadname );
        return MAX_PROFILE;
    }

    // grab a pointer to the object
    pobj  = ProList.lst + iobj;

    // load the character profile
    pobj->icap = load_one_character_profile_vfs( tmploadname, islot, bfalse );
    islot = REF_TO_INT( pobj->icap );

    // Load the model for this iobj
    pobj->imad = load_one_model_profile_vfs( tmploadname, ( MAD_REF )islot );

    // Load the enchantment for this iobj
    make_newloadname( tmploadname, "/enchant.txt", newloadname );
    pobj->ieve = load_one_enchant_profile_vfs( newloadname, ( EVE_REF )islot );

    // Load the AI script for this iobj
    make_newloadname( tmploadname, "/script.txt", newloadname );
    pobj->iai = load_ai_script_vfs( newloadname );

    // Load the messages for this iobj
    make_newloadname( tmploadname, "/message.txt", newloadname );
    load_all_messages_vfs( newloadname, iobj );

    // Load the particles for this iobj
    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/part%d.txt", tmploadname, cnt );

        // Make sure it's referenced properly
        pobj->prtpip[cnt] = load_one_particle_profile_vfs( newloadname, ( PIP_REF )MAX_PIP );
    }

    pobj->skins = load_profile_skins_vfs( tmploadname, iobj );

    // Load the waves for this iobj
    for ( cnt = 0; cnt < MAX_WAVE; cnt++ )
    {
        STRING  szLoadName, wavename;

        snprintf( wavename, SDL_arraysize( wavename ), "/sound%d", cnt );
        make_newloadname( tmploadname, wavename, szLoadName );
        pobj->wavelist[cnt] = sound_load_chunk_vfs( szLoadName );
    }

    // Load the random naming table for this icap
    make_newloadname( tmploadname, "/naming.txt", newloadname );
    pro_load_chop_vfs( iobj, newloadname );

    // Fix lighting if need be
    if ( CapStack.lst[pobj->icap].uniformlit )
    {
        mad_make_equally_lit_ref( pobj->imad );
    }

    // mark the profile as loaded
    strncpy( pobj->name, tmploadname, SDL_arraysize( pobj->name ) );
    pobj->loaded = btrue;

    return islot;
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
        DisplayMsg.ary[cnt].time = 0;
    }

    for ( cnt = 0; cnt < MAXTOTALMESSAGE; cnt++ )
    {
        MessageOffset.ary[cnt] = 0;
    }

    message_buffer[0] = CSTR_END;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
const char * pro_create_chop( const PRO_REF iprofile )
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
    pcap = CapStack.lst + ppro->icap;

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
bool_t pro_load_chop_vfs( const PRO_REF iprofile, const char *szLoadname )
{
    /// BB@> load the chop for the given profile
    pro_t * ppro;

    if ( !VALID_PRO_RANGE( iprofile ) ) return bfalse;
    ppro = ProList.lst + iprofile;

    // clear out any current definition
    chop_definition_init( &( ppro->chop ) );

    return chop_load_vfs( &chop_mem, szLoadname, &( ppro->chop ) );
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
bool_t chop_load_vfs( chop_data_t * pdata, const char *szLoadname, chop_definition_t * pdefinition )
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
                int itmp;
                pdefinition->section[which_section].size  = section_count;
                itmp = ( int )pdata->chop_count - ( int )section_count;
                pdefinition->section[which_section].start = MAX( 0, itmp );
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
        int itmp;
        pdefinition->section[which_section].size  = section_count;
        itmp = ( int )pdata->chop_count - ( int )section_count;
        pdefinition->section[which_section].start = MAX( 0, itmp );
    }

    vfs_close( fileread );

    return section_count > 0;
}

//--------------------------------------------------------------------------------------------
bool_t chop_export_vfs( const char *szSaveName, const char * szChop )
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
    while ( cnt < MAXCAPNAMESIZE && CSTR_END != cTmp )
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
