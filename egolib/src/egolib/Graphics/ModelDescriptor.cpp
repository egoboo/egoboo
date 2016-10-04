#include "ModelDescriptor.hpp"
#include "egolib/Graphics/MD2Model.hpp"
#include "egolib/strutil.h"
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/fileutil.h"
#include "egolib/Logic/ObjectSlot.hpp"

namespace Ego
{

constexpr int STRING_SWITCH(int a, int b)
{
    return a | (b << 16);
}

ModelAction ModelDescriptor::stringToAction(const std::string &action) const
{
    if(action.size() >= 2) {

        switch(STRING_SWITCH(action[0], action[1]))
        {
            case STRING_SWITCH('D','A'): return ACTION_DA;         //"Dance ( Standing still )"
            case STRING_SWITCH('D','B'): return ACTION_DB;         //"Dance ( Bored )"
            case STRING_SWITCH('D','C'): return ACTION_DC;         //"Dance ( Bored )"
            case STRING_SWITCH('D','D'): return ACTION_DD;         //"Dance ( Bored )"
            case STRING_SWITCH('U','A'): return ACTION_UA;         //"Unarmed"
            case STRING_SWITCH('U','B'): return ACTION_UB;         //"Unarmed"
            case STRING_SWITCH('U','C'): return ACTION_UC;         //"Unarmed"
            case STRING_SWITCH('U','D'): return ACTION_UD;         //"Unarmed"
            case STRING_SWITCH('T','A'): return ACTION_TA;         //"Thrust"
            case STRING_SWITCH('T','B'): return ACTION_TB;         //"Thrust"
            case STRING_SWITCH('T','C'): return ACTION_TC;         //"Thrust"
            case STRING_SWITCH('T','D'): return ACTION_TD;         //"Thrust"
            case STRING_SWITCH('C','A'): return ACTION_CA;         //"Crush"
            case STRING_SWITCH('C','B'): return ACTION_CB;         //"Crush"
            case STRING_SWITCH('C','C'): return ACTION_CC;         //"Crush"
            case STRING_SWITCH('C','D'): return ACTION_CD;         //"Crush"
            case STRING_SWITCH('S','A'): return ACTION_SA;         //"Slash"
            case STRING_SWITCH('S','B'): return ACTION_SB;         //"Slash"
            case STRING_SWITCH('S','C'): return ACTION_SC;         //"Slash"
            case STRING_SWITCH('S','D'): return ACTION_SD;         //"Slash"
            case STRING_SWITCH('B','A'): return ACTION_BA;         //"Bash"
            case STRING_SWITCH('B','B'): return ACTION_BB;         //"Bash"
            case STRING_SWITCH('B','C'): return ACTION_BC;         //"Bash"
            case STRING_SWITCH('B','D'): return ACTION_BD;         //"Bash"
            case STRING_SWITCH('L','A'): return ACTION_LA;         //"Longbow"
            case STRING_SWITCH('L','B'): return ACTION_LB;         //"Longbow"
            case STRING_SWITCH('L','C'): return ACTION_LC;         //"Longbow"
            case STRING_SWITCH('L','D'): return ACTION_LD;         //"Longbow"
            case STRING_SWITCH('X','A'): return ACTION_XA;         //"Crossbow"
            case STRING_SWITCH('X','B'): return ACTION_XB;         //"Crossbow"
            case STRING_SWITCH('X','C'): return ACTION_XC;         //"Crossbow"
            case STRING_SWITCH('X','D'): return ACTION_XD;         //"Crossbow"
            case STRING_SWITCH('F','A'): return ACTION_FA;         //"Flinged"
            case STRING_SWITCH('F','B'): return ACTION_FB;         //"Flinged"
            case STRING_SWITCH('F','C'): return ACTION_FC;         //"Flinged"
            case STRING_SWITCH('F','D'): return ACTION_FD;         //"Flinged"
            case STRING_SWITCH('P','A'): return ACTION_PA;         //"Parry"
            case STRING_SWITCH('P','B'): return ACTION_PB;         //"Parry"
            case STRING_SWITCH('P','C'): return ACTION_PC;         //"Parry"
            case STRING_SWITCH('P','D'): return ACTION_PD;         //"Parry"
            case STRING_SWITCH('E','A'): return ACTION_EA;         //"Evade"
            case STRING_SWITCH('E','B'): return ACTION_EB;         //"Evade"
            case STRING_SWITCH('R','A'): return ACTION_RA;         //"Roll"
            case STRING_SWITCH('Z','A'): return ACTION_ZA;         //"Zap Magic"
            case STRING_SWITCH('Z','B'): return ACTION_ZB;         //"Zap Magic"
            case STRING_SWITCH('Z','C'): return ACTION_ZC;         //"Zap Magic"
            case STRING_SWITCH('Z','D'): return ACTION_ZD;         //"Zap Magic"
            case STRING_SWITCH('W','A'): return ACTION_WA;         //"Sneak"
            case STRING_SWITCH('W','B'): return ACTION_WB;         //"Walk"
            case STRING_SWITCH('W','C'): return ACTION_WC;         //"Run"
            case STRING_SWITCH('W','D'): return ACTION_WD;         //"Push"
            case STRING_SWITCH('J','A'): return ACTION_JA;         //"Jump"
            case STRING_SWITCH('J','B'): return ACTION_JB;         //"Jump ( Falling ) ( Drop left )"
            case STRING_SWITCH('J','C'): return ACTION_JC;         //"Jump ( Falling ) ( Drop right )"
            case STRING_SWITCH('H','A'): return ACTION_HA;         //"Hit ( Taking damage )"
            case STRING_SWITCH('H','B'): return ACTION_HB;         //"Hit ( Taking damage )"
            case STRING_SWITCH('H','C'): return ACTION_HC;         //"Hit ( Taking damage )"
            case STRING_SWITCH('H','D'): return ACTION_HD;         //"Hit ( Taking damage )"
            case STRING_SWITCH('K','A'): return ACTION_KA;         //"Killed"
            case STRING_SWITCH('K','B'): return ACTION_KB;         //"Killed"
            case STRING_SWITCH('K','C'): return ACTION_KC;         //"Killed"
            case STRING_SWITCH('K','D'): return ACTION_KD;         //"Killed"
            case STRING_SWITCH('M','A'): return ACTION_MA;         //"Drop Item Left"
            case STRING_SWITCH('M','B'): return ACTION_MB;         //"Drop Item Right"
            case STRING_SWITCH('M','C'): return ACTION_MC;         //"Cheer"
            case STRING_SWITCH('M','D'): return ACTION_MD;         //"Show Off"
            case STRING_SWITCH('M','E'): return ACTION_ME;         //"Grab Item Left"
            case STRING_SWITCH('M','F'): return ACTION_MF;         //"Grab Item Right"
            case STRING_SWITCH('M','G'): return ACTION_MG;         //"Open Chest"
            case STRING_SWITCH('M','H'): return ACTION_MH;         //"Sit ( Riding a mount )"
            case STRING_SWITCH('M','I'): return ACTION_MI;         //"Ride"
            case STRING_SWITCH('M','J'): return ACTION_MJ;         //"Activated ( For items )"
            case STRING_SWITCH('M','K'): return ACTION_MK;         //"Snoozing"
            case STRING_SWITCH('M','L'): return ACTION_ML;         //"Unlock"
            case STRING_SWITCH('M','M'): return ACTION_MM;         //"Held Left"
            case STRING_SWITCH('M','N'): return ACTION_MN;         //"Held Right
            default:
                //fall through
            break;
        }
    }

    return ACTION_COUNT;
}

ModelDescriptor::ModelDescriptor(const std::string &folderPath) :
    _name(folderPath),      //Make up a name for the model...  IMPORT\TEMP0000.OBJ
    _actionMap(),
    _actionValid(),
    _actionStart(),
    _actionEnd(),
    _md2Model(nullptr)
{
    // Clear out all actions and reset to invalid
    _actionMap.fill(ACTION_COUNT);
    _actionValid.fill(false);
    _actionStart.fill(0);
    _actionEnd.fill(0);

    for(size_t i = 0; i < LIP_COUNT; ++i) {
        for(size_t j = 0; j < FRAMELIP_COUNT; ++j) {
            _framelipToWalkframe[i][j] = 0;
        }
    }

    // load the model from the file
    _md2Model = MD2Model::loadFromFile(folderPath + "/tris.md2");
    if(!_md2Model) {
        throw std::runtime_error("File not found: " + folderPath + "/tris.md2");
    }

    /// @details Egoboo md2 models were designed with 1 tile = 32x32 units, but internally Egoboo uses
    ///      1 tile = 128x128 units. Previously, this was handled by sprinkling a bunch of
    ///      commands that multiplied various quantities by 4 or by 4.125 throughout the code.
    ///      It was very counterintuitive, and caused me no end of headaches...  Of course the
    ///      solution is to scale the model!
    _md2Model->scaleModel(-3.5f, 3.5f, 3.5f);

    // Create the actions table for this imad
    ripActions();
    healActions(folderPath + "/copy.txt");

    // Create table for doing transition from one type of walk to another...
    // Clear 'em all to start
    for(MD2_Frame &frame : _md2Model->getFrames())
    {
        frame.framelip = 0;
    }

    // Need to figure out how far into action each frame is
    initializeFrameLip(ACTION_WA);
    initializeFrameLip(ACTION_WB);
    initializeFrameLip(ACTION_WC);

    // Now do the same, in reverse, for walking animations
    initializeWalkFrame(LIPDA, ACTION_DA);
    initializeWalkFrame(LIPWA, ACTION_WA);
    initializeWalkFrame(LIPWB, ACTION_WB);
    initializeWalkFrame(LIPWC, ACTION_WC);    
}

const std::string& ModelDescriptor::getName() const
{
    return _name;
}

bool ModelDescriptor::isActionValid(int action) const
{
    if(action < 0 || action >= _actionValid.size()) {
        return false;
    }
    return _actionValid[action];
}


ModelAction ModelDescriptor::getAction(int action) const
{
    // you are pretty much guaranteed that ACTION_DA will be valid for a model,
    // I guess it could be invalid if the model had no frames or something
    ModelAction retval = ACTION_DA;
    if (!isActionValid(ACTION_DA))
    {
        retval = ACTION_COUNT;
    }

    // track down a valid value
    if ( isActionValid(action) )
    {
        return static_cast<ModelAction>(action);
    }
    else if ( ACTION_COUNT != _actionMap[action] )
    {
        // do a "recursive" search for a valid action
        // we should never really have to check more than once if the map is prepared
        // properly BUT you never can tell. Make sure we do not get a runaway loop by
        // you never go farther than ACTION_COUNT steps and that you never see the
        // original action again

        ModelAction tnc = _actionMap[action];
        for (size_t cnt = 0; cnt < ACTION_COUNT; cnt++)
        {
            if ( tnc >= ACTION_COUNT || tnc == action ) break;

            if (isActionValid(tnc))
            {
                return tnc;
            }
            tnc = _actionMap[tnc];
        }
    }

    return retval;    
}

BIT_FIELD ModelDescriptor::getMadFX(int action) const
{
    if ( !isActionValid(action) ) return EMPTY_BIT_FIELD;

    //Loop through all frames in animation and collect all FX bits that are set
    BIT_FIELD retval = EMPTY_BIT_FIELD;
    const std::vector<MD2_Frame> &frames = _md2Model->getFrames();
    for (size_t cnt = _actionStart[action]; cnt <= _actionEnd[action]; cnt++)
    {
        SET_BIT(retval, frames[cnt].framefx);
    }

    return retval;
}

ModelAction ModelDescriptor::randomizeAction(ModelAction action, int slot) const
{
    // a valid slot?
    if ( slot < 0 || slot >= SLOT_COUNT ) return action;

    // a valid action?
    if (action >= ACTION_COUNT) return action;

    const int diff = slot * 2;

    //---- non-randomizable actions
    switch(action)
    {
        case ACTION_MG:         // MG      = Open Chest
        case ACTION_MH:         // MH      = Sit
        case ACTION_MI:         // MI      = Ride
        case ACTION_MJ:         // MJ      = Object Activated
        case ACTION_MK:         // MK      = Snoozing
        case ACTION_ML:         // ML      = Unlock
        case ACTION_JA:         // JA      = Jump
        case ACTION_RA:         // RA      = Roll
            return action;

        //---- do a couple of special actions that have left/right
        case ACTION_EA:
        case ACTION_EB:
          action = static_cast<ModelAction>(ACTION_JB + slot);   // EA/EB = Evade left/right
        break;
        case ACTION_JB:
        case ACTION_JC:
          action = static_cast<ModelAction>(ACTION_JB + slot);   // JB/JC = Dropped item left/right
        break;
        case ACTION_MA:
        case ACTION_MB:
          action = static_cast<ModelAction>(ACTION_MA + slot);   // MA/MB = Drop left/right item
        break;
        case ACTION_MC:
        case ACTION_MD:
          action = static_cast<ModelAction>(ACTION_MC + slot);   // MC/MD = Slam left/right
        break;
        case ACTION_ME:
        case ACTION_MF:
          action = static_cast<ModelAction>(ACTION_ME + slot);   // ME/MF = Grab item left/right
        break;
        case ACTION_MM:
        case ACTION_MN:
          action = static_cast<ModelAction>(ACTION_MM + slot);   // MM/MN = Held left/right
        break;

        default:
            if ( ACTION_IS_TYPE(action, W) ) return action;  // WA - WD = Walk
    }

    //---- actions that can be randomized, but are not left/right sensitive
    // D = dance (idle animation)
    if (action == ACTION_DA || action == ACTION_DB || action == ACTION_DC || action == ACTION_DD)      action = static_cast<ModelAction>(ACTION_DA + Random::next(3));

    //---- handle all the normal attack/defense animations
    // U = unarmed
    else if ( ACTION_IS_TYPE( action, U ) ) action = static_cast<ModelAction>(ACTION_UA + diff + Random::next(1));
    // T = thrust
    else if ( ACTION_IS_TYPE( action, T ) ) action = static_cast<ModelAction>(ACTION_TA + diff + Random::next(1));
    // C = chop
    else if ( ACTION_IS_TYPE( action, C ) ) action = static_cast<ModelAction>(ACTION_CA + diff + Random::next(1));
    // S = slice
    else if ( ACTION_IS_TYPE( action, S ) ) action = static_cast<ModelAction>(ACTION_SA + diff + Random::next(1));
    // B = bash
    else if ( ACTION_IS_TYPE( action, B ) ) action = static_cast<ModelAction>(ACTION_BA + diff + Random::next(1));
    // L = longbow
    else if ( ACTION_IS_TYPE( action, L ) ) action = static_cast<ModelAction>(ACTION_LA + diff + Random::next(1));
    // X = crossbow
    else if ( ACTION_IS_TYPE( action, X ) ) action = static_cast<ModelAction>(ACTION_XA + diff + Random::next(1));
    // F = fling
    else if ( ACTION_IS_TYPE( action, F ) ) action = static_cast<ModelAction>(ACTION_FA + diff + Random::next(1));
    // P = parry/block
    else if ( ACTION_IS_TYPE( action, P ) ) action = static_cast<ModelAction>(ACTION_PA + diff + Random::next(1));
    // Z = zap
    else if ( ACTION_IS_TYPE( action, Z ) ) action = static_cast<ModelAction>(ACTION_ZA + diff + Random::next(1));

    //---- these are passive actions
    // H = hurt
    else if ( ACTION_IS_TYPE( action, H ) ) action = static_cast<ModelAction>(ACTION_HA + Random::next(3));
    // K = killed
    else if ( ACTION_IS_TYPE( action, K ) ) action = static_cast<ModelAction>(ACTION_KA + Random::next(3));

    return action;    
}

void ModelDescriptor::ripActions()
{
    // Clear out all actions and reset to invalid
    _actionMap.fill(ACTION_COUNT);
    _actionStart.fill(-1);
    _actionEnd.fill(-1);
    _actionValid.fill(false);

    // is there anything to do?
    if ( _md2Model->getFrames().empty() ) return;

    // Make a default dance action (ACTION_DA) to be the 1st frame of the animation
    _actionMap[ACTION_DA]   = ACTION_DA;
    _actionValid[ACTION_DA] = true;
    _actionStart[ACTION_DA] = 0;
    _actionEnd[ACTION_DA]   = 0;

    //Make movement actions map default to each other
    _actionMap[ACTION_WC] = ACTION_WB;
    _actionMap[ACTION_WB] = ACTION_WA;
    _actionMap[ACTION_WA] = ACTION_DA;

    // Now go huntin' to see what each iframe is, look for runs of same action
    ModelAction last_action = ACTION_COUNT;
    int iframe = 0;
    for(const MD2_Frame &frame : _md2Model->getFrames())
    {
        ModelAction action_now = stringToAction(frame.name);
        
        if (action_now == ACTION_COUNT) {
			Log::get().warn("Got no action for frame name '%s', ignoring (%s)\n", frame.name, _name.c_str());
            iframe++;
            continue;
        }

        if ( last_action != action_now )
        {
            // start a new action
            _actionMap[action_now]   = action_now;
            _actionStart[action_now] = iframe;
            _actionEnd[action_now]   = iframe;
            _actionValid[action_now] = true;

            last_action = action_now;
        }
        else
        {
            // keep expanding the action_end until the end of the action
            _actionEnd[action_now] = iframe;
        }

        parseFrameDescriptors(frame.name, iframe);
        iframe++;
    }
}

void ModelDescriptor::parseFrameDescriptors(const char * cFrameName, int frame)
{
    char name_action[16], name_fx[16];
    int name_count;
    int cnt;

    static int token_count = -1;
    static const char * tokens[] = { "I", "S", "F", "P", "A", "G", "D", "C",          /* the normal command tokens */
                                     "LA", "LG", "LD", "LC", "RA", "RG", "RD", "RC", NULL
                                   }; /* the "bad" token aliases */

    // check for a valid frame number
    if(frame >= _md2Model->getFrames().size())
    {
        return; 
    }

    MD2_Frame &pframe = _md2Model->getFrames()[frame];

    // this should only be initialized the first time through
    if (token_count < 0)
    {
        token_count = 0;
        for (cnt = 0; nullptr != tokens[token_count] && cnt < 256; cnt++)
        {
            token_count++;
        }
    }

    // set the default values
    BIT_FIELD fx = 0;
    pframe.framefx = fx;

    // check for a non-trivial frame name
    if ( !VALID_CSTR(cFrameName) ) return;

    // skip over whitespace
    const char* ptmp     = cFrameName;
    const char* ptmp_end = cFrameName + 16;
    for ( /* nothing */; ptmp < ptmp_end && Ego::isspace(*ptmp); ptmp++) {};

    // copy non-numerical text
    char* paction     = name_action;
    char* paction_end = name_action + 16;
    for ( /* nothing */; ptmp < ptmp_end && paction < paction_end && !Ego::isspace(*ptmp); ptmp++, paction++ )
    {
        if (Ego::isdigit(*ptmp)) break;
        *paction = *ptmp;
    }
    if ( paction < paction_end ) *paction = CSTR_END;

    name_fx[0] = CSTR_END;
    sscanf( ptmp, "%d %15s", &name_count, name_fx ); //ZF> NOTE: return value not used
    name_action[15] = CSTR_END;
    name_fx[15] = CSTR_END;

    // check for a non-trivial fx command
    if ( name_fx[0] == '\0' ) return;

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
            //Ignore trailing zeros. Some older models appended zeros to all frames
            if(*ptmp != '0') {
				Log::get().warn( "Model %s, frame %d, frame name \"%s\" has unknown frame effects command \"%s\"\n", _name.c_str(), frame, cFrameName, ptmp );
            }
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
				Log::get().warn( "Model %s, frame %d, frame name \"%s\" has a frame effects command in an improper configuration \"%s\"\n", _name.c_str(), frame, cFrameName, tokens[token_index] );
            }
        }
    }

    pframe.framefx = fx;
}

