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
#include "egolib/font_bmp.h"

namespace Ego {
namespace GUI {

MessageLog::MessageLog() :
    _messages() {}

void MessageLog::draw(DrawingContext& drawingContext) {
    float yOffset = getY();

    //Render all text and remove old messages
    _messages.remove_if([this, &yOffset](Message& message) {
        const int millisRemaining = static_cast<int64_t>(message.lifeTime) - Core::System::get().getSystemService().getTicks();
        if (millisRemaining <= 0) return true;
        yOffset = _gameEngine->getUIManager()->drawBitmapFontString(Vector2f(getX(), yOffset), message.text, 0, millisRemaining > MESSAGE_FADE_TIME_MS ? 1.0f : millisRemaining / static_cast<float>(MESSAGE_FADE_TIME_MS));
        return Core::System::get().getSystemService().getTicks() > message.lifeTime;
    });
}

void MessageLog::addMessage(const std::string &message) {
    //Insert new message at the back
    _messages.emplace_back(message, Core::System::get().getSystemService().getTicks() + egoboo_config_t::get().hud_messageDuration.getValue() * 10);

    //Remove oldest messages if we have too many (FIFO)
    while (_messages.size() > egoboo_config_t::get().hud_simultaneousMessages_max.getValue()) {
        _messages.pop_front();
    }
}

} //GUI
} //Ego
