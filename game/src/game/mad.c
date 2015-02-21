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

/// @file  game/mad.c
/// @brief The files for handling Egoboo's internal model definitions.
/// @details

#include "game/mad.h"
#include "game/graphic.h"
#include "game/graphic_texture.h"
#include "game/script_compile.h"
#include "game/graphics/MD2Model.hpp"
#include "game/entities/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Stack<mad_t, MAX_MAD> MadStack;

static STRING  szModelName     = EMPTY_CSTR;      ///< MD2 Model name
static char    cActionName[ACTION_COUNT][2];      ///< Two letter name code
static STRING  cActionComent[ACTION_COUNT];       ///< Strings explaining the action codes

static int     action_number( const char * cFrameName );
static mad_t * action_check_copy_vfs( mad_t * pmad, const char* loadname );
static mad_t * action_copy_correct( mad_t * pmad, int actiona, int actionb );

static mad_t * mad_get_framefx( mad_t * pmad, const char * cFrameName, int frame );
static mad_t * mad_get_walk_frame( mad_t * pmad, int lip, int action );
static mad_t * mad_make_framelip( mad_t * pmad, int action );
static mad_t * mad_rip_actions( mad_t * pmad );

static mad_t * mad_finalize( mad_t * pmad );
static mad_t * mad_heal_actions( mad_t * pmad, const char * loadname );
static mad_t * mad_make_equally_lit( mad_t * pmad );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static mad_t * mad_ctor( mad_t * pmad );
static mad_t * mad_dtor( mad_t * pmad );
static mad_t * mad_reconstruct( mad_t * pmad );
static bool  mad_free( mad_t * pmad );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void model_system_begin()
{
    MadStack_ctor();
}

//--------------------------------------------------------------------------------------------
void model_system_end()
{
    MadStack_dtor();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void MadStack_ctor()
{
    MAD_REF cnt;

    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        mad_ctor( MadStack.get_ptr( cnt ) );
    }
}

//--------------------------------------------------------------------------------------------
void MadStack_dtor()
{
    MAD_REF cnt;

    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        mad_dtor( MadStack.get_ptr( cnt ) );
    }
}