void ModelDescriptor::initializeWalkFrame(int lip, ModelAction action)
{
    int action_stt, action_end;

    action = getAction(action);
    if ( action >= ACTION_COUNT || !_actionValid[action] )
    {
        // make a fake action
        action_stt = _actionStart[ACTION_DA];
        action_end = _actionStart[ACTION_DA];
    }
    else
    {
        action_stt = _actionStart[action];
        action_end = _actionEnd[action];
    }

    // count the number of frames
    int action_count = 1 + ( action_end - action_stt );

    // scan through all the frames of the framelip
    for ( size_t frame = 0; frame < FRAMELIP_COUNT; frame++ )
    {
        int framealong = 0;

        if ( action_count > 0 )
        {
            // this SHOULD produce a number between 0 and (action_count - 1),
            // but there could be rounding error
            framealong = ( frame * action_count ) / FRAMELIP_COUNT;

            framealong = std::min( framealong, action_count - 1 );
        }

        _framelipToWalkframe[lip][frame] = action_stt + framealong;
    }
}

void ModelDescriptor::makeEquallyLit()
{
    _md2Model->makeEquallyLit();
}

void ModelDescriptor::initializeFrameLip(ModelAction action)
{
    action = getAction(action);

    if ( !isActionValid(action) ) return;

    // grab the animation info
    const int action_stt = _actionStart[action];
    const int action_end = _actionEnd[action];
    const int action_count = 1 + ( action_end - action_stt );

    // scan through all the frames of the action
    for (int frame = action_stt; frame <= action_end; frame++)
    {
        // grab a valid frame
        if (frame >= _md2Model->getFrames().size()) break;

        // calculate the framelip.
        // this should produce a number between 0 and FRAMELIP_COUNT-1, but
        // watch out for possible rounding errors
        int framelip = (( frame - action_stt ) * FRAMELIP_COUNT ) / action_count;

        // limit the framelip to the valid range
        _md2Model->getFrames()[frame].framelip = std::min<size_t>(framelip, FRAMELIP_COUNT - 1);
    }
}

