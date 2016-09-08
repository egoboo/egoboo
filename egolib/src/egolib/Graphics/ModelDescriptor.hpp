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

/// @file egolib/Graphics/ModelDescriptor.hpp
/// @author Johan Jansen aka Zefz
#pragma once

#include "egolib/typedef.h"

//Forward declarations
class MD2Model;

//Macros
#define ACTION_IS_TYPE( VAL, CHR ) ((VAL >= ACTION_##CHR##A) && (VAL <= ACTION_##CHR##D))

/// The various model actions
enum ModelAction : uint8_t
{
    ACTION_DA = 0,         ///< DA - Dance ( Typical standing )
    ACTION_DB,             ///< DB - Dance ( Bored )
    ACTION_DC,             ///< DC - Dance ( Bored )
    ACTION_DD,             ///< DD - Dance ( Bored )
    ACTION_UA,             ///< UA - Unarmed Attack ( Left )
    ACTION_UB,             ///< UB - Unarmed Attack ( Left )
    ACTION_UC,             ///< UC - Unarmed Attack ( Right )
    ACTION_UD,             ///< UD - Unarmed Attack ( Right )
    ACTION_TA,             ///< TA - Thrust Attack ( Left )
    ACTION_TB,             ///< TB - Thrust Attack ( Left )
    ACTION_TC,             ///< TC - Thrust Attack ( Right )
    ACTION_TD,             ///< TD - Thrust Attack ( Right )
    ACTION_CA,             ///< CA - Chop Attack ( Left )
    ACTION_CB,             ///< CB - Chop Attack ( Left )
    ACTION_CC,             ///< CC - Chop Attack ( Right )
    ACTION_CD,             ///< CD - Chop Attack ( Right )
    ACTION_SA,             ///< SA - Slice Attack ( Left )
    ACTION_SB,             ///< SB - Slice Attack ( Left )
    ACTION_SC,             ///< SC - Slice Attack ( Right )
    ACTION_SD,             ///< SD - Slice Attack ( Right )
    ACTION_BA,             ///< BA - Bash Attack ( Left )
    ACTION_BB,             ///< BB - Bash Attack ( Left )
    ACTION_BC,             ///< BC - Bash Attack ( Right )
    ACTION_BD,             ///< BD - Bash Attack ( Right )
    ACTION_LA,             ///< LA - Longbow Attack ( Left )
    ACTION_LB,             ///< LB - Longbow Attack ( Left )
    ACTION_LC,             ///< LC - Longbow Attack ( Right )
    ACTION_LD,             ///< LD - Longbow Attack ( Right )
    ACTION_XA,             ///< XA - Crossbow Attack ( Left )
    ACTION_XB,             ///< XB - Crossbow Attack ( Left )
    ACTION_XC,             ///< XC - Crossbow Attack ( Right )
    ACTION_XD,             ///< XD - Crossbow Attack ( Right )
    ACTION_FA,             ///< FA - Flinged Attack ( Left )
    ACTION_FB,             ///< FB - Flinged Attack ( Left )
    ACTION_FC,             ///< FC - Flinged Attack ( Right )
    ACTION_FD,             ///< FD - Flinged Attack ( Right )
    ACTION_PA,             ///< PA - Parry or Block ( Left )
    ACTION_PB,             ///< PB - Parry or Block ( Left )
    ACTION_PC,             ///< PC - Parry or Block ( Right )
    ACTION_PD,             ///< PD - Parry or Block ( Right )
    ACTION_EA,             ///< EA - Evade
    ACTION_EB,             ///< EB - Evade
    ACTION_RA,             ///< RA - Roll
    ACTION_ZA,             ///< ZA - Zap Magic ( Left )
    ACTION_ZB,             ///< ZB - Zap Magic ( Left )
    ACTION_ZC,             ///< ZC - Zap Magic ( Right )
    ACTION_ZD,             ///< ZD - Zap Magic ( Right )
    ACTION_WA,             ///< WA - Sneak
    ACTION_WB,             ///< WB - Walk
    ACTION_WC,             ///< WC - Run
    ACTION_WD,             ///< WD - Push
    ACTION_JA,             ///< JA - Jump
    ACTION_JB,             ///< JB - Falling ( End of Jump ) ( Dropped Item left )
    ACTION_JC,             ///< JC - Falling [ Dropped item right ]
    ACTION_HA,             ///< HA - Hit
    ACTION_HB,             ///< HB - Hit
    ACTION_HC,             ///< HC - Hit
    ACTION_HD,             ///< HD - Hit
    ACTION_KA,             ///< KA - Killed
    ACTION_KB,             ///< KB - Killed
    ACTION_KC,             ///< KC - Killed
    ACTION_KD,             ///< KD - Killed
    ACTION_MA,             ///< MA - Misc ( Drop Left Item )
    ACTION_MB,             ///< MB - Misc ( Drop Right Item )
    ACTION_MC,             ///< MC - Misc ( Cheer/Slam Left )
    ACTION_MD,             ///< MD - Misc ( Show Off/Slam Right/Rise from ground )
    ACTION_ME,             ///< ME - Misc ( Grab Item Left )
    ACTION_MF,             ///< MF - Misc ( Grab Item Right )
    ACTION_MG,             ///< MG - Misc ( Open Chest )
    ACTION_MH,             ///< MH - Misc ( Sit )
    ACTION_MI,             ///< MI - Misc ( Ride )
    ACTION_MJ,             ///< MJ - Misc ( Object Activated )
    ACTION_MK,             ///< MK - Misc ( Snoozing )
    ACTION_ML,             ///< ML - Misc ( Unlock )
    ACTION_MM,             ///< MM - Misc ( Held Left )
    ACTION_MN,             ///< MN - Misc ( Held Right )
    ACTION_COUNT
};

