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

/// @file game/GUI/MessageLog.hpp
/// @author Johan Jansen

#pragma once

#include "game/GUI/Component.hpp"

namespace Ego {
namespace GUI {

class MessageLog : public Component {
public:
    MessageLog();

    virtual void draw(DrawingContext& drawingContext) override;

    void addMessage(const std::string &message);

private:
    static constexpr uint32_t MESSAGE_DURATION_MS = 3000;       //How many milliseconds a message should be rendered before it is removed
    static constexpr uint32_t MESSAGE_FADE_TIME_MS = 700;       //How many milliseconds it takes to fade away completely

    struct Message {
    public:
        Message(const std::string& setText, uint32_t ticks) :
            text(setText),
            lifeTime(ticks) {
            //ctor
        }

        std::string text;
        uint32_t lifeTime;
    };

    std::list<Message> _messages;
};

} // namespace GUI
} // namespace Ego
