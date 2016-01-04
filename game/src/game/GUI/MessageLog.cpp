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

/// @file game/GUI/MessageLog.cpp
/// @author Johan Jansen

#include "game/GUI/MessageLog.hpp"
#include "game/renderer_2d.h"
#include "egolib/font_bmp.h"

namespace Ego
{
namespace GUI 
{

MessageLog::MessageLog() :
	_messages()
{
}

void MessageLog::draw()
{
	float yOffset = getY();

	//Render all text and remove old messages
	_messages.remove_if([this, &yOffset](Message& message){
        const int millisRemaining = static_cast<int64_t>(message.lifeTime) - SDL_GetTicks();
        yOffset = drawBitmapFontString(getX(), yOffset, message.text, millisRemaining > MESSAGE_FADE_TIME_MS ? 1.0f : millisRemaining / static_cast<float>(MESSAGE_FADE_TIME_MS));
		return SDL_GetTicks() > message.lifeTime;
	});
}

float MessageLog::drawBitmapFontString(const float stt_x, const float stt_y, const std::string &text, const float alpha)
{
	//Current render position
	float x = stt_x;
	float y = stt_y;

    const std::shared_ptr<Ego::Texture> &fontTexture = TextureManager::get().getTexture("mp_data/font_new_shadow");

	for(size_t cnt = 0; cnt < text.length(); ++cnt)
    {
    	const uint8_t cTmp = text[cnt];

        // Check each new word for wrapping
        if ('~' == cTmp || C_LINEFEED_CHAR == cTmp || C_CARRIAGE_RETURN_CHAR == cTmp || std::isspace(cTmp)) {
            int endx = x + font_bmp_length_of_word(text.c_str() + cnt - 1);

            if (endx > getWidth()) {

                // Wrap the end and cut off spaces and tabs
                x = stt_x + fontyspacing;
                y += fontyspacing;
                while (std::isspace(text[cnt]) || '~' == text[cnt]) {
                    cnt++;
                }

	            continue;
            }
        }

        // Use squiggle for tab
        if ( '~' == cTmp ) {
            x = (std::floor(x / TABADD) + 1.0f) * TABADD;
        }

        //Linefeed
        else if (C_LINEFEED_CHAR == cTmp) {
            x = stt_x;
            y += fontyspacing;
        }

        //other whitespace
        else if (std::isspace(cTmp)) {
            uint8_t iTmp = asciitofont[cTmp];
            x += fontxspacing[iTmp] / 2;
        }

        // Normal letter
        else {
            uint8_t iTmp = asciitofont[cTmp];
            draw_one_font(fontTexture, iTmp, x, y, alpha);
            x += fontxspacing[iTmp];
        }
    }

    return y + fontyspacing;
}

void MessageLog::addMessage(const std::string &message)
{
    //Insert new message at the back
	_messages.emplace_back(message, SDL_GetTicks() + egoboo_config_t::get().hud_messageDuration.getValue() * 10);

    //Remove oldest messages if we have too many (FIFO)
    while(_messages.size() > egoboo_config_t::get().hud_simultaneousMessages_max.getValue()) {
        _messages.pop_front();
    }
}

} //GUI
} //Ego
