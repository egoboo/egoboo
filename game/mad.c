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

/// @file mad.c
/// @brief The files for handling egoboo's internal model definitions
/// @details

#include "mad.h"

#include "cap_file.h"
#include "particle.h"

#include "log.h"
#include "script_compile.h"
#include "graphic.h"
#include "texture.h"
#include "sound.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

ego_md2_t  ego_md2_data[MAX_PROFILE]; ///< the old-style md2 data

mad_t   MadList[MAX_PROFILE];

static STRING  szModelName     = EMPTY_CSTR;      ///< MD2 Model name
static char    cActionName[ACTION_COUNT][2];      ///< Two letter name code
static STRING  cActionComent[ACTION_COUNT];       ///< Strings explaining the action codes

static Uint16 action_number();
//static Uint16 action_frame();
static void   action_check_copy( const char* loadname, Uint16 object );
static void   action_copy_correct( Uint16 object, Uint16 actiona, Uint16 actionb );

static void   mad_get_framefx( int frame );
static void   mad_get_walk_frame( Uint16 object, int lip, int action );
static void   mad_make_framelip( Uint16 object, int action );
static void   mad_rip_actions( Uint16 object );

static void mad_finalize( Uint16 object );
static void mad_heal_actions( Uint16 object, const char * loadname );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//static void md2_fix_normals( Uint16 modelindex );
//static void md2_get_transvertices( Uint16 modelindex );
// static int  vertexconnected( md2_ogl_commandlist_t * pclist, int vertex );

static mad_t * mad_init( mad_t * pmad );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16 action_number()
{
    /// @details ZZ@> This function returns the number of the action in cFrameName, or
    ///    it returns NOACTION if it could not find a match

    int cnt;
    char tmp_str[16];
    char first, second;

    sscanf( cFrameName, " %15s", tmp_str );

    first  = tmp_str[0];
    second = tmp_str[1];

    for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
    {
        if ( first == cActionName[cnt][0] && second == cActionName[cnt][1] )
        {
            return cnt;
        }
    }

    return NOACTION;
}

////--------------------------------------------------------------------------------------------
//Uint16 action_frame()
//{
//    /// @details ZZ@> This function returns the frame number in the third and fourth characters
//    ///    of cFrameName
//
//    int number;
//    char tmp_str[16];
//
//    sscanf( cFrameName, " %15s%d", tmp_str, &number );
//
//    return number;
//}

//--------------------------------------------------------------------------------------------
void action_copy_correct( Uint16 object, Uint16 actiona, Uint16 actionb )
{
    /// @details ZZ@> This function makes sure both actions are valid if either of them
    ///    are valid.  It will copy start and ends to mirror the valid action.

    mad_t * pmad;

    if( actiona >= ACTION_COUNT || actionb >= ACTION_COUNT ) return;

    if ( object > MAX_PROFILE || !MadList[object].loaded ) return;
    pmad = MadList + object;

    // With the new system using the action_map, this is all that is really necessary
    if( ACTION_COUNT == pmad->action_map[actiona] )
    {
        if( pmad->action_valid[actionb] )
        {
            pmad->action_map[actiona] = actionb;
        }
        else if ( ACTION_COUNT != pmad->action_map[actionb] )
        {
            pmad->action_map[actiona] = pmad->action_map[actionb];
        }
    }
    else if ( ACTION_COUNT == pmad->action_map[actionb] )
    {
        if( pmad->action_valid[actiona] )
        {
            pmad->action_map[actionb] = actiona;
        }
        else if ( ACTION_COUNT != pmad->action_map[actiona] )
        {
            pmad->action_map[actionb] = pmad->action_map[actiona];
        }
    }

    //if ( pmad->action_valid[actiona] == pmad->action_valid[actionb] )
    //{
    //    // They are either both valid or both invalid, in either case we can't help
    //    return;
    //}
    //else
    //{
    //    // Fix the invalid one
    //    if ( !pmad->action_valid[actiona] )
    //    {
    //        // Fix actiona
    //        pmad->action_valid[actiona] = btrue;
    //        pmad->action_stt[actiona] = pmad->action_stt[actionb];
    //        pmad->action_end[actiona]   = pmad->action_end[actionb];
    //    }
    //    else
    //    {
    //        // Fix actionb
    //        pmad->action_valid[actionb] = btrue;
    //        pmad->action_stt[actionb] = pmad->action_stt[actiona];
    //        pmad->action_end[actionb]   = pmad->action_end[actiona];
    //    }
    //}
}

