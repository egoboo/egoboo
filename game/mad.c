#include "mad.h"

#include "log.h"
#include "script_compile.h"
#include "graphic.h"
#include "particle.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
static char cActionName[ACTION_COUNT][2]; // Two letter name code

mad_t   MadList[MAX_PROFILE];

Uint16  msgtotal      = 0;                                  // The number of messages
Uint32  msgtotalindex = 0;                                  // Where to put letter
Uint32  msgindex[MAXTOTALMESSAGE];                          // Where it is
char    msgtext[MESSAGEBUFFERSIZE];                         // The text buffer

char    cFrameName[16];                                     // MD2 Frame Name

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
static void load_all_messages( const char *loadname, Uint16 object );
static void get_message( FILE* fileread );
static void md2_fix_normals( Uint16 modelindex );
static void md2_get_transvertices( Uint16 modelindex );
// static int  vertexconnected( md2_ogl_commandlist_t * pclist, int vertex );

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
Uint16 action_number()
{
    // ZZ> This function returns the number of the action in cFrameName, or
    //     it returns NOACTION if it could not find a match
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

// --------------------------------------------------------------------------------------------
Uint16 action_frame()
{
    // ZZ> This function returns the frame number in the third and fourth characters
    //     of cFrameName
    int number;
    sscanf( &cFrameName[2], "%d", &number );
    return number;
}

// --------------------------------------------------------------------------------------------
Uint16 test_frame_name( char letter )
{
    // ZZ> This function returns btrue if the 4th, 5th, 6th, or 7th letters
    //     of the frame name matches the input argument
    if ( cFrameName[4] == letter ) return btrue;
    if ( cFrameName[4] == 0 ) return bfalse;
    if ( cFrameName[5] == letter ) return btrue;
    if ( cFrameName[5] == 0 ) return bfalse;
    if ( cFrameName[6] == letter ) return btrue;
    if ( cFrameName[6] == 0 ) return bfalse;
    if ( cFrameName[7] == letter ) return btrue;

    return bfalse;
}

// --------------------------------------------------------------------------------------------
void action_copy_correct( Uint16 object, Uint16 actiona, Uint16 actionb )
{
    // ZZ> This function makes sure both actions are valid if either of them
    //     are valid.  It will copy start and ends to mirror the valid action.

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

// --------------------------------------------------------------------------------------------
void action_check_copy( const char* loadname, Uint16 object )
{
    // ZZ> This function copies a model's actions
    FILE *fileread;
    int actiona, actionb;
    char szOne[16], szTwo[16];

    if ( object > MAX_PROFILE || !MadList[object].loaded ) return;

    MadList[object].msgstart = 0;
    fileread = fopen( loadname, "r" );
    if ( fileread )
    {
        while ( goto_colon( NULL, fileread, btrue ) )
        {
            fscanf( fileread, "%s%s", szOne, szTwo );

            actiona = action_which( szOne[0] );
            actionb = action_which( szTwo[0] );

            action_copy_correct( object, actiona + 0, actionb + 0 );
            action_copy_correct( object, actiona + 1, actionb + 1 );
            action_copy_correct( object, actiona + 2, actionb + 2 );
            action_copy_correct( object, actiona + 3, actionb + 3 );
        }

        fclose( fileread );
    }
}

// --------------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------------
void mad_get_framefx( int frame )
{
    // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
    //     Drop timings
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

// --------------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------------
void mad_make_equally_lit( int model )
{
    // ZZ> This function makes ultra low poly models look better
    int frame, cnt, vert;
    if ( MadList[model].loaded )
    {
        frame = MadList[model].md2.framestart;

        for ( cnt = 0; cnt < MadList[model].md2.frames; cnt++ )
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

// --------------------------------------------------------------------------------------------
void load_action_names( const char* loadname )
{
    // ZZ> This function loads all of the 2 letter action names
    FILE* fileread;
    int cnt;
    char first, second;

    fileread = fopen( loadname, "r" );
    if ( fileread )
    {
        cnt = 0;

        while ( cnt < ACTION_COUNT )
        {
            goto_colon( NULL, fileread, bfalse );
            fscanf( fileread, "%c%c", &first, &second );
            cActionName[cnt][0] = first;
            cActionName[cnt][1] = second;
            cnt++;
        }

        fclose( fileread );
    }
}

// --------------------------------------------------------------------------------------------
int load_one_model_profile( const char* tmploadname, Uint16 object, int skin )
{
    int numskins, numicon;
    STRING newloadname;
    int cnt;
    mad_t * pmad;

    if ( object > MAX_PROFILE ) return 0;
    pmad = MadList + object;

    // clear out the mad
    memset( pmad, 0, sizeof(mad_t) );

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

    md2_load_one( newloadname, &(MadList[object].md2) );
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
        sprintf( newloadname, "%s" SLASH_STR "part%d.txt", tmploadname, cnt );

        // Make sure it's referenced properly
        pmad->prtpip[cnt] = load_one_particle_profile( newloadname );
    }

    // Load the skins and icons
    pmad->skinstart = skin;
    numskins = 0;
    numicon = 0;
    for ( cnt = 0; cnt < MAXSKIN; cnt++)
    {
        snprintf( newloadname, sizeof(newloadname), "%s" SLASH_STR "tris%d", tmploadname, cnt );
        if ( INVALID_TX_ID != ego_texture_load( TxTexture + (skin + numskins), newloadname, TRANSCOLOR ) )
        {
            numskins++;

            snprintf( newloadname, sizeof(newloadname), "%s" SLASH_STR "icon%d", tmploadname, cnt );
            if ( INVALID_TX_ID != ego_texture_load( TxIcon + globalicon_count, newloadname, INVALID_KEY ) )
            {
                for ( /* nothing */ ; numicon < numskins; numicon++ )
                {
                    skintoicon[skin + numicon] = globalicon_count;
                    if ( SPELLBOOK == object )
                    {
                        if ( bookicon_count < MAXSKIN )
                        {
                            bookicon[bookicon_count] = globalicon_count;
                            bookicon_count++;
                        }
                    }
                }

                globalicon_count++;
            }
        }
    }

    if ( 0 == numskins )
    {
        // If we didn't get a skin, set it to the water texture
        pmad->skinstart = TX_WATER_TOP;
        numskins = 1;
        if (cfg.dev_mode)
        {
            log_message( "NOTE: Object is missing a skin (%s)!\n", tmploadname );
        }
    }

    pmad->skins = numskins;

    return numskins;
}

// --------------------------------------------------------------------------------------------
void mad_rip_actions( Uint16 object )
{
    // ZZ> This function creates the frame lists for each action based on the
    //     name of each md2 frame in the model

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
    MadList[object].actionstart[ACTION_DA] = MadList[object].md2.framestart;
    MadList[object].actionend[ACTION_DA] = MadList[object].md2.framestart + 1;

    // Now go huntin' to see what each frame is, look for runs of same action
    md2_rip_frame_name( 0 );
    lastaction = action_number();  framesinaction = 0;
    frame = 0;

    while ( frame < MadList[object].md2.frames )
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
                MadList[object].actionstart[lastaction] = MadList[object].md2.framestart + frame - framesinaction;
                MadList[object].actionend[lastaction] = MadList[object].md2.framestart + frame;
            }

            framesinaction = 1;
            lastaction = action;
        }

        mad_get_framefx( MadList[object].md2.framestart + frame );
        frame++;
    }

    // Write the old action
    if ( lastaction < ACTION_COUNT )
    {
        MadList[object].actionvalid[lastaction] = btrue;
        MadList[object].actionstart[lastaction] = MadList[object].md2.framestart + frame - framesinaction;
        MadList[object].actionend[lastaction]   = MadList[object].md2.framestart + frame;
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
    for ( frame = 0; frame < MadList[object].md2.frames; frame++ )
    {
        Md2FrameList[frame+MadList[object].md2.framestart].framelip = 0;
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

// ---------------------------------------------------------------------------------------------
void md2_fix_normals( Uint16 modelindex )
{
    // ZZ> This function helps light not flicker so much
    int cnt, tnc;
    Uint16 indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
    Uint16 indexofnextnextnextnext;
    Uint32 frame;

    frame = MadList[modelindex].md2.framestart;
    cnt = 0;

    while ( cnt < MadList[modelindex].md2.vertices )
    {
        tnc = 0;

        while ( tnc < MadList[modelindex].md2.frames )
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

// ---------------------------------------------------------------------------------------------
void md2_get_transvertices( Uint16 modelindex )
{
    // ZZ> This function gets the number of vertices to transform for a model...
    //     That means every one except the grip ( unconnected ) vertices

    // if (modelindex == 0)
    // {
    //    for ( cnt = 0; cnt < MadList[modelindex].vertices; cnt++ )
    //    {
    //        printf("%d-%d\n", cnt, vertexconnected( modelindex, cnt ) );
    //    }
    // }

    MadList[modelindex].transvertices = MadList[modelindex].md2.vertices;
}

// ---------------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
void load_all_messages( const char *loadname, Uint16 object )
{
    // ZZ> This function loads all of an objects messages
    FILE *fileread;

    MadList[object].msgstart = 0;
    fileread = fopen( loadname, "r" );
    if ( fileread )
    {
        MadList[object].msgstart = msgtotal;

        while ( goto_colon( NULL, fileread, btrue ) )
        {
            get_message( fileread );
        }

        fclose( fileread );
    }
}

// --------------------------------------------------------------------------------------------
void get_message( FILE* fileread )
{
    // ZZ> This function loads a string into the message buffer, making sure it
    //     is null terminated.
    int cnt;
    char cTmp;
    STRING szTmp;

    if ( msgtotalindex >= MESSAGEBUFFERSIZE )
    {
        msgtotalindex = MESSAGEBUFFERSIZE - 1;
        msgtext[msgtotalindex] = '\0';
        return;
    }

    if ( msgtotal >= MAXTOTALMESSAGE )
    {
        return;
    }

    msgindex[msgtotal] = msgtotalindex;
    fscanf( fileread, "%255s", szTmp );
    szTmp[255] = '\0';

    cTmp = szTmp[0];
    cnt = 1;
    while ( '\0' != cTmp && msgtotalindex < MESSAGEBUFFERSIZE - 1 )
    {
        if ( cTmp == '_' )  cTmp = ' ';

        msgtext[msgtotalindex] = cTmp;
        msgtotalindex++;
        cTmp = szTmp[cnt];
        cnt++;
    }

    msgtext[msgtotalindex] = '\0';
    msgtotalindex++;
    msgtotal++;
}
