#include "spawn_file.h"
#include "char.h"
#include "game.h"

#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
spawn_file_info_t * spawn_file_info_init( spawn_file_info_t *pinfo )
{
    // BB> safe values for all parameters

    if( NULL == pinfo ) return pinfo;

    memset( pinfo, 0, sizeof(spawn_file_info_t) );

    pinfo->attach = ATTACH_NONE;
    pinfo->team   = TEAM_NULL;

    return pinfo;
}

//--------------------------------------------------------------------------------------------
spawn_file_info_t * spawn_file_info_reinit( spawn_file_info_t *pinfo )
{
    Uint16 old_parent;

    if( NULL == pinfo ) return pinfo;

    // save the parent data just in case
    old_parent = pinfo->parent;

    // init the data
    spawn_file_info_init( pinfo );

    // restore the parent data
    pinfo->parent = old_parent;

    return pinfo;
}

//--------------------------------------------------------------------------------------------
bool_t spawn_file_scan( vfs_FILE * fileread, spawn_file_info_t *pinfo )
{
    char cTmp, delim;
    bool_t retval;

    // trap bad pointers
    if ( NULL == fileread || NULL == pinfo ) return bfalse;

    spawn_file_info_reinit( pinfo );

    // check for another entry, either the "#" or ":" delimiters
    delim = goto_delimiter_list( pinfo->spawn_coment, fileread, "#:", btrue );
    if ( CSTR_END == delim ) return bfalse;

    retval = bfalse;
    if( ':' == delim )
    {
        retval = btrue;

        pinfo->do_spawn = btrue;

        fget_string( fileread, pinfo->spawn_name, SDL_arraysize(pinfo->spawn_name) );
        str_decode( pinfo->spawn_name, SDL_arraysize(pinfo->spawn_name), pinfo->spawn_name );

        pinfo->pname = pinfo->spawn_name;
        if ( 0 == strcmp( pinfo->spawn_name, "NONE") )
        {
            // Random pinfo->pname
            pinfo->pname = NULL;
        }

        pinfo->slot = fget_int( fileread );

        pinfo->pos.x = fget_float( fileread ) * TILE_SIZE;
        pinfo->pos.y = fget_float( fileread ) * TILE_SIZE;
        pinfo->pos.z = fget_float( fileread ) * TILE_SIZE;

        pinfo->facing = FACE_NORTH;
        pinfo->attach = ATTACH_NONE;
        cTmp = fget_first_letter( fileread );
        if ( 'S' == toupper(cTmp) )       pinfo->facing = FACE_SOUTH;
        else if ( 'E' == toupper(cTmp) )  pinfo->facing = FACE_EAST;
        else if ( 'W' == toupper(cTmp) )  pinfo->facing = FACE_WEST;
        else if ( '?' == toupper(cTmp) )  pinfo->facing = FACE_RANDOM;
        else if ( 'L' == toupper(cTmp) )  pinfo->attach = ATTACH_LEFT;
        else if ( 'R' == toupper(cTmp) )  pinfo->attach = ATTACH_RIGHT;
        else if ( 'I' == toupper(cTmp) )  pinfo->attach = ATTACH_INVENTORY;

        pinfo->money   = fget_int( fileread );
        pinfo->skin    = fget_int( fileread );
        pinfo->passage = fget_int( fileread );
        pinfo->content = fget_int( fileread );
        pinfo->level   = fget_int( fileread );

        if (pinfo->skin >= MAX_SKIN)
        {
            int irand = RANDIE;
            pinfo->skin = irand % MAX_SKIN;     // Randomize skin?
        }

        pinfo->stat = fget_bool( fileread );

        fget_first_letter( fileread );   // BAD! Unused ghost value

        cTmp = fget_first_letter( fileread );
        pinfo->team = ( cTmp - 'A' ) % TEAM_MAX;
    }
    else if( '#' == delim )
    {
        STRING szTmp1, szTmp2;
        int    iTmp, fields;

        pinfo->do_spawn = bfalse;

        fields = vfs_scanf( fileread, "%255s%255s%d", szTmp1, szTmp2, &iTmp );
        if( 3 == fields && 0 == strcmp(szTmp1, "dependency") )
        {
            retval = btrue;

            // seed the info with the data
            strncpy( pinfo->spawn_coment, szTmp2, SDL_arraysize(pinfo->spawn_coment) );
            pinfo->slot = iTmp;
        }
    }

    return retval;
}