void ModelDescriptor::healActions(const std::string &filePath)
{
    actionCopyCorrect(ACTION_DA, ACTION_DB);  // All dances should be safe
    actionCopyCorrect(ACTION_DB, ACTION_DC);
    actionCopyCorrect(ACTION_DC, ACTION_DD);
    actionCopyCorrect(ACTION_DB, ACTION_DC);
    actionCopyCorrect(ACTION_DA, ACTION_DB);
    actionCopyCorrect(ACTION_UA, ACTION_UB);
    actionCopyCorrect(ACTION_UB, ACTION_UC);
    actionCopyCorrect(ACTION_UC, ACTION_UD);
    actionCopyCorrect(ACTION_TA, ACTION_TB);
    actionCopyCorrect(ACTION_TC, ACTION_TD);
    actionCopyCorrect(ACTION_CA, ACTION_CB);
    actionCopyCorrect(ACTION_CC, ACTION_CD);
    actionCopyCorrect(ACTION_SA, ACTION_SB);
    actionCopyCorrect(ACTION_SC, ACTION_SD);
    actionCopyCorrect(ACTION_BA, ACTION_BB);
    actionCopyCorrect(ACTION_BC, ACTION_BD);
    actionCopyCorrect(ACTION_LA, ACTION_LB);
    actionCopyCorrect(ACTION_LC, ACTION_LD);
    actionCopyCorrect(ACTION_XA, ACTION_XB);
    actionCopyCorrect(ACTION_XC, ACTION_XD);
    actionCopyCorrect(ACTION_FA, ACTION_FB);
    actionCopyCorrect(ACTION_FC, ACTION_FD);
    actionCopyCorrect(ACTION_PA, ACTION_PB);
    actionCopyCorrect(ACTION_PC, ACTION_PD);
    actionCopyCorrect(ACTION_ZA, ACTION_ZB);
    actionCopyCorrect(ACTION_ZC, ACTION_ZD);
    actionCopyCorrect(ACTION_WA, ACTION_WB);
    actionCopyCorrect(ACTION_WB, ACTION_WC);
    actionCopyCorrect(ACTION_WC, ACTION_WD);
    actionCopyCorrect(ACTION_DA, ACTION_WD);  // All walks should be safe
    actionCopyCorrect(ACTION_WC, ACTION_WD);
    actionCopyCorrect(ACTION_WB, ACTION_WC);
    actionCopyCorrect(ACTION_WA, ACTION_WB);
    actionCopyCorrect(ACTION_JA, ACTION_JB);
    actionCopyCorrect(ACTION_JB, ACTION_JC);
    actionCopyCorrect(ACTION_DA, ACTION_JC);  // All jumps should be safe
    actionCopyCorrect(ACTION_JB, ACTION_JC);
    actionCopyCorrect(ACTION_JA, ACTION_JB);
    actionCopyCorrect(ACTION_HA, ACTION_HB);
    actionCopyCorrect(ACTION_HB, ACTION_HC);
    actionCopyCorrect(ACTION_HC, ACTION_HD);
    actionCopyCorrect(ACTION_HB, ACTION_HC);
    actionCopyCorrect(ACTION_HA, ACTION_HB);
    actionCopyCorrect(ACTION_KA, ACTION_KB);
    actionCopyCorrect(ACTION_KB, ACTION_KC);
    actionCopyCorrect(ACTION_KC, ACTION_KD);
    actionCopyCorrect(ACTION_KB, ACTION_KC);
    actionCopyCorrect(ACTION_KA, ACTION_KB);
    actionCopyCorrect(ACTION_MH, ACTION_MI);
    actionCopyCorrect(ACTION_DA, ACTION_MM);
    actionCopyCorrect(ACTION_MM, ACTION_MN);

    // Copy entire actions to save frame space COPY.TXT
    std::unique_ptr<ReadContext> ctxt = nullptr;
    try {
        ctxt = std::make_unique<ReadContext>(filePath);
    } catch (...) {
        return;
    }
        while (ctxt->skipToColon(true))
        {
            std::string szOne, szTwo;

            vfs_read_string_lit( *ctxt, szOne );
            ModelAction actiona = ModelDescriptor::charToAction(szOne[0]);

            vfs_read_string_lit( *ctxt, szTwo );
            ModelAction actionb = ModelDescriptor::charToAction(szTwo[0]);

            actionCopyCorrect(static_cast<ModelAction>(actiona + 0), static_cast<ModelAction>(actionb + 0));
            actionCopyCorrect(static_cast<ModelAction>(actiona + 1), static_cast<ModelAction>(actionb + 1));
            actionCopyCorrect(static_cast<ModelAction>(actiona + 2), static_cast<ModelAction>(actionb + 2));
            actionCopyCorrect(static_cast<ModelAction>(actiona + 3), static_cast<ModelAction>(actionb + 3));
        }
}

