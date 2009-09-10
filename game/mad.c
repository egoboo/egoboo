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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "mad.h"

#include "log.h"
#include "script_compile.h"
#include "graphic.h"
#include "particle.h"
#include "texture.h"
#include "sound.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static char cActionName[ACTION_COUNT][2]; // Two letter name code

mad_t   MadList[MAX_PROFILE];

DECLARE_STACK( ACCESS_TYPE_NONE, int, MessageOffset );

Uint32  message_buffer_carat = 0;                           // Where to put letter
char    message_buffer[MESSAGEBUFFERSIZE];                  // The text buffer

char    cFrameName[16];                                     // MD2 Frame Name

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void load_all_messages( const char *loadname, Uint16 object );
static void get_message( vfs_FILE* fileread );
static void md2_fix_normals( Uint16 modelindex );
static void md2_get_transvertices( Uint16 modelindex );
// static int  vertexconnected( md2_ogl_commandlist_t * pclist, int vertex );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16 action_number()
{
    // ZZ> This function returns the number of the action in cFrameName, or
    //    it returns NOACTION if it could not find a match
    int cnt;
    char first, second;

    first = cFrameName[0];
    second = cFrameName[1];

    for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
    {
        if ( first == cActionName[cnt][0] && second == cActionName[cnt][1] )
        {
            return cnt;
        }
    }

    return NOACTION;
}

//--------------------------------------------------------------------------------------------
Uint16 action_frame()
{
    // ZZ> This function returns the frame number in the third and fourth characters
    //    of cFrameName
    int number;

    sscanf( &cFrameName[2], "%d", &number );

    return number;
}