/// Model tags
enum ModelFrameEffects : uint32_t
{
    MADFX_INVICTUS       = ( 1 <<  0 ),                    ///< I  Make the character invincible
    MADFX_ACTLEFT        = ( 1 <<  1 ),                    ///< AL Activate left item
    MADFX_ACTRIGHT       = ( 1 <<  2 ),                    ///< AR Activate right item
    MADFX_GRABLEFT       = ( 1 <<  3 ),                    ///< GL GO Grab left/Grab only item
    MADFX_GRABRIGHT      = ( 1 <<  4 ),                    ///< GR Grab right item
    MADFX_DROPLEFT       = ( 1 <<  5 ),                    ///< DL Drop the item in the left/only grip
    MADFX_DROPRIGHT      = ( 1 <<  6 ),                    ///< DR Drop the item in the right grip
    MADFX_STOP           = ( 1 <<  7 ),                    ///< S  Stop movement
    MADFX_FOOTFALL       = ( 1 <<  8 ),                    ///< F  Play a footfall sound
    MADFX_CHARLEFT       = ( 1 <<  9 ),                    ///< CL Grab a character with the left/only grip
    MADFX_CHARRIGHT      = ( 1 << 10 ),                    ///< CR Grab a character with the right grip
    MADFX_POOF           = ( 1 << 11 )                     ///< P  Poof the character
};

/// Animation walking
/// For smooth transitions 'tween walking rates
enum ActionLip : uint8_t
{
    LIPDA,
    LIPWA,                      
    LIPWB,
    LIPWC,
    LIP_COUNT
};

namespace Ego
{

class ModelDescriptor : public Id::NonCopyable
{
public:
    static const size_t FRAMELIP_COUNT = 16;

    ModelDescriptor(const std::string &folderPath);

    const std::string& getName() const;

    const std::shared_ptr<MD2Model>& getMD2() const;

    /// @details translate the action that was given into a valid action for the model
    ///
    /// returns ACTION_COUNT on a complete failure, or the default ACTION_DA if it exists
    ModelAction getAction(int action) const;

    /**
    * @brief
    *   Gets all ModelFrameEffects that are in all of the animation frames in the specified action
    **/
    BIT_FIELD getMadFX(int action) const;

    /**
    * @return
    *   true if this model has a valid animation for the specified action
    **/
    bool isActionValid(int action) const;

    /// @details this function actually determines whether the action follows the
    ///               pattern of ACTION_?A, ACTION_?B, ACTION_?C, ACTION_?D, with
    ///               A and B being for the left hand, and C and D being for the right hand
    ModelAction randomizeAction(ModelAction action, int slot=0) const;

    void makeEquallyLit();

    int getFrameLipToWalkFrame(int lip, int framelip) const;

    bool isFrameValid(int action, int frame) const;

    int getFirstFrame(int action) const { return _actionStart[action]; }
    int getLastFrame(int action) const { return _actionEnd[action]; }

    /// @details This function changes a letter into an action code
    static ModelAction charToAction(char cTmp);

private:

    /// \details  This function creates the iframe lists for each action based on the
    ///    name of each md2 iframe in the model
    void ripActions();

    /// @details This function figures out the IFrame invulnerability, and Attack, Grab, and
    ///               Drop timings
    ///
    ///          BB@> made a bit more sturdy parser that is not going to confuse strings like "LCRA"
    ///               which would not crop up if the convention of L or R going first was applied universally.
    ///               However, there are existing (and common) models which use the opposite convention, leading
    ///               to the possibility that an fx string "LARC" could be interpreted as ACTLEFT, CHARRIGHT, *and*
    ///               ACTRIGHT.
    void parseFrameDescriptors(const char * cFrameName, int frame);

    /// @author ZZ
    /// @details This helps make walking look right
    void initializeWalkFrame(int lip, ModelAction action);

    /// @author ZZ
    /// @details This helps make walking look right
    void initializeFrameLip(ModelAction action);

    /**
    * @brief
    *   Helper function to convert a 2 character string to an action
    * @return
    *   The ModelAction represented by the 2 character string or ACTION_COUNT if it fails
    **/
    ModelAction stringToAction(const std::string &action) const;

    /**
    * @brief
    *   Make sure actions are made valid if a similar one exists
    *   e.g if ACTION_DB or ACTION_DC is missing then it will be mapped to a valid ACTION_DA instead
    **/
    void healActions(const std::string &filePath);

    /// @details This function makes sure both actions are valid if either of them
    ///    are valid.  It will copy start and ends to mirror the valid action.
    void actionCopyCorrect(ModelAction actiona, ModelAction actionb);

private:
    std::string _name;

    uint16_t _framelipToWalkframe[LIP_COUNT][FRAMELIP_COUNT];  ///< For walk animations

    std::array<ModelAction, ACTION_COUNT> _actionMap;  ///< actual action = action_map[requested action]
    std::array<bool, ACTION_COUNT> _actionValid;       ///< false if not valid
    std::array<int, ACTION_COUNT> _actionStart;        ///< First frame of animation
    std::array<int, ACTION_COUNT> _actionEnd;          ///< The last frame

    std::shared_ptr<MD2Model> _md2Model;               ///< actual MD2 model
};

} //Ego