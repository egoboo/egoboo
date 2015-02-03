#pragma once

#include "game/gamestates/GameState.hpp"
#include "game/graphic.h"

class MainMenuState : public GameState
{
public:
	MainMenuState();

	void update() override;

	void beginState() override;

protected:
	void drawContainer() override;
};