//--------------------------------------------------------------------------------------------
Uint16 test_frame_name( char letter )
{
    // ZZ> This function returns btrue if the 4th, 5th, 6th, or 7th letters
    //    of the frame name matches the input argument
    if ( cFrameName[4] == letter ) return btrue;
    if ( cFrameName[4] == 0 ) return bfalse;
    if ( cFrameName[5] == letter ) return btrue;
    if ( cFrameName[5] == 0 ) return bfalse;
    if ( cFrameName[6] == letter ) return btrue;
    if ( cFrameName[6] == 0 ) return bfalse;
    if ( cFrameName[7] == letter ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void action_copy_correct( Uint16 object, Uint16 actiona, Uint16 actionb )
{
    // ZZ> This function makes sure both actions are valid if either of them
    //    are valid.  It will copy start and ends to mirror the valid action.

    if ( object > MAX_PROFILE || !MadList[object].loaded ) return;

    if ( MadList[object].actionvalid[actiona] == MadList[object].actionvalid[actionb] )
    {
        // They are either both valid or both invalid, in either case we can't help
        return;
    }
    else
    {
        // Fix the invalid one
        if ( !MadList[object].actionvalid[actiona] )
        {
            // Fix actiona
            MadList[object].actionvalid[actiona] = btrue;
            MadList[object].actionstart[actiona] = MadList[object].actionstart[actionb];
            MadList[object].actionend[actiona] = MadList[object].actionend[actionb];
        }
        else
        {
            // Fix actionb
            MadList[object].actionvalid[actionb] = btrue;
            MadList[object].actionstart[actionb] = MadList[object].actionstart[actiona];
            MadList[object].actionend[actionb] = MadList[object].actionend[actiona];
        }
    }
}

//--------------------------------------------------------------------------------------------
void action_check_copy( const char* loadname, Uint16 object )
{
    // ZZ> This function copies a model's actions
    vfs_FILE *fileread;
    int actiona, actionb;
    char szOne[16], szTwo[16];

    if ( object > MAX_PROFILE || !MadList[object].loaded ) return;

    MadList[object].message_start = 0;
    fileread = vfs_openRead( loadname );
    if ( fileread )
    {
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
}

//--------------------------------------------------------------------------------------------
int action_which( char cTmp )
{
    // ZZ> This function changes a letter into an action code
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
    // ZZ> This helps make walking look right
    int frame = 0;
    int framesinaction = MadList[object].actionend[action] - MadList[object].actionstart[action];

    while ( frame < 16 )
    {
        int framealong = 0;
        if ( framesinaction > 0 )
        {
            framealong = ( ( frame * framesinaction / 16 ) + 2 ) % framesinaction;
        }

        MadList[object].frameliptowalkframe[lip][frame] = MadList[object].actionstart[action] + framealong;
        frame++;
    }
}

//--------------------------------------------------------------------------------------------
void mad_get_framefx( int frame )
{
    // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
    //    Drop timings
    Uint16 fx = 0;
    if ( test_frame_name( 'I' ) )
        fx = fx | MADFX_INVICTUS;
    if ( test_frame_name( 'L' ) )
    {
        if ( test_frame_name( 'A' ) )
            fx = fx | MADFX_ACTLEFT;
        if ( test_frame_name( 'G' ) )
            fx = fx | MADFX_GRABLEFT;
        if ( test_frame_name( 'D' ) )
            fx = fx | MADFX_DROPLEFT;
        if ( test_frame_name( 'C' ) )
            fx = fx | MADFX_CHARLEFT;
    }
    if ( test_frame_name( 'R' ) )
    {
        if ( test_frame_name( 'A' ) )
            fx = fx | MADFX_ACTRIGHT;
        if ( test_frame_name( 'G' ) )
            fx = fx | MADFX_GRABRIGHT;
        if ( test_frame_name( 'D' ) )
            fx = fx | MADFX_DROPRIGHT;
        if ( test_frame_name( 'C' ) )
            fx = fx | MADFX_CHARRIGHT;
    }
    if ( test_frame_name( 'S' ) )
        fx = fx | MADFX_STOP;
    if ( test_frame_name( 'F' ) )
        fx = fx | MADFX_FOOTFALL;
    if ( test_frame_name( 'P' ) )
        fx = fx | MADFX_POOF;

    Md2FrameList[frame].framefx = fx;
}

//--------------------------------------------------------------------------------------------
void mad_make_framelip( Uint16 object, int action )
{
    // ZZ> This helps make walking look right
    int frame, framesinaction;
    if ( MadList[object].actionvalid[action] )
    {
        framesinaction = MadList[object].actionend[action] - MadList[object].actionstart[action];
        frame = MadList[object].actionstart[action];

        while ( frame < MadList[object].actionend[action] )
        {
            Md2FrameList[frame].framelip = ( frame - MadList[object].actionstart[action] ) * 15 / framesinaction;
            Md2FrameList[frame].framelip = ( Md2FrameList[frame].framelip ) & 15;
            frame++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void mad_make_equally_lit( int model )
{
    // ZZ> This function makes ultra low poly models look better
    int frame, cnt, vert;
    if ( MadList[model].loaded )
    {
        frame = MadList[model].md2_data.framestart;

        for ( cnt = 0; cnt < MadList[model].md2_data.frames; cnt++ )
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
    // ZZ> This function loads all of the 2 letter action names
    vfs_FILE* fileread;
    int cnt;
    char first, second;

    fileread = vfs_openRead( loadname );
    if ( fileread )
    {
        cnt = 0;

        while ( cnt < ACTION_COUNT )
        {
            goto_colon( NULL, fileread, bfalse );
            vfs_scanf( fileread, "%c%c", &first, &second );
            cActionName[cnt][0] = first;
            cActionName[cnt][1] = second;
            cnt++;
        }

        vfs_close( fileread );
    }
}

//--------------------------------------------------------------------------------------------
int load_one_model_skins( const char * tmploadname, Uint16 object )
{
    int min_skin_tx, min_icon_tx;
    int max_skin, max_icon, max_tex;
    int iskin, iicon;
    int cnt;

    STRING newloadname;

    mad_t * pmad;

    if ( object > MAX_PROFILE ) return 0;
    pmad = MadList + object;

    // Load the skins and icons
    max_skin    = max_icon    = -1;
    min_skin_tx = min_icon_tx = INVALID_TEXTURE;
    for ( cnt = 0; cnt < MAXSKIN; cnt++)
    {
        snprintf( newloadname, SDL_arraysize(newloadname), "%s" SLASH_STR "tris%d", tmploadname, cnt );

        pmad->tex_ref[cnt] = TxTexture_load_one( newloadname, INVALID_TEXTURE, TRANSCOLOR );
        if ( INVALID_TEXTURE != pmad->tex_ref[cnt] )
        {
            max_skin = cnt;
            if ( INVALID_TEXTURE == min_skin_tx )
            {
                min_skin_tx = pmad->tex_ref[cnt];
            }
        }

        snprintf( newloadname, SDL_arraysize(newloadname), "%s" SLASH_STR "icon%d", tmploadname, cnt );
        pmad->ico_ref[cnt] = TxTexture_load_one( newloadname, INVALID_TEXTURE, INVALID_KEY );

        if ( INVALID_TEXTURE != pmad->ico_ref[cnt] )
        {
            max_icon = cnt;

            if ( INVALID_TEXTURE == min_icon_tx )
            {
                min_icon_tx = pmad->ico_ref[cnt];
            }

            if ( SPELLBOOK == object )
            {
                if ( bookicon_count < MAXSKIN )
                {
                    bookicon_ref[bookicon_count] = pmad->ico_ref[cnt];
                    bookicon_count++;
                }
            }
        }
    }

    if ( max_skin < 0 )
    {
        // If we didn't get a skin, set it to the water texture
        max_skin = 0;
        pmad->tex_ref[cnt] = TX_WATER_TOP;

        if (cfg.dev_mode)
        {
            log_message( "NOTE: Object is missing a skin (%s)!\n", tmploadname );
        }
    }

    max_tex = MAX(max_skin, max_icon);

    // fill in any missing textures
    iskin = min_skin_tx;
    iicon = min_icon_tx;
    for ( cnt = 0; cnt <= max_tex; cnt++ )
    {
        if ( INVALID_TEXTURE != pmad->tex_ref[cnt] && iskin != pmad->tex_ref[cnt] )
        {
            iskin = pmad->tex_ref[cnt];
        }

        if ( INVALID_TEXTURE != pmad->ico_ref[cnt] && iicon != pmad->ico_ref[cnt] )
        {
            iicon = pmad->ico_ref[cnt];
        }

        pmad->tex_ref[cnt] = iskin;
        pmad->ico_ref[cnt] = iicon;
    }

    return max_tex + 1;
}

//--------------------------------------------------------------------------------------------
int load_one_model_profile( const char* tmploadname, Uint16 object )
{
    int     cnt;
    mad_t * pmad;
    STRING  newloadname;

    if ( object > MAX_PROFILE ) return 0;
    pmad = MadList + object;

    // clear out the mad
    memset( pmad, 0, sizeof(mad_t) );

    // clear out the textures
    for ( cnt = 0; cnt < MAXSKIN; cnt++)
    {
        pmad->tex_ref[cnt] = INVALID_TEXTURE;
        pmad->ico_ref[cnt] = INVALID_TEXTURE;
    }

    // mark it as used
    pmad->loaded = btrue;

    // Make up a name for the model...  IMPORT\TEMP0000.OBJ
    strncpy( pmad->name, tmploadname, SDL_arraysize(pmad->name) );
    pmad->name[ SDL_arraysize(pmad->name) - 1 ] = '\0';

    // Load the AI script for this object
    make_newloadname( tmploadname, SLASH_STR "script.txt", newloadname );

    // Create a reference to the one we just loaded
    pmad->ai = load_ai_script( newloadname );

    // Load the object model
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

    md2_load_one( vfs_resolveReadFilename(newloadname), &(MadList[object].md2_data) );
    // md2_fix_normals( object );        // Fix them normals
    md2_get_transvertices( object );  // Figure out how many vertices to transform

    pmad->md2_ptr = md2_loadFromFile( newloadname );

    // Create the actions table for this object
    mad_rip_actions( object );

    // Copy entire actions to save frame space COPY.TXT
    make_newloadname( tmploadname, SLASH_STR "copy.txt", newloadname );
    action_check_copy( newloadname, object );

    // Load the messages for this object
    make_newloadname( tmploadname, SLASH_STR "message.txt", newloadname );
    load_all_messages( newloadname, object );

    // Load the particles for this object
    for ( cnt = 0; cnt < MAX_PIP_PER_PROFILE; cnt++ )
    {
        snprintf( newloadname, SDL_arraysize( newloadname), "%s" SLASH_STR "part%d.txt", tmploadname, cnt );

        // Make sure it's referenced properly
        pmad->prtpip[cnt] = load_one_particle_profile( newloadname );
    }

    pmad->skins = load_one_model_skins( tmploadname, object );

    // Load the waves for this object
    for ( cnt = 0; cnt < MAX_WAVE; cnt++ )
    {
        STRING  szLoadName, wavename;

        snprintf( wavename, SDL_arraysize( wavename), SLASH_STR "sound%d", cnt );
        make_newloadname( tmploadname, wavename, szLoadName );
        pmad->wavelist[cnt] = sound_load_chunk( szLoadName );
    }

    return pmad->skins;
}

//--------------------------------------------------------------------------------------------
void mad_rip_actions( Uint16 object )
{
    // ZZ> This function creates the frame lists for each action based on the
    //    name of each md2 frame in the model

    int frame, framesinaction;
    int action, lastaction;

    // Clear out all actions and reset to invalid
    action = 0;

    while ( action < ACTION_COUNT )
    {
        MadList[object].actionvalid[action] = bfalse;
        action++;
    }

    // Set the primary dance action to be the first frame, just as a default
    MadList[object].actionvalid[ACTION_DA] = btrue;
    MadList[object].actionstart[ACTION_DA] = MadList[object].md2_data.framestart;
    MadList[object].actionend[ACTION_DA] = MadList[object].md2_data.framestart + 1;

    // Now go huntin' to see what each frame is, look for runs of same action
    md2_rip_frame_name( 0 );
    lastaction = action_number();  framesinaction = 0;
    frame = 0;

    while ( frame < MadList[object].md2_data.frames )
    {
        md2_rip_frame_name( frame );
        action = action_number();
        if ( lastaction == action )
        {
            framesinaction++;
        }
        else
        {
            // Write the old action
            if ( lastaction < ACTION_COUNT )
            {
                MadList[object].actionvalid[lastaction] = btrue;
                MadList[object].actionstart[lastaction] = MadList[object].md2_data.framestart + frame - framesinaction;
                MadList[object].actionend[lastaction] = MadList[object].md2_data.framestart + frame;
            }

            framesinaction = 1;
            lastaction = action;
        }

        mad_get_framefx( MadList[object].md2_data.framestart + frame );
        frame++;
    }

    // Write the old action
    if ( lastaction < ACTION_COUNT )
    {
        MadList[object].actionvalid[lastaction] = btrue;
        MadList[object].actionstart[lastaction] = MadList[object].md2_data.framestart + frame - framesinaction;
        MadList[object].actionend[lastaction]   = MadList[object].md2_data.framestart + frame;
    }

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

    // Create table for doing transition from one type of walk to another...
    // Clear 'em all to start
    for ( frame = 0; frame < MadList[object].md2_data.frames; frame++ )
    {
        Md2FrameList[frame+MadList[object].md2_data.framestart].framelip = 0;
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

//---------------------------------------------------------------------------------------------
void md2_fix_normals( Uint16 modelindex )
{
    // ZZ> This function helps light not flicker so much
    int cnt, tnc;
    Uint16 indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
    Uint16 indexofnextnextnextnext;
    Uint32 frame;

    frame = MadList[modelindex].md2_data.framestart;
    cnt = 0;

    while ( cnt < MadList[modelindex].md2_data.vertices )
    {
        tnc = 0;

        while ( tnc < MadList[modelindex].md2_data.frames )
        {
            indexofcurrent = Md2FrameList[frame].vrta[cnt];
            indexofnext = Md2FrameList[frame+1].vrta[cnt];
            indexofnextnext = Md2FrameList[frame+2].vrta[cnt];
            indexofnextnextnext = Md2FrameList[frame+3].vrta[cnt];
            indexofnextnextnextnext = Md2FrameList[frame+4].vrta[cnt];
            if ( indexofcurrent == indexofnextnext && indexofnext != indexofcurrent )
            {
                Md2FrameList[frame+1].vrta[cnt] = indexofcurrent;
            }
            if ( indexofcurrent == indexofnextnextnext )
            {
                if ( indexofnext != indexofcurrent )
                {
                    Md2FrameList[frame+1].vrta[cnt] = indexofcurrent;
                }
                if ( indexofnextnext != indexofcurrent )
                {
                    Md2FrameList[frame+2].vrta[cnt] = indexofcurrent;
                }
            }
            if ( indexofcurrent == indexofnextnextnextnext )
            {
                if ( indexofnext != indexofcurrent )
                {
                    Md2FrameList[frame+1].vrta[cnt] = indexofcurrent;
                }
                if ( indexofnextnext != indexofcurrent )
                {
                    Md2FrameList[frame+2].vrta[cnt] = indexofcurrent;
                }
                if ( indexofnextnextnext != indexofcurrent )
                {
                    Md2FrameList[frame+3].vrta[cnt] = indexofcurrent;
                }
            }

            tnc++;
        }

        cnt++;
    }
}

//---------------------------------------------------------------------------------------------
void md2_get_transvertices( Uint16 modelindex )
{
    // ZZ> This function gets the number of vertices to transform for a model...
    //    That means every one except the grip ( unconnected ) vertices

    // if (modelindex == 0)
    // {
    //   for ( cnt = 0; cnt < MadList[modelindex].vertices; cnt++ )
    //   {
    //       printf("%d-%d\n", cnt, vertexconnected( modelindex, cnt ) );
    //   }
    // }

    MadList[modelindex].transvertices = MadList[modelindex].md2_data.vertices;
}

//---------------------------------------------------------------------------------------------
/*int vertexconnected( md2_ogl_commandlist_t * pclist, int vertex )
{
    // ZZ> This function returns 1 if the model vertex is connected, 0 otherwise
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
void load_all_messages( const char *loadname, Uint16 object )
{
    // ZZ> This function loads all of an objects messages
    vfs_FILE *fileread;

    MadList[object].message_start = 0;
    fileread = vfs_openRead( loadname );
    if ( fileread )
    {
        MadList[object].message_start = MessageOffset.count;

        while ( goto_colon( NULL, fileread, btrue ) )
        {
            get_message( fileread );
        }

        vfs_close( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void get_message( vfs_FILE* fileread )
{
    // ZZ> This function loads a string into the message buffer, making sure it
    //    is null terminated.
    int cnt;
    char cTmp;
    STRING szTmp;

    if ( message_buffer_carat >= MESSAGEBUFFERSIZE )
    {
        message_buffer_carat = MESSAGEBUFFERSIZE - 1;
        message_buffer[message_buffer_carat] = '\0';
        return;
    }

    if ( MessageOffset.count >= MAXTOTALMESSAGE )
    {
        return;
    }

    MessageOffset.lst[MessageOffset.count] = message_buffer_carat;
    fget_string( fileread, szTmp, SDL_arraysize(szTmp) );
    szTmp[255] = '\0';

    cTmp = szTmp[0];
    cnt = 1;
    while ( '\0' != cTmp && message_buffer_carat < MESSAGEBUFFERSIZE - 1 )
    {
        if ( '_' == cTmp )  cTmp = ' ';

        message_buffer[message_buffer_carat] = cTmp;
        message_buffer_carat++;
        cTmp = szTmp[cnt];
        cnt++;
    }

    message_buffer[message_buffer_carat] = '\0';
    message_buffer_carat++;
    MessageOffset.count++;
}

//--------------------------------------------------------------------------------------------
bool_t release_one_mad( Uint16 imad )
{
    int cnt;
    mad_t * pmad;

    if( !VALID_MAD_RANGE(imad) ) return bfalse;
    pmad = MadList + imad;

    if( !pmad->loaded ) return btrue;

    // free any md2 data
    md2_freeModel( pmad->md2_ptr );

    // free all sounds
    for ( cnt = 0; cnt < MAX_WAVE; cnt++ )
    {
        sound_free_chunk( pmad->wavelist[cnt] );
    }

    // free any local pips
    for( cnt = 0; cnt<MAX_PIP_PER_PROFILE; cnt++ )
    {
        release_one_pip(cnt);
    }

    memset( pmad, 0, sizeof(mad_t) );

    pmad->ai = 0;

    pmad->loaded   = bfalse;
    strncpy( pmad->name, "*NONE*", SDL_arraysize(pmad->name) );

    return btrue;
}

