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

/// @file egolib/game/GameStates/PlayingState.hpp
/// @details Main state where the players are currently playing a module
/// @author Johan Jansen

#pragma once

#include "egolib/game/GameStates/GameState.hpp"
#include "egolib/Entities/Forward.hpp"

//Forward declarations
namespace Ego::GUI { 
class CharacterWindow;
class MiniMap;
class CharacterStatus;
class MessageLog;
}

class PlayingState : public GameState
{
public:
    PlayingState();

    ~PlayingState();

    void update() override;

    void beginState() override;

    void draw(Ego::GUI::DrawingContext& drawingContext) override {
        drawContainer(drawingContext);
    }

    bool notifyKeyboardKeyPressed(const Ego::Events::KeyboardKeyPressedEvent& e) override;

    const std::shared_ptr<Ego::GUI::MiniMap>& getMiniMap() const;

    void addStatusMonitor(const std::shared_ptr<Object> &object);

    std::shared_ptr<Object> getStatusCharacter(size_t index);

    void displayCharacterWindow(uint8_t statusNumber);

    const std::shared_ptr<Ego::GUI::MessageLog>& getMessageLog() const;

protected:
    void drawContainer(Ego::GUI::DrawingContext& drawingContext) override;

private:
    void updateStatusBarPosition();

private:
    std::shared_ptr<Ego::GUI::MiniMap> _miniMap;
    std::shared_ptr<Ego::GUI::MessageLog> _messageLog;
    std::vector<std::weak_ptr<Ego::GUI::CharacterStatus>> _statusList;
    std::array<std::weak_ptr<Ego::GUI::CharacterWindow>, 8> _characterWindows;
};
