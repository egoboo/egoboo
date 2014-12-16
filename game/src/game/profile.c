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

/// @file game/profile.c
/// @brief Implementation of functions for controlling and accessing object profiles
/// @details

#include "game/profile.inl"

#include "egolib/log.h"
#include "egolib/egoboo_setup.h"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/vfs.h"

#include "game/graphic_texture.h"
#include "game/renderer_2d.h"
#include "game/script_compile.h"
#include "game/game.h"

#include "egolib/bsp.inl"
#include "game/ChrList.inl"
#include "game/PrtList.inl"
#include "game/mesh.inl"
#include "game/particle.inl"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static ego_bool _profile_system_initialized = ego_false;

chop_data_t  chop_mem = {0, 0};
pro_import_t import_data;

size_t bookicon_count   = 0;
TX_REF bookicon_ref[MAX_SKIN];                      // The first book icon

INSTANTIATE_LIST( ACCESS_TYPE_NONE, pro_t, ProList, MAX_PROFILE );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void profile_load_all_messages_vfs( const char *loadname, pro_t *pobject );

static ego_bool obj_verify_file_vfs( const char * tmploadname );
static int obj_read_slot_vfs( const char * tmploadname );

static ego_bool release_one_pro_data( const PRO_REF iobj );
static ego_bool release_one_profile_textures( const PRO_REF iobj );
static ego_bool pro_init( pro_t * pobj );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( pro_t, ProList, MAX_PROFILE );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_profiles( void )
{
    /// @author ZZ
    /// @details This function initializes all of the model profiles

    int tnc;

    // initialize all the sub-profile lists
    PipStack_init_all();
    EveStack_init_all();
    CapStack_init_all();
    MadStack_reconstruct_all();

    // initialize the profile list
    ProList_init();

    // fix the book icon list
    for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
    {
        bookicon_ref[tnc] = INVALID_TX_REF;
    }
    bookicon_count = 0;
}

//--------------------------------------------------------------------------------------------
void release_all_profiles( void )
{
    /// @author ZZ
    /// @details This function clears out all of the model data

    // release the allocated data in all profiles (sounds, textures, etc.)
    release_all_pro_data();

    // relese every type of sub-profile and re-initalize the lists
    PipStack_release_all();
    EveStack_release_all();
    CapStack_release_all();
    MadStack_release_all();

    // re-initialize the profile list
    ProList_init();
}

//--------------------------------------------------------------------------------------------
void profile_system_begin( void )
{
    /// @author BB
    /// @details initialize the profile list and load up some intialization files
    ///     necessary for the the profile loading code to work

    if ( _profile_system_initialized )
    {
        // release all profile data and reinitialize the profile list
        release_all_profiles();

        // initialize the models
        model_system_end();

        _profile_system_initialized = ego_false;
    }

    // initialize all the profile lists
    init_all_profiles();

    // initialize the models
    model_system_begin();

    // initialize the script compiler
    script_compiler_init();

    // necessary for loading up the copy.txt file
    load_action_names_vfs( "mp_data/actions.txt" );

    // necessary for properly reading the "message.txt"
    DisplayMsg_reset();

    // necessary for reading "naming.txt" properly
    chop_data_init( &chop_mem );

    // something that is used in the game that is somewhat related to the profile stuff
    init_slot_idsz();

    // let the code know that everything is initialized
    _profile_system_initialized = ego_true;
}