//--------------------------------------------------------------------------------------------
void MadStack_reinit()
{
    // Re-initialize all mad_t.
    for (MAD_REF i = 0; i < MAX_MAD; i++)
    {
        mad_t *mad = MadStack.get_ptr(i);

        // Blank out all the data, including the entity data.
		BLANK_STRUCT_PTR(mad);

        mad_reconstruct(mad);
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int action_number( const char * cFrameName )
{
    /// @author ZZ
    /// @details This function returns the number of the action in cFrameName, or
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

//--------------------------------------------------------------------------------------------
mad_t * action_copy_correct( mad_t * pmad, int actiona, int actionb )
{
    /// @author ZZ
    /// @details This function makes sure both actions are valid if either of them
    ///    are valid.  It will copy start and ends to mirror the valid action.

    if ( NULL == pmad ) return pmad;

    if ( actiona < 0 || actiona >= ACTION_COUNT ) return pmad;
    if ( actionb < 0 || actionb >= ACTION_COUNT ) return pmad;

    // With the new system using the action_map, this is all that is really necessary
    if ( ACTION_COUNT == pmad->action_map[actiona] )
    {
        if ( pmad->action_valid[actionb] )
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
        if ( pmad->action_valid[actiona] )
        {
            pmad->action_map[actionb] = actiona;
        }
        else if ( ACTION_COUNT != pmad->action_map[actiona] )
        {
            pmad->action_map[actionb] = pmad->action_map[actiona];
        }
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
mad_t * action_check_copy_vfs( mad_t * pmad, const char* loadname )
{
    /// @author ZZ
    /// @details This function copies a model's actions
    int actiona, actionb;
    char szOne[16] = EMPTY_CSTR;
    char szTwo[16] = EMPTY_CSTR;

    if ( NULL == pmad ) return pmad;

    ReadContext ctxt(loadname);
    if (!ctxt.ensureOpen())
    {
        return pmad;
    }
    while (ctxt.skipToColon(true))
    {
        vfs_read_string_lit( ctxt, szOne, SDL_arraysize( szOne ) );
        actiona = action_which( szOne[0] );

        vfs_read_string_lit( ctxt, szTwo, SDL_arraysize( szTwo ) );
        actionb = action_which( szTwo[0] );

        action_copy_correct( pmad, actiona + 0, actionb + 0 );
        action_copy_correct( pmad, actiona + 1, actionb + 1 );
        action_copy_correct( pmad, actiona + 2, actionb + 2 );
        action_copy_correct( pmad, actiona + 3, actionb + 3 );
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
int action_which( char cTmp )
{
    /// @author ZZ
    /// @details This function changes a letter into an action code
    int action;

    switch ( char_toupper( cTmp ) )
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
            // case 'W': action = ACTION_WA; break;   /// @note ZF@> Can't do this, attack animation WALK is used for doing nothing (for example charging spells)
        case 'H': action = ACTION_HA; break;
        case 'K': action = ACTION_KA; break;
        default:  action = ACTION_DA; break;
    }

    return action;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int mad_get_action( mad_t * pmad, int action )
{
    /// @author BB
    /// @details translate the action that was given into a valid action for the model
    ///
    /// returns ACTION_COUNT on a complete failure, or the default ACTION_DA if it exists

    int     retval;

    if ( NULL == pmad ) return ACTION_COUNT;

    // you are pretty much guaranteed that ACTION_DA will be valid for a model,
    // I guess it could be invalid if the model had no frames or something
    retval = ACTION_DA;
    if ( !pmad->action_valid[ACTION_DA] )
    {
        retval = ACTION_COUNT;
    }

    // check for a valid action range
    if ( action < 0 || action > ACTION_COUNT ) return retval;

    // track down a valid value
    if ( pmad->action_valid[action] )
    {
        retval = action;
    }
    else if ( ACTION_COUNT != pmad->action_map[action] )
    {
        int cnt, tnc;

        // do a "recursive" search for a valid action
        // we should never really have to check more than once if the map is prepared
        // properly BUT you never can tell. Make sure we do not get a runaway loop by
        // you never go farther than ACTION_COUNT steps and that you never see the
        // original action again

        tnc = pmad->action_map[action];
        for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
        {
            if ( tnc >= ACTION_COUNT || tnc < 0 || tnc == action ) break;

            if ( pmad->action_valid[tnc] )
            {
                retval = tnc;
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD mad_get_madfx( mad_t * pmad, int action )
{
    BIT_FIELD retval = EMPTY_BIT_FIELD;

    if ( NULL == pmad ) return 0;

    if ( nullptr == pmad->md2_ptr ) return 0;

    if ( action < 0 || action >= ACTION_COUNT ) return 0;

    if ( !pmad->action_valid[action] ) return 0;

    const std::vector<MD2_Frame> &frames = pmad->md2_ptr->getFrames();
    for (int cnt = pmad->action_stt[action]; cnt <= pmad->action_end[action]; cnt++)
    {
        SET_BIT(retval, frames[cnt].framefx);
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
mad_t *  mad_get_walk_frame( mad_t * pmad, int lip, int action )
{
    /// @author ZZ
    /// @details This helps make walking look right
    int frame = 0;
    int action_count;
    int action_stt, action_end;

    if ( NULL == pmad ) return pmad;

    action = mad_get_action( pmad, action );
    if ( action >= ACTION_COUNT || !pmad->action_valid[action] )
    {
        // make a fake action
        action_stt = pmad->action_stt[ACTION_DA];
        action_end = pmad->action_stt[ACTION_DA];
    }
    else
    {
        action_stt = pmad->action_stt[action];
        action_end = pmad->action_end[action];
    }

    // count the number of frames
    action_count = 1 + ( action_end - action_stt );

    // scan through all the frames of the framelip
    for ( frame = 0; frame < FRAMELIP_COUNT; frame++ )
    {
        int framealong = 0;

        if ( action_count > 0 )
        {
            // this SHOULD produce a number between 0 and (action_count - 1),
            // but there could be rounding error
            framealong = ( frame * action_count ) / FRAMELIP_COUNT;

            framealong = std::min( framealong, action_count - 1 );
        }

        pmad->framelip_to_walkframe[lip][frame] = action_stt + framealong;
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
mad_t * mad_get_framefx( mad_t * pmad, const char * cFrameName, int frame )
{
    /// @author ZZ
    /// @details This function figures out the IFrame invulnerability, and Attack, Grab, and
    ///               Drop timings
    ///
    ///          BB@> made a bit more sturdy parser that is not going to confuse strings like "LCRA"
    ///               which would not crop up if the convention of L or R going first was applied universally.
    ///               However, there are existing (and common) models which use the opposite convention, leading
    ///               to the possibility that an fx string "LARC" could be interpreted as ACTLEFT, CHARRIGHT, *and*
    ///               ACTRIGHT.

    BIT_FIELD fx = 0;
    char name_action[16], name_fx[16];
    int name_count;
    int fields;
    int cnt;

    static int token_count = -1;
    static const char * tokens[] = { "I", "S", "F", "P", "A", "G", "D", "C",          /* the normal command tokens */
                                     "LA", "LG", "LD", "LC", "RA", "RG", "RD", "RC", NULL
                                   }; /* the "bad" token aliases */

    const char * ptmp, * ptmp_end;
    char *paction, *paction_end;

    if ( NULL == pmad ) return pmad;

    if ( pmad->md2_ptr == nullptr ) return pmad;

    // check for a valid frame number
    if(frame >= pmad->md2_ptr->getFrames().size()){
        return pmad; 
    }
    MD2_Frame &pframe = pmad->md2_ptr->getFrames()[frame];

    // this should only be initializwd the first time through
    if ( token_count < 0 )
    {
        token_count = 0;
        for ( cnt = 0; NULL != tokens[token_count] && cnt < 256; cnt++ ) token_count++;
    }

    // set the default values
    fx = 0;
    pframe.framefx = fx;

    // check for a non-trivial frame name
    if ( !VALID_CSTR( cFrameName ) ) return pmad;

    // skip over whitespace
    ptmp     = cFrameName;
    ptmp_end = cFrameName + 16;
    for ( /* nothing */; ptmp < ptmp_end && isspace(( unsigned )( *ptmp ) ); ptmp++ ) {};

    // copy non-numerical text
    paction     = name_action;
    paction_end = name_action + 16;
    for ( /* nothing */; ptmp < ptmp_end && paction < paction_end && !isspace(( unsigned )( *ptmp ) ); ptmp++, paction++ )
    {
        if ( isdigit(( unsigned )( *ptmp ) ) ) break;
        *paction = *ptmp;
    }
    if ( paction < paction_end ) *paction = CSTR_END;

    name_fx[0] = CSTR_END;
    fields = sscanf( ptmp, "%d %15s", &name_count, name_fx );
    name_action[15] = CSTR_END;
    name_fx[15] = CSTR_END;

    // check for a non-trivial fx command
    if ( !VALID_CSTR( name_fx ) ) return pmad;

    // scan the fx string for valid commands
    ptmp     = name_fx;
    ptmp_end = name_fx + 15;
    while ( CSTR_END != *ptmp && ptmp < ptmp_end )
    {
        size_t len;
        int token_index = -1;
        for ( cnt = 0; cnt < token_count; cnt++ )
        {
            len = strlen( tokens[cnt] );
            if ( 0 == strncmp( tokens[cnt], ptmp, len ) )
            {
                ptmp += len;
                token_index = cnt;
                break;
            }
        }

        if ( -1 == token_index )
        {
            //log_debug( "Model %s, frame %d, frame name \"%s\" has unknown frame effects command \"%s\"\n", szModelName, frame, cFrameName, ptmp );
            ptmp++;
        }
        else
        {
            bool bad_form = false;
            switch ( token_index )
            {
                case  0: // "I" == invulnerable
                    SET_BIT( fx, MADFX_INVICTUS );
                    break;

                case  1: // "S" == stop
                    SET_BIT( fx, MADFX_STOP );
                    break;

                case  2: // "F" == footfall
                    SET_BIT( fx, MADFX_FOOTFALL );
                    break;

                case  3: // "P" == poof
                    SET_BIT( fx, MADFX_POOF );
                    break;

                case  4: // "A" == action

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        SET_BIT( fx, ( 'L' == *ptmp ) ? MADFX_ACTLEFT : MADFX_ACTRIGHT );
                        ptmp++;
                    }
                    break;

                case  5: // "G" == grab

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        SET_BIT( fx, ( 'L' == *ptmp ) ? MADFX_GRABLEFT : MADFX_GRABRIGHT );
                        ptmp++;
                    }
                    break;

                case  6: // "D" == drop

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        fx |= ( 'L' == *ptmp ) ? MADFX_DROPLEFT : MADFX_DROPRIGHT;
                        ptmp++;
                    }
                    break;

                case  7: // "C" == grab a character

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        SET_BIT( fx, ( 'L' == *ptmp ) ? MADFX_CHARLEFT : MADFX_CHARRIGHT );
                        ptmp++;
                    }
                    break;

                case  8: // "LA"
                    bad_form = true;
                    SET_BIT( fx, MADFX_ACTLEFT );
                    break;

                case  9: // "LG"
                    bad_form = true;
                    SET_BIT( fx, MADFX_GRABLEFT );
                    break;

                case 10: // "LD"
                    bad_form = true;
                    SET_BIT( fx, MADFX_DROPLEFT );
                    break;

                case 11: // "LC"
                    bad_form = true;
                    SET_BIT( fx, MADFX_CHARLEFT );
                    break;

                case 12: // "RA"
                    bad_form = true;
                    SET_BIT( fx, MADFX_ACTRIGHT );
                    break;

                case 13: // "RG"
                    bad_form = true;
                    SET_BIT( fx, MADFX_GRABRIGHT );
                    break;

                case 14: // "RD"
                    bad_form = true;
                    SET_BIT( fx, MADFX_DROPRIGHT );
                    break;

                case 15: // "RC"
                    bad_form = true;
                    SET_BIT( fx, MADFX_CHARRIGHT );
                    break;
            }

            if ( bad_form && -1 != token_index )
            {
                log_warning( "Model %s, frame %d, frame name \"%s\" has a frame effects command in an improper configuration \"%s\"\n", szModelName, frame, cFrameName, tokens[token_index] );
            }
        }
    }

    pframe.framefx = fx;

    return pmad;
}

//--------------------------------------------------------------------------------------------
mad_t * mad_make_framelip( mad_t * pmad, int action )
{
    /// @author ZZ
    /// @details This helps make walking look right

    if ( NULL == pmad || nullptr == pmad->md2_ptr ) return pmad;

    action = mad_get_action( pmad, action );
    if ( ACTION_COUNT == action ) return pmad;

    if ( !pmad->action_valid[action] ) return pmad;

    // grab the animation info
    int action_stt = pmad->action_stt[action];
    int action_end = pmad->action_end[action];
    int action_count = 1 + ( action_end - action_stt );

    // scan through all the frames of the action
    for (int frame = action_stt; frame <= action_end; frame++)
    {
        // grab a valid frame
        if (frame >= pmad->md2_ptr->getFrames().size()) break;

        // calculate the framelip.
        // this should produce a number between 0 and FRAMELIP_COUNT-1, but
        // watch out for possible rounding errors
        int framelip = (( frame - action_stt ) * FRAMELIP_COUNT ) / action_count;

        // limit the framelip to the valid range
        pmad->md2_ptr->getFrames()[frame].framelip = std::min(framelip, FRAMELIP_COUNT - 1);
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
mad_t * mad_make_equally_lit( mad_t * pmad )
{
    /// @author ZZ
    /// @details This function makes ultra low poly models look better

    if ( NULL == pmad || pmad->md2_ptr == nullptr ) return pmad;

    pmad->md2_ptr->makeEquallyLit();

    return pmad;
}

//--------------------------------------------------------------------------------------------
void load_action_names_vfs( const char* loadname )
{
    /// @author ZZ
    /// @details This function loads all of the 2 letter action names
    ReadContext ctxt(loadname);
    if (!ctxt.ensureOpen())
    {
        return;
    }

    for (size_t cnt = 0; cnt < ACTION_COUNT; ++cnt)
    {
        if (ctxt.skipToColon(false))
        {
            // The 1st letter.
            char frst = ctxt.readPrintable();
            // The 2nd letter.
            char scnd = ctxt.readPrintable();
            // Read comment.
            std::string comment = ctxt.readToEndOfLine();

            cActionName[cnt][0] = frst;
            cActionName[cnt][1] = scnd;
            strncpy(cActionComent[cnt], comment.c_str(), SDL_arraysize(cActionComent[cnt]));
        }
        else
        {
            cActionName[cnt][0] = CSTR_END;
            cActionComent[cnt][0] = CSTR_END;
        }
    }
}

//--------------------------------------------------------------------------------------------
MAD_REF load_one_model_profile_vfs( const char* tmploadname, const MAD_REF imad )
{
    mad_t * pmad;
    STRING  newloadname;

    if ( !VALID_MAD_RANGE( imad ) ) return INVALID_MAD_REF;
    pmad = MadStack.get_ptr( imad );

    // clear out the mad
    mad_reconstruct( pmad );

    // mark it as used
    pmad->loaded = true;

    // Make up a name for the model...  IMPORT\TEMP0000.OBJ
    strncpy( pmad->name, tmploadname, SDL_arraysize( pmad->name ) );
    pmad->name[ SDL_arraysize( pmad->name ) - 1 ] = CSTR_END;

    // Load the imad model
    make_newloadname( tmploadname, "/tris.md2", newloadname );

    // do this for now. maybe make it dynamic later...
    //pmad->md2_ref = imad;

    // load the model from the file
    pmad->md2_ptr = MD2Model::loadFromFile(newloadname);

    // set the model's file name
    szModelName[0] = CSTR_END;
    if (pmad->md2_ptr != nullptr)
    {
        strncpy(szModelName, newloadname, SDL_arraysize(szModelName));

        /// @author BB
        /// @details Egoboo md2 models were designed with 1 tile = 32x32 units, but internally Egoboo uses
        ///      1 tile = 128x128 units. Previously, this was handled by sprinkling a bunch of
        ///      commands that multiplied various quantities by 4 or by 4.125 throughout the code.
        ///      It was very counterintuitive, and caused me no end of headaches...  Of course the
        ///      solution is to scale the model!
        pmad->md2_ptr->scaleModel(-3.5f, 3.5f, 3.5f);
    }

    // Create the actions table for this imad
    mad_rip_actions( pmad );
    mad_heal_actions( pmad, tmploadname );
    mad_finalize( pmad );

    return imad;
}

//--------------------------------------------------------------------------------------------
mad_t * mad_heal_actions( mad_t * pmad, const char * tmploadname )
{
    STRING newloadname;

    if ( NULL == pmad ) return pmad;

    // Make sure actions are made valid if a similar one exists
    action_copy_correct( pmad, ACTION_DA, ACTION_DB );  // All dances should be safe
    action_copy_correct( pmad, ACTION_DB, ACTION_DC );
    action_copy_correct( pmad, ACTION_DC, ACTION_DD );
    action_copy_correct( pmad, ACTION_DB, ACTION_DC );
    action_copy_correct( pmad, ACTION_DA, ACTION_DB );
    action_copy_correct( pmad, ACTION_UA, ACTION_UB );
    action_copy_correct( pmad, ACTION_UB, ACTION_UC );
    action_copy_correct( pmad, ACTION_UC, ACTION_UD );
    action_copy_correct( pmad, ACTION_TA, ACTION_TB );
    action_copy_correct( pmad, ACTION_TC, ACTION_TD );
    action_copy_correct( pmad, ACTION_CA, ACTION_CB );
    action_copy_correct( pmad, ACTION_CC, ACTION_CD );
    action_copy_correct( pmad, ACTION_SA, ACTION_SB );
    action_copy_correct( pmad, ACTION_SC, ACTION_SD );
    action_copy_correct( pmad, ACTION_BA, ACTION_BB );
    action_copy_correct( pmad, ACTION_BC, ACTION_BD );
    action_copy_correct( pmad, ACTION_LA, ACTION_LB );
    action_copy_correct( pmad, ACTION_LC, ACTION_LD );
    action_copy_correct( pmad, ACTION_XA, ACTION_XB );
    action_copy_correct( pmad, ACTION_XC, ACTION_XD );
    action_copy_correct( pmad, ACTION_FA, ACTION_FB );
    action_copy_correct( pmad, ACTION_FC, ACTION_FD );
    action_copy_correct( pmad, ACTION_PA, ACTION_PB );
    action_copy_correct( pmad, ACTION_PC, ACTION_PD );
    action_copy_correct( pmad, ACTION_ZA, ACTION_ZB );
    action_copy_correct( pmad, ACTION_ZC, ACTION_ZD );
    action_copy_correct( pmad, ACTION_WA, ACTION_WB );
    action_copy_correct( pmad, ACTION_WB, ACTION_WC );
    action_copy_correct( pmad, ACTION_WC, ACTION_WD );
    action_copy_correct( pmad, ACTION_DA, ACTION_WD );  // All walks should be safe
    action_copy_correct( pmad, ACTION_WC, ACTION_WD );
    action_copy_correct( pmad, ACTION_WB, ACTION_WC );
    action_copy_correct( pmad, ACTION_WA, ACTION_WB );
    action_copy_correct( pmad, ACTION_JA, ACTION_JB );
    action_copy_correct( pmad, ACTION_JB, ACTION_JC );
    action_copy_correct( pmad, ACTION_DA, ACTION_JC );  // All jumps should be safe
    action_copy_correct( pmad, ACTION_JB, ACTION_JC );
    action_copy_correct( pmad, ACTION_JA, ACTION_JB );
    action_copy_correct( pmad, ACTION_HA, ACTION_HB );
    action_copy_correct( pmad, ACTION_HB, ACTION_HC );
    action_copy_correct( pmad, ACTION_HC, ACTION_HD );
    action_copy_correct( pmad, ACTION_HB, ACTION_HC );
    action_copy_correct( pmad, ACTION_HA, ACTION_HB );
    action_copy_correct( pmad, ACTION_KA, ACTION_KB );
    action_copy_correct( pmad, ACTION_KB, ACTION_KC );
    action_copy_correct( pmad, ACTION_KC, ACTION_KD );
    action_copy_correct( pmad, ACTION_KB, ACTION_KC );
    action_copy_correct( pmad, ACTION_KA, ACTION_KB );
    action_copy_correct( pmad, ACTION_MH, ACTION_MI );
    action_copy_correct( pmad, ACTION_DA, ACTION_MM );
    action_copy_correct( pmad, ACTION_MM, ACTION_MN );

    // Copy entire actions to save frame space COPY.TXT
    make_newloadname( tmploadname, "/copy.txt", newloadname );
    action_check_copy_vfs( pmad, newloadname );

    return pmad;
}

//--------------------------------------------------------------------------------------------
mad_t * mad_finalize( mad_t * pmad )
{
    if ( NULL == pmad ) return pmad;

    if (nullptr == pmad->md2_ptr) return pmad;

    // Create table for doing transition from one type of walk to another...
    // Clear 'em all to start
    for(MD2_Frame &frame : pmad->md2_ptr->getFrames())
    {
        frame.framelip = 0;
    }

    // Need to figure out how far into action each frame is
    mad_make_framelip( pmad, ACTION_WA );
    mad_make_framelip( pmad, ACTION_WB );
    mad_make_framelip( pmad, ACTION_WC );

    // Now do the same, in reverse, for walking animations
    mad_get_walk_frame( pmad, LIPDA, ACTION_DA );
    mad_get_walk_frame( pmad, LIPWA, ACTION_WA );
    mad_get_walk_frame( pmad, LIPWB, ACTION_WB );
    mad_get_walk_frame( pmad, LIPWC, ACTION_WC );

    return pmad;
}

//--------------------------------------------------------------------------------------------
mad_t * mad_rip_actions( mad_t * pmad )
{
    /// \author ZZ
    /// \details  This function creates the iframe lists for each action_now based on the
    ///    name of each md2 iframe in the model
    int action_now, last_action;

    if ( NULL == pmad ) return pmad;

    if ( nullptr == pmad->md2_ptr ) return pmad;

    // Clear out all actions and reset to invalid
    for ( action_now = 0; action_now < ACTION_COUNT; action_now++ )
    {
        pmad->action_map[action_now]   = ACTION_COUNT;
        pmad->action_stt[action_now]   = -1;
        pmad->action_end[action_now]   = -1;
        pmad->action_valid[action_now] = false;
    }


    // is there anything to do?
    if ( pmad->md2_ptr->getFrames().empty() ) return pmad;

    // Make a default dance action (ACTION_DA) to be the 1st frame of the animation
    pmad->action_map[ACTION_DA]   = ACTION_DA;
    pmad->action_valid[ACTION_DA] = true;
    pmad->action_stt[ACTION_DA]   = 0;
    pmad->action_end[ACTION_DA]   = 0;

    // Now go huntin' to see what each iframe is, look for runs of same action_now
    last_action = ACTION_COUNT;
    int iframe = 0;
    for(const MD2_Frame &frame : pmad->md2_ptr->getFrames())
    {
        action_now = action_number(frame.name);
        
        if (action_now == NOACTION) {
            log_warning("Got no action for frame name '%s', ignoring\n", frame.name);
            iframe++;
            continue;
        }

        if ( last_action != action_now )
        {
            // start a new action
            pmad->action_map[action_now]   = action_now;
            pmad->action_stt[action_now]   = iframe;
            pmad->action_end[action_now]   = iframe;
            pmad->action_valid[action_now] = true;

            last_action = action_now;
        }
        else
        {
            // keep expanding the action_end until the end of the action
            pmad->action_end[action_now] = iframe;
        }

        mad_get_framefx(pmad, frame.name, iframe);
        iframe++;
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool mad_free( mad_t * pmad )
{
    /// Free all allocated memory

    if ( !LOADED_PMAD( pmad ) ) return false;

    pmad->md2_ptr = nullptr;

    return true;
}

//--------------------------------------------------------------------------------------------
mad_t * mad_reconstruct( mad_t * pmad )
{
    if ( NULL == pmad ) return NULL;

    mad_dtor( pmad );
    mad_ctor( pmad );

    return pmad;
}

//--------------------------------------------------------------------------------------------
mad_t * mad_ctor( mad_t * pmad )
{
    /// @author BB
    /// @details initialize the data to safe values

    int action;

    if ( NULL == pmad ) return NULL;

    // this macro makes all "= 0", "= false", and "= 0.0f" statements redundant
    BLANK_STRUCT_PTR( pmad )

    strncpy( pmad->name, "*NONE*", SDL_arraysize( pmad->name ) );

    // Clear out all actions and reset to invalid
    for ( action = 0; action < ACTION_COUNT; action++ )
    {
        pmad->action_map[action] = ACTION_COUNT;
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
mad_t * mad_dtor( mad_t * pmad )
{
    /// @author BB
    /// @details deinitialize the character data

    if ( !LOADED_PMAD( pmad ) ) return NULL;

    // free any allocated data
    mad_free( pmad );

    // blank the data
    BLANK_STRUCT_PTR( pmad )

    // "destruct" the base object
    pmad->loaded = false;

    return pmad;
}

//--------------------------------------------------------------------------------------------
void MadStack_reconstruct_all()
{
    MAD_REF cnt;

    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        mad_reconstruct( MadStack.get_ptr( cnt ) );
    }
}

//--------------------------------------------------------------------------------------------
void MadStack_release_all()
{
    MAD_REF cnt;

    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        MadStack_release_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool MadStack_release_one( const MAD_REF imad )
{
    mad_t * pmad;

    if ( !VALID_MAD_RANGE( imad ) ) return false;
    pmad = MadStack.get_ptr( imad );

    if ( !pmad->loaded ) return true;

    // free any md2 data
    mad_reconstruct( pmad );

    return true;
}

//--------------------------------------------------------------------------------------------
int randomize_action( int action, int slot )
{
    /// @author BB
    /// @details this function actually determines whether the action fillows the
    ///               pattern of ACTION_?A, ACTION_?B, ACTION_?C, ACTION_?D, with
    ///               A and B being for the left hand, and C and D being for the right hand

    int diff = 0;

    // a valid slot?
    if ( slot < 0 || slot >= SLOT_COUNT ) return action;

    // a valid action?
    if ( action < 0 || action >= ACTION_COUNT ) return false;

    diff = slot * 2;

    //---- non-randomizable actions
    if ( ACTION_MG == action ) return action;       // MG      = Open Chest
    else if ( ACTION_MH == action ) return action;       // MH      = Sit
    else if ( ACTION_MI == action ) return action;       // MI      = Ride
    else if ( ACTION_MJ == action ) return action;       // MJ      = Object Activated
    else if ( ACTION_MK == action ) return action;       // MK      = Snoozing
    else if ( ACTION_ML == action ) return action;       // ML      = Unlock
    else if ( ACTION_JA == action ) return action;       // JA      = Jump
    else if ( ACTION_RA == action ) return action;       // RA      = Roll
    else if ( ACTION_IS_TYPE( action, W ) ) return action;  // WA - WD = Walk

    //---- do a couple of special actions that have left/right
    else if ( ACTION_EA == action || ACTION_EB == action ) action = ACTION_JB + slot;   // EA/EB = Evade left/right
    else if ( ACTION_JB == action || ACTION_JC == action ) action = ACTION_JB + slot;   // JB/JC = Dropped item left/right
    else if ( ACTION_MA == action || ACTION_MB == action ) action = ACTION_MA + slot;   // MA/MB = Drop left/right item
    else if ( ACTION_MC == action || ACTION_MD == action ) action = ACTION_MC + slot;   // MC/MD = Slam left/right
    else if ( ACTION_ME == action || ACTION_MF == action ) action = ACTION_ME + slot;   // ME/MF = Grab item left/right
    else if ( ACTION_MM == action || ACTION_MN == action ) action = ACTION_MM + slot;   // MM/MN = Held left/right

    //---- actions that can be randomized, but are not left/right sensitive
    // D = dance
    else if ( ACTION_IS_TYPE( action, D ) ) action = ACTION_TYPE( D ) + generate_randmask( 0, 3 );

    //---- handle all the normal attack/defense animations
    // U = unarmed
    else if ( ACTION_IS_TYPE( action, U ) ) action = ACTION_TYPE( U ) + diff + generate_randmask( 0, 1 );
    // T = thrust
    else if ( ACTION_IS_TYPE( action, T ) ) action = ACTION_TYPE( T ) + diff + generate_randmask( 0, 1 );
    // C = chop
    else if ( ACTION_IS_TYPE( action, C ) ) action = ACTION_TYPE( C ) + diff + generate_randmask( 0, 1 );
    // S = slice
    else if ( ACTION_IS_TYPE( action, S ) ) action = ACTION_TYPE( S ) + diff + generate_randmask( 0, 1 );
    // B = bash
    else if ( ACTION_IS_TYPE( action, B ) ) action = ACTION_TYPE( B ) + diff + generate_randmask( 0, 1 );
    // L = longbow
    else if ( ACTION_IS_TYPE( action, L ) ) action = ACTION_TYPE( L ) + diff + generate_randmask( 0, 1 );
    // X = crossbow
    else if ( ACTION_IS_TYPE( action, X ) ) action = ACTION_TYPE( X ) + diff + generate_randmask( 0, 1 );
    // F = fling
    else if ( ACTION_IS_TYPE( action, F ) ) action = ACTION_TYPE( F ) + diff + generate_randmask( 0, 1 );
    // P = parry/block
    else if ( ACTION_IS_TYPE( action, P ) ) action = ACTION_TYPE( P ) + diff + generate_randmask( 0, 1 );
    // Z = zap
    else if ( ACTION_IS_TYPE( action, Z ) ) action = ACTION_TYPE( Z ) + diff + generate_randmask( 0, 1 );

    //---- these are passive actions
    // H = hurt
    else if ( ACTION_IS_TYPE( action, H ) ) action = ACTION_TYPE( H ) + generate_randmask( 0, 3 );
    // K = killed
    else if ( ACTION_IS_TYPE( action, K ) ) action = ACTION_TYPE( K ) + generate_randmask( 0, 3 );

    return action;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int mad_get_action_ref( const MAD_REF imad, int action )
{
    /// @author BB
    /// @details translate the action that was given into a valid action for the model
    ///
    /// returns ACTION_COUNT on a complete failure, or the default ACTION_DA if it exists

    if ( !LOADED_MAD( imad ) ) return ACTION_COUNT;

    return mad_get_action( MadStack.get_ptr( imad ), action );
}

//--------------------------------------------------------------------------------------------
Uint32 mad_get_madfx_ref( const MAD_REF imad, int action )
{
    if ( !LOADED_MAD( imad ) ) return 0;

    return mad_get_madfx( MadStack.get_ptr( imad ), action );
}

//--------------------------------------------------------------------------------------------
void mad_make_equally_lit_ref( const MAD_REF imad )
{
    if ( LOADED_MAD( imad ) )
    {
        mad_t *mad = MadStack.get_ptr(imad);
        if(mad->md2_ptr != nullptr) {
            mad->md2_ptr->makeEquallyLit();
        }
    }
}

//--------------------------------------------------------------------------------------------
// OBSOLETE
//--------------------------------------------------------------------------------------------

//Uint16 test_frame_name( char letter )
//{
//    /// @author ZZ
/// @details This function returns true if the 4th, 5th, 6th, or 7th letters
//    ///    of the frame name matches the input argument
//
//    if ( letter   == cFrameName[4] ) return true;
//    if ( CSTR_END == cFrameName[4] ) return false;
//    if ( letter   == cFrameName[5] ) return true;
//    if ( CSTR_END == cFrameName[5] ) return false;
//    if ( letter   == cFrameName[6] ) return true;
//    if ( CSTR_END == cFrameName[6] ) return false;
//    if ( letter   == cFrameName[7] ) return true;
//
//    return false;
//}

//--------------------------------------------------------------------------------------------
//void md2_fix_normals( const MAD_REF imad )
//{
//    /// @author ZZ
/// @details This function helps light not flicker so much
//    int cnt, tnc;
//    int indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
//    int indexofnextnextnextnext;
//    int frame;
//
//    frame = ego_md2_data[MadStack.lst[imad].md2_ref].framestart;
//    cnt = 0;
//
//    while ( cnt < ego_md2_data[MadStack.lst[imad].md2_ref].vertex_lst )
//    {
//        tnc = 0;
//
//        while ( tnc < ego_md2_data[MadStack.lst[imad].md2_ref].frames )
//        {
//            indexofcurrent = pframe->vrta[cnt];
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

//--------------------------------------------------------------------------------------------
//void md2_get_transvertices( const MAD_REF imad )
//{
//    /// @author ZZ
/// @details This function gets the number of vertices to transform for a model...
//    //    That means every one except the grip ( unconnected ) vertices
//
//    // if (imad == 0)
//    // {
//    //   for ( cnt = 0; cnt < MadStack.lst[imad].vertex_lst; cnt++ )
//    //   {
//    //       printf("%d-%d\n", cnt, vertexconnected( imad, cnt ) );
//    //   }
//    // }
//
//    MadStack.lst[imad].transvertices = ego_md2_data[MadStack.lst[imad].md2_ref].vertex_lst;
//}

////--------------------------------------------------------------------------------------------
//int vertexconnected( md2_ogl_commandlist_t * pclist, int vertex )
//{
//    /// @author ZZ
/// @details This function returns 1 if the model vertex is connected, 0 otherwise
//    int cnt, tnc, entry;
//
//    entry = 0;
//
//    for ( cnt = 0; cnt < pclist->count; cnt++ )
//    {
//        for ( tnc = 0; tnc < pclist->size[cnt]; tnc++ )
//        {
//            if ( pclist->vrt[entry] == vertex )
//            {
//                // The vertex is used
//                return 1;
//            }
//
//            entry++;
//        }
//    }
//
//    // The vertex is not used
//    return 0;
//}

////--------------------------------------------------------------------------------------------
//int action_frame()
//{
//    /// @author ZZ
/// @details This function returns the frame number in the third and fourth characters
//    ///    of cFrameName
//
//    int number;
//    char tmp_str[16];
//
//    sscanf( cFrameName, " %15s%d", tmp_str, &number );
//
//    return number;
//}

