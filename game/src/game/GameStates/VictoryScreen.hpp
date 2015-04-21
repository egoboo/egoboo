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

/// @file game/GameStates/VictoryScreen.hpp
/// @details After beating a module, this screen is display with some end-game text
/// @author Johan Jansen

#pragma once

#include "game/GameStates/GameState.hpp"

//Forward declarations
class PlayingState;

class VictoryScreen : public GameState
{
public:
    VictoryScreen(PlayingState *playingState, const bool forceExit = false);

    void update() override;

    void beginState() override;

    //Disable copying class
    VictoryScreen(const VictoryScreen& copy) = delete;
    VictoryScreen& operator=(const VictoryScreen&) = delete;
    
protected:
    void drawContainer() override;

private:
    PlayingState *_playingState;
};
