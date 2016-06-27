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
#include "game/Logic/Player.hpp"
#include "game/GUI/ProgressBar.hpp"
#include "game/game.h" //for update_wld

namespace Ego {
namespace GUI {

CharacterStatus::CharacterStatus(const std::shared_ptr<Object> &object) :
    _object(object),
    _chargeBar(std::make_shared<GUI::ProgressBar>())
{
    //ctor
}



void CharacterStatus::draw_one_character_icon(const ObjectRef item, float x, float y, bool draw_ammo, Uint8 draw_sparkle)
{
	/// @author BB
	/// @details Draw an icon for the given item at the position <x,y>.
	///     If the object is invalid, draw the null icon instead of failing
	///     If NOSPARKLE is specified the default item sparkle will be used (default behaviour)

	Object * pitem = _currentModule->getObjectHandler().get(item);

	// grab the icon reference
	std::shared_ptr<const Texture> icon_ref = (pitem != nullptr) ? pitem->getIcon() : TextureManager::get().getTexture("mp_data/nullicon");

	// draw the icon
	if (draw_sparkle == NOSPARKLE) draw_sparkle = (NULL == pitem) ? NOSPARKLE : pitem->sparkle;
	draw_game_icon(icon_ref, x, y, draw_sparkle, update_wld, -1);

	// draw the ammo, if requested
	if (draw_ammo && (NULL != pitem))
	{
		if (0 != pitem->ammomax && pitem->ammoknown)
		{
			if ((!pitem->getProfile()->isStackable()) || pitem->ammo > 1)
			{
				// Show amount of ammo left
				_gameEngine->getUIManager()->drawBitmapFontString(Vector2f(x, y - 8), std::to_string(pitem->ammo));
			}
		}
	}
}

float CharacterStatus::draw_one_bar(uint8_t bartype, float x_stt, float y_stt, int ticks, int maxticks)
{
	/// @author ZZ
	/// @details This function draws a bar and returns the y position for the next one

	const float scale = 1.0f;

    Vector2f size;
	Rectangle2f tx_rect, sc_rect;

	float tx_width, tx_height, img_width;
	float tab_width, tick_width, tick_height;

	int total_ticks = maxticks;
	int tmp_bartype = bartype;

	float x_left = x_stt;
	float x = x_stt;
	float y = y_stt;

	if (maxticks <= 0 || ticks < 0 || bartype > NUMBAR) return y;

	// limit the values to reasonable ones
	if (total_ticks > MAXTICK) total_ticks = MAXTICK;
	if (ticks > total_ticks) ticks = total_ticks;

	// grab a pointer to the bar texture
	const std::shared_ptr<Texture> &tx_ptr = TextureManager::get().getTexture("mp_data/bars");

	// allow the bitmap to be scaled to arbitrary size
	tx_width = 128.0f;
	tx_height = 128.0f;
	img_width = 112.0f;
	if (NULL != tx_ptr)
	{
		tx_width = tx_ptr->getWidth();
		tx_height = tx_ptr->getHeight();
		img_width = tx_ptr->getSourceWidth();
	}

	// calculate the bar parameters
	tick_width = img_width / 14.0f;
	tick_height = img_width / 7.0f;
	tab_width = img_width / 3.5f;

	//---- Draw the tab
	tmp_bartype = bartype;

    tx_rect = Rectangle2f(Point2f(0.0f / tx_width, tick_height * (tmp_bartype + 0) / tx_height),
                          Point2f(tab_width / tx_width, tick_height * (tmp_bartype + 1) / tx_height));

    size = tx_rect.getSize() * scale;
    size = Vector2f(size.x() * tx_width, size.y() * tx_height);
    
    sc_rect = Rectangle2f(Point2f(x, y),
                          Point2f(x, y) + size);

	_gameEngine->getUIManager()->drawQuad2D(tx_ptr, sc_rect, tx_rect, true);

	// make the new left-hand margin after the tab
	x_left = x_stt + size.x();
	x = x_left;

	//---- Draw the full rows of ticks
	while (ticks >= NUMTICK)
	{
		tmp_bartype = bartype;

        tx_rect = Rectangle2f(Point2f(tab_width / tx_width, tick_height * (tmp_bartype + 0) / tx_height),
                              Point2f(img_width / tx_width, tick_height * (tmp_bartype + 1) / tx_height));

        size = tx_rect.getSize() * scale;
        size = Vector2f(size.x() * tx_width, size.y() * tx_height);

        sc_rect = Rectangle2f(Point2f(x, y),
                              Point2f(x, y) + size);

		_gameEngine->getUIManager()->drawQuad2D(tx_ptr, sc_rect, tx_rect, true);

		y += size.y();
		ticks -= NUMTICK;
		total_ticks -= NUMTICK;
	}

	if (ticks > 0)
	{
		int full_ticks = NUMTICK - ticks;
		int empty_ticks = NUMTICK - (std::min(NUMTICK, total_ticks) - ticks);

		//---- draw a partial row of full ticks
        tx_rect = Rectangle2f(Point2f(tab_width / tx_width, tick_height * (tmp_bartype + 0) / tx_height),
                              Point2f((img_width - tick_width * full_ticks) / tx_width, tick_height * (tmp_bartype + 1) / tx_height));

        size = tx_rect.getSize() * scale;
        size = Vector2f(size.x() * tx_width, size.y() * tx_height);

        sc_rect = Rectangle2f(Point2f(x, y),
                              Point2f(x, y) + size);

		_gameEngine->getUIManager()->drawQuad2D(tx_ptr, sc_rect, tx_rect, true);

		// move to the right after drawing the full ticks
		x += size.x();

		//---- draw a partial row of empty ticks
		tmp_bartype = 0;

        tx_rect = Rectangle2f(Point2f(tab_width / tx_width, tick_height * (tmp_bartype + 0) / tx_height),
                              Point2f((img_width - tick_width * empty_ticks) / tx_width, tick_height * (tmp_bartype + 1) / tx_height));

        size = tx_rect.getSize() * scale;
        size = Vector2f(size.x() * tx_width, size.y() * tx_height);

        sc_rect = Rectangle2f(Point2f(x, y),
                              Point2f(x, y) + size);

		_gameEngine->getUIManager()->drawQuad2D(tx_ptr, sc_rect, tx_rect, true);

		y += size.y();
		ticks = 0;
		total_ticks -= NUMTICK;
	}

	// reset the x position
	x = x_left;

	// Draw full rows of empty ticks
	while (total_ticks >= NUMTICK)
	{
		tmp_bartype = 0;

        tx_rect = Rectangle2f(Point2f(tab_width / tx_width, tick_height * (tmp_bartype + 0) / tx_height),
                              Point2f(img_width / tx_width, tick_height * (tmp_bartype + 1) / tx_height));

        size = tx_rect.getSize() * scale;
        size = Vector2f(size.x() * tx_width, size.y() * tx_height);

        sc_rect = Rectangle2f(Point2f(x, y),
                              Point2f(x, y) + size);

		_gameEngine->getUIManager()->drawQuad2D(tx_ptr, sc_rect, tx_rect, true);

		y += size.y();
		total_ticks -= NUMTICK;
	}

	// Draw the last of the empty ones
	if (total_ticks > 0)
	{
		int remaining = NUMTICK - total_ticks;

		//---- draw a partial row of empty ticks
		tmp_bartype = 0;

        tx_rect = Rectangle2f(Point2f(tab_width / tx_width, tick_height * (tmp_bartype + 0) / tx_height),
                              Point2f((img_width - tick_width * remaining) / tx_width, tick_height * (tmp_bartype + 1) / tx_height));

        size = tx_rect.getSize() * scale;
        size = Vector2f(size.x() * tx_width, size.y() * tx_height);

        sc_rect = Rectangle2f(Point2f(x, y),
                              Point2f(x, y) + size);

		_gameEngine->getUIManager()->drawQuad2D(tx_ptr, sc_rect, tx_rect, true);

		y += size.y();
	}

	return y;
}

float CharacterStatus::draw_one_xp_bar(float x, float y, uint8_t ticks)
{
	/// @author ZF
	/// @details This function draws a xp bar and returns the y position for the next one

	const std::shared_ptr<Texture> &texture = TextureManager::get().getTexture("mp_data/xpbar");

	ticks = std::min(ticks, (uint8_t)NUMTICK);

	Renderer::get().setColour(Math::Colour4f::white());

    Vector2f size;
    uint8_t cnt;
    Rectangle2f tx_rect, sc_rect;

	//---- Draw the tab (always colored)

    size = Vector2f(16, XPTICK);

    tx_rect = Rectangle2f(Point2f(0, XPTICK / 16),
                          Point2f(32.0f / 128, XPTICK * 2 / 16));

    sc_rect = Rectangle2f(Point2f(x, y), Point2f(x, y) + size);

	_gameEngine->getUIManager()->drawQuad2D(texture, sc_rect, tx_rect, true);

	x += size.x();

	//---- Draw the filled ones
    tx_rect = Rectangle2f(Point2f(0.0f, XPTICK / 16.0f),
                          Point2f(32 / 128.0f, 2 * XPTICK / 16.0f));

	size = Vector2f(XPTICK,XPTICK);

	for (cnt = 0; cnt < ticks; cnt++)
	{
        sc_rect = Rectangle2f(Point2f(x + (cnt * size.x()),y),
                              Point2f(x + (cnt * size.x()) + size.x(), y + size.y()));

		_gameEngine->getUIManager()->drawQuad2D(texture, sc_rect, tx_rect, true);
	}

	//---- Draw the remaining empty ones
    tx_rect = Rectangle2f(Point2f(0.0f, 0.0f),
                          Point2f(32 / 128.0f, XPTICK / 16.0f));

	for ( /*nothing*/; cnt < NUMTICK; cnt++)
	{
        sc_rect = Rectangle2f(Point2f(x + (cnt * size.x()), y),
                              Point2f(x + (cnt * size.x()) + size.x(), y + size.y()));

		_gameEngine->getUIManager()->drawQuad2D(texture, sc_rect, tx_rect, true);
	}

	return y + size.y();
}

float CharacterStatus::draw_character_xp_bar(const ObjectRef character, float x, float y)
{
	Object * pchr;

	if (!_currentModule->getObjectHandler().exists(character)) return y;
	pchr = _currentModule->getObjectHandler().get(character);

	//Draw the small XP progress bar
	if (pchr->experiencelevel < MAXLEVEL - 1)
	{
		uint8_t  curlevel = pchr->experiencelevel + 1;
		uint32_t xplastlevel = pchr->getProfile()->getXPNeededForLevel(curlevel - 1);
		uint32_t xpneed = pchr->getProfile()->getXPNeededForLevel(curlevel);

		while (pchr->experience < xplastlevel && curlevel > 1) {
			curlevel--;
			xplastlevel = pchr->getProfile()->getXPNeededForLevel(curlevel - 1);
		}

		float fraction = ((float)(pchr->experience - xplastlevel)) / (float)std::max<uint32_t>(xpneed - xplastlevel, 1);
		int   numticks = fraction * NUMTICK;

		y = draw_one_xp_bar(x, y, Math::constrain(numticks, 0, NUMTICK));
	}

	return y;
}


void CharacterStatus::draw() {
    //If object we are monitoring no longer exist, then destroy this GUI component
    const std::shared_ptr<Object> pchr = _object.lock();
    if (!pchr) {
        destroy();
        return;
    }

    int yOffset = getY();

    // draw the name
    yOffset = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(getX() + 8, yOffset), pchr->getName(false, false, true));