ModelAction ModelDescriptor::charToAction(char cTmp)
{
    switch ( Ego::toupper( cTmp ) )
    {
        case 'D': return ACTION_DA;
        case 'U': return ACTION_UA;
        case 'T': return ACTION_TA;
        case 'C': return ACTION_CA;
        case 'S': return ACTION_SA;
        case 'B': return ACTION_BA;
        case 'L': return ACTION_LA;
        case 'X': return ACTION_XA;
        case 'F': return ACTION_FA;
        case 'P': return ACTION_PA;
        case 'Z': return ACTION_ZA;
        // case 'W': action = ACTION_WA; break;   /// @note ZF@> Can't do this, attack animation WALK is used for doing nothing (for example charging spells)
        case 'H': return ACTION_HA;
        case 'K': return ACTION_KA;
        default:  return ACTION_DA;
    }
}

void ModelDescriptor::actionCopyCorrect(ModelAction actiona, ModelAction actionb)
{
    // With the new system using the action_map, this is all that is really necessary
    if ( ACTION_COUNT == _actionMap[actiona] )
    {
        if ( _actionValid[actionb] )
        {
            _actionMap[actiona] = actionb;
        }
        else if ( ACTION_COUNT != _actionMap[actionb] )
        {
            _actionMap[actiona] = _actionMap[actionb];
        }
    }
    else if ( ACTION_COUNT == _actionMap[actionb] )
    {
        if ( _actionValid[actiona] )
        {
            _actionMap[actionb] = actiona;
        }
        else if ( ACTION_COUNT != _actionMap[actiona] )
        {
            _actionMap[actionb] = _actionMap[actiona];
        }
    }
}

const std::shared_ptr<MD2Model>& ModelDescriptor::getMD2() const
{
    return _md2Model;
}

bool ModelDescriptor::isFrameValid(int action, int frame) const
{
    if(!isActionValid(action)) return false;
    if (frame < _actionStart[action]) return false;
    if (frame > _actionEnd[action]) return false;

    return true;
}

int ModelDescriptor::getFrameLipToWalkFrame(int lip, int framelip) const
{
    assert(lip >= 0 && lip < LIP_COUNT && framelip >= 0 && framelip < FRAMELIP_COUNT);
    return _framelipToWalkframe[lip][framelip]; 
}

} //Ego