//--------------------------------------------------------------------------------------------
void profile_system_end( void )
{
    /// @author BB
    /// @details initialize the profile list and load up some intialization files
    ///     necessary for the the profile loading code to work

    if ( _profile_system_initialized )
    {
        // release all profile data and reinitialize the profile list
        release_all_profiles();

        // initialize the models
        model_system_end();

        _profile_system_initialized = ego_false;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_bool pro_init( pro_t * pobj )
{
    int cnt;

    if ( NULL == pobj ) return ego_false;

    if ( pobj->loaded )
    {
        log_warning( "pro_init() - trying to init an object in use" );
    }

    //Free any dynamically allocated memory
    if ( NULL != pobj->message_ary ) free( pobj->message_ary );

    //---- reset everything to safe values
    BLANK_STRUCT_PTR( pobj )

    pobj->icap = INVALID_CAP_REF;
    pobj->imad = INVALID_MAD_REF;
    pobj->ieve = INVALID_EVE_REF;

    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        pobj->prtpip[cnt] = INVALID_PIP_REF;
    }

    chop_definition_init( &( pobj->chop ) );

    // do the final invalidation
    pobj->loaded   = C_FALSE;
    strncpy( pobj->name, "*NONE*", SDL_arraysize( pobj->name ) );

    // clear out the textures
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        pobj->tex_ref[cnt] = INVALID_TX_REF;
        pobj->ico_ref[cnt] = INVALID_TX_REF;
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
// The "private" ProList management functions
//--------------------------------------------------------------------------------------------
int ProList_find_free_ref( const PRO_REF iobj )
{
    /// @author BB
    /// @details if an object of index iobj exists on the free list, return the free list index
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
size_t ProList_pop_free( const int index )
{
    /// @author BB
    /// @details pop off whatever object exists at the free list index idx

    size_t retval = INVALID_PRO_REF;
    size_t loops  = 0;

    if ( index >= 0 && index < ProList.free_count )
    {
        // the user has specified a valid index in the free stack
        // that they want to use. make that happen.

        // from the conditions, ProList.free_count must be greater than 1
        size_t itop = ProList.free_count - 1;

        // move the desired index to the top of the stack
        SWAP( size_t, ProList.free_ref[index], ProList.free_ref[itop] );
    }

    // do the normal "pop" operation
    while ( ProList.free_count > 0 )
    {
        ProList.free_count--;
        ProList.update_guid++;

        retval = ProList.free_ref[ProList.free_count];

        // completely remove it from the free list
        ProList.free_ref[ProList.free_count] = INVALID_PRO_IDX;

        if ( VALID_PRO_RANGE( retval ) )
        {
            break;
        }

        loops++;
    }

    if ( loops > 0 )
    {
        log_warning( "%s - there is something wrong with the free stack. %d loops.\n", __FUNCTION__, loops );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_bool ProList_push_free( const PRO_REF iobj )
{
    /// @author BB
    /// @details push an object onto the free stack

    ego_bool retval;

#if defined(_DEBUG)
    // determine whether this character is already in the list of free objects
    // that is an error
    if ( -1 != ProList_find_free_ref( iobj ) ) return ego_false;
#endif

    // push it on the free stack
    retval = ego_false;
    if ( ProList.free_count < MAX_PROFILE )
    {
        ProList.free_ref[ProList.free_count] = REF_TO_INT( iobj );

        ProList.free_count++;
        ProList.update_guid++;

        retval = ego_true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int ProList_find_used_ref( const PRO_REF iobj )
{
    /// @author BB
    /// @details if an object of index iobj exists on the used list, return the used list index
    ///     otherwise return -1

    int cnt, retval;

    // determine whether this character is already in the list of used textures
    // that is an error
    retval = -1;
    for ( cnt = 0; cnt < ProList.used_count; cnt++ )
    {
        if ( iobj == ProList.used_ref[cnt] )
        {
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t ProList_pop_used( const int index )
{
    /// @author BB
    /// @details pop off whatever object exists at the used list index idx

    size_t retval = INVALID_PRO_REF;
    size_t loops  = 0;

    if ( index >= 0 && index < ProList.used_count )
    {
        // the user has specified a valid index in the used stack
        // that they want to use. make that happen.

        // from the conditions, ProList.used_count must be greater than 1
        size_t itop = ProList.used_count - 1;

        // move the desired index to the top of the stack
        SWAP( size_t, ProList.used_ref[index], ProList.used_ref[itop] );
    }

    // do the normal "pop" operation
    while ( ProList.used_count > 0 )
    {
        ProList.used_count--;
        ProList.update_guid++;

        retval = ProList.used_ref[ProList.used_count];

        // completely remove it from the used list
        ProList.used_ref[ProList.used_count] = INVALID_PRO_REF;

        if ( VALID_PRO_RANGE( retval ) )
        {
            break;
        }

        loops++;
    }

    if ( loops > 0 )
    {
        log_warning( "%s - there is something wrong with the used stack. %d loops.\n", __FUNCTION__, loops );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_bool ProList_push_used( const PRO_REF iobj )
{
    /// @author BB
    /// @details push an object onto the used stack

    ego_bool retval;

#if defined(_DEBUG)
    // determine whether this character is already in the list of used objects
    // that is an error
    if ( -1 != ProList_find_used_ref( iobj ) ) return ego_false;
#endif

    // push it on the used stack
    retval = ego_false;
    if ( ProList.used_count < MAX_PROFILE )
    {
        ProList.used_ref[ProList.used_count] = REF_TO_INT( iobj );

        ProList.used_count++;
        ProList.update_guid++;

        retval = ego_true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// The "public" ProList management functions
//--------------------------------------------------------------------------------------------
void ProList_init( void )
{
    /// @author BB
    /// @details initialize all the objects and the object free list.
    ///     call before ever using the object list.

    PRO_REF cnt;
    pro_t * pobj;

    ProList.free_count = 0;
    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        pobj = ProList.lst + cnt;

        // make sure we don't get a stupid warning
        pobj->loaded = C_FALSE;

        pro_init( pobj );

        ProList_push_free( cnt );
    }
}

//--------------------------------------------------------------------------------------------
size_t ProList_get_free_ref( const PRO_REF override )
{
    /// @author ZZ
    /// @details This function returns the next free character or INVALID_PRO_REF if there are none

    size_t retval = INVALID_PRO_REF;

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
        free_idx = ProList_find_free_ref( override );
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
ego_bool ProList_free_one( const PRO_REF iobj )
{
    /// @author ZZ
    /// @details This function sticks an object back on the free object stack

    if ( !VALID_PRO_RANGE( iobj ) ) return ego_false;

    // object "destructor"
    // inilializes an object to safe values
    pro_init( ProList.lst + iobj );

    return ProList_push_free( iobj );
}

//--------------------------------------------------------------------------------------------
// object functions
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_bool release_one_profile_textures( const PRO_REF iobj )
{
    int tnc;
    pro_t  * pobj;

    if ( !LOADED_PRO( iobj ) ) return ego_false;
    pobj = ProList.lst + iobj;

    for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
    {
        TX_REF itex;

        itex = pobj->tex_ref[tnc];
        if ( itex > TX_SPECIAL_LAST )
        {
            TxList_free_one( itex );
        }
        pobj->tex_ref[tnc] = INVALID_TX_REF;

        itex = pobj->ico_ref[tnc] ;
        if ( itex > TX_SPECIAL_LAST )
        {
            TxList_free_one( itex );
        }
        pobj->ico_ref[tnc] = INVALID_TX_REF;
    }

    // reset the bookicon stuff if this object is a book
    if ( SPELLBOOK == iobj )
    {
        for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
        {
            bookicon_ref[tnc] = INVALID_TX_REF;
        }
        bookicon_count = 0;
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
void release_all_profile_textures( void )
{
    PRO_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_profile_textures( cnt );
    }
}

//--------------------------------------------------------------------------------------------
ego_bool release_one_pro_data( const PRO_REF iobj )
{
    int cnt;
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return ego_false;
    pobj = ProList.lst + iobj;

    // free all sounds
    for ( cnt = 0; cnt < MAX_WAVE; cnt++ )
    {
        sound_free_chunk( pobj->wavelist[cnt] );
        pobj->wavelist[cnt] = NULL;
    }

    // release whatever textures are being used
    release_one_profile_textures( iobj );
    
    // release messages
    if ( NULL != pobj->message_ary )
    {
        free( pobj->message_ary );
        pobj->message_ary = NULL;
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
ego_bool release_one_pro( const PRO_REF iobj )
{
    pro_t * pobj;

    if ( !VALID_PRO_RANGE( iobj ) ) return ego_false;

    if ( !LOADED_PRO_RAW( iobj ) ) return ego_true;
    pobj = ProList.lst + iobj;

    // release all of the sub-profiles
    CapStack_release_one( pobj->icap );
    MadStack_release_one( pobj->imad );
    //EveStack_release_one( pobj->ieve );

    release_one_local_pips( iobj );

    // release the allocated data
    release_one_pro_data( iobj );

    pro_init( pobj );

    return ego_true;
}

//--------------------------------------------------------------------------------------------
void release_all_pro( void )
{
    /// @author BB
    /// @details release the allocated data for all objects

    PRO_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_pro( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_pro_data( void )
{
    /// @author BB
    /// @details release the allocated data for all objects

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
    TX_REF min_skin_tx, min_icon_tx, tmp_tx;
    int    max_skin, max_icon, max_tex;
    TX_REF iskin, iicon;
    int    cnt;

    STRING newloadname;

    pro_t * pobj;

    if ( !VALID_PRO_RANGE( object ) ) return 0;
    pobj = ProList.lst + object;

    // blank out any existing values
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        pobj->tex_ref[cnt] = INVALID_TX_REF;
        pobj->ico_ref[cnt] = INVALID_TX_REF;
    }

    // Load the skins and icons
    max_skin    = max_icon    = -1;
    min_skin_tx = min_icon_tx = INVALID_TX_REF;
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        // do the texture
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/tris%d", tmploadname, cnt );

        tmp_tx = TxList_load_one_vfs( newloadname, INVALID_TX_REF, TRANSCOLOR );
        if ( VALID_TX_RANGE( tmp_tx ) )
        {
            pobj->tex_ref[cnt] = tmp_tx;
            max_skin = cnt;

            if ( !VALID_TX_RANGE( min_skin_tx ) )
            {
                min_skin_tx = tmp_tx;
            }
        }

        // do the icon
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/icon%d", tmploadname, cnt );

        tmp_tx = TxList_load_one_vfs( newloadname, INVALID_TX_REF, INVALID_KEY );
        if ( VALID_TX_RANGE( tmp_tx ) )
        {
            pobj->ico_ref[cnt] = tmp_tx;
            max_icon = cnt;

            if ( !VALID_TX_RANGE( min_icon_tx ) )
            {
                min_icon_tx = tmp_tx;
            }

            if ( SPELLBOOK == object )
            {
                if ( bookicon_count < MAX_SKIN )
                {
                    bookicon_ref[bookicon_count] = tmp_tx;
                    bookicon_count++;
                }
            }
        }
    }

    // If we didn't get a skin, set it to the water texture
    if ( max_skin < 0 )
    {
        max_skin = 0;
        pobj->tex_ref[max_skin] = TX_WATER_TOP;
        log_debug( "Object is missing a skin (%s)!\n", tmploadname );
    }

    // If we didn't get a icon, set it to the NULL icon
    if ( max_icon < 0 )
    {
        max_icon = 0;
        pobj->tex_ref[max_icon] = TX_ICON_NULL;
        log_debug( "Object is missing an icon (%s)!\n", tmploadname );
    }

    max_tex = MAX( max_skin, max_icon );

    // fill in all missing skin graphics up to MAX_SKIN
    iskin = min_skin_tx;
    iicon = min_icon_tx;
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        if ( VALID_TX_RANGE( pobj->tex_ref[cnt] ) )
        {
            iskin = pobj->tex_ref[cnt];
        }

        if ( VALID_TX_RANGE( pobj->ico_ref[cnt] ) )
        {
            iicon = pobj->ico_ref[cnt];
        }

        pobj->tex_ref[cnt] = iskin;
        pobj->ico_ref[cnt] = iicon;
    }

    return max_tex + 1;
}

//--------------------------------------------------------------------------------------------
void profile_add_one_message( pro_t *pobject, const ego_message_t add_message )
{
    /// @author ZF
    /// @details This adds one string to the list of messages associated with a profile. The function will
    //              dynamically allocate more memory if there are more messages than array size

    size_t cnt, length;

    if ( NULL == pobject ) return;

    //Is this the first message that is added? Then we need to allocate an dynamic array!
    if ( NULL == pobject->message_ary )
    {
        pobject->message_count = 0;
        pobject->message_length = 10;
        pobject->message_ary = ( ego_message_t * ) malloc( pobject->message_length * sizeof( ego_message_t ) );
    }

    //Do we need to increase the size of the array?
    if ( pobject->message_count + 1 >= pobject->message_length )
    {
        pobject->message_length += 10;
        pobject->message_ary = ( ego_message_t* ) realloc( pobject->message_ary, pobject->message_length * sizeof( ego_message_t ) );
    }

    length = strlen( add_message );
    //if( length >= EGO_MESSAGE_SIZE ) log_warning("Trying to add message for %s - message is too long: \"%s\", length is %d while max is %d\n", pobject->name, add_message, length, EGO_MESSAGE_SIZE);

    //replace underscore with whitespace
    for ( cnt = 0; cnt < length; cnt++ )
    {
        pobject->message_ary[pobject->message_count][cnt] = ( add_message[cnt] == '_' ) ? ' ' : add_message[cnt];
    }

    //Make sure it is null terminated
    pobject->message_ary[pobject->message_count][length] = CSTR_END;

    //Keep track of number of messages in array
    pobject->message_count++;
}

//--------------------------------------------------------------------------------------------
void profile_load_all_messages_vfs( const char *loadname, pro_t *pobject )
{
    /// @author ZF
    /// @details This function loads all messages for an object

    vfs_FILE *fileread;

    fileread = vfs_openRead( loadname );
    if ( fileread )
    {
        STRING line;

        while ( goto_colon_vfs( NULL, fileread, C_TRUE ) )
        {
            //Load one line
            vfs_get_string( fileread, line, SDL_arraysize( line ) );
            profile_add_one_message( pobject, line );
        }

        vfs_close( fileread );
    }
}

//--------------------------------------------------------------------------------------------
ego_bool release_one_local_pips( const PRO_REF iobj )
{
    int cnt;
    pro_t * pobj;

    if ( !VALID_PRO_RANGE( iobj ) ) return ego_false;

    if ( !LOADED_PRO_RAW( iobj ) ) return ego_true;
    pobj = ProList.lst + iobj;

    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        PipStack_release_one( pobj->prtpip[cnt] );
        pobj->prtpip[cnt] = INVALID_PIP_REF;
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
void release_all_local_pips( void )
{
    // clear out the local pips

    PRO_REF object;
    pro_t * pobj;

    int cnt;

    for ( object = 0; object < MAX_PROFILE; object++ )
    {
        pobj = ProList.lst + object;

        if ( !pobj->loaded ) continue;

        for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
        {
            PipStack_release_one( pobj->prtpip[cnt] );
            pobj->prtpip[cnt] = INVALID_PIP_REF;
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
    slot = vfs_get_next_int( fileread );

    vfs_close( fileread );

    return slot;
}

//--------------------------------------------------------------------------------------------
ego_bool obj_verify_file_vfs( const char * tmploadname )
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
    /// @author ZZ
    /// @details This function loads one object and returns the object slot

    int cnt;
    STRING newloadname;
    ego_bool required;

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
        else if ( VALID_CAP_RANGE( slot_override ) && slot_override > PMod->importamount * MAX_IMPORT_PER_PLAYER )
        {
            log_debug( "load_one_profile_vfs() - Not able to open file \"%s\"\n", tmploadname );
        }

        return MAX_PROFILE;
    }

    // convert the slot to a profile reference
    iobj = ( PRO_REF )islot;

    // throw an error code if we are trying to load over an existing profile
    // without permission
    if ( LOADED_PRO( iobj ) )
    {
        pro_t * pobj_tmp = ProList.lst + iobj;

        // Make sure global objects don't load over existing models
        if ( required && SPELLBOOK == iobj )
        {
            log_error( "load_one_profile_vfs() - object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, tmploadname );
        }
        else if ( required && overrideslots )
        {
            log_error( "load_one_profile_vfs() - object slot %i used twice (%s, %s)\n", REF_TO_INT( iobj ), pobj_tmp->name, tmploadname );
        }
        else
        {
            // Stop, we don't want to override it
            return MAX_PROFILE;
        }
    }

    // allocate/reallocate this slot
    iobj = ( PRO_REF )ProList_get_free_ref( iobj );
    if ( !VALID_PRO_RANGE( iobj ) )
    {
        log_warning( "load_one_profile_vfs() - Cannot allocate object %d (\"%s\")\n", REF_TO_INT( iobj ), tmploadname );
        return MAX_PROFILE;
    }

    // grab a pointer to the object
    pobj  = ProList.lst + iobj;

    // load the character profile
    pobj->icap = CapStack_load_one( tmploadname, islot, ego_false );
    islot = REF_TO_INT( pobj->icap );

    // Load the model for this iobj
    pobj->imad = load_one_model_profile_vfs( tmploadname, ( MAD_REF )islot );

    // Load the enchantment for this iobj
    make_newloadname( tmploadname, "/enchant.txt", newloadname );
    pobj->ieve = EveStack_losd_one( newloadname, ( EVE_REF )islot );

    // Load the messages for this iobj, do this before loading the AI script
    make_newloadname( tmploadname, "/message.txt", newloadname );
    profile_load_all_messages_vfs( newloadname, pobj );

    // Load the particles for this iobj
    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/part%d.txt", tmploadname, cnt );

        // Make sure it's referenced properly
        pobj->prtpip[cnt] = PipStack_load_one( newloadname, INVALID_PIP_REF );
    }

    pobj->skin_gfx_cnt = load_profile_skins_vfs( tmploadname, iobj );

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
    pobj->loaded = C_TRUE;

    return islot;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
const char * pro_create_chop( const PRO_REF iprofile )
{
    /// @author BB
    /// @details use the profile's chop to generate a name. Return "*NONE*" on a falure.

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
    pcap = CapStack_get_ptr( ppro->icap );

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
ego_bool pro_load_chop_vfs( const PRO_REF iprofile, const char *szLoadname )
{
    /// @author BB
    /// @details load the chop for the given profile

    pro_t * ppro;

    if ( !VALID_PRO_RANGE( iprofile ) ) return ego_false;
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
    /// @author ZZ
    /// @details This function prepares the name chopper for use
    /// @note  BB@> It may actually be useful to blank the chop buffer

    if ( NULL == pdata ) return pdata;

    pdata->chop_count = 0;
    pdata->carat      = 0;

    return pdata;
}

//--------------------------------------------------------------------------------------------
const char * chop_create( chop_data_t * pdata, chop_definition_t * pdefinition )
{
    /// @author ZZ
    /// @details This function generates a random name.  Return "Blah" on a falure.

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
ego_bool chop_load_vfs( chop_data_t * pdata, const char *szLoadname, chop_definition_t * pdefinition )
{
    /// @author ZZ
    /// @details This function reads a naming.txt file into the chop data buffer and sets the
    ///               values of a chop definition

    int       which_section, section_count;
    STRING    tmp_buffer = EMPTY_CSTR;
    vfs_FILE *fileread;

    chop_definition_t local_definition;

    if ( NULL == pdata || pdata->carat >= CHOPDATACHUNK ) return ego_false;

    fileread = vfs_openRead( szLoadname );
    if ( NULL == fileread ) return ego_false;

    // in case we get a stupid value.
    // we could create a dynamically allocated struct in this case...
    if ( NULL == pdefinition ) pdefinition = &local_definition;

    // clear out any old definition
    chop_definition_init( pdefinition );

    which_section = 0;
    section_count = 0;
    while ( which_section < MAXSECTION && pdata->carat < CHOPDATACHUNK && goto_colon_vfs( NULL, fileread, C_TRUE ) )
    {
        vfs_get_string( fileread, tmp_buffer, SDL_arraysize( tmp_buffer ) );

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
ego_bool chop_export_vfs( const char *szSaveName, const char * szChop )
{
    /// @author ZZ
    /// @details This function exports a simple string to the naming.txt file

    vfs_FILE* filewrite;
    char cTmp;
    int cnt, tnc;

    if ( !VALID_CSTR( szChop ) ) return ego_false;

    // Can it export?
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return ego_false;

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

    return ego_true;
}
