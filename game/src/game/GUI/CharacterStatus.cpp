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

/// @file game/GUI/CharacterStatus.hpp
/// @author Johan Jansen

#include "CharacterStatus.hpp"
#include "game/Entities/_Include.hpp"
#include "game/renderer_2d.h"
#include "game/player.h"

CharacterStatus::CharacterStatus(const std::shared_ptr<Object> &object) :
    _object(object)
{
    //ctor
}

void CharacterStatus::draw()
{
    //If object we are monitoring no longer exist, then destroy this GUI component
    const std::shared_ptr<Object> pchr = _object.lock();
    if(!pchr) {
        destroy();
        return;
    }

    int life_pips = SFP8_TO_SINT(pchr->life);
    int life_pips_max = pchr->getAttribute(Ego::Attribute::MAX_LIFE);
    int mana_pips = SFP8_TO_SINT(pchr->getMana());
    int mana_pips_max = pchr->getAttribute(Ego::Attribute::MAX_MANA);
    int yOffset = getY();

    // draw the name
    yOffset = draw_string_raw(getX() + 8, yOffset, "%s", pchr->getName(false, false, true).c_str());

    // draw the character's money
    yOffset = draw_string_raw(getX() + 8, yOffset, "$%4d", pchr->getMoney()) + 8;

    bool levelUp = false;
    if(pchr->isPlayer()) {
        levelUp = PlaStack.get_ptr(pchr->is_which_player)->_unspentLevelUp;
    }

    // draw the character's main icon
    draw_one_character_icon(pchr->getCharacterID(), getX() + 40, yOffset, false, levelUp ? COLOR_YELLOW : NOSPARKLE);

    // draw the left hand item icon
    draw_one_character_icon(pchr->holdingwhich[SLOT_LEFT], getX() + 8, yOffset, true, NOSPARKLE);

    // draw the right hand item icon
    draw_one_character_icon(pchr->holdingwhich[SLOT_RIGHT], getX() + 72, yOffset, true, NOSPARKLE);

    // skip to the next row
    yOffset += 32;

    //Draw the small XP progress bar
    yOffset = draw_character_xp_bar(pchr->getCharacterID(), getX() + 16, yOffset);

    // Draw the life bar
    if (pchr->isAlive())
    {
        yOffset = draw_one_bar(pchr->life_color, getX(), yOffset, life_pips, life_pips_max);
    }
    else
    {
        // Draw a black bar
        yOffset = draw_one_bar(0, getX(), yOffset, 0, life_pips_max);
    }

    // Draw the mana bar
    if (mana_pips_max > 0)
    {
        yOffset = draw_one_bar(pchr->mana_color, getX(), yOffset, mana_pips, mana_pips_max);
    }

    //After rendering we know how high this GUI component actually is
    setHeight(yOffset - getY());
}
