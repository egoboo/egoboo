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

#include "texture.h"
#include "log.h"
#include "script_compile.h"
#include "game.h"
#include "mesh.inl"
#include "bsp.h"

#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"

#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t _profile_initialized = bfalse;

chop_data_t  chop_mem = {0, 0};
pro_import_t import_data;

size_t bookicon_count   = 0;
TX_REF bookicon_ref[MAX_SKIN];                      // The first book icon

INSTANTIATE_LIST( ACCESS_TYPE_NONE, pro_t, ProList, MAX_PROFILE );
INSTANTIATE_STATIC_ARY( MessageOffsetAry, MessageOffset );

Uint32  message_buffer_carat = 0;                           // Where to put letter
char    message_buffer[MESSAGEBUFFERSIZE] = EMPTY_CSTR;     // The text buffer

#if defined(__cplusplus)
obj_BSP_t obj_BSP_root;
#else
obj_BSP_t obj_BSP_root = { BSP_TREE_INIT_VALS };
#endif

int BSP_chr_count = 0;
int BSP_prt_count = 0;

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
    load_action_names( "mp_data/actions.txt" );

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
void profile_system_end()
{
    /// @details BB@> initialize the profile list and load up some intialization files
    ///     necessary for the the profile loading code to work

    if ( _profile_initialized )
    {
        // release all profile data and reinitialize the profile list
        release_all_profiles();

        // delete the object BSP data
        obj_BSP_dtor( &obj_BSP_root );

        _profile_initialized = bfalse;
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
        pobj->tex_ref[cnt] = INVALID_TX_TEXTURE;
        pobj->ico_ref[cnt] = INVALID_TX_TEXTURE;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// The "private" ProList management functions
//--------------------------------------------------------------------------------------------
int ProList_search_free( const PRO_REF by_reference iobj )
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
        retval = ProList.free_ref[ProList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ProList_push_free( const PRO_REF by_reference iobj )
{
    /// @details BB@> push an object onto the free stack

    bool_t retval;

#if defined(USE_DEBUG)
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
size_t ProList_get_free( const PRO_REF by_reference override )
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
bool_t ProList_free_one( const PRO_REF by_reference iobj )
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
bool_t release_one_profile_textures( const PRO_REF by_reference iobj )
{
    int tnc;
    pro_t  * pobj;

    if ( !LOADED_PRO( iobj ) ) return bfalse;
    pobj = ProList.lst + iobj;

    for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
    {
        TX_REF itex;

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
bool_t release_one_pro_data( const PRO_REF by_reference iobj )
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
bool_t release_one_pro( const PRO_REF by_reference iobj )
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
int load_profile_skins( const char * tmploadname, const PRO_REF by_reference object )
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
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s" SLASH_STR "tris%d", tmploadname, cnt );

        pobj->tex_ref[cnt] = TxTexture_load_one( newloadname, ( TX_REF )INVALID_TX_TEXTURE, TRANSCOLOR );
        if ( INVALID_TX_TEXTURE != pobj->tex_ref[cnt] )
        {
            max_skin = cnt;
            if ( INVALID_TX_TEXTURE == min_skin_tx )
            {
                min_skin_tx = pobj->tex_ref[cnt];
            }
        }

        snprintf( newloadname, SDL_arraysize( newloadname ), "%s" SLASH_STR "icon%d", tmploadname, cnt );
        pobj->ico_ref[cnt] = TxTexture_load_one( newloadname, ( TX_REF )INVALID_TX_TEXTURE, INVALID_KEY );

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
void load_all_messages( const char *loadname, const PRO_REF by_reference object )
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
bool_t release_one_local_pips( const PRO_REF by_reference iobj )
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
    return ( 0 != vfs_exists( szLoadName ) );
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

    int islot;     // this has to be a signed value for this function to work properly

    PRO_REF iobj;
    pro_t * pobj;

    required = !VALID_CAP_RANGE( slot_override );

    // get a slot value
    islot = pro_get_slot( tmploadname, slot_override );

    // throw an error code if the slot is invalid of if the file doesn't exist
    if ( islot < 0 || islot > MAX_PROFILE )
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
            log_error( "load_one_profile() - object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, tmploadname );
        }
        else if ( required && overrideslots )
        {
            log_error( "load_one_profile() - object slot %i used twice (%s, %s)\n", REF_TO_INT( iobj ), pobj->name, tmploadname );
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
        log_warning( "load_one_profile() - Cannot allocate object %d (\"%s\")\n", REF_TO_INT( iobj ), tmploadname );
        return MAX_PROFILE;
    }

    // grab a pointer to the object
    pobj  = ProList.lst + iobj;

    // load the character profile
    pobj->icap = load_one_character_profile( tmploadname, islot, bfalse );
    islot = REF_TO_INT( pobj->icap );

    // Load the model for this iobj
    pobj->imad = load_one_model_profile( tmploadname, ( MAD_REF )islot );

    // Load the enchantment for this iobj
    make_newloadname( tmploadname, SLASH_STR "enchant.txt", newloadname );
    pobj->ieve = load_one_enchant_profile( newloadname, ( EVE_REF )islot );

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
        pobj->prtpip[cnt] = load_one_particle_profile( newloadname, ( PIP_REF )MAX_PIP );
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
    if ( CapStack.lst[pobj->icap].uniformlit )
    {
        mad_make_equally_lit( pobj->imad );
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
const char * pro_create_chop( const PRO_REF by_reference iprofile )
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
bool_t pro_load_chop( const PRO_REF by_reference iprofile, const char *szLoadname )
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
bool_t obj_BSP_alloc( obj_BSP_t * pbsp, int depth )
{
    BSP_tree_t * rv;

    if ( NULL == pbsp ) return bfalse;

    obj_BSP_free( pbsp );

    rv = BSP_tree_ctor( &( pbsp->tree ), 3, depth );

    // make a 3D BSP tree, depth copied from the mesh depth
    return ( NULL != rv );
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_free( obj_BSP_t * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    BSP_tree_free( &( pbsp->tree ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_ctor( obj_BSP_t * pbsp, mesh_BSP_t * pmesh_bsp )
{
    /// @details BB@> Create a new BSP tree for the mesh.
    //     These parameters duplicate the max resolution of the old system.

    int          cnt;
    float        bsp_size;
    BSP_tree_t * t;

    if ( NULL == pbsp || NULL == pmesh_bsp ) return bfalse;

    memset( pbsp, 0, sizeof( *pbsp ) );

    // allocate the data
    obj_BSP_alloc( pbsp, pmesh_bsp->tree.depth );

    t = &( pbsp->tree );

    // copy the volume from the mesh
    t->bbox.mins.ary[0] = pmesh_bsp->volume.mins[OCT_X];
    t->bbox.mins.ary[1] = pmesh_bsp->volume.mins[OCT_Y];

    t->bbox.maxs.ary[0] = pmesh_bsp->volume.maxs[OCT_X];
    t->bbox.maxs.ary[1] = pmesh_bsp->volume.maxs[OCT_Y];

    // make some extra space in the z direction
    bsp_size = MAX( ABS( t->bbox.mins.ary[OCT_X] ), ABS( t->bbox.maxs.ary[OCT_X] ) );
    bsp_size = MAX( bsp_size, MAX( ABS( t->bbox.mins.ary[OCT_Y] ), ABS( t->bbox.maxs.ary[OCT_Y] ) ) );
    bsp_size = MAX( bsp_size, MAX( ABS( t->bbox.mins.ary[OCT_Z] ), ABS( t->bbox.maxs.ary[OCT_Z] ) ) );

    t->bbox.mins.ary[2] = -bsp_size * 2;
    t->bbox.maxs.ary[2] =  bsp_size * 2;

    // calculate the mid positions
    for ( cnt = 0; cnt < 3; cnt++ )
    {
        t->bbox.mids.ary[cnt] = 0.5f * ( t->bbox.mins.ary[cnt] + t->bbox.maxs.ary[cnt] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_dtor( obj_BSP_t * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    // deallocate everything
    obj_BSP_free( pbsp );

    // run the destructors on all of the sub-objects
    BSP_tree_dtor( &( pbsp->tree ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_insert_chr( obj_BSP_t * pbsp, chr_t * pchr )
{
    /// @details BB@> insert a character's BSP_leaf_t into the BSP_tree_t

    bool_t       retval;
    BSP_leaf_t * pleaf;
    BSP_tree_t * ptree;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    if ( NULL == pbsp ) return bfalse;
    ptree = &( pbsp->tree );

    pleaf = &( pchr->bsp_leaf );
    if ( pchr != ( chr_t * )( pleaf->data ) )
    {
        // some kind of error. re-initialize the data.
        pleaf->data      = pchr;
        pleaf->index     = GET_INDEX_PCHR( pchr );
        pleaf->data_type = 1;
    };

    retval = bfalse;
    if ( !oct_bb_empty( pchr->chr_prt_cv ) )
    {
        oct_bb_t tmp_oct;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_chr_bb( pchr, 0.0f, 1.0f, &tmp_oct );

        // convert the bounding box
        BSP_aabb_from_oct_bb( &( pleaf->bbox ), &tmp_oct );

        // insert the leaf
        retval = BSP_tree_insert_leaf( ptree, pleaf );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_insert_prt( obj_BSP_t * pbsp, prt_t * pprt )
{
    /// @details BB@> insert a particle's BSP_leaf_t into the BSP_tree_t

    bool_t       retval;
    BSP_leaf_t * pleaf;
    BSP_tree_t * ptree;
    pro_t      * ppro;
    pip_t      * ppip;

    bool_t       has_enchant, does_damage, does_grog_daze, needs_bump, does_special_effect;

    if ( NULL == pbsp ) return bfalse;
    ptree = &( pbsp->tree );

    if ( !ACTIVE_PPRT( pprt ) || pprt->is_hidden ) return bfalse;

    if ( !LOADED_PRO( pprt->profile_ref ) ) return bfalse;
    ppro = ProList.lst + pprt->profile_ref;

    has_enchant = LOADED_EVE( ppro->ieve );
    does_damage = ( ABS( pprt->damage.base ) + ABS( pprt->damage.rand ) ) > 0;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack.lst + pprt->pip_ref;

    does_grog_daze = ( 0 != ppip->grogtime ) || ( 0 != ppip->dazetime );
    needs_bump     = ppip->endbump || ppip->endground || ( ppip->bumpspawn_amount > 0 ) || ( ppip->bumpmoney > 0 );

    does_special_effect = ppip->causepancake;

    if ( 0 == pprt->bump.size && !has_enchant && !does_damage && !does_grog_daze && !needs_bump && !does_special_effect )
        return bfalse;

    pleaf = &( pprt->bsp_leaf );
    if ( pprt != ( prt_t * )( pleaf->data ) )
    {
        // some kind of error. re-initialize the data.
        pleaf->data      = pprt;
        pleaf->index     = GET_INDEX_PPRT( pprt );
        pleaf->data_type = 1;
    };

    retval = bfalse;
    if ( ACTIVE_PPRT( pprt ) )
    {
        oct_bb_t tmp_oct;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_prt_bb( pprt, 0.0f, 1.0f, &tmp_oct );

        // convert the bounding box
        BSP_aabb_from_oct_bb( &( pleaf->bbox ), &tmp_oct );

        retval = BSP_tree_insert_leaf( ptree, pleaf );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_empty( obj_BSP_t * pbsp )
{
    CHR_REF ichr;
    PRT_REF iprt;

    if ( NULL == pbsp ) return bfalse;

    // unlink all the BSP nodes
    BSP_tree_clear_nodes( &( pbsp->tree ), btrue );

    // unlink all used character nodes
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        ChrList.lst[ichr].bsp_leaf.next = NULL;
    }

    // unlink all used particle nodes
    BSP_prt_count = 0;
    for ( iprt = 0; iprt < TOTAL_MAX_PRT; iprt++ )
    {
        PrtList.lst[iprt].bsp_leaf.next = NULL;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_fill( obj_BSP_t * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    // insert the characters
    BSP_chr_count = 0;
    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        // reset a couple of things here
        pchr->holdingweight     = 0;
        pchr->onwhichplatform   = ( CHR_REF )MAX_CHR;

        // try to insert the character
        if ( obj_BSP_insert_chr( pbsp, pchr ) )
        {
            BSP_chr_count++;
        }
    }
    CHR_END_LOOP()

    // insert the particles
    BSP_prt_count = 0;
    PRT_BEGIN_LOOP_DISPLAY( iprt, pprt )
    {
        // reset a couple of things here
        pprt->onwhichplatform   = ( CHR_REF )MAX_CHR;

        // try to insert the particle
        if ( obj_BSP_insert_prt( pbsp, pprt ) )
        {
            BSP_prt_count++;
        }
    }
    PRT_END_LOOP()

    return btrue;
}

//--------------------------------------------------------------------------------------------
int obj_BSP_collide( obj_BSP_t * pbsp, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst )
{
    /// @details BB@> fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    if ( NULL == pbsp || NULL == paabb || NULL == colst ) return 0;

    // infinite nodes
    return BSP_tree_collide( &( pbsp->tree ), paabb, colst );
}

////--------------------------------------------------------------------------------------------
//bool_t obj_BSP_insert_leaf( obj_BSP_t * pbsp, BSP_leaf_t * pleaf, int depth, int address_x[], int address_y[], int address_z[] )
//{
//    int i;
//    bool_t retval;
//    Uint32 index;
//    BSP_branch_t * pbranch, * pbranch_new;
//    BSP_tree_t * ptree = &( pbsp->tree );
//
//    retval = bfalse;
//    if ( depth < 0 )
//    {
//        // this can only happen if the node does not intersect the BSP bounding box
//        pleaf->next = ptree->infinite;
//        ptree->infinite = pleaf;
//        retval = btrue;
//    }
//    else if ( 0 == depth )
//    {
//        // this can only happen if the object should be in the root node list
//        pleaf->next = ptree->root->nodes;
//        ptree->root->nodes = pleaf;
//        retval = btrue;
//    }
//    else
//    {
//        // insert the node into the tree at this point
//        pbranch = ptree->root;
//        for ( i = 0; i < depth; i++ )
//        {
//            index = (( Uint32 )address_x[i] ) | ((( Uint32 )address_y[i] ) << 1 ) | ((( Uint32 )address_z[i] ) << 2 ) ;
//
//            pbranch_new = BSP_tree_ensure_branch( ptree, pbranch, index );
//            if ( NULL == pbranch_new ) break;
//
//            pbranch = pbranch_new;
//        };
//
//        // insert the node in this branch
//        retval = BSP_tree_insert( ptree, pbranch, pleaf, -1 );
//    }
//
//    return retval;
//}
//

////--------------------------------------------------------------------------------------------
//bool_t obj_BSP_collide_branch( BSP_branch_t * pbranch, oct_bb_t * pvbranch, oct_bb_t * pvobj, int_ary_t * colst )
//{
//    /// @details BB@> Recursively search the BSP tree for collisions with the pvobj
//    //      Return bfalse if we need to break out of the recursive search for any reson.
//
//    Uint32 i;
//    oct_bb_t    int_ov, tmp_ov;
//    float x_mid, y_mid, z_mid;
//    int address_x, address_y, address_z;
//
//    if ( NULL == colst ) return bfalse;
//    if ( NULL == pvbranch || oct_bb_empty( *pvbranch ) ) return bfalse;
//    if ( NULL == pvobj  || oct_bb_empty( *pvobj ) ) return bfalse;
//
//    // return if the object does not intersect the branch
//    if ( !oct_bb_intersection( *pvobj, *pvbranch, &int_ov ) )
//    {
//        return bfalse;
//    }
//
//    if ( !obj_BSP_collide_nodes( pbranch->nodes, pvobj, colst ) )
//    {
//        return bfalse;
//    };
//
//    // check for collisions with any of the children
//    x_mid = ( pvbranch->maxs[OCT_X] + pvbranch->mins[OCT_X] ) * 0.5f;
//    y_mid = ( pvbranch->maxs[OCT_Y] + pvbranch->mins[OCT_Y] ) * 0.5f;
//    z_mid = ( pvbranch->maxs[OCT_Z] + pvbranch->mins[OCT_Z] ) * 0.5f;
//    for ( i = 0; i < pbranch->child_count; i++ )
//    {
//        // scan all the children
//        if ( NULL == pbranch->children[i] ) continue;
//
//        // create the volume of this node
//        address_x = i & ( 1 << 0 );
//        address_y = i & ( 1 << 1 );
//        address_z = i & ( 1 << 2 );
//
//        tmp_ov = *( pvbranch );
//
//        if ( 0 == address_x )
//        {
//            tmp_ov.maxs[OCT_X] = x_mid;
//        }
//        else
//        {
//            tmp_ov.mins[OCT_X] = x_mid;
//        }
//
//        if ( 0 == address_y )
//        {
//            tmp_ov.maxs[OCT_Y] = y_mid;
//        }
//        else
//        {
//            tmp_ov.mins[OCT_X] = y_mid;
//        }
//
//        if ( 0 == address_z )
//        {
//            tmp_ov.maxs[OCT_Z] = z_mid;
//        }
//        else
//        {
//            tmp_ov.mins[OCT_Z] = z_mid;
//        }
//
//        if ( oct_bb_intersection( *pvobj, tmp_ov, &int_ov ) )
//        {
//            // potential interaction with the child. go recursive!
//            bool_t ret = obj_BSP_collide_branch( pbranch->children[i], &( tmp_ov ), pvobj, colst );
//            if ( !ret ) return ret;
//        }
//    }
//
//    return btrue;
//}
//

////--------------------------------------------------------------------------------------------
//bool_t obj_BSP_collide_nodes( BSP_leaf_t leaf_lst[], oct_bb_t * pvobj, int_ary_t * colst )
//{
//    /// @details BB@> check for collisions with the given node list
//
//    BSP_leaf_t * pleaf;
//    oct_bb_t    int_ov, * pnodevol;
//
//    if ( NULL == leaf_lst || NULL == pvobj ) return bfalse;
//
//    if ( 0 == int_ary_get_size( colst ) || int_ary_get_top( colst ) >= int_ary_get_size( colst ) ) return bfalse;
//
//    // check for collisions with any of the nodes of this branch
//    for ( pleaf = leaf_lst; NULL != pleaf; pleaf = pleaf->next )
//    {
//        if ( NULL == pleaf ) EGOBOO_ASSERT( bfalse );
//
//        // get the volume of the node
//        pnodevol = NULL;
//        if ( 1 == pleaf->data_type )
//        {
//            chr_t * pchr = ( chr_t* )pleaf->data;
//            pnodevol = &( pchr->chr_prt_cv );
//        }
//        else if ( 2 == pleaf->data_type )
//        {
//            prt_t * pprt = ( prt_t* )pleaf->data;
//            pnodevol = &( pprt->chr_prt_cv );
//        }
//        else
//        {
//            continue;
//        }
//
//        if ( oct_bb_intersection( *pvobj, *pnodevol, &int_ov ) )
//        {
//            // we have a possible intersection
//            int_ary_push_back( colst, pleaf->index *(( 1 == pleaf->data_type ) ? 1 : -1 ) );
//
//            if ( int_ary_get_top( colst ) >= int_ary_get_size( colst ) )
//            {
//                // too many nodes. break out of the search.
//                return bfalse;
//            };
//        }
