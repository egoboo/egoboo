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

/// @file game/GameStates/SelectPlayersState.hpp
/// @details Select which of the 4 players are going to play
/// @author Johan Jansen

#pragma once

#include "game/GameStates/GameState.hpp"

//Forward declaration
class LoadPlayerElement;
class Button;

class SelectPlayersState : public GameState
{
public:
    SelectPlayersState();

    void update() override;

    void beginState() override;

protected:
    void drawContainer() override;

private:
    std::vector<std::shared_ptr<LoadPlayerElement>> _selectedPlayers;
    std::vector<std::shared_ptr<Button>>             _playerButtons;
    std::shared_ptr<Button>                         _continueButton;
};
