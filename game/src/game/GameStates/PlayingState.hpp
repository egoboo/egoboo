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

/// @file game/GameStates/PlayingState.hpp
/// @details Main state where the players are currently playing a module
/// @author Johan Jansen

#pragma once

#include "game/GameStates/GameState.hpp"

//Forward declarations
class CameraSystem;
class MiniMap;
class Object;

class PlayingState : public GameState
{
public:
    PlayingState(std::shared_ptr<CameraSystem> cameraSystem);

    ~PlayingState();

    void update() override;

    void beginState() override;

    bool notifyKeyDown(const int keyCode) override;

    const std::shared_ptr<MiniMap>& getMiniMap() const;

    void addStatusMonitor(const std::shared_ptr<Object> &object);

protected:
    void drawContainer() override;

private:
	std::shared_ptr<CameraSystem> _cameraSystem;
    std::shared_ptr<MiniMap> _miniMap;
};