    // draw the character's money
    std::ostringstream os;
    os << '$' << std::setw(4) << pchr->getMoney();
    yOffset = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(getX() + 8, yOffset), os.str()) + 8;

    bool levelUp = false;
    if (pchr->isPlayer()) {
        levelUp = _currentModule->getPlayer(pchr->is_which_player)->hasUnspentLevel();
    }

    // draw the character's main icon
    draw_one_character_icon(pchr->getObjRef(), getX() + 40, yOffset, false, levelUp ? COLOR_YELLOW : NOSPARKLE);

    // draw the left hand item icon
    draw_one_character_icon(pchr->holdingwhich[SLOT_LEFT], getX() + 8, yOffset, true, NOSPARKLE);

    // draw the right hand item icon
    draw_one_character_icon(pchr->holdingwhich[SLOT_RIGHT], getX() + 72, yOffset, true, NOSPARKLE);

    // skip to the next row
    yOffset += 32;

    //Draw the small XP progress bar
    yOffset = draw_character_xp_bar(pchr->getObjRef(), getX() + 16, yOffset);

    // Draw the life bar
    if (pchr->isAlive()) {
        yOffset = draw_one_bar(pchr->getAttribute(Attribute::LIFE_BARCOLOR), getX(), yOffset, pchr->getLife(), pchr->getAttribute(Attribute::MAX_LIFE));
    } else {
        // Draw a black bar
        yOffset = draw_one_bar(0, getX(), yOffset, 0, pchr->getAttribute(Attribute::MAX_LIFE));
    }

    // Draw the mana bar
    int mana_pips_max = pchr->getAttribute(Attribute::MAX_MANA);
    if (mana_pips_max > 0) {
        yOffset = draw_one_bar(pchr->getAttribute(Attribute::MANA_BARCOLOR), getX(), yOffset, pchr->getMana(), mana_pips_max);
    }

    //After rendering we know how high this GUI component actually is
    setHeight(yOffset - getY());

    //Finally draw charge bar if applicable
    if (pchr->isPlayer()) {
        const std::shared_ptr<Player> &player = _currentModule->getPlayer(pchr->is_which_player);
        if (player->getChargeBarFrame() >= update_wld) {
            _chargeBar->setVisible(true);
            _chargeBar->setMaxValue(player->getBarMaxCharge());
            _chargeBar->setValue(player->getBarCurrentCharge());
            _chargeBar->setTickWidth(player->getBarPipWidth());
            _chargeBar->setSize(getWidth(), 16);
            _chargeBar->setPosition(getX() - _chargeBar->getWidth() - 5, getY() + getHeight() / 2 - _chargeBar->getHeight() / 2);
            _chargeBar->draw();
        } else {
            _chargeBar->setVisible(false);
        }
    }
}

} // namespace GUI
} // namespace Ego