//--------------------------------------------------------------------------------------------
int mad_get_action( Uint16 imad, int action )
{
    /// @detaills BB@> translate the action that was given into a valid action for the model
    ///
    /// returns ACTION_COUNT on a complete failure, or the default ACTION_DA if

    int     retval;
    mad_t * pmad;

    if( !LOADED_MAD(imad) ) return ACTION_COUNT;
    pmad = MadList + imad;

    // you are pretty much guaranteed that ACTION_DA will be valid for a model,
    // I guess it could be invalid if the model had no fraes or something
    retval = ACTION_DA;
    if( !pmad->action_valid[ACTION_DA] )
    {
        retval = ACTION_COUNT;
    }

    // check for a valid action range
    if( action < 0 || action > ACTION_COUNT ) return retval;

    // track down a valid value
    if( pmad->action_valid[action] )
    {
        retval = action;
    }
    else if( ACTION_COUNT != pmad->action_map[action] )
    {
        int cnt, tnc;

        // do a "recursive search for a valid action
        // we should never really have to check more than once if the map is prepared
        // properly BUT you never can tell. Make sure we do not get a runaway loop by
        // you never go farther than ACTION_COUNT steps and that you never see the
        // original action again

        tnc = pmad->action_map[action];
        for(cnt = 0; cnt<ACTION_COUNT; cnt++)
        {
            if( tnc >= ACTION_COUNT || tnc < 0 || tnc ==action ) break;

            if( pmad->action_valid[tnc] )
            {
                retval = tnc;
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void action_check_copy( const char* loadname, Uint16 object )
{
    /// @details ZZ@> This function copies a model's actions
    vfs_FILE *fileread;
    int actiona, actionb;
    char szOne[16] = EMPTY_CSTR;
    char szTwo[16] = EMPTY_CSTR;

    if ( !LOADED_MAD(object) ) return;

    fileread = vfs_openRead( loadname );
    if ( NULL == fileread ) return;

    while ( goto_colon( NULL, fileread, btrue ) )
    {
        fget_string( fileread, szOne, SDL_arraysize(szOne) );
        actiona = action_which( szOne[0] );

        fget_string( fileread, szTwo, SDL_arraysize(szTwo) );
        actionb = action_which( szTwo[0] );

        action_copy_correct( object, actiona + 0, actionb + 0 );
        action_copy_correct( object, actiona + 1, actionb + 1 );
        action_copy_correct( object, actiona + 2, actionb + 2 );
        action_copy_correct( object, actiona + 3, actionb + 3 );
    }

    vfs_close( fileread );

}

//--------------------------------------------------------------------------------------------
int action_which( char cTmp )
{
    /// @details ZZ@> This function changes a letter into an action code
    int action;

    switch ( toupper(cTmp) )
    {
        case 'D': action = ACTION_DA; break;
        case 'U': action = ACTION_UA; break;
        case 'T': action = ACTION_TA; break;
        case 'C': action = ACTION_CA; break;
        case 'S': action = ACTION_SA; break;
        case 'B': action = ACTION_BA; break;
        case 'L': action = ACTION_LA; break;
        case 'X': action = ACTION_XA; break;
        case 'F': action = ACTION_FA; break;
        case 'P': action = ACTION_PA; break;
        case 'Z': action = ACTION_ZA; break;
            // case 'W': action = ACTION_WA; break;   // Zefz: Can't do this, attack animation WALK is used for doing nothing (for example charging spells)
        case 'H': action = ACTION_HA; break;
        case 'K': action = ACTION_KA; break;
        default:  action = ACTION_DA; break;
    }

    return action;
}

//--------------------------------------------------------------------------------------------
void mad_get_walk_frame( Uint16 object, int lip, int action )
{
    /// @details ZZ@> This helps make walking look right
    int frame = 0;
    int framesinaction, action_stt;
    mad_t * pmad;

    if( !LOADED_MAD(object) ) return;
    pmad = MadList + object;

    action = mad_get_action(object, action);
    if( ACTION_COUNT == action )
    {
        framesinaction = 1;
        action_stt     = pmad->action_stt[ACTION_DA];
    }
    else
    {
        framesinaction = pmad->action_end[action] - pmad->action_stt[action];
        action_stt     = pmad->action_stt[action];
    }

    for ( frame = 0; frame < 16; frame++  )
    {
        int framealong = 0;

        if ( framesinaction > 0 )
        {
            framealong = ( ( frame * framesinaction / 16 ) + 2 ) % framesinaction;
        }

        pmad->frameliptowalkframe[lip][frame] = action_stt + framealong;
    }
}

//--------------------------------------------------------------------------------------------
void mad_get_framefx( int frame )
{
    /// @details ZZ@> This function figures out the IFrame invulnerability, and Attack, Grab, and
    ///               Drop timings
    ///
    ///          BB@> made a bit more sturdy parser that is not going to confuse strings like LCRA
    ///               which would not crop up if the convention of L or R going first was applied universally.
    ///               However, there are existing (and common) models which use the opposite convention, leading
    ///               to the possibility that an fx string LARC could be interpreted as act left, char right AND
    ///               act right.

    Uint32 fx = 0;
    char name_action[16], name_fx[16];
    int name_count;
    int fields;
    int cnt;

    static int token_count = -1;
    static const char * tokens[] = { "I","S","F","P","A","G","D","C",                 /* the normal command tokens */
                                     "LA","LG","LD","LC","RA","RG","RD","RC", NULL }; /* the "bad" token aliases */

    char * ptmp, * ptmp_end, *paction, *paction_end;

    // this should only be initializwd the first time through
    if( token_count < 0 )
    {
        token_count = 0;
        for( cnt = 0; NULL != tokens[token_count] && cnt<256; cnt++ ) token_count++;
    }

    // check for a valid frame number
    if( frame >= MAXFRAME ) return;

    // set the default values
    fx = 0;
    Md2FrameList[frame].framefx = fx;

    // check for a non-trivial frame name
    if( !VALID_CSTR(cFrameName) ) return;

    // skip over whitespace
    ptmp     = cFrameName;
    ptmp_end = cFrameName + 16;
    for( /* nothing */; ptmp < ptmp_end && isspace(*ptmp); ptmp++ ) {};

    // copy non-numerical text
    paction     = name_action;
    paction_end = name_action + 16;
    for( /* nothing */; ptmp < ptmp_end && paction < paction_end && !isspace(*ptmp); ptmp++, paction++ )
    {
        if( isdigit( *ptmp ) ) break;
        *paction = *ptmp;
    }
    if( paction < paction_end ) *paction = '\0';

    name_fx[0] = '\0';
    fields = sscanf( ptmp, "%d %15s", &name_count, name_fx );
    name_action[15] = '\0';
    name_fx[15] = '\0';

    // check for a non-trivial fx command
    if( !VALID_CSTR(name_fx) ) return;

    // scan the fx string for valid commands
    ptmp     = name_fx;
    ptmp_end = name_fx + 15;
    while( '\0' != *ptmp && ptmp < ptmp_end )
    {
        int len;
        int token_index = -1;
        for( cnt = 0; cnt<token_count; cnt++ )
        {
            len = strlen( tokens[cnt] );
            if( 0 == strncmp( tokens[cnt], ptmp, len ) )
            {
                ptmp += len;
                token_index = cnt;
                break;
            }
        }

        if( -1 == token_index )
        {
            if( cfg.dev_mode )
            {
                log_warning( "Model %s, frame %d, frame name \"%s\" has unknown frame effects command \"%s\"\n", szModelName, frame, cFrameName, ptmp );
            }
            ptmp++;
        }
        else
        {
            bool_t bad_form = bfalse;
            switch( token_index )
            {
                case  0: // "I" == invulnerable
                    fx |= MADFX_INVICTUS;
                    break;

                case  1: // "S" == stop
                    fx |= MADFX_STOP;
                    break;

                case  2: // "F" == footfall
                    fx |= MADFX_FOOTFALL;
                    break;

                case  3: // "P" == poof
                    fx |= MADFX_POOF;
                    break;

                case  4: // "A" == action

                    // get any modifiers
                    while( ('\0' != *ptmp && ptmp < ptmp_end) && ('R' == *ptmp || 'L' == *ptmp ) )
                    {
                        fx |= ( 'L' == *ptmp ) ? MADFX_ACTLEFT : MADFX_ACTRIGHT;
                        ptmp++;
                    }
                    break;

                case  5: // "G" == grab

                    // get any modifiers
                    while( ('\0' != *ptmp && ptmp < ptmp_end) && ('R' == *ptmp || 'L' == *ptmp ) )
                    {
                        fx |= ( 'L' == *ptmp ) ? MADFX_GRABLEFT : MADFX_GRABRIGHT;
                        ptmp++;
                    }
                    break;

                case  6: // "D" == drop

                    // get any modifiers
                    while( ('\0' != *ptmp && ptmp < ptmp_end) && ('R' == *ptmp || 'L' == *ptmp ) )
                    {
                        fx |= ( 'L' == *ptmp ) ? MADFX_DROPLEFT : MADFX_DROPRIGHT;
                        ptmp++;
                    }
                    break;

                case  7: // "C" == grab a character

                    // get any modifiers
                    while( ('\0' != *ptmp && ptmp < ptmp_end) && ('R' == *ptmp || 'L' == *ptmp ) )
                    {
                        fx |= ( 'L' == *ptmp ) ? MADFX_CHARLEFT : MADFX_CHARRIGHT;
                        ptmp++;
                    }
                    break;

                case  8: // "LA"
                    bad_form = btrue;
                    fx |= MADFX_ACTLEFT;
                    break;

                case  9: // "LG"
                    bad_form = btrue;
                    fx |= MADFX_GRABLEFT;
                    break;

                case 10: // "LD"
                    bad_form = btrue;
                    fx |= MADFX_DROPLEFT;
                    break;

                case 11: // "LC"
                    bad_form = btrue;
                    fx |= MADFX_CHARLEFT;
                    break;

                case 12: // "RA"
                    bad_form = btrue;
                    fx |= MADFX_ACTRIGHT;
                    break;

                case 13: // "RG"
                    bad_form = btrue;
                    fx |= MADFX_GRABRIGHT;
                    break;

                case 14: // "RD"
                    bad_form = btrue;
                    fx |= MADFX_DROPRIGHT;
                    break;

                case 15: // "RC"
                    bad_form = btrue;
                    fx |= MADFX_CHARRIGHT;
                    break;
            }

            if( bad_form && -1 != token_index )
            {
                log_warning( "Model %s, frame %d, frame name \"%s\" has a frame effects command in an improper configuration \"%s\"\n", szModelName, frame, cFrameName, tokens[token_index] );
            }
        }
    }

    Md2FrameList[frame].framefx = fx;
}

//--------------------------------------------------------------------------------------------
void mad_make_framelip( Uint16 object, int action )
{
    /// @details ZZ@> This helps make walking look right

    int frame, framesinaction;
    mad_t * pmad;

    if( !LOADED_MAD(object) ) return;
    pmad = MadList + object;

    action = mad_get_action( object, action );
    if( ACTION_COUNT == action || ACTION_DA == action ) return;

    if ( !pmad->action_valid[action] ) return;

    framesinaction = pmad->action_end[action] - pmad->action_stt[action];
    frame = pmad->action_stt[action];

    while ( frame < pmad->action_end[action] )
    {
        Md2FrameList[frame].framelip = ( frame - pmad->action_stt[action] ) * 15 / framesinaction;
        Md2FrameList[frame].framelip = ( Md2FrameList[frame].framelip ) & 15;
        frame++;
    }

}

//--------------------------------------------------------------------------------------------
void mad_make_equally_lit( int model )
{
    /// @details ZZ@> This function makes ultra low poly models look better
    int frame, cnt, vert;
    if ( MadList[model].loaded )
    {
        frame = ego_md2_data[MadList[model].md2_ref].framestart;

        for ( cnt = 0; cnt < ego_md2_data[MadList[model].md2_ref].frames; cnt++ )
        {
            vert = 0;

            while ( vert < MAXVERTICES )
            {
                Md2FrameList[frame].vrta[vert] = EQUALLIGHTINDEX;
                vert++;
            }

            frame++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void load_action_names( const char* loadname )
{
    /// @details ZZ@> This function loads all of the 2 letter action names

    vfs_FILE* fileread;
    int cnt;

    char first = '\0', second = '\0';
    STRING comment;
    bool_t found;

    fileread = vfs_openRead( loadname );
    if ( !fileread ) return;

    for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
    {
        comment[0] = '\0';

        found = bfalse;
        if( goto_colon( NULL, fileread, bfalse ) )
        {
            if( vfs_scanf( fileread, " %c%c %s", &first, &second, &comment ) >= 2 )
            {
                found = btrue;
            }
        }

        if( found )
        {
            cActionName[cnt][0] = first;
            cActionName[cnt][1] = second;
            cActionComent[cnt][0] = '\0';

            if( VALID_CSTR(comment) )
            {
                strncpy( cActionComent[cnt], comment, SDL_arraysize(cActionComent[cnt]) );
                cActionComent[cnt][255] = '\0';
            }
        }
        else
        {
            cActionName[cnt][0] = '\0';
            cActionComent[cnt][0] = '\0';
        }
    }

    vfs_close( fileread );

}

//--------------------------------------------------------------------------------------------
Uint16 load_one_model_profile( const char* tmploadname, Uint16 imad )
{
    mad_t * pmad;
    STRING  newloadname;

    if ( !VALID_MAD_RANGE(imad) ) return MAX_MAD;
    pmad = MadList + imad;

    // clear out the mad
    mad_init( pmad );

    // mark it as used
    pmad->loaded = btrue;

    // do this for now. maybe make it dynamic later...
    pmad->md2_ref = imad;

    // Make up a name for the model...  IMPORT\TEMP0000.OBJ
    strncpy( pmad->name, tmploadname, SDL_arraysize(pmad->name) );
    pmad->name[ SDL_arraysize(pmad->name) - 1 ] = CSTR_END;

    // Load the imad model
    make_newloadname( tmploadname, SLASH_STR "tris.md2", newloadname );

#ifdef __unix__

    // unix is case sensitive, but sometimes this file is called tris.MD2
    if ( access( newloadname, R_OK ) )
    {
        make_newloadname( tmploadname, SLASH_STR "tris.MD2", newloadname );

        // still no luck !
        if ( access( newloadname, R_OK ) )
        {
            log_warning( "Cannot open: %s\n", newloadname );
        }
    }

#endif

    szModelName[0] = '\0';
    if( md2_load_one( vfs_resolveReadFilename(newloadname), &(ego_md2_data[MadList[imad].md2_ref]) ) )
    {
        strncpy( szModelName, vfs_resolveReadFilename(newloadname), SDL_arraysize(szModelName) );
    }

    //md2_fix_normals( imad );        // Fix them normals
    //md2_get_transvertices( imad );  // Figure out how many vertices to transform

    pmad->md2_ptr = md2_loadFromFile( newloadname );

    // Create the actions table for this imad
    mad_rip_actions( imad );
    mad_heal_actions( imad, tmploadname );
    mad_finalize( imad );

    return imad;
}

//--------------------------------------------------------------------------------------------
void mad_heal_actions( Uint16 object, const char * tmploadname )
{
    STRING newloadname;

    if( !LOADED_MAD(object) ) return;

    // Make sure actions are made valid if a similar one exists
    action_copy_correct( object, ACTION_DA, ACTION_DB );  // All dances should be safe
    action_copy_correct( object, ACTION_DB, ACTION_DC );
    action_copy_correct( object, ACTION_DC, ACTION_DD );
    action_copy_correct( object, ACTION_DB, ACTION_DC );
    action_copy_correct( object, ACTION_DA, ACTION_DB );
    action_copy_correct( object, ACTION_UA, ACTION_UB );
    action_copy_correct( object, ACTION_UB, ACTION_UC );
    action_copy_correct( object, ACTION_UC, ACTION_UD );
    action_copy_correct( object, ACTION_TA, ACTION_TB );
    action_copy_correct( object, ACTION_TC, ACTION_TD );
    action_copy_correct( object, ACTION_CA, ACTION_CB );
    action_copy_correct( object, ACTION_CC, ACTION_CD );
    action_copy_correct( object, ACTION_SA, ACTION_SB );
    action_copy_correct( object, ACTION_SC, ACTION_SD );
    action_copy_correct( object, ACTION_BA, ACTION_BB );
    action_copy_correct( object, ACTION_BC, ACTION_BD );
    action_copy_correct( object, ACTION_LA, ACTION_LB );
    action_copy_correct( object, ACTION_LC, ACTION_LD );
    action_copy_correct( object, ACTION_XA, ACTION_XB );
    action_copy_correct( object, ACTION_XC, ACTION_XD );
    action_copy_correct( object, ACTION_FA, ACTION_FB );
    action_copy_correct( object, ACTION_FC, ACTION_FD );
    action_copy_correct( object, ACTION_PA, ACTION_PB );
    action_copy_correct( object, ACTION_PC, ACTION_PD );
    action_copy_correct( object, ACTION_ZA, ACTION_ZB );
    action_copy_correct( object, ACTION_ZC, ACTION_ZD );
    action_copy_correct( object, ACTION_WA, ACTION_WB );
    action_copy_correct( object, ACTION_WB, ACTION_WC );
    action_copy_correct( object, ACTION_WC, ACTION_WD );
    action_copy_correct( object, ACTION_DA, ACTION_WD );  // All walks should be safe
    action_copy_correct( object, ACTION_WC, ACTION_WD );
    action_copy_correct( object, ACTION_WB, ACTION_WC );
    action_copy_correct( object, ACTION_WA, ACTION_WB );
    action_copy_correct( object, ACTION_JA, ACTION_JB );
    action_copy_correct( object, ACTION_JB, ACTION_JC );
    action_copy_correct( object, ACTION_DA, ACTION_JC );  // All jumps should be safe
    action_copy_correct( object, ACTION_JB, ACTION_JC );
    action_copy_correct( object, ACTION_JA, ACTION_JB );
    action_copy_correct( object, ACTION_HA, ACTION_HB );
    action_copy_correct( object, ACTION_HB, ACTION_HC );
    action_copy_correct( object, ACTION_HC, ACTION_HD );
    action_copy_correct( object, ACTION_HB, ACTION_HC );
    action_copy_correct( object, ACTION_HA, ACTION_HB );
    action_copy_correct( object, ACTION_KA, ACTION_KB );
    action_copy_correct( object, ACTION_KB, ACTION_KC );
    action_copy_correct( object, ACTION_KC, ACTION_KD );
    action_copy_correct( object, ACTION_KB, ACTION_KC );
    action_copy_correct( object, ACTION_KA, ACTION_KB );
    action_copy_correct( object, ACTION_MH, ACTION_MI );
    action_copy_correct( object, ACTION_DA, ACTION_MM );
    action_copy_correct( object, ACTION_MM, ACTION_MN );

    // Copy entire actions to save frame space COPY.TXT
    make_newloadname( tmploadname, SLASH_STR "copy.txt", newloadname );
    action_check_copy( newloadname, object );

}

//--------------------------------------------------------------------------------------------
void mad_finalize( Uint16 object )
{
    int frame;

    mad_t * pmad;
    ego_md2_t * pmd2;

    if( !LOADED_MAD(object) ) return;
    pmad = MadList + object;
    pmd2 = ego_md2_data + pmad->md2_ref;

    // Create table for doing transition from one type of walk to another...
    // Clear 'em all to start
    for ( frame = 0; frame < pmd2->frames; frame++ )
    {
        Md2FrameList[pmd2->framestart + frame].framelip = 0;
    }

    // Need to figure out how far into action each frame is
    mad_make_framelip( object, ACTION_WA );
    mad_make_framelip( object, ACTION_WB );
    mad_make_framelip( object, ACTION_WC );

    // Now do the same, in reverse, for walking animations
    mad_get_walk_frame( object, LIPDA, ACTION_DA );
    mad_get_walk_frame( object, LIPWA, ACTION_WA );
    mad_get_walk_frame( object, LIPWB, ACTION_WB );
    mad_get_walk_frame( object, LIPWC, ACTION_WC );
}

//--------------------------------------------------------------------------------------------
void mad_rip_actions( Uint16 object )
{
    /// @details ZZ@> This function creates the frame lists for each action based on the
    ///    name of each md2 frame in the model

    int frame, framesinaction;
    int action, lastaction;

    mad_t * pmad;
    ego_md2_t * pmd2;

    if( !LOADED_MAD(object) ) return;
    pmad = MadList + object;
    pmd2 = ego_md2_data + pmad->md2_ref;

    // Clear out all actions and reset to invalid
    for ( action = 0; action < ACTION_COUNT; action++ )
    {
        pmad->action_map[action]   = ACTION_COUNT;
        pmad->action_valid[action] = bfalse;
    }

    // Set the primary dance action to be the first frame, just as a default
    pmad->action_map[ACTION_DA]   = ACTION_DA;
    pmad->action_valid[ACTION_DA] = btrue;
    pmad->action_stt[ACTION_DA]   = pmd2->framestart;
    pmad->action_end[ACTION_DA]   = pmd2->framestart + 1;

    // Now go huntin' to see what each frame is, look for runs of same action
    md2_rip_frame_name( 0 );
    lastaction = action_number();  framesinaction = 0;
    frame = 0;

    while ( frame < pmd2->frames )
    {
        md2_rip_frame_name( frame );
        action = action_number();

        pmad->action_map[action] = action;

        if ( lastaction == action )
        {
            framesinaction++;
        }
        else
        {
            // Write the old action
            if ( lastaction < ACTION_COUNT )
            {
                pmad->action_valid[lastaction] = btrue;
                pmad->action_stt[lastaction]   = pmd2->framestart + frame - framesinaction;
                pmad->action_end[lastaction]   = pmd2->framestart + frame;
            }

            framesinaction = 1;
            lastaction = action;
        }

        mad_get_framefx( pmd2->framestart + frame );
        frame++;
    }

    // Write the old action
    if ( lastaction < ACTION_COUNT )
    {
        pmad->action_valid[lastaction] = btrue;
        pmad->action_stt[lastaction]   = pmd2->framestart + frame - framesinaction;
        pmad->action_end[lastaction]   = pmd2->framestart + frame;
    }
}

//---------------------------------------------------------------------------------------------
//void md2_fix_normals( Uint16 modelindex )
//{
//    /// @details ZZ@> This function helps light not flicker so much
//    int cnt, tnc;
//    Uint16 indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
//    Uint16 indexofnextnextnextnext;
//    Uint32 frame;
//
//    frame = ego_md2_data[MadList[modelindex].md2_ref].framestart;
//    cnt = 0;
//
//    while ( cnt < ego_md2_data[MadList[modelindex].md2_ref].vertices )
//    {
//        tnc = 0;
//
//        while ( tnc < ego_md2_data[MadList[modelindex].md2_ref].frames )
//        {
//            indexofcurrent = Md2FrameList[frame].vrta[cnt];
//            indexofnext = Md2FrameList[frame+1].vrta[cnt];
//            indexofnextnext = Md2FrameList[frame+2].vrta[cnt];
//            indexofnextnextnext = Md2FrameList[frame+3].vrta[cnt];
//            indexofnextnextnextnext = Md2FrameList[frame+4].vrta[cnt];
//            if ( indexofcurrent == indexofnextnext && indexofnext != indexofcurrent )
//            {
//                Md2FrameList[frame+1].vrta[cnt] = indexofcurrent;
//            }
//            if ( indexofcurrent == indexofnextnextnext )
//            {
//                if ( indexofnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+1].vrta[cnt] = indexofcurrent;
//                }
//                if ( indexofnextnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+2].vrta[cnt] = indexofcurrent;
//                }
//            }
//            if ( indexofcurrent == indexofnextnextnextnext )
//            {
//                if ( indexofnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+1].vrta[cnt] = indexofcurrent;
//                }
//                if ( indexofnextnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+2].vrta[cnt] = indexofcurrent;
//                }
//                if ( indexofnextnextnext != indexofcurrent )
//                {
//                    Md2FrameList[frame+3].vrta[cnt] = indexofcurrent;
//                }
//            }
//
//            tnc++;
//        }
//
//        cnt++;
//    }
//}

//---------------------------------------------------------------------------------------------
//void md2_get_transvertices( Uint16 modelindex )
//{
//    /// @details ZZ@> This function gets the number of vertices to transform for a model...
//    //    That means every one except the grip ( unconnected ) vertices
//
//    // if (modelindex == 0)
//    // {
//    //   for ( cnt = 0; cnt < MadList[modelindex].vertices; cnt++ )
//    //   {
//    //       printf("%d-%d\n", cnt, vertexconnected( modelindex, cnt ) );
//    //   }
//    // }
//
//    MadList[modelindex].transvertices = ego_md2_data[MadList[modelindex].md2_ref].vertices;
//}

//---------------------------------------------------------------------------------------------
/*int vertexconnected( md2_ogl_commandlist_t * pclist, int vertex )
{
    /// @details ZZ@> This function returns 1 if the model vertex is connected, 0 otherwise
    int cnt, tnc, entry;

    entry = 0;

    for ( cnt = 0; cnt < pclist->count; cnt++ )
    {
        for ( tnc = 0; tnc < pclist->size[cnt]; tnc++ )
        {
            if ( pclist->vrt[entry] == vertex )
            {
                // The vertex is used
                return 1;
            }

            entry++;
        }
    }

    // The vertex is not used
    return 0;
}*/

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mad_t * mad_init( mad_t * pmad )
{
    int action;

    if( NULL == pmad ) return pmad;

    memset( pmad, 0, sizeof(mad_t) );

    strncpy( pmad->name, "*NONE*", SDL_arraysize(pmad->name) );

    // Clear out all actions and reset to invalid
    for ( action = 0; action < ACTION_COUNT; action++ )
    {
       pmad->action_map[action]   = ACTION_COUNT;
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
void init_all_mad()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        mad_init( MadList + cnt );
    }

    Md2FrameList_index = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_mad()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_mad( cnt );
    }

    Md2FrameList_index = 0;
}

//---------------------------------------------------------------------------------------------
bool_t release_one_mad( Uint16 imad )
{
    mad_t * pmad;

    if( !VALID_MAD_RANGE(imad) ) return bfalse;
    pmad = MadList + imad;

    if( !pmad->loaded ) return btrue;

    // free any md2 data
    md2_freeModel( pmad->md2_ptr );

    mad_init( pmad );

    pmad->loaded   = bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
int randomize_action(int action, int slot)
{
    /// @details BB@> this function actually determines whether the action fillows the
    ///               pattern of ACTION_?A, ACTION_?B, ACTION_?C, ACTION_?D, with
    ///               A and B being for the left hand, and C and D being for the right hand

    int diff = 0;

    // a valid slot?
    if( slot < 0 || slot >= SLOT_COUNT ) return action;

    // a valid action?
    if( action < 0 || action >= ACTION_COUNT) return bfalse;

    diff = slot * 2;

    //---- non-randomizable actions
         if( ACTION_MG == action ) return action;        // MG      = Open Chest
    else if( ACTION_MH == action ) return action;        // MH      = Sit
    else if( ACTION_MI == action ) return action;        // MI      = Ride
    else if( ACTION_MJ == action ) return action;        // MJ      = Object Activated
    else if( ACTION_MK == action ) return action;        // MK      = Snoozing
    else if( ACTION_ML == action ) return action;        // ML      = Unlock
    else if( ACTION_JA == action ) return action;        // JA      = Jump
    else if( ACTION_RA == action ) return action;        // RA      = Roll
    else if( ACTION_IS_TYPE(action, W) ) return action;  // WA - WD = Walk

    //---- do a couple of special actions that have left/right
    else if( ACTION_EA == action || ACTION_EB == action ) action = ACTION_JB + slot;    // EA/EB = Evade left/right
    else if( ACTION_JB == action || ACTION_JC == action ) action = ACTION_JB + slot;    // JB/JC = Dropped item left/right
    else if( ACTION_MA == action || ACTION_MB == action ) action = ACTION_MA + slot;    // MA/MB = Drop left/right item
    else if( ACTION_MC == action || ACTION_MD == action ) action = ACTION_MC + slot;    // MC/MD = Slam left/right
    else if( ACTION_ME == action || ACTION_MF == action ) action = ACTION_ME + slot;    // ME/MF = Grab item left/right
    else if( ACTION_MM == action || ACTION_MN == action ) action = ACTION_MM + slot;    // MM/MN = Held left/right

    //---- actions that can be randomized, but are not left/right sensitive
    // D = dance
    else if( ACTION_IS_TYPE(action, D) )
    {
        action = ACTION_TYPE(D) + generate_randmask( 0, 3 );
    }

    //---- handle all the normal attack/defense animations

    // U = unarmed
    else if( ACTION_IS_TYPE(action, U) ) action = ACTION_TYPE(U) + diff + generate_randmask( 0, 1 );
    // T = thrust
    else if( ACTION_IS_TYPE(action, T) ) action = ACTION_TYPE(T) + diff + generate_randmask( 0, 1 );
    // C = chop
    else if( ACTION_IS_TYPE(action, C) ) action = ACTION_TYPE(C) + diff + generate_randmask( 0, 1 );
    // S = slice
    else if( ACTION_IS_TYPE(action, S) ) action = ACTION_TYPE(S) + diff + generate_randmask( 0, 1 );
    // B = bash
    else if( ACTION_IS_TYPE(action, B) ) action = ACTION_TYPE(B) + diff + generate_randmask( 0, 1 );
    // L = longbow
    else if( ACTION_IS_TYPE(action, L) ) action = ACTION_TYPE(L) + diff + generate_randmask( 0, 1 );
    // X = crossbow
    else if( ACTION_IS_TYPE(action, X) ) action = ACTION_TYPE(X) + diff + generate_randmask( 0, 1 );
    // F = fling
    else if( ACTION_IS_TYPE(action, F) ) action = ACTION_TYPE(F) + diff + generate_randmask( 0, 1 );
    // P = parry/block
    else if( ACTION_IS_TYPE(action, P) ) action = ACTION_TYPE(P) + diff + generate_randmask( 0, 1 );
    // Z = zap
    else if( ACTION_IS_TYPE(action, Z) ) action = ACTION_TYPE(Z) + diff + generate_randmask( 0, 1 );

    //---- these are passive actions
    // H = hit
    else if( ACTION_IS_TYPE(action, H) ) action = ACTION_TYPE(H) + diff + generate_randmask( 0, 1 );
    // K= killed
    else if( ACTION_IS_TYPE(action, K) ) action = ACTION_TYPE(K) + diff + generate_randmask( 0, 1 );

    return action;
}

////--------------------------------------------------------------------------------------------
//Uint16 test_frame_name( char letter )
//{
//    /// @details ZZ@> This function returns btrue if the 4th, 5th, 6th, or 7th letters
//    ///    of the frame name matches the input argument
//
//    if ( cFrameName[4] == letter ) return btrue;
//    if ( cFrameName[4] == 0 ) return bfalse;
//    if ( cFrameName[5] == letter ) return btrue;
//    if ( cFrameName[5] == 0 ) return bfalse;
//    if ( cFrameName[6] == letter ) return btrue;
//    if ( cFrameName[6] == 0 ) return bfalse;
//    if ( cFrameName[7] == letter ) return btrue;
//
//    return bfalse;
//